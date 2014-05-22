/* Callgraph based interprocedural optimizations.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
   Free Software Foundation, Inc.
   Contributed by Jan Hubicka

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

/* This module implements main driver of compilation process as well as
   few basic interprocedural optimizers.

   The main scope of this file is to act as an interface in between
   tree based frontends and the backend (and middle end)

   The front-end is supposed to use following functionality:

    - cgraph_finalize_function

      This function is called once front-end has parsed whole body of function
      and it is certain that the function body nor the declaration will change.

      (There is one exception needed for implementing GCC extern inline
	function.)

    - varpool_finalize_variable

      This function has same behavior as the above but is used for static
      variables.

    - cgraph_finalize_compilation_unit

      This function is called once (source level) compilation unit is finalized
      and it will no longer change.

      In the the call-graph construction and local function
      analysis takes place here.  Bodies of unreachable functions are released
      to conserve memory usage.

      The function can be called multiple times when multiple source level
      compilation units are combined (such as in C frontend)

    - cgraph_optimize

      In this unit-at-a-time compilation the intra procedural analysis takes
      place here.  In particular the static functions whose address is never
      taken are marked as local.  Backend can then use this information to
      modify calling conventions, do better inlining or similar optimizations.

    - cgraph_mark_needed_node
    - varpool_mark_needed_node

      When function or variable is referenced by some hidden way the call-graph
      data structure must be updated accordingly by this function.
      There should be little need to call this function and all the references
      should be made explicit to cgraph code.  At present these functions are
      used by C++ frontend to explicitly mark the keyed methods.

    - analyze_expr callback

      This function is responsible for lowering tree nodes not understood by
      generic code into understandable ones or alternatively marking
      callgraph and varpool nodes referenced by the as needed.

      ??? On the tree-ssa genericizing should take place here and we will avoid
      need for these hooks (replacing them by genericizing hook)

        Analyzing of all functions is deferred
	to cgraph_finalize_compilation_unit and expansion into cgraph_optimize.

	In cgraph_finalize_compilation_unit the reachable functions are
	analyzed.  During analysis the call-graph edges from reachable
	functions are constructed and their destinations are marked as
	reachable.  References to functions and variables are discovered too
	and variables found to be needed output to the assembly file.  Via
	mark_referenced call in assemble_variable functions referenced by
	static variables are noticed too.

	The intra-procedural information is produced and its existence
	indicated by global_info_ready.  Once this flag is set it is impossible
	to change function from !reachable to reachable and thus
	assemble_variable no longer call mark_referenced.

	Finally the call-graph is topologically sorted and all reachable functions
	that has not been completely inlined or are not external are output.

	??? It is possible that reference to function or variable is optimized
	out.  We can not deal with this nicely because topological order is not
	suitable for it.  For tree-ssa we may consider another pass doing
	optimization and re-discovering reachable functions.

	??? Reorganize code so variables are output very last and only if they
	really has been referenced by produced code, so we catch more cases
	where reference has been optimized out.  */


#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "rtl.h"
#include "tree-flow.h"
#include "tree-inline.h"
#include "langhooks.h"
#include "pointer-set.h"
#include "toplev.h"
#include "flags.h"
#include "ggc.h"
#include "debug.h"
#include "target.h"
#include "cgraph.h"
#include "diagnostic.h"
#include "timevar.h"
#include "params.h"
#include "fibheap.h"
#include "intl.h"
#include "function.h"
#include "ipa-prop.h"
#include "gimple.h"
#include "tree-iterator.h"
#include "tree-pass.h"
#include "tree-dump.h"
#include "output.h"
#include "coverage.h"
#include "plugin.h"

static void cgraph_expand_all_functions (void);
static void cgraph_mark_functions_to_output (void);
static void cgraph_expand_function (struct cgraph_node *);
static void cgraph_output_pending_asms (void);
static void cgraph_analyze_function (struct cgraph_node *);

static FILE *cgraph_dump_file;

/* A vector of FUNCTION_DECLs declared as static constructors.  */
static GTY (()) VEC(tree, gc) *static_ctors;
/* A vector of FUNCTION_DECLs declared as static destructors.  */
static GTY (()) VEC(tree, gc) *static_dtors;

/* Used for vtable lookup in thunk adjusting.  */
static GTY (()) tree vtable_entry_type;

/* When target does not have ctors and dtors, we call all constructor
   and destructor by special initialization/destruction function
   recognized by collect2.

   When we are going to build this function, collect all constructors and
   destructors and turn them into normal functions.  */

static void
record_cdtor_fn (tree fndecl)
{
  struct cgraph_node *node;
  if (targetm.have_ctors_dtors
      || (!DECL_STATIC_CONSTRUCTOR (fndecl)
	  && !DECL_STATIC_DESTRUCTOR (fndecl)))
    return;

  if (DECL_STATIC_CONSTRUCTOR (fndecl))
    {
      VEC_safe_push (tree, gc, static_ctors, fndecl);
      DECL_STATIC_CONSTRUCTOR (fndecl) = 0;
    }
  if (DECL_STATIC_DESTRUCTOR (fndecl))
    {
      VEC_safe_push (tree, gc, static_dtors, fndecl);
      DECL_STATIC_DESTRUCTOR (fndecl) = 0;
    }
  node = cgraph_node (fndecl);
  node->local.disregard_inline_limits = 1;
  cgraph_mark_reachable_node (node);
}

/* Define global constructors/destructor functions for the CDTORS, of
   which they are LEN.  The CDTORS are sorted by initialization
   priority.  If CTOR_P is true, these are constructors; otherwise,
   they are destructors.  */

static void
build_cdtor (bool ctor_p, tree *cdtors, size_t len)
{
  size_t i;

  i = 0;
  while (i < len)
    {
      tree body;
      tree fn;
      priority_type priority;

      priority = 0;
      body = NULL_TREE;
      /* Find the next batch of constructors/destructors with the same
	 initialization priority.  */
      do
	{
	  priority_type p;
	  fn = cdtors[i];
	  p = ctor_p ? DECL_INIT_PRIORITY (fn) : DECL_FINI_PRIORITY (fn);
	  if (!body)
	    priority = p;
	  else if (p != priority)
	    break;
	  append_to_statement_list (build_function_call_expr (UNKNOWN_LOCATION,
							      fn, 0),
				    &body);
	  ++i;
	}
      while (i < len);
      gcc_assert (body != NULL_TREE);
      /* Generate a function to call all the function of like
	 priority.  */
      cgraph_build_static_cdtor (ctor_p ? 'I' : 'D', body, priority);
    }
}

/* Comparison function for qsort.  P1 and P2 are actually of type
   "tree *" and point to static constructors.  DECL_INIT_PRIORITY is
   used to determine the sort order.  */

static int
compare_ctor (const void *p1, const void *p2)
{
  tree f1;
  tree f2;
  int priority1;
  int priority2;

  f1 = *(const tree *)p1;
  f2 = *(const tree *)p2;
  priority1 = DECL_INIT_PRIORITY (f1);
  priority2 = DECL_INIT_PRIORITY (f2);

  if (priority1 < priority2)
    return -1;
  else if (priority1 > priority2)
    return 1;
  else
    /* Ensure a stable sort.  */
    return (const tree *)p1 - (const tree *)p2;
}

/* Comparison function for qsort.  P1 and P2 are actually of type
   "tree *" and point to static destructors.  DECL_FINI_PRIORITY is
   used to determine the sort order.  */

static int
compare_dtor (const void *p1, const void *p2)
{
  tree f1;
  tree f2;
  int priority1;
  int priority2;

  f1 = *(const tree *)p1;
  f2 = *(const tree *)p2;
  priority1 = DECL_FINI_PRIORITY (f1);
  priority2 = DECL_FINI_PRIORITY (f2);

  if (priority1 < priority2)
    return -1;
  else if (priority1 > priority2)
    return 1;
  else
    /* Ensure a stable sort.  */
    return (const tree *)p1 - (const tree *)p2;
}

/* Generate functions to call static constructors and destructors
   for targets that do not support .ctors/.dtors sections.  These
   functions have magic names which are detected by collect2.  */

static void
cgraph_build_cdtor_fns (void)
{
  if (!VEC_empty (tree, static_ctors))
    {
      gcc_assert (!targetm.have_ctors_dtors);
      qsort (VEC_address (tree, static_ctors),
	     VEC_length (tree, static_ctors),
	     sizeof (tree),
	     compare_ctor);
      build_cdtor (/*ctor_p=*/true,
		   VEC_address (tree, static_ctors),
		   VEC_length (tree, static_ctors));
      VEC_truncate (tree, static_ctors, 0);
    }

  if (!VEC_empty (tree, static_dtors))
    {
      gcc_assert (!targetm.have_ctors_dtors);
      qsort (VEC_address (tree, static_dtors),
	     VEC_length (tree, static_dtors),
	     sizeof (tree),
	     compare_dtor);
      build_cdtor (/*ctor_p=*/false,
		   VEC_address (tree, static_dtors),
		   VEC_length (tree, static_dtors));
      VEC_truncate (tree, static_dtors, 0);
    }
}

/* Determine if function DECL is needed.  That is, visible to something
   either outside this translation unit, something magic in the system
   configury.  */

bool
cgraph_decide_is_function_needed (struct cgraph_node *node, tree decl)
{
  /* If the user told us it is used, then it must be so.  */
  if (node->local.externally_visible)
    return true;

  /* ??? If the assembler name is set by hand, it is possible to assemble
     the name later after finalizing the function and the fact is noticed
     in assemble_name then.  This is arguably a bug.  */
  if (DECL_ASSEMBLER_NAME_SET_P (decl)
      && TREE_SYMBOL_REFERENCED (DECL_ASSEMBLER_NAME (decl)))
    return true;

  /* With -fkeep-inline-functions we are keeping all inline functions except
     for extern inline ones.  */
  if (flag_keep_inline_functions
      && DECL_DECLARED_INLINE_P (decl)
      && !DECL_EXTERNAL (decl)
      && !lookup_attribute ("always_inline", DECL_ATTRIBUTES (decl)))
     return true;

  /* If we decided it was needed before, but at the time we didn't have
     the body of the function available, then it's still needed.  We have
     to go back and re-check its dependencies now.  */
  if (node->needed)
    return true;

  /* Externally visible functions must be output.  The exception is
     COMDAT functions that must be output only when they are needed.

     When not optimizing, also output the static functions. (see
     PR24561), but don't do so for always_inline functions, functions
     declared inline and nested functions.  These was optimized out
     in the original implementation and it is unclear whether we want
     to change the behavior here.  */
  if (((TREE_PUBLIC (decl)
	|| (!optimize && !node->local.disregard_inline_limits
	    && !DECL_DECLARED_INLINE_P (decl)
	    && !node->origin))
       && !flag_whole_program
       && !flag_lto
       && !flag_whopr)
      && !DECL_COMDAT (decl) && !DECL_EXTERNAL (decl))
    return true;

  /* Constructors and destructors are reachable from the runtime by
     some mechanism.  */
  if (DECL_STATIC_CONSTRUCTOR (decl) || DECL_STATIC_DESTRUCTOR (decl))
    return true;

  return false;
}

/* Process CGRAPH_NEW_FUNCTIONS and perform actions necessary to add these
   functions into callgraph in a way so they look like ordinary reachable
   functions inserted into callgraph already at construction time.  */

bool
cgraph_process_new_functions (void)
{
  bool output = false;
  tree fndecl;
  struct cgraph_node *node;

  /*  Note that this queue may grow as its being processed, as the new
      functions may generate new ones.  */
  while (cgraph_new_nodes)
    {
      node = cgraph_new_nodes;
      fndecl = node->decl;
      cgraph_new_nodes = cgraph_new_nodes->next_needed;
      switch (cgraph_state)
	{
	case CGRAPH_STATE_CONSTRUCTION:
	  /* At construction time we just need to finalize function and move
	     it into reachable functions list.  */

	  node->next_needed = NULL;
	  cgraph_finalize_function (fndecl, false);
	  cgraph_mark_reachable_node (node);
	  output = true;
	  break;

	case CGRAPH_STATE_IPA:
	case CGRAPH_STATE_IPA_SSA:
	  /* When IPA optimization already started, do all essential
	     transformations that has been already performed on the whole
	     cgraph but not on this function.  */

	  gimple_register_cfg_hooks ();
	  if (!node->analyzed)
	    cgraph_analyze_function (node);
	  push_cfun (DECL_STRUCT_FUNCTION (fndecl));
	  current_function_decl = fndecl;
	  compute_inline_parameters (node);
	  if ((cgraph_state == CGRAPH_STATE_IPA_SSA
	      && !gimple_in_ssa_p (DECL_STRUCT_FUNCTION (fndecl)))
	      /* When not optimizing, be sure we run early local passes anyway
		 to expand OMP.  */
	      || !optimize)
	    execute_pass_list (pass_early_local_passes.pass.sub);
	  free_dominance_info (CDI_POST_DOMINATORS);
	  free_dominance_info (CDI_DOMINATORS);
	  pop_cfun ();
	  current_function_decl = NULL;
	  break;

	case CGRAPH_STATE_EXPANSION:
	  /* Functions created during expansion shall be compiled
	     directly.  */
	  node->process = 0;
	  cgraph_expand_function (node);
	  break;

	default:
	  gcc_unreachable ();
	  break;
	}
      cgraph_call_function_insertion_hooks (node);
    }
  return output;
}

/* As an GCC extension we allow redefinition of the function.  The
   semantics when both copies of bodies differ is not well defined.
   We replace the old body with new body so in unit at a time mode
   we always use new body, while in normal mode we may end up with
   old body inlined into some functions and new body expanded and
   inlined in others.

   ??? It may make more sense to use one body for inlining and other
   body for expanding the function but this is difficult to do.  */

static void
cgraph_reset_node (struct cgraph_node *node)
{
  /* If node->process is set, then we have already begun whole-unit analysis.
     This is *not* testing for whether we've already emitted the function.
     That case can be sort-of legitimately seen with real function redefinition
     errors.  I would argue that the front end should never present us with
     such a case, but don't enforce that for now.  */
  gcc_assert (!node->process);

  /* Reset our data structures so we can analyze the function again.  */
  memset (&node->local, 0, sizeof (node->local));
  memset (&node->global, 0, sizeof (node->global));
  memset (&node->rtl, 0, sizeof (node->rtl));
  node->analyzed = false;
  node->local.redefined_extern_inline = true;
  node->local.finalized = false;

  cgraph_node_remove_callees (node);

  /* We may need to re-queue the node for assembling in case
     we already proceeded it and ignored as not needed or got
     a re-declaration in IMA mode.  */
  if (node->reachable)
    {
      struct cgraph_node *n;

      for (n = cgraph_nodes_queue; n; n = n->next_needed)
	if (n == node)
	  break;
      if (!n)
	node->reachable = 0;
    }
}

static void
cgraph_lower_function (struct cgraph_node *node)
{
  if (node->lowered)
    return;

  if (node->nested)
    lower_nested_functions (node->decl);
  gcc_assert (!node->nested);

  tree_lowering_passes (node->decl);
  node->lowered = true;
}

/* DECL has been parsed.  Take it, queue it, compile it at the whim of the
   logic in effect.  If NESTED is true, then our caller cannot stand to have
   the garbage collector run at the moment.  We would need to either create
   a new GC context, or just not compile right now.  */

void
cgraph_finalize_function (tree decl, bool nested)
{
  struct cgraph_node *node = cgraph_node (decl);

  if (node->local.finalized)
    cgraph_reset_node (node);

  node->pid = cgraph_max_pid ++;
  notice_global_symbol (decl);
  node->local.finalized = true;
  node->lowered = DECL_STRUCT_FUNCTION (decl)->cfg != NULL;
  node->finalized_by_frontend = true;
  record_cdtor_fn (node->decl);

  if (cgraph_decide_is_function_needed (node, decl))
    cgraph_mark_needed_node (node);

  /* Since we reclaim unreachable nodes at the end of every language
     level unit, we need to be conservative about possible entry points
     there.  */
  if ((TREE_PUBLIC (decl) && !DECL_COMDAT (decl) && !DECL_EXTERNAL (decl)))
    cgraph_mark_reachable_node (node);

  /* If we've not yet emitted decl, tell the debug info about it.  */
  if (!TREE_ASM_WRITTEN (decl))
    (*debug_hooks->deferred_inline_function) (decl);

  /* Possibly warn about unused parameters.  */
  if (warn_unused_parameter)
    do_warn_unused_parameter (decl);

  if (!nested)
    ggc_collect ();
}

/* C99 extern inline keywords allow changing of declaration after function
   has been finalized.  We need to re-decide if we want to mark the function as
   needed then.   */

void
cgraph_mark_if_needed (tree decl)
{
  struct cgraph_node *node = cgraph_node (decl);
  if (node->local.finalized && cgraph_decide_is_function_needed (node, decl))
    cgraph_mark_needed_node (node);
}

/* Verify cgraph nodes of given cgraph node.  */
void
verify_cgraph_node (struct cgraph_node *node)
{
  struct cgraph_edge *e;
  struct function *this_cfun = DECL_STRUCT_FUNCTION (node->decl);
  struct function *saved_cfun = cfun;
  basic_block this_block;
  gimple_stmt_iterator gsi;
  bool error_found = false;

  if (errorcount || sorrycount)
    return;

  timevar_push (TV_CGRAPH_VERIFY);
  /* debug_generic_stmt needs correct cfun */
  set_cfun (this_cfun);
  for (e = node->callees; e; e = e->next_callee)
    if (e->aux)
      {
	error ("aux field set for edge %s->%s",
	       identifier_to_locale (cgraph_node_name (e->caller)),
	       identifier_to_locale (cgraph_node_name (e->callee)));
	error_found = true;
      }
  if (node->count < 0)
    {
      error ("Execution count is negative");
      error_found = true;
    }
  if (node->global.inlined_to && node->local.externally_visible)
    {
      error ("Externally visible inline clone");
      error_found = true;
    }
  if (node->global.inlined_to && node->address_taken)
    {
      error ("Inline clone with address taken");
      error_found = true;
    }
  if (node->global.inlined_to && node->needed)
    {
      error ("Inline clone is needed");
      error_found = true;
    }
  for (e = node->callers; e; e = e->next_caller)
    {
      if (e->count < 0)
	{
	  error ("caller edge count is negative");
	  error_found = true;
	}
      if (e->frequency < 0)
	{
	  error ("caller edge frequency is negative");
	  error_found = true;
	}
      if (e->frequency > CGRAPH_FREQ_MAX)
	{
	  error ("caller edge frequency is too large");
	  error_found = true;
	}
      if (gimple_has_body_p (e->caller->decl)
          && !e->caller->global.inlined_to
          && (e->frequency
	      != compute_call_stmt_bb_frequency (e->caller->decl,
						 gimple_bb (e->call_stmt))))
	{
	  error ("caller edge frequency %i does not match BB freqency %i",
	  	 e->frequency,
		 compute_call_stmt_bb_frequency (e->caller->decl,
						 gimple_bb (e->call_stmt)));
	  error_found = true;
	}
      if (!e->inline_failed)
	{
	  if (node->global.inlined_to
	      != (e->caller->global.inlined_to
		  ? e->caller->global.inlined_to : e->caller))
	    {
	      error ("inlined_to pointer is wrong");
	      error_found = true;
	    }
	  if (node->callers->next_caller)
	    {
	      error ("multiple inline callers");
	      error_found = true;
	    }
	}
      else
	if (node->global.inlined_to)
	  {
	    error ("inlined_to pointer set for noninline callers");
	    error_found = true;
	  }
    }
  if (!node->callers && node->global.inlined_to)
    {
      error ("inlined_to pointer is set but no predecessors found");
      error_found = true;
    }
  if (node->global.inlined_to == node)
    {
      error ("inlined_to pointer refers to itself");
      error_found = true;
    }

  if (!cgraph_node (node->decl))
    {
      error ("node not found in cgraph_hash");
      error_found = true;
    }

  if (node->clone_of)
    {
      struct cgraph_node *n;
      for (n = node->clone_of->clones; n; n = n->next_sibling_clone)
        if (n == node)
	  break;
      if (!n)
	{
	  error ("node has wrong clone_of");
	  error_found = true;
	}
    }
  if (node->clones)
    {
      struct cgraph_node *n;
      for (n = node->clones; n; n = n->next_sibling_clone)
        if (n->clone_of != node)
	  break;
      if (n)
	{
	  error ("node has wrong clone list");
	  error_found = true;
	}
    }
  if ((node->prev_sibling_clone || node->next_sibling_clone) && !node->clone_of)
    {
       error ("node is in clone list but it is not clone");
       error_found = true;
    }
  if (!node->prev_sibling_clone && node->clone_of && node->clone_of->clones != node)
    {
      error ("node has wrong prev_clone pointer");
      error_found = true;
    }
  if (node->prev_sibling_clone && node->prev_sibling_clone->next_sibling_clone != node)
    {
      error ("double linked list of clones corrupted");
      error_found = true;
    }
  if (node->same_comdat_group)
    {
      struct cgraph_node *n = node->same_comdat_group;

      if (!DECL_ONE_ONLY (node->decl))
	{
	  error ("non-DECL_ONE_ONLY node in a same_comdat_group list");
	  error_found = true;
	}
      if (n == node)
	{
	  error ("node is alone in a comdat group");
	  error_found = true;
	}
      do
	{
	  if (!n->same_comdat_group)
	    {
	      error ("same_comdat_group is not a circular list");
	      error_found = true;
	      break;
	    }
	  n = n->same_comdat_group;
	}
      while (n != node);
    }

  if (node->analyzed && gimple_has_body_p (node->decl)
      && !TREE_ASM_WRITTEN (node->decl)
      && (!DECL_EXTERNAL (node->decl) || node->global.inlined_to)
      && !flag_wpa)
    {
      if (this_cfun->cfg)
	{
	  /* The nodes we're interested in are never shared, so walk
	     the tree ignoring duplicates.  */
	  struct pointer_set_t *visited_nodes = pointer_set_create ();
	  /* Reach the trees by walking over the CFG, and note the
	     enclosing basic-blocks in the call edges.  */
	  FOR_EACH_BB_FN (this_block, this_cfun)
	    for (gsi = gsi_start_bb (this_block);
                 !gsi_end_p (gsi);
                 gsi_next (&gsi))
	      {
		gimple stmt = gsi_stmt (gsi);
		tree decl;
		if (is_gimple_call (stmt) && (decl = gimple_call_fndecl (stmt)))
		  {
		    struct cgraph_edge *e = cgraph_edge (node, stmt);
		    if (e)
		      {
			if (e->aux)
			  {
			    error ("shared call_stmt:");
			    debug_gimple_stmt (stmt);
			    error_found = true;
			  }
			if (e->callee->same_body_alias)
			  {
			    error ("edge points to same body alias:");
			    debug_tree (e->callee->decl);
			    error_found = true;
			  }
			e->aux = (void *)1;
		      }
		    else
		      {
			error ("missing callgraph edge for call stmt:");
			debug_gimple_stmt (stmt);
			error_found = true;
		      }
		  }
	      }
	  pointer_set_destroy (visited_nodes);
	}
      else
	/* No CFG available?!  */
	gcc_unreachable ();

      for (e = node->callees; e; e = e->next_callee)
	{
	  if (!e->aux && !e->indirect_call)
	    {
	      error ("edge %s->%s has no corresponding call_stmt",
		     identifier_to_locale (cgraph_node_name (e->caller)),
		     identifier_to_locale (cgraph_node_name (e->callee)));
	      debug_gimple_stmt (e->call_stmt);
	      error_found = true;
	    }
	  e->aux = 0;
	}
    }
  if (error_found)
    {
      dump_cgraph_node (stderr, node);
      internal_error ("verify_cgraph_node failed");
    }
  set_cfun (saved_cfun);
  timevar_pop (TV_CGRAPH_VERIFY);
}

/* Verify whole cgraph structure.  */
void
verify_cgraph (void)
{
  struct cgraph_node *node;

  if (sorrycount || errorcount)
    return;

  for (node = cgraph_nodes; node; node = node->next)
    verify_cgraph_node (node);
}

/* Output all asm statements we have stored up to be output.  */

static void
cgraph_output_pending_asms (void)
{
  struct cgraph_asm_node *can;

  if (errorcount || sorrycount)
    return;

  for (can = cgraph_asm_nodes; can; can = can->next)
    assemble_asm (can->asm_str);
  cgraph_asm_nodes = NULL;
}

/* Analyze the function scheduled to be output.  */
static void
cgraph_analyze_function (struct cgraph_node *node)
{
  tree save = current_function_decl;
  tree decl = node->decl;

  current_function_decl = decl;
  push_cfun (DECL_STRUCT_FUNCTION (decl));

  assign_assembler_name_if_neeeded (node->decl);

  /* Make sure to gimplify bodies only once.  During analyzing a
     function we lower it, which will require gimplified nested
     functions, so we can end up here with an already gimplified
     body.  */
  if (!gimple_body (decl))
    gimplify_function_tree (decl);
  dump_function (TDI_generic, decl);

  cgraph_lower_function (node);
  node->analyzed = true;

  pop_cfun ();
  current_function_decl = save;
}

/* Look for externally_visible and used attributes and mark cgraph nodes
   accordingly.

   We cannot mark the nodes at the point the attributes are processed (in
   handle_*_attribute) because the copy of the declarations available at that
   point may not be canonical.  For example, in:

    void f();
    void f() __attribute__((used));

   the declaration we see in handle_used_attribute will be the second
   declaration -- but the front end will subsequently merge that declaration
   with the original declaration and discard the second declaration.

   Furthermore, we can't mark these nodes in cgraph_finalize_function because:

    void f() {}
    void f() __attribute__((externally_visible));

   is valid.

   So, we walk the nodes at the end of the translation unit, applying the
   attributes at that point.  */

static void
process_function_and_variable_attributes (struct cgraph_node *first,
                                          struct varpool_node *first_var)
{
  struct cgraph_node *node;
  struct varpool_node *vnode;

  for (node = cgraph_nodes; node != first; node = node->next)
    {
      tree decl = node->decl;
      if (DECL_PRESERVE_P (decl))
	{
	  mark_decl_referenced (decl);
	  if (node->local.finalized)
	     cgraph_mark_needed_node (node);
	}
      if (lookup_attribute ("externally_visible", DECL_ATTRIBUTES (decl)))
	{
	  if (! TREE_PUBLIC (node->decl))
	    warning_at (DECL_SOURCE_LOCATION (node->decl), OPT_Wattributes,
			"%<externally_visible%>"
			" attribute have effect only on public objects");
	  else if (node->local.finalized)
	     cgraph_mark_needed_node (node);
	}
    }
  for (vnode = varpool_nodes; vnode != first_var; vnode = vnode->next)
    {
      tree decl = vnode->decl;
      if (DECL_PRESERVE_P (decl))
	{
	  mark_decl_referenced (decl);
	  vnode->force_output = true;
	  if (vnode->finalized)
	    varpool_mark_needed_node (vnode);
	}
      if (lookup_attribute ("externally_visible", DECL_ATTRIBUTES (decl)))
	{
	  if (! TREE_PUBLIC (vnode->decl))
	    warning_at (DECL_SOURCE_LOCATION (vnode->decl), OPT_Wattributes,
			"%<externally_visible%>"
			" attribute have effect only on public objects");
	  else if (vnode->finalized)
	    varpool_mark_needed_node (vnode);
	}
    }
}

/* Process CGRAPH_NODES_NEEDED queue, analyze each function (and transitively
   each reachable functions) and build cgraph.
   The function can be called multiple times after inserting new nodes
   into beginning of queue.  Just the new part of queue is re-scanned then.  */

static void
cgraph_analyze_functions (void)
{
  /* Keep track of already processed nodes when called multiple times for
     intermodule optimization.  */
  static struct cgraph_node *first_analyzed;
  struct cgraph_node *first_processed = first_analyzed;
  static struct varpool_node *first_analyzed_var;
  struct cgraph_node *node, *next;

  process_function_and_variable_attributes (first_processed,
					    first_analyzed_var);
  first_processed = cgraph_nodes;
  first_analyzed_var = varpool_nodes;
  varpool_analyze_pending_decls ();
  if (cgraph_dump_file)
    {
      fprintf (cgraph_dump_file, "Initial entry points:");
      for (node = cgraph_nodes; node != first_analyzed; node = node->next)
	if (node->needed)
	  fprintf (cgraph_dump_file, " %s", cgraph_node_name (node));
      fprintf (cgraph_dump_file, "\n");
    }
  cgraph_process_new_functions ();

  /* Propagate reachability flag and lower representation of all reachable
     functions.  In the future, lowering will introduce new functions and
     new entry points on the way (by template instantiation and virtual
     method table generation for instance).  */
  while (cgraph_nodes_queue)
    {
      struct cgraph_edge *edge;
      tree decl = cgraph_nodes_queue->decl;

      node = cgraph_nodes_queue;
      cgraph_nodes_queue = cgraph_nodes_queue->next_needed;
      node->next_needed = NULL;

      /* ??? It is possible to create extern inline function and later using
	 weak alias attribute to kill its body. See
	 gcc.c-torture/compile/20011119-1.c  */
      if (!DECL_STRUCT_FUNCTION (decl))
	{
	  cgraph_reset_node (node);
	  continue;
	}

      if (!node->analyzed)
	cgraph_analyze_function (node);

      for (edge = node->callees; edge; edge = edge->next_callee)
	if (!edge->callee->reachable)
	  cgraph_mark_reachable_node (edge->callee);

      if (node->same_comdat_group)
	{
	  for (next = node->same_comdat_group;
	       next != node;
	       next = next->same_comdat_group)
	    cgraph_mark_reachable_node (next);
	}

      /* If decl is a clone of an abstract function, mark that abstract
	 function so that we don't release its body. The DECL_INITIAL() of that
         abstract function declaration will be later needed to output debug info.  */
      if (DECL_ABSTRACT_ORIGIN (decl))
	{
	  struct cgraph_node *origin_node = cgraph_node (DECL_ABSTRACT_ORIGIN (decl));
	  origin_node->abstract_and_needed = true;
	}

      /* We finalize local static variables during constructing callgraph
         edges.  Process their attributes too.  */
      process_function_and_variable_attributes (first_processed,
						first_analyzed_var);
      first_processed = cgraph_nodes;
      first_analyzed_var = varpool_nodes;
      varpool_analyze_pending_decls ();
      cgraph_process_new_functions ();
    }

  /* Collect entry points to the unit.  */
  if (cgraph_dump_file)
    {
      fprintf (cgraph_dump_file, "Unit entry points:");
      for (node = cgraph_nodes; node != first_analyzed; node = node->next)
	if (node->needed)
	  fprintf (cgraph_dump_file, " %s", cgraph_node_name (node));
      fprintf (cgraph_dump_file, "\n\nInitial ");
      dump_cgraph (cgraph_dump_file);
    }

  if (cgraph_dump_file)
    fprintf (cgraph_dump_file, "\nReclaiming functions:");

  for (node = cgraph_nodes; node != first_analyzed; node = next)
    {
      tree decl = node->decl;
      next = node->next;

      if (node->local.finalized && !gimple_has_body_p (decl))
	cgraph_reset_node (node);

      if (!node->reachable && gimple_has_body_p (decl))
	{
	  if (cgraph_dump_file)
	    fprintf (cgraph_dump_file, " %s", cgraph_node_name (node));
	  cgraph_remove_node (node);
	  continue;
	}
      else
	node->next_needed = NULL;
      gcc_assert (!node->local.finalized || gimple_has_body_p (decl));
      gcc_assert (node->analyzed == node->local.finalized);
    }
  if (cgraph_dump_file)
    {
      fprintf (cgraph_dump_file, "\n\nReclaimed ");
      dump_cgraph (cgraph_dump_file);
    }
  first_analyzed = cgraph_nodes;
  ggc_collect ();
}


/* Analyze the whole compilation unit once it is parsed completely.  */

void
cgraph_finalize_compilation_unit (void)
{
  timevar_push (TV_CGRAPH);

  /* Do not skip analyzing the functions if there were errors, we
     miss diagnostics for following functions otherwise.  */

  /* Emit size functions we didn't inline.  */
  finalize_size_functions ();

  /* Call functions declared with the "constructor" or "destructor"
     attribute.  */
  cgraph_build_cdtor_fns ();

  /* Mark alias targets necessary and emit diagnostics.  */
  finish_aliases_1 ();

  if (!quiet_flag)
    {
      fprintf (stderr, "\nAnalyzing compilation unit\n");
      fflush (stderr);
    }

  /* Gimplify and lower all functions, compute reachability and
     remove unreachable nodes.  */
  cgraph_analyze_functions ();

  /* Mark alias targets necessary and emit diagnostics.  */
  finish_aliases_1 ();

  /* Gimplify and lower thunks.  */
  cgraph_analyze_functions ();

  /* Finally drive the pass manager.  */
  cgraph_optimize ();

  timevar_pop (TV_CGRAPH);
}


/* Figure out what functions we want to assemble.  */

static void
cgraph_mark_functions_to_output (void)
{
  struct cgraph_node *node;
#ifdef ENABLE_CHECKING
  bool check_same_comdat_groups = false;

  for (node = cgraph_nodes; node; node = node->next)
    gcc_assert (!node->process);
#endif

  for (node = cgraph_nodes; node; node = node->next)
    {
      tree decl = node->decl;
      struct cgraph_edge *e;

      gcc_assert (!node->process || node->same_comdat_group);
      if (node->process)
	continue;

      for (e = node->callers; e; e = e->next_caller)
	if (e->inline_failed)
	  break;

      /* We need to output all local functions that are used and not
	 always inlined, as well as those that are reachable from
	 outside the current compilation unit.  */
      if (node->analyzed
	  && !node->global.inlined_to
	  && (node->needed
	      || (e && node->reachable))
	  && !TREE_ASM_WRITTEN (decl)
	  && !DECL_EXTERNAL (decl))
	{
	  node->process = 1;
	  if (node->same_comdat_group)
	    {
	      struct cgraph_node *next;
	      for (next = node->same_comdat_group;
		   next != node;
		   next = next->same_comdat_group)
		next->process = 1;
	    }
	}
      else if (node->same_comdat_group)
	{
#ifdef ENABLE_CHECKING
	  check_same_comdat_groups = true;
#endif
	}
      else
	{
	  /* We should've reclaimed all functions that are not needed.  */
#ifdef ENABLE_CHECKING
	  if (!node->global.inlined_to
	      && gimple_has_body_p (decl)
	      && !DECL_EXTERNAL (decl))
	    {
	      dump_cgraph_node (stderr, node);
	      internal_error ("failed to reclaim unneeded function");
	    }
#endif
	  gcc_assert (node->global.inlined_to
		      || !gimple_has_body_p (decl)
		      || DECL_EXTERNAL (decl));

	}

    }
#ifdef ENABLE_CHECKING
  if (check_same_comdat_groups)
    for (node = cgraph_nodes; node; node = node->next)
      if (node->same_comdat_group && !node->process)
	{
	  tree decl = node->decl;
	  if (!node->global.inlined_to
	      && gimple_has_body_p (decl)
	      && !DECL_EXTERNAL (decl))
	    {
	      dump_cgraph_node (stderr, node);
	      internal_error ("failed to reclaim unneeded function");
	    }
	}
#endif
}

/* DECL is FUNCTION_DECL.  Initialize datastructures so DECL is a function
   in lowered gimple form.
   
   Set current_function_decl and cfun to newly constructed empty function body.
   return basic block in the function body.  */

static basic_block
init_lowered_empty_function (tree decl)
{
  basic_block bb;

  current_function_decl = decl;
  allocate_struct_function (decl, false);
  gimple_register_cfg_hooks ();
  init_empty_tree_cfg ();
  init_tree_ssa (cfun);
  init_ssa_operands ();
  cfun->gimple_df->in_ssa_p = true;
  DECL_INITIAL (decl) = make_node (BLOCK);

  DECL_SAVED_TREE (decl) = error_mark_node;
  cfun->curr_properties |=
    (PROP_gimple_lcf | PROP_gimple_leh | PROP_cfg | PROP_referenced_vars |
     PROP_ssa);

  /* Create BB for body of the function and connect it properly.  */
  bb = create_basic_block (NULL, (void *) 0, ENTRY_BLOCK_PTR);
  make_edge (ENTRY_BLOCK_PTR, bb, 0);
  make_edge (bb, EXIT_BLOCK_PTR, 0);

  return bb;
}

/* Adjust PTR by the constant FIXED_OFFSET, and by the vtable
   offset indicated by VIRTUAL_OFFSET, if that is
   non-null. THIS_ADJUSTING is nonzero for a this adjusting thunk and
   zero for a result adjusting thunk.  */

static tree
thunk_adjust (gimple_stmt_iterator * bsi,
	      tree ptr, bool this_adjusting,
	      HOST_WIDE_INT fixed_offset, tree virtual_offset)
{
  gimple stmt;
  tree ret;

  if (this_adjusting
      && fixed_offset != 0)
    {
      stmt = gimple_build_assign (ptr,
				  fold_build2_loc (input_location,
						   POINTER_PLUS_EXPR,
						   TREE_TYPE (ptr), ptr,
						   size_int (fixed_offset)));
      gsi_insert_after (bsi, stmt, GSI_NEW_STMT);
    }

  /* If there's a virtual offset, look up that value in the vtable and
     adjust the pointer again.  */
  if (virtual_offset)
    {
      tree vtabletmp;
      tree vtabletmp2;
      tree vtabletmp3;
      tree offsettmp;

      if (!vtable_entry_type)
	{
	  tree vfunc_type = make_node (FUNCTION_TYPE);
	  TREE_TYPE (vfunc_type) = integer_type_node;
	  TYPE_ARG_TYPES (vfunc_type) = NULL_TREE;
	  layout_type (vfunc_type);

	  vtable_entry_type = build_pointer_type (vfunc_type);
	}

      vtabletmp =
	create_tmp_var (build_pointer_type
			(build_pointer_type (vtable_entry_type)), "vptr");

      /* The vptr is always at offset zero in the object.  */
      stmt = gimple_build_assign (vtabletmp,
				  build1 (NOP_EXPR, TREE_TYPE (vtabletmp),
					  ptr));
      gsi_insert_after (bsi, stmt, GSI_NEW_STMT);
      mark_symbols_for_renaming (stmt);
      find_referenced_vars_in (stmt);

      /* Form the vtable address.  */
      vtabletmp2 = create_tmp_var (TREE_TYPE (TREE_TYPE (vtabletmp)),
				   "vtableaddr");
      stmt = gimple_build_assign (vtabletmp2,
				  build1 (INDIRECT_REF,
					  TREE_TYPE (vtabletmp2), vtabletmp));
      gsi_insert_after (bsi, stmt, GSI_NEW_STMT);
      mark_symbols_for_renaming (stmt);
      find_referenced_vars_in (stmt);

      /* Find the entry with the vcall offset.  */
      stmt = gimple_build_assign (vtabletmp2,
				  fold_build2_loc (input_location,
						   POINTER_PLUS_EXPR,
						   TREE_TYPE (vtabletmp2),
						   vtabletmp2,
						   fold_convert (sizetype,
								 virtual_offset)));
      gsi_insert_after (bsi, stmt, GSI_NEW_STMT);

      /* Get the offset itself.  */
      vtabletmp3 = create_tmp_var (TREE_TYPE (TREE_TYPE (vtabletmp2)),
				   "vcalloffset");
      stmt = gimple_build_assign (vtabletmp3,
				  build1 (INDIRECT_REF,
					  TREE_TYPE (vtabletmp3),
					  vtabletmp2));
      gsi_insert_after (bsi, stmt, GSI_NEW_STMT);
      mark_symbols_for_renaming (stmt);
      find_referenced_vars_in (stmt);

      /* Cast to sizetype.  */
      offsettmp = create_tmp_var (sizetype, "offset");
      stmt = gimple_build_assign (offsettmp, fold_convert (sizetype, vtabletmp3));
      gsi_insert_after (bsi, stmt, GSI_NEW_STMT);
      mark_symbols_for_renaming (stmt);
      find_referenced_vars_in (stmt);

      /* Adjust the `this' pointer.  */
      ptr = fold_build2_loc (input_location,
			     POINTER_PLUS_EXPR, TREE_TYPE (ptr), ptr,
			     offsettmp);
    }

  if (!this_adjusting
      && fixed_offset != 0)
    /* Adjust the pointer by the constant.  */
    {
      tree ptrtmp;

      if (TREE_CODE (ptr) == VAR_DECL)
        ptrtmp = ptr;
      else
        {
          ptrtmp = create_tmp_var (TREE_TYPE (ptr), "ptr");
          stmt = gimple_build_assign (ptrtmp, ptr);
	  gsi_insert_after (bsi, stmt, GSI_NEW_STMT);
	  mark_symbols_for_renaming (stmt);
	  find_referenced_vars_in (stmt);
	}
      ptr = fold_build2_loc (input_location,
			     POINTER_PLUS_EXPR, TREE_TYPE (ptrtmp), ptrtmp,
			     size_int (fixed_offset));
    }

  /* Emit the statement and gimplify the adjustment expression.  */
  ret = create_tmp_var (TREE_TYPE (ptr), "adjusted_this");
  stmt = gimple_build_assign (ret, ptr);
  mark_symbols_for_renaming (stmt);
  find_referenced_vars_in (stmt);
  gsi_insert_after (bsi, stmt, GSI_NEW_STMT);

  return ret;
}

/* Produce assembler for thunk NODE.  */

static void
assemble_thunk (struct cgraph_node *node)
{
  bool this_adjusting = node->thunk.this_adjusting;
  HOST_WIDE_INT fixed_offset = node->thunk.fixed_offset;
  HOST_WIDE_INT virtual_value = node->thunk.virtual_value;
  tree virtual_offset = NULL;
  tree alias = node->thunk.alias;
  tree thunk_fndecl = node->decl;
  tree a = DECL_ARGUMENTS (thunk_fndecl);

  current_function_decl = thunk_fndecl;

  if (this_adjusting
      && targetm.asm_out.can_output_mi_thunk (thunk_fndecl, fixed_offset,
					      virtual_value, alias))
    {
      const char *fnname;
      tree fn_block;
      
      DECL_RESULT (thunk_fndecl)
	= build_decl (DECL_SOURCE_LOCATION (thunk_fndecl),
		      RESULT_DECL, 0, integer_type_node);
      fnname = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (thunk_fndecl));

      /* The back end expects DECL_INITIAL to contain a BLOCK, so we
	 create one.  */
      fn_block = make_node (BLOCK);
      BLOCK_VARS (fn_block) = a;
      DECL_INITIAL (thunk_fndecl) = fn_block;
      init_function_start (thunk_fndecl);
      cfun->is_thunk = 1;
      assemble_start_function (thunk_fndecl, fnname);

      targetm.asm_out.output_mi_thunk (asm_out_file, thunk_fndecl,
				       fixed_offset, virtual_value, alias);

      assemble_end_function (thunk_fndecl, fnname);
      init_insn_lengths ();
      free_after_compilation (cfun);
      set_cfun (NULL);
      TREE_ASM_WRITTEN (thunk_fndecl) = 1;
    }
  else
    {
      tree restype;
      basic_block bb, then_bb, else_bb, return_bb;
      gimple_stmt_iterator bsi;
      int nargs = 0;
      tree arg;
      int i;
      tree resdecl;
      tree restmp = NULL;
      VEC(tree, heap) *vargs;

      gimple call;
      gimple ret;

      DECL_IGNORED_P (thunk_fndecl) = 1;
      bitmap_obstack_initialize (NULL);

      if (node->thunk.virtual_offset_p)
        virtual_offset = size_int (virtual_value);

      /* Build the return declaration for the function.  */
      restype = TREE_TYPE (TREE_TYPE (thunk_fndecl));
      if (DECL_RESULT (thunk_fndecl) == NULL_TREE)
	{
	  resdecl = build_decl (input_location, RESULT_DECL, 0, restype);
	  DECL_ARTIFICIAL (resdecl) = 1;
	  DECL_IGNORED_P (resdecl) = 1;
	  DECL_RESULT (thunk_fndecl) = resdecl;
	}
      else
	resdecl = DECL_RESULT (thunk_fndecl);

      bb = then_bb = else_bb = return_bb = init_lowered_empty_function (thunk_fndecl);

      bsi = gsi_start_bb (bb);

      /* Build call to the function being thunked.  */
      if (!VOID_TYPE_P (restype))
	{
	  if (!is_gimple_reg_type (restype))
	    {
	      restmp = resdecl;
	      cfun->local_decls = tree_cons (NULL_TREE, restmp, cfun->local_decls);
	      BLOCK_VARS (DECL_INITIAL (current_function_decl)) = restmp;
	    }
	  else
            restmp = create_tmp_var_raw (restype, "retval");
	}

      for (arg = a; arg; arg = TREE_CHAIN (arg))
        nargs++;
      vargs = VEC_alloc (tree, heap, nargs);
      if (this_adjusting)
        VEC_quick_push (tree, vargs,
			thunk_adjust (&bsi,
				      a, 1, fixed_offset,
				      virtual_offset));
      else
        VEC_quick_push (tree, vargs, a);
      for (i = 1, arg = TREE_CHAIN (a); i < nargs; i++, arg = TREE_CHAIN (arg))
        VEC_quick_push (tree, vargs, arg);
      call = gimple_build_call_vec (build_fold_addr_expr_loc (0, alias), vargs);
      VEC_free (tree, heap, vargs);
      gimple_call_set_cannot_inline (call, true);
      gimple_call_set_from_thunk (call, true);
      if (restmp)
        gimple_call_set_lhs (call, restmp);
      gsi_insert_after (&bsi, call, GSI_NEW_STMT);
      mark_symbols_for_renaming (call);
      find_referenced_vars_in (call);
      update_stmt (call);

      if (restmp && !this_adjusting)
        {
	  tree true_label = NULL_TREE;

	  if (TREE_CODE (TREE_TYPE (restmp)) == POINTER_TYPE)
	    {
	      gimple stmt;
	      /* If the return type is a pointer, we need to
		 protect against NULL.  We know there will be an
		 adjustment, because that's why we're emitting a
		 thunk.  */
	      then_bb = create_basic_block (NULL, (void *) 0, bb);
	      return_bb = create_basic_block (NULL, (void *) 0, then_bb);
	      else_bb = create_basic_block (NULL, (void *) 0, else_bb);
	      remove_edge (single_succ_edge (bb));
	      true_label = gimple_block_label (then_bb);
	      stmt = gimple_build_cond (NE_EXPR, restmp,
	      				fold_convert (TREE_TYPE (restmp),
						      integer_zero_node),
	      			        NULL_TREE, NULL_TREE);
	      gsi_insert_after (&bsi, stmt, GSI_NEW_STMT);
	      make_edge (bb, then_bb, EDGE_TRUE_VALUE);
	      make_edge (bb, else_bb, EDGE_FALSE_VALUE);
	      make_edge (return_bb, EXIT_BLOCK_PTR, 0);
	      make_edge (then_bb, return_bb, EDGE_FALLTHRU);
	      make_edge (else_bb, return_bb, EDGE_FALLTHRU);
	      bsi = gsi_last_bb (then_bb);
	    }

	  restmp = thunk_adjust (&bsi, restmp, /*this_adjusting=*/0,
			         fixed_offset, virtual_offset);
	  if (true_label)
	    {
	      gimple stmt;
	      bsi = gsi_last_bb (else_bb);
	      stmt = gimple_build_assign (restmp, fold_convert (TREE_TYPE (restmp),
								integer_zero_node));
	      gsi_insert_after (&bsi, stmt, GSI_NEW_STMT);
	      bsi = gsi_last_bb (return_bb);
	    }
	}
      else
        gimple_call_set_tail (call, true);

      /* Build return value.  */
      ret = gimple_build_return (restmp);
      gsi_insert_after (&bsi, ret, GSI_NEW_STMT);

      delete_unreachable_blocks ();
      update_ssa (TODO_update_ssa);

      cgraph_remove_same_body_alias (node);
      /* Since we want to emit the thunk, we explicitly mark its name as
	 referenced.  */
      mark_decl_referenced (thunk_fndecl);
      cgraph_add_new_function (thunk_fndecl, true);
      bitmap_obstack_release (NULL);
    }
  current_function_decl = NULL;
}

/* Expand function specified by NODE.  */

static void
cgraph_expand_function (struct cgraph_node *node)
{
  tree decl = node->decl;

  /* We ought to not compile any inline clones.  */
  gcc_assert (!node->global.inlined_to);

  announce_function (decl);
  node->process = 0;

  gcc_assert (node->lowered);

  /* Generate RTL for the body of DECL.  */
  tree_rest_of_compilation (decl);

  /* Make sure that BE didn't give up on compiling.  */
  gcc_assert (TREE_ASM_WRITTEN (decl));
  current_function_decl = NULL;
  if (node->same_body)
    {
      struct cgraph_node *alias, *next;
      bool saved_alias = node->alias;
      for (alias = node->same_body;
      	   alias && alias->next; alias = alias->next)
        ;
      /* Walk aliases in the order they were created; it is possible that
         thunks reffers to the aliases made earlier.  */
      for (; alias; alias = next)
        {
	  next = alias->previous;
	  if (!alias->thunk.thunk_p)
	    assemble_alias (alias->decl,
			    DECL_ASSEMBLER_NAME (alias->thunk.alias));
	  else
	    assemble_thunk (alias);
	}
      node->alias = saved_alias;
    }
  gcc_assert (!cgraph_preserve_function_body_p (decl));
  cgraph_release_function_body (node);
  /* Eliminate all call edges.  This is important so the GIMPLE_CALL no longer
     points to the dead function body.  */
  cgraph_node_remove_callees (node);

  cgraph_function_flags_ready = true;
}

/* Return true when CALLER_DECL should be inlined into CALLEE_DECL.  */

bool
cgraph_inline_p (struct cgraph_edge *e, cgraph_inline_failed_t *reason)
{
  *reason = e->inline_failed;
  return !e->inline_failed;
}



/* Expand all functions that must be output.

   Attempt to topologically sort the nodes so function is output when
   all called functions are already assembled to allow data to be
   propagated across the callgraph.  Use a stack to get smaller distance
   between a function and its callees (later we may choose to use a more
   sophisticated algorithm for function reordering; we will likely want
   to use subsections to make the output functions appear in top-down
   order).  */

static void
cgraph_expand_all_functions (void)
{
  struct cgraph_node *node;
  struct cgraph_node **order = XCNEWVEC (struct cgraph_node *, cgraph_n_nodes);
  int order_pos, new_order_pos = 0;
  int i;

  order_pos = cgraph_postorder (order);
  gcc_assert (order_pos == cgraph_n_nodes);

  /* Garbage collector may remove inline clones we eliminate during
     optimization.  So we must be sure to not reference them.  */
  for (i = 0; i < order_pos; i++)
    if (order[i]->process)
      order[new_order_pos++] = order[i];

  for (i = new_order_pos - 1; i >= 0; i--)
    {
      node = order[i];
      if (node->process)
	{
	  gcc_assert (node->reachable);
	  node->process = 0;
	  cgraph_expand_function (node);
	}
    }
  cgraph_process_new_functions ();

  free (order);

}

/* This is used to sort the node types by the cgraph order number.  */

enum cgraph_order_sort_kind
{
  ORDER_UNDEFINED = 0,
  ORDER_FUNCTION,
  ORDER_VAR,
  ORDER_ASM
};

struct cgraph_order_sort
{
  enum cgraph_order_sort_kind kind;
  union
  {
    struct cgraph_node *f;
    struct varpool_node *v;
    struct cgraph_asm_node *a;
  } u;
};

/* Output all functions, variables, and asm statements in the order
   according to their order fields, which is the order in which they
   appeared in the file.  This implements -fno-toplevel-reorder.  In
   this mode we may output functions and variables which don't really
   need to be output.  */

static void
cgraph_output_in_order (void)
{
  int max;
  struct cgraph_order_sort *nodes;
  int i;
  struct cgraph_node *pf;
  struct varpool_node *pv;
  struct cgraph_asm_node *pa;

  max = cgraph_order;
  nodes = XCNEWVEC (struct cgraph_order_sort, max);

  varpool_analyze_pending_decls ();

  for (pf = cgraph_nodes; pf; pf = pf->next)
    {
      if (pf->process)
	{
	  i = pf->order;
	  gcc_assert (nodes[i].kind == ORDER_UNDEFINED);
	  nodes[i].kind = ORDER_FUNCTION;
	  nodes[i].u.f = pf;
	}
    }

  for (pv = varpool_nodes_queue; pv; pv = pv->next_needed)
    {
      i = pv->order;
      gcc_assert (nodes[i].kind == ORDER_UNDEFINED);
      nodes[i].kind = ORDER_VAR;
      nodes[i].u.v = pv;
    }

  for (pa = cgraph_asm_nodes; pa; pa = pa->next)
    {
      i = pa->order;
      gcc_assert (nodes[i].kind == ORDER_UNDEFINED);
      nodes[i].kind = ORDER_ASM;
      nodes[i].u.a = pa;
    }

  /* In toplevel reorder mode we output all statics; mark them as needed.  */
  for (i = 0; i < max; ++i)
    {
      if (nodes[i].kind == ORDER_VAR)
        {
	  varpool_mark_needed_node (nodes[i].u.v);
	}
    }
  varpool_empty_needed_queue ();

  for (i = 0; i < max; ++i)
    {
      switch (nodes[i].kind)
	{
	case ORDER_FUNCTION:
	  nodes[i].u.f->process = 0;
	  cgraph_expand_function (nodes[i].u.f);
	  break;

	case ORDER_VAR:
	  varpool_assemble_decl (nodes[i].u.v);
	  break;

	case ORDER_ASM:
	  assemble_asm (nodes[i].u.a->asm_str);
	  break;

	case ORDER_UNDEFINED:
	  break;

	default:
	  gcc_unreachable ();
	}
    }

  cgraph_asm_nodes = NULL;
  free (nodes);
}

/* Return true when function body of DECL still needs to be kept around
   for later re-use.  */
bool
cgraph_preserve_function_body_p (tree decl)
{
  struct cgraph_node *node;

  gcc_assert (cgraph_global_info_ready);
  /* Look if there is any clone around.  */
  node = cgraph_node (decl);
  if (node->clones)
    return true;
  return false;
}

static void
ipa_passes (void)
{
  set_cfun (NULL);
  current_function_decl = NULL;
  gimple_register_cfg_hooks ();
  bitmap_obstack_initialize (NULL);

  invoke_plugin_callbacks (PLUGIN_ALL_IPA_PASSES_START, NULL);

  if (!in_lto_p)
    execute_ipa_pass_list (all_small_ipa_passes);

  /* If pass_all_early_optimizations was not scheduled, the state of
     the cgraph will not be properly updated.  Update it now.  */
  if (cgraph_state < CGRAPH_STATE_IPA_SSA)
    cgraph_state = CGRAPH_STATE_IPA_SSA;

  if (!in_lto_p)
    {
      /* Generate coverage variables and constructors.  */
      coverage_finish ();

      /* Process new functions added.  */
      set_cfun (NULL);
      current_function_decl = NULL;
      cgraph_process_new_functions ();

      execute_ipa_summary_passes
	((struct ipa_opt_pass_d *) all_regular_ipa_passes);
    }

  /* Some targets need to handle LTO assembler output specially.  */
  if (flag_generate_lto)
    targetm.asm_out.lto_start ();

  execute_ipa_summary_passes ((struct ipa_opt_pass_d *) all_lto_gen_passes);

  if (!in_lto_p)
    ipa_write_summaries ();

  if (flag_generate_lto)
    targetm.asm_out.lto_end ();

  if (!flag_ltrans)
    execute_ipa_pass_list (all_regular_ipa_passes);
  invoke_plugin_callbacks (PLUGIN_ALL_IPA_PASSES_END, NULL);

  bitmap_obstack_release (NULL);
}


/* Perform simple optimizations based on callgraph.  */

void
cgraph_optimize (void)
{
  if (errorcount || sorrycount)
    return;

#ifdef ENABLE_CHECKING
  verify_cgraph ();
#endif

  /* Frontend may output common variables after the unit has been finalized.
     It is safe to deal with them here as they are always zero initialized.  */
  varpool_analyze_pending_decls ();

  timevar_push (TV_CGRAPHOPT);
  if (pre_ipa_mem_report)
    {
      fprintf (stderr, "Memory consumption before IPA\n");
      dump_memory_report (false);
    }
  if (!quiet_flag)
    fprintf (stderr, "Performing interprocedural optimizations\n");
  cgraph_state = CGRAPH_STATE_IPA;

  /* Don't run the IPA passes if there was any error or sorry messages.  */
  if (errorcount == 0 && sorrycount == 0)
    ipa_passes ();

  /* Do nothing else if any IPA pass found errors.  */
  if (errorcount || sorrycount)
    {
      timevar_pop (TV_CGRAPHOPT);
      return;
    }

  /* This pass remove bodies of extern inline functions we never inlined.
     Do this later so other IPA passes see what is really going on.  */
  cgraph_remove_unreachable_nodes (false, dump_file);
  cgraph_global_info_ready = true;
  if (cgraph_dump_file)
    {
      fprintf (cgraph_dump_file, "Optimized ");
      dump_cgraph (cgraph_dump_file);
      dump_varpool (cgraph_dump_file);
    }
  if (post_ipa_mem_report)
    {
      fprintf (stderr, "Memory consumption after IPA\n");
      dump_memory_report (false);
    }
  timevar_pop (TV_CGRAPHOPT);

  /* Output everything.  */
  (*debug_hooks->assembly_start) ();
  if (!quiet_flag)
    fprintf (stderr, "Assembling functions:\n");
#ifdef ENABLE_CHECKING
  verify_cgraph ();
#endif

  cgraph_materialize_all_clones ();
  cgraph_mark_functions_to_output ();

  cgraph_state = CGRAPH_STATE_EXPANSION;
  if (!flag_toplevel_reorder)
    cgraph_output_in_order ();
  else
    {
      cgraph_output_pending_asms ();

      cgraph_expand_all_functions ();
      varpool_remove_unreferenced_decls ();

      varpool_assemble_pending_decls ();
    }
  cgraph_process_new_functions ();
  cgraph_state = CGRAPH_STATE_FINISHED;

  if (cgraph_dump_file)
    {
      fprintf (cgraph_dump_file, "\nFinal ");
      dump_cgraph (cgraph_dump_file);
    }
#ifdef ENABLE_CHECKING
  verify_cgraph ();
  /* Double check that all inline clones are gone and that all
     function bodies have been released from memory.  */
  if (!(sorrycount || errorcount))
    {
      struct cgraph_node *node;
      bool error_found = false;

      for (node = cgraph_nodes; node; node = node->next)
	if (node->analyzed
	    && (node->global.inlined_to
		|| gimple_has_body_p (node->decl)))
	  {
	    error_found = true;
	    dump_cgraph_node (stderr, node);
	  }
      if (error_found)
	internal_error ("nodes with unreleased memory found");
    }
#endif
}


/* Generate and emit a static constructor or destructor.  WHICH must
   be one of 'I' (for a constructor) or 'D' (for a destructor).  BODY
   is a STATEMENT_LIST containing GENERIC statements.  PRIORITY is the
   initialization priority for this constructor or destructor.  */

void
cgraph_build_static_cdtor (char which, tree body, int priority)
{
  static int counter = 0;
  char which_buf[16];
  tree decl, name, resdecl;

  /* The priority is encoded in the constructor or destructor name.
     collect2 will sort the names and arrange that they are called at
     program startup.  */
  sprintf (which_buf, "%c_%.5d_%d", which, priority, counter++);
  name = get_file_function_name (which_buf);

  decl = build_decl (input_location, FUNCTION_DECL, name,
		     build_function_type (void_type_node, void_list_node));
  current_function_decl = decl;

  resdecl = build_decl (input_location,
			RESULT_DECL, NULL_TREE, void_type_node);
  DECL_ARTIFICIAL (resdecl) = 1;
  DECL_RESULT (decl) = resdecl;
  DECL_CONTEXT (resdecl) = decl;

  allocate_struct_function (decl, false);

  TREE_STATIC (decl) = 1;
  TREE_USED (decl) = 1;
  DECL_ARTIFICIAL (decl) = 1;
  DECL_NO_INSTRUMENT_FUNCTION_ENTRY_EXIT (decl) = 1;
  DECL_SAVED_TREE (decl) = body;
  if (!targetm.have_ctors_dtors)
    {
      TREE_PUBLIC (decl) = 1;
      DECL_PRESERVE_P (decl) = 1;
    }
  DECL_UNINLINABLE (decl) = 1;

  DECL_INITIAL (decl) = make_node (BLOCK);
  TREE_USED (DECL_INITIAL (decl)) = 1;

  DECL_SOURCE_LOCATION (decl) = input_location;
  cfun->function_end_locus = input_location;

  switch (which)
    {
    case 'I':
      DECL_STATIC_CONSTRUCTOR (decl) = 1;
      decl_init_priority_insert (decl, priority);
      break;
    case 'D':
      DECL_STATIC_DESTRUCTOR (decl) = 1;
      decl_fini_priority_insert (decl, priority);
      break;
    default:
      gcc_unreachable ();
    }

  gimplify_function_tree (decl);

  cgraph_add_new_function (decl, false);
  cgraph_mark_needed_node (cgraph_node (decl));
  set_cfun (NULL);
}

void
init_cgraph (void)
{
  cgraph_dump_file = dump_begin (TDI_cgraph, NULL);
}

/* The edges representing the callers of the NEW_VERSION node were
   fixed by cgraph_function_versioning (), now the call_expr in their
   respective tree code should be updated to call the NEW_VERSION.  */

static void
update_call_expr (struct cgraph_node *new_version)
{
  struct cgraph_edge *e;

  gcc_assert (new_version);

  /* Update the call expr on the edges to call the new version.  */
  for (e = new_version->callers; e; e = e->next_caller)
    {
      struct function *inner_function = DECL_STRUCT_FUNCTION (e->caller->decl);
      gimple_call_set_fndecl (e->call_stmt, new_version->decl);
      maybe_clean_eh_stmt_fn (inner_function, e->call_stmt);
    }
}


/* Create a new cgraph node which is the new version of
   OLD_VERSION node.  REDIRECT_CALLERS holds the callers
   edges which should be redirected to point to
   NEW_VERSION.  ALL the callees edges of OLD_VERSION
   are cloned to the new version node.  Return the new
   version node.  */

static struct cgraph_node *
cgraph_copy_node_for_versioning (struct cgraph_node *old_version,
				 tree new_decl,
				 VEC(cgraph_edge_p,heap) *redirect_callers)
 {
   struct cgraph_node *new_version;
   struct cgraph_edge *e, *new_e;
   struct cgraph_edge *next_callee;
   unsigned i;

   gcc_assert (old_version);

   new_version = cgraph_node (new_decl);

   new_version->analyzed = true;
   new_version->local = old_version->local;
   new_version->global = old_version->global;
   new_version->rtl = new_version->rtl;
   new_version->reachable = true;
   new_version->count = old_version->count;

   /* Clone the old node callees.  Recursive calls are
      also cloned.  */
   for (e = old_version->callees;e; e=e->next_callee)
     {
       new_e = cgraph_clone_edge (e, new_version, e->call_stmt,
				  e->lto_stmt_uid, 0, e->frequency,
				  e->loop_nest, true);
       new_e->count = e->count;
     }
   /* Fix recursive calls.
      If OLD_VERSION has a recursive call after the
      previous edge cloning, the new version will have an edge
      pointing to the old version, which is wrong;
      Redirect it to point to the new version. */
   for (e = new_version->callees ; e; e = next_callee)
     {
       next_callee = e->next_callee;
       if (e->callee == old_version)
	 cgraph_redirect_edge_callee (e, new_version);

       if (!next_callee)
	 break;
     }
   for (i = 0; VEC_iterate (cgraph_edge_p, redirect_callers, i, e); i++)
     {
       /* Redirect calls to the old version node to point to its new
	  version.  */
       cgraph_redirect_edge_callee (e, new_version);
     }

   return new_version;
 }

 /* Perform function versioning.
    Function versioning includes copying of the tree and
    a callgraph update (creating a new cgraph node and updating
    its callees and callers).

    REDIRECT_CALLERS varray includes the edges to be redirected
    to the new version.

    TREE_MAP is a mapping of tree nodes we want to replace with
    new ones (according to results of prior analysis).
    OLD_VERSION_NODE is the node that is versioned.
    It returns the new version's cgraph node.
    ARGS_TO_SKIP lists arguments to be omitted from functions
    */

struct cgraph_node *
cgraph_function_versioning (struct cgraph_node *old_version_node,
			    VEC(cgraph_edge_p,heap) *redirect_callers,
			    VEC (ipa_replace_map_p,gc)* tree_map,
			    bitmap args_to_skip)
{
  tree old_decl = old_version_node->decl;
  struct cgraph_node *new_version_node = NULL;
  tree new_decl;

  if (!tree_versionable_function_p (old_decl))
    return NULL;

  /* Make a new FUNCTION_DECL tree node for the
     new version. */
  if (!args_to_skip)
    new_decl = copy_node (old_decl);
  else
    new_decl = build_function_decl_skip_args (old_decl, args_to_skip);

  /* Generate a new name for the new version. */
  DECL_NAME (new_decl) = clone_function_name (old_decl);
  SET_DECL_ASSEMBLER_NAME (new_decl, DECL_NAME (new_decl));
  SET_DECL_RTL (new_decl, NULL);

  /* Create the new version's call-graph node.
     and update the edges of the new node. */
  new_version_node =
    cgraph_copy_node_for_versioning (old_version_node, new_decl,
				     redirect_callers);

  /* Copy the OLD_VERSION_NODE function tree to the new version.  */
  tree_function_versioning (old_decl, new_decl, tree_map, false, args_to_skip);

  /* Update the new version's properties.
     Make The new version visible only within this translation unit.  Make sure
     that is not weak also.
     ??? We cannot use COMDAT linkage because there is no
     ABI support for this.  */
  cgraph_make_decl_local (new_version_node->decl);
  DECL_VIRTUAL_P (new_version_node->decl) = 0;
  new_version_node->local.externally_visible = 0;
  new_version_node->local.local = 1;
  new_version_node->lowered = true;

  /* Update the call_expr on the edges to call the new version node. */
  update_call_expr (new_version_node);

  cgraph_call_function_insertion_hooks (new_version_node);
  return new_version_node;
}

/* Produce separate function body for inline clones so the offline copy can be
   modified without affecting them.  */
struct cgraph_node *
save_inline_function_body (struct cgraph_node *node)
{
  struct cgraph_node *first_clone, *n;

  gcc_assert (node == cgraph_node (node->decl));

  cgraph_lower_function (node);

  first_clone = node->clones;

  first_clone->decl = copy_node (node->decl);
  cgraph_insert_node_to_hashtable (first_clone);
  gcc_assert (first_clone == cgraph_node (first_clone->decl));
  if (first_clone->next_sibling_clone)
    {
      for (n = first_clone->next_sibling_clone; n->next_sibling_clone; n = n->next_sibling_clone)
        n->clone_of = first_clone;
      n->clone_of = first_clone;
      n->next_sibling_clone = first_clone->clones;
      if (first_clone->clones)
        first_clone->clones->prev_sibling_clone = n;
      first_clone->clones = first_clone->next_sibling_clone;
      first_clone->next_sibling_clone->prev_sibling_clone = NULL;
      first_clone->next_sibling_clone = NULL;
      gcc_assert (!first_clone->prev_sibling_clone);
    }
  first_clone->clone_of = NULL;
  node->clones = NULL;

  if (first_clone->clones)
    for (n = first_clone->clones; n != first_clone;)
      {
        gcc_assert (n->decl == node->decl);
	n->decl = first_clone->decl;
	if (n->clones)
	  n = n->clones;
	else if (n->next_sibling_clone)
	  n = n->next_sibling_clone;
	else
	  {
	    while (n != first_clone && !n->next_sibling_clone)
	      n = n->clone_of;
	    if (n != first_clone)
	      n = n->next_sibling_clone;
	  }
      }

  /* Copy the OLD_VERSION_NODE function tree to the new version.  */
  tree_function_versioning (node->decl, first_clone->decl, NULL, true, NULL);

  DECL_EXTERNAL (first_clone->decl) = 0;
  DECL_COMDAT_GROUP (first_clone->decl) = NULL_TREE;
  TREE_PUBLIC (first_clone->decl) = 0;
  DECL_COMDAT (first_clone->decl) = 0;
  VEC_free (ipa_opt_pass, heap,
            first_clone->ipa_transforms_to_apply);
  first_clone->ipa_transforms_to_apply = NULL;

#ifdef ENABLE_CHECKING
  verify_cgraph_node (first_clone);
#endif
  return first_clone;
}

/* Given virtual clone, turn it into actual clone.  */
static void
cgraph_materialize_clone (struct cgraph_node *node)
{
  bitmap_obstack_initialize (NULL);
  /* Copy the OLD_VERSION_NODE function tree to the new version.  */
  tree_function_versioning (node->clone_of->decl, node->decl,
  			    node->clone.tree_map, true,
			    node->clone.args_to_skip);
  if (cgraph_dump_file)
    {
      dump_function_to_file (node->clone_of->decl, cgraph_dump_file, dump_flags);
      dump_function_to_file (node->decl, cgraph_dump_file, dump_flags);
    }

  /* Function is no longer clone.  */
  if (node->next_sibling_clone)
    node->next_sibling_clone->prev_sibling_clone = node->prev_sibling_clone;
  if (node->prev_sibling_clone)
    node->prev_sibling_clone->next_sibling_clone = node->next_sibling_clone;
  else
    node->clone_of->clones = node->next_sibling_clone;
  node->next_sibling_clone = NULL;
  node->prev_sibling_clone = NULL;
  if (!node->clone_of->analyzed && !node->clone_of->clones)
    {
      cgraph_release_function_body (node->clone_of);
      cgraph_node_remove_callees (node->clone_of);
    }
  node->clone_of = NULL;
  bitmap_obstack_release (NULL);
}

/* If necessary, change the function declaration in the call statement
   associated with E so that it corresponds to the edge callee.  */

gimple
cgraph_redirect_edge_call_stmt_to_callee (struct cgraph_edge *e)
{
  tree decl = gimple_call_fndecl (e->call_stmt);
  gimple new_stmt;

  if (!decl || decl == e->callee->decl
      /* Don't update call from same body alias to the real function.  */
      || cgraph_get_node (decl) == cgraph_get_node (e->callee->decl))
    return e->call_stmt;

  if (cgraph_dump_file)
    {
      fprintf (cgraph_dump_file, "updating call of %s/%i -> %s/%i: ",
	       cgraph_node_name (e->caller), e->caller->uid,
	       cgraph_node_name (e->callee), e->callee->uid);
      print_gimple_stmt (cgraph_dump_file, e->call_stmt, 0, dump_flags);
    }

  if (e->callee->clone.combined_args_to_skip)
    {
      gimple_stmt_iterator gsi;

      new_stmt
	= gimple_call_copy_skip_args (e->call_stmt,
				      e->callee->clone.combined_args_to_skip);

      if (gimple_vdef (new_stmt)
	  && TREE_CODE (gimple_vdef (new_stmt)) == SSA_NAME)
	SSA_NAME_DEF_STMT (gimple_vdef (new_stmt)) = new_stmt;

      gsi = gsi_for_stmt (e->call_stmt);
      gsi_replace (&gsi, new_stmt, true);
    }
  else
    new_stmt = e->call_stmt;

  gimple_call_set_fndecl (new_stmt, e->callee->decl);

  cgraph_set_call_stmt_including_clones (e->caller, e->call_stmt, new_stmt);

  if (cgraph_dump_file)
    {
      fprintf (cgraph_dump_file, "  updated to:");
      print_gimple_stmt (cgraph_dump_file, e->call_stmt, 0, dump_flags);
    }
  return new_stmt;
}

/* Once all functions from compilation unit are in memory, produce all clones
   and update all calls.  We might also do this on demand if we don't want to
   bring all functions to memory prior compilation, but current WHOPR
   implementation does that and it is is bit easier to keep everything right in
   this order.  */
void
cgraph_materialize_all_clones (void)
{
  struct cgraph_node *node;
  bool stabilized = false;

  if (cgraph_dump_file)
    fprintf (cgraph_dump_file, "Materializing clones\n");
#ifdef ENABLE_CHECKING
  verify_cgraph ();
#endif

  /* We can also do topological order, but number of iterations should be
     bounded by number of IPA passes since single IPA pass is probably not
     going to create clones of clones it created itself.  */
  while (!stabilized)
    {
      stabilized = true;
      for (node = cgraph_nodes; node; node = node->next)
        {
	  if (node->clone_of && node->decl != node->clone_of->decl
	      && !gimple_has_body_p (node->decl))
	    {
	      if (gimple_has_body_p (node->clone_of->decl))
	        {
		  if (cgraph_dump_file)
		    {
		      fprintf (cgraph_dump_file, "clonning %s to %s\n",
			       cgraph_node_name (node->clone_of),
			       cgraph_node_name (node));
		      if (node->clone.tree_map)
		        {
			  unsigned int i;
		          fprintf (cgraph_dump_file, "   replace map: ");
			  for (i = 0; i < VEC_length (ipa_replace_map_p,
			  			      node->clone.tree_map);
						      i++)
			    {
			      struct ipa_replace_map *replace_info;
			      replace_info = VEC_index (ipa_replace_map_p,
			      				node->clone.tree_map,
							i);
			      print_generic_expr (cgraph_dump_file, replace_info->old_tree, 0);
			      fprintf (cgraph_dump_file, " -> ");
			      print_generic_expr (cgraph_dump_file, replace_info->new_tree, 0);
			      fprintf (cgraph_dump_file, "%s%s;",
			      	       replace_info->replace_p ? "(replace)":"",
				       replace_info->ref_p ? "(ref)":"");
			    }
			  fprintf (cgraph_dump_file, "\n");
			}
		      if (node->clone.args_to_skip)
			{
		          fprintf (cgraph_dump_file, "   args_to_skip: ");
		          dump_bitmap (cgraph_dump_file, node->clone.args_to_skip);
			}
		      if (node->clone.args_to_skip)
			{
		          fprintf (cgraph_dump_file, "   combined_args_to_skip:");
		          dump_bitmap (cgraph_dump_file, node->clone.combined_args_to_skip);
			}
		    }
		  cgraph_materialize_clone (node);
	        }
	      else
		stabilized = false;
	    }
	}
    }
  for (node = cgraph_nodes; node; node = node->next)
    if (!node->analyzed && node->callees)
      cgraph_node_remove_callees (node);
  if (cgraph_dump_file)
    fprintf (cgraph_dump_file, "Materialization Call site updates done.\n");
#ifdef ENABLE_CHECKING
  verify_cgraph ();
#endif
  cgraph_remove_unreachable_nodes (false, cgraph_dump_file);
}

#include "gt-cgraphunit.h"
