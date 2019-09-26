/* Detection of Static Control Parts (SCoP) for Graphite.
   Copyright (C) 2009-2017 Free Software Foundation, Inc.
   Contributed by Sebastian Pop <sebastian.pop@amd.com> and
   Tobias Grosser <grosser@fim.uni-passau.de>.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#define USES_ISL

#include "config.h"

#ifdef HAVE_isl

#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "cfghooks.h"
#include "domwalk.h"
#include "params.h"
#include "tree.h"
#include "gimple.h"
#include "ssa.h"
#include "fold-const.h"
#include "gimple-iterator.h"
#include "tree-cfg.h"
#include "tree-ssa-loop-manip.h"
#include "tree-ssa-loop-niter.h"
#include "tree-ssa-loop.h"
#include "tree-into-ssa.h"
#include "tree-ssa.h"
#include "cfgloop.h"
#include "tree-data-ref.h"
#include "tree-scalar-evolution.h"
#include "tree-pass.h"
#include "tree-ssa-propagate.h"
#include "gimple-pretty-print.h"
#include "graphite.h"

class debug_printer
{
private:
  FILE *dump_file;

public:
  void
  set_dump_file (FILE *f)
  {
    gcc_assert (f);
    dump_file = f;
  }

  friend debug_printer &
  operator<< (debug_printer &output, int i)
  {
    fprintf (output.dump_file, "%d", i);
    return output;
  }
  friend debug_printer &
  operator<< (debug_printer &output, const char *s)
  {
    fprintf (output.dump_file, "%s", s);
    return output;
  }
} dp;

#define DEBUG_PRINT(args) do \
    {								\
      if (dump_file && (dump_flags & TDF_DETAILS)) { args; }	\
    } while (0);

/* Pretty print to FILE all the SCoPs in DOT format and mark them with
   different colors.  If there are not enough colors, paint the
   remaining SCoPs in gray.

   Special nodes:
   - "*" after the node number denotes the entry of a SCoP,
   - "#" after the node number denotes the exit of a SCoP,
   - "()" around the node number denotes the entry or the
     exit nodes of the SCOP.  These are not part of SCoP.  */

DEBUG_FUNCTION void
dot_all_sese (FILE *file, vec<sese_l>& scops)
{
  /* Disable debugging while printing graph.  */
  int tmp_dump_flags = dump_flags;
  dump_flags = 0;

  fprintf (file, "digraph all {\n");

  basic_block bb;
  FOR_ALL_BB_FN (bb, cfun)
    {
      int part_of_scop = false;

      /* Use HTML for every bb label.  So we are able to print bbs
	 which are part of two different SCoPs, with two different
	 background colors.  */
      fprintf (file, "%d [label=<\n  <TABLE BORDER=\"0\" CELLBORDER=\"1\" ",
	       bb->index);
      fprintf (file, "CELLSPACING=\"0\">\n");

      /* Select color for SCoP.  */
      sese_l *region;
      int i;
      FOR_EACH_VEC_ELT (scops, i, region)
	{
	  bool sese_in_region = bb_in_sese_p (bb, *region);
	  if (sese_in_region || (region->exit->dest == bb)
	      || (region->entry->dest == bb))
	    {
	      const char *color;
	      switch (i % 17)
		{
		case 0: /* red */
		  color = "#e41a1c";
		  break;
		case 1: /* blue */
		  color = "#377eb8";
		  break;
		case 2: /* green */
		  color = "#4daf4a";
		  break;
		case 3: /* purple */
		  color = "#984ea3";
		  break;
		case 4: /* orange */
		  color = "#ff7f00";
		  break;
		case 5: /* yellow */
		  color = "#ffff33";
		  break;
		case 6: /* brown */
		  color = "#a65628";
		  break;
		case 7: /* rose */
		  color = "#f781bf";
		  break;
		case 8:
		  color = "#8dd3c7";
		  break;
		case 9:
		  color = "#ffffb3";
		  break;
		case 10:
		  color = "#bebada";
		  break;
		case 11:
		  color = "#fb8072";
		  break;
		case 12:
		  color = "#80b1d3";
		  break;
		case 13:
		  color = "#fdb462";
		  break;
		case 14:
		  color = "#b3de69";
		  break;
		case 15:
		  color = "#fccde5";
		  break;
		case 16:
		  color = "#bc80bd";
		  break;
		default: /* gray */
		  color = "#999999";
		}

	      fprintf (file, "    <TR><TD WIDTH=\"50\" BGCOLOR=\"%s\">",
		       color);

	      if (!sese_in_region)
		fprintf (file, " (");

	      if (bb == region->entry->dest && bb == region->exit->dest)
		fprintf (file, " %d*# ", bb->index);
	      else if (bb == region->entry->dest)
		fprintf (file, " %d* ", bb->index);
	      else if (bb == region->exit->dest)
		fprintf (file, " %d# ", bb->index);
	      else
		fprintf (file, " %d ", bb->index);

	      fprintf (file, "{lp_%d}", bb->loop_father->num);

	      if (!sese_in_region)
		fprintf (file, ")");

	      fprintf (file, "</TD></TR>\n");
	      part_of_scop = true;
	    }
	}

	if (!part_of_scop)
	  {
	    fprintf (file, "    <TR><TD WIDTH=\"50\" BGCOLOR=\"#ffffff\">");
	    fprintf (file, " %d {lp_%d} </TD></TR>\n", bb->index,
		     bb->loop_father->num);
	  }
	fprintf (file, "  </TABLE>>, shape=box, style=\"setlinewidth(0)\"]\n");
    }

    FOR_ALL_BB_FN (bb, cfun)
      {
	edge e;
	edge_iterator ei;
	FOR_EACH_EDGE (e, ei, bb->succs)
	  fprintf (file, "%d -> %d;\n", bb->index, e->dest->index);
      }

  fputs ("}\n\n", file);

  /* Enable debugging again.  */
  dump_flags = tmp_dump_flags;
}

/* Display SCoP on stderr.  */

DEBUG_FUNCTION void
dot_sese (sese_l& scop)
{
  vec<sese_l> scops;
  scops.create (1);

  if (scop)
    scops.safe_push (scop);

  dot_all_sese (stderr, scops);

  scops.release ();
}

DEBUG_FUNCTION void
dot_cfg ()
{
  vec<sese_l> scops;
  scops.create (1);
  dot_all_sese (stderr, scops);
  scops.release ();
}

/* Return true if BB is empty, contains only DEBUG_INSNs.  */

static bool
trivially_empty_bb_p (basic_block bb)
{
  gimple_stmt_iterator gsi;

  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    if (gimple_code (gsi_stmt (gsi)) != GIMPLE_DEBUG)
      return false;

  return true;
}

/* Returns true when P1 and P2 are close phis with the same
   argument.  */

static inline bool
same_close_phi_node (gphi *p1, gphi *p2)
{
  return (types_compatible_p (TREE_TYPE (gimple_phi_result (p1)),
			      TREE_TYPE (gimple_phi_result (p2)))
	  && operand_equal_p (gimple_phi_arg_def (p1, 0),
			      gimple_phi_arg_def (p2, 0), 0));
}

static void make_close_phi_nodes_unique (basic_block bb);

/* Remove the close phi node at GSI and replace its rhs with the rhs
   of PHI.  */

static void
remove_duplicate_close_phi (gphi *phi, gphi_iterator *gsi)
{
  gimple *use_stmt;
  use_operand_p use_p;
  imm_use_iterator imm_iter;
  tree res = gimple_phi_result (phi);
  tree def = gimple_phi_result (gsi->phi ());

  gcc_assert (same_close_phi_node (phi, gsi->phi ()));

  FOR_EACH_IMM_USE_STMT (use_stmt, imm_iter, def)
    {
      FOR_EACH_IMM_USE_ON_STMT (use_p, imm_iter)
	SET_USE (use_p, res);

      update_stmt (use_stmt);

      /* It is possible that we just created a duplicate close-phi
	 for an already-processed containing loop.  Check for this
	 case and clean it up.  */
      if (gimple_code (use_stmt) == GIMPLE_PHI
	  && gimple_phi_num_args (use_stmt) == 1)
	make_close_phi_nodes_unique (gimple_bb (use_stmt));
    }

  remove_phi_node (gsi, true);
}

/* Removes all the close phi duplicates from BB.  */

static void
make_close_phi_nodes_unique (basic_block bb)
{
  gphi_iterator psi;

  for (psi = gsi_start_phis (bb); !gsi_end_p (psi); gsi_next (&psi))
    {
      gphi_iterator gsi = psi;
      gphi *phi = psi.phi ();

      /* At this point, PHI should be a close phi in normal form.  */
      gcc_assert (gimple_phi_num_args (phi) == 1);

      /* Iterate over the next phis and remove duplicates.  */
      gsi_next (&gsi);
      while (!gsi_end_p (gsi))
	if (same_close_phi_node (phi, gsi.phi ()))
	  remove_duplicate_close_phi (phi, &gsi);
	else
	  gsi_next (&gsi);
    }
}

/* Return true when NAME is defined in LOOP.  */

static bool
defined_in_loop_p (tree name, loop_p loop)
{
  gcc_assert (TREE_CODE (name) == SSA_NAME);
  return loop == loop_containing_stmt (SSA_NAME_DEF_STMT (name));
}

/* Transforms LOOP to the canonical loop closed SSA form.  */

static void
canonicalize_loop_closed_ssa (loop_p loop)
{
  edge e = single_exit (loop);
  basic_block bb;

  if (!e || e->flags & EDGE_ABNORMAL)
    return;

  bb = e->dest;

  if (single_pred_p (bb))
    {
      e = split_block_after_labels (bb);
      DEBUG_PRINT (dp << "Splitting bb_" << bb->index << ".\n");
      make_close_phi_nodes_unique (e->src);
    }
  else
    {
      gphi_iterator psi;
      basic_block close = split_edge (e);

      e = single_succ_edge (close);
      DEBUG_PRINT (dp << "Splitting edge (" << e->src->index << ","
		      << e->dest->index << ")\n");

      for (psi = gsi_start_phis (bb); !gsi_end_p (psi); gsi_next (&psi))
	{
	  gphi *phi = psi.phi ();
	  unsigned i;

	  for (i = 0; i < gimple_phi_num_args (phi); i++)
	    if (gimple_phi_arg_edge (phi, i) == e)
	      {
		tree res, arg = gimple_phi_arg_def (phi, i);
		use_operand_p use_p;
		gphi *close_phi;

		/* Only add close phi nodes for SSA_NAMEs defined in LOOP.  */
		if (TREE_CODE (arg) != SSA_NAME
		    || !defined_in_loop_p (arg, loop))
		  continue;

		close_phi = create_phi_node (NULL_TREE, close);
		res = create_new_def_for (arg, close_phi,
					  gimple_phi_result_ptr (close_phi));
		add_phi_arg (close_phi, arg,
			     gimple_phi_arg_edge (close_phi, 0),
			     UNKNOWN_LOCATION);
		use_p = gimple_phi_arg_imm_use_ptr (phi, i);
		replace_exp (use_p, res);
		update_stmt (phi);
	      }
	}

      make_close_phi_nodes_unique (close);
    }

  /* The code above does not properly handle changes in the post dominance
     information (yet).  */
  recompute_all_dominators ();
}

/* Converts the current loop closed SSA form to a canonical form
   expected by the Graphite code generation.

   The loop closed SSA form has the following invariant: a variable
   defined in a loop that is used outside the loop appears only in the
   phi nodes in the destination of the loop exit.  These phi nodes are
   called close phi nodes.

   The canonical loop closed SSA form contains the extra invariants:

   - when the loop contains only one exit, the close phi nodes contain
   only one argument.  That implies that the basic block that contains
   the close phi nodes has only one predecessor, that is a basic block
   in the loop.

   - the basic block containing the close phi nodes does not contain
   other statements.

   - there exist only one phi node per definition in the loop.
*/

static void
canonicalize_loop_closed_ssa_form (void)
{
  checking_verify_loop_closed_ssa (true);

  loop_p loop;
  FOR_EACH_LOOP (loop, 0)
    canonicalize_loop_closed_ssa (loop);

  rewrite_into_loop_closed_ssa (NULL, TODO_update_ssa);
  update_ssa (TODO_update_ssa);

  checking_verify_loop_closed_ssa (true);
}

/* Can all ivs be represented by a signed integer?
   As isl might generate negative values in its expressions, signed loop ivs
   are required in the backend.  */

static bool
loop_ivs_can_be_represented (loop_p loop)
{
  unsigned type_long_long = TYPE_PRECISION (long_long_integer_type_node);
  for (gphi_iterator psi = gsi_start_phis (loop->header); !gsi_end_p (psi);
       gsi_next (&psi))
    {
      gphi *phi = psi.phi ();
      tree res = PHI_RESULT (phi);
      tree type = TREE_TYPE (res);

      if (TYPE_UNSIGNED (type) && TYPE_PRECISION (type) >= type_long_long)
	return false;
    }

  return true;
}

/* Returns a COND_EXPR statement when BB has a single predecessor, the
   edge between BB and its predecessor is not a loop exit edge, and
   the last statement of the single predecessor is a COND_EXPR.  */

static gcond *
single_pred_cond_non_loop_exit (basic_block bb)
{
  if (single_pred_p (bb))
    {
      edge e = single_pred_edge (bb);
      basic_block pred = e->src;
      gimple *stmt;

      if (loop_depth (pred->loop_father) > loop_depth (bb->loop_father))
	return NULL;

      stmt = last_stmt (pred);

      if (stmt && gimple_code (stmt) == GIMPLE_COND)
	return as_a<gcond *> (stmt);
    }

  return NULL;
}

namespace
{

/* Build the maximal scop containing LOOPs and add it to SCOPS.  */

class scop_detection
{
public:
  scop_detection () : scops (vNULL) {}

  ~scop_detection ()
  {
    scops.release ();
  }

  /* A marker for invalid sese_l.  */
  static sese_l invalid_sese;

  /* Return the SCOPS in this SCOP_DETECTION.  */

  vec<sese_l>
  get_scops ()
  {
    return scops;
  }

  /* Return an sese_l around the LOOP.  */

  sese_l get_sese (loop_p loop);

  /* Return the closest dominator with a single entry edge.  In case of a
     back-loop the back-edge is not counted.  */

  static edge get_nearest_dom_with_single_entry (basic_block dom);

  /* Return the closest post-dominator with a single exit edge.  In case of a
     back-loop the back-edge is not counted.  */

  static edge get_nearest_pdom_with_single_exit (basic_block dom);

  /* Merge scops at same loop depth and returns the new sese.
     Returns a new SESE when merge was successful, INVALID_SESE otherwise.  */

  sese_l merge_sese (sese_l first, sese_l second) const;

  /* Build scop outer->inner if possible.  */

  sese_l build_scop_depth (sese_l s, loop_p loop);

  /* If loop and loop->next are valid scops, try to merge them.  */

  sese_l build_scop_breadth (sese_l s1, loop_p loop);

  /* Return true when LOOP is a valid scop, that is a Static Control Part, a
     region of code that can be represented in the polyhedral model.  SCOP
     defines the region we analyse.  */

  bool loop_is_valid_in_scop (loop_p loop, sese_l scop) const;

  /* Return true when BEGIN is the preheader edge of a loop with a single exit
     END.  */

  static bool region_has_one_loop (sese_l s);

  /* Add to SCOPS a scop starting at SCOP_BEGIN and ending at SCOP_END.  */

  void add_scop (sese_l s);

  /* Returns true if S1 subsumes/surrounds S2.  */
  static bool subsumes (sese_l s1, sese_l s2);

  /* Remove a SCoP which is subsumed by S1.  */
  void remove_subscops (sese_l s1);

  /* Returns true if S1 intersects with S2.  Since we already know that S1 does
     not subsume S2 or vice-versa, we only check for entry bbs.  */

  static bool intersects (sese_l s1, sese_l s2);

  /* Remove one of the scops when it intersects with any other.  */

  void remove_intersecting_scops (sese_l s1);

  /* Return true when the body of LOOP has statements that can be represented
     as a valid scop.  */

  bool loop_body_is_valid_scop (loop_p loop, sese_l scop) const;

  /* Return true when BB contains a harmful operation for a scop: that
     can be a function call with side effects, the induction variables
     are not linear with respect to SCOP, etc.  The current open
     scop should end before this statement.  */

  bool harmful_stmt_in_bb (sese_l scop, basic_block bb) const;

  /* Return true when a statement in SCOP cannot be represented by Graphite.
     The assumptions are that L1 dominates L2, and SCOP->entry dominates L1.
     Limit the number of bbs between adjacent loops to
     PARAM_SCOP_MAX_NUM_BBS_BETWEEN_LOOPS.  */

  bool harmful_loop_in_region (sese_l scop) const;

  /* Return true only when STMT is simple enough for being handled by Graphite.
     This depends on SCOP, as the parameters are initialized relatively to
     this basic block, the linear functions are initialized based on the
     outermost loop containing STMT inside the SCOP.  BB is the place where we
     try to evaluate the STMT.  */

  bool stmt_simple_for_scop_p (sese_l scop, gimple *stmt,
			       basic_block bb) const;

  /* Something like "n * m" is not allowed.  */

  static bool graphite_can_represent_init (tree e);

  /* Return true when SCEV can be represented in the polyhedral model.

     An expression can be represented, if it can be expressed as an
     affine expression.  For loops (i, j) and parameters (m, n) all
     affine expressions are of the form:

     x1 * i + x2 * j + x3 * m + x4 * n + x5 * 1 where x1..x5 element of Z

     1 i + 20 j + (-2) m + 25

     Something like "i * n" or "n * m" is not allowed.  */

  static bool graphite_can_represent_scev (tree scev);

  /* Return true when EXPR can be represented in the polyhedral model.

     This means an expression can be represented, if it is linear with respect
     to the loops and the strides are non parametric.  LOOP is the place where
     the expr will be evaluated.  SCOP defines the region we analyse.  */

  static bool graphite_can_represent_expr (sese_l scop, loop_p loop,
					   tree expr);

  /* Return true if the data references of STMT can be represented by Graphite.
     We try to analyze the data references in a loop contained in the SCOP.  */

  static bool stmt_has_simple_data_refs_p (sese_l scop, gimple *stmt);

  /* Remove the close phi node at GSI and replace its rhs with the rhs
     of PHI.  */

  static void remove_duplicate_close_phi (gphi *phi, gphi_iterator *gsi);

  /* Returns true when Graphite can represent LOOP in SCOP.
     FIXME: For the moment, graphite cannot be used on loops that iterate using
     induction variables that wrap.  */

  static bool can_represent_loop_1 (loop_p loop, sese_l scop);

  /* Return true when all the loops within LOOP can be represented by
     Graphite.  */

  static bool can_represent_loop (loop_p loop, sese_l scop);

  /* Returns the number of pbbs that are in loops contained in SCOP.  */

  static int nb_pbbs_in_loops (scop_p scop);

  static bool graphite_can_represent_stmt (sese_l, gimple *, basic_block);

private:
  vec<sese_l> scops;
};

sese_l scop_detection::invalid_sese (NULL, NULL);

/* Return an sese_l around the LOOP.  */

sese_l
scop_detection::get_sese (loop_p loop)
{
  if (!loop)
    return invalid_sese;

  if (!loops_state_satisfies_p (LOOPS_HAVE_PREHEADERS))
    return invalid_sese;
  edge scop_end = single_exit (loop);
  if (!scop_end)
    return invalid_sese;
  edge scop_begin = loop_preheader_edge (loop);
  sese_l s (scop_begin, scop_end);
  return s;
}

/* Return the closest dominator with a single entry edge.  */

edge
scop_detection::get_nearest_dom_with_single_entry (basic_block dom)
{
  if (!dom->preds)
    return NULL;

  /* If any of the dominators has two predecessors but one of them is a back
     edge, then that basic block also qualifies as a dominator with single
     entry.  */
  if (dom->preds->length () == 2)
    {
      /* If e1->src dominates e2->src then e1->src will also dominate dom.  */
      edge e1 = (*dom->preds)[0];
      edge e2 = (*dom->preds)[1];
      loop_p l = dom->loop_father;
      loop_p l1 = e1->src->loop_father;
      loop_p l2 = e2->src->loop_father;
      if (l != l1 && l == l2
	  && dominated_by_p (CDI_DOMINATORS, e2->src, e1->src))
	return e1;
      if (l != l2 && l == l1
	  && dominated_by_p (CDI_DOMINATORS, e1->src, e2->src))
	return e2;
    }

  while (dom->preds->length () != 1)
    {
      if (dom->preds->length () < 1)
	return NULL;
      dom = get_immediate_dominator (CDI_DOMINATORS, dom);
      if (!dom->preds)
	return NULL;
    }
  return (*dom->preds)[0];
}

/* Return the closest post-dominator with a single exit edge.  In case of a
   back-loop the back-edge is not counted.  */

edge
scop_detection::get_nearest_pdom_with_single_exit (basic_block pdom)
{
  if (!pdom->succs)
    return NULL;

  /* If any of the post-dominators has two successors but one of them is a back
     edge, then that basic block also qualifies as a post-dominator with single
     exit. */
  if (pdom->succs->length () == 2)
    {
      /* If e1->dest post-dominates e2->dest then e1->dest will also
	 post-dominate pdom.  */
      edge e1 = (*pdom->succs)[0];
      edge e2 = (*pdom->succs)[1];
      loop_p l = pdom->loop_father;
      loop_p l1 = e1->dest->loop_father;
      loop_p l2 = e2->dest->loop_father;
      if (l != l1 && l == l2
	  && dominated_by_p (CDI_POST_DOMINATORS, e2->dest, e1->dest))
	return e1;
      if (l != l2 && l == l1
	  && dominated_by_p (CDI_POST_DOMINATORS, e1->dest, e2->dest))
	return e2;
    }

  while (pdom->succs->length () != 1)
    {
      if (pdom->succs->length () < 1)
	return NULL;
      pdom = get_immediate_dominator (CDI_POST_DOMINATORS, pdom);
      if (!pdom->succs)
	return NULL;
    }

  return (*pdom->succs)[0];
}

/* Merge scops at same loop depth and returns the new sese.
   Returns a new SESE when merge was successful, INVALID_SESE otherwise.  */

sese_l
scop_detection::merge_sese (sese_l first, sese_l second) const
{
  /* In the trivial case first/second may be NULL.  */
  if (!first)
    return second;
  if (!second)
    return first;

  DEBUG_PRINT (dp << "[scop-detection] try merging sese s1: ";
	       print_sese (dump_file, first);
	       dp << "[scop-detection] try merging sese s2: ";
	       print_sese (dump_file, second));

  /* Assumption: Both the sese's should be at the same loop depth or one scop
     should subsume the other like in case of nested loops.  */

  /* Find the common dominators for entry,
     and common post-dominators for the exit.  */
  basic_block dom = nearest_common_dominator (CDI_DOMINATORS,
					      get_entry_bb (first),
					      get_entry_bb (second));

  edge entry = get_nearest_dom_with_single_entry (dom);

  if (!entry || (entry->flags & EDGE_IRREDUCIBLE_LOOP))
    return invalid_sese;

  basic_block pdom = nearest_common_dominator (CDI_POST_DOMINATORS,
					       get_exit_bb (first),
					       get_exit_bb (second));
  pdom = nearest_common_dominator (CDI_POST_DOMINATORS, dom, pdom);

  edge exit = get_nearest_pdom_with_single_exit (pdom);

  if (!exit || (exit->flags & EDGE_IRREDUCIBLE_LOOP))
    return invalid_sese;

  sese_l combined (entry, exit);

  DEBUG_PRINT (dp << "[scop-detection] checking combined sese: ";
	       print_sese (dump_file, combined));

  /* FIXME: We could iterate to find the dom which dominates pdom, and pdom
     which post-dominates dom, until it stabilizes.  Also, ENTRY->SRC and
     EXIT->DEST should be in the same loop nest.  */
  if (!dominated_by_p (CDI_DOMINATORS, pdom, dom)
      || loop_depth (entry->src->loop_father)
	 != loop_depth (exit->dest->loop_father))
    return invalid_sese;

  /* For now we just bail out when there is a loop exit in the region
     that is not also the exit of the region.  We could enlarge the
     region to cover the loop that region exits to.  See PR79977.  */
  if (loop_outer (entry->src->loop_father))
    {
      vec<edge> exits = get_loop_exit_edges (entry->src->loop_father);
      for (unsigned i = 0; i < exits.length (); ++i)
	{
	  if (exits[i] != exit
	      && bb_in_region (exits[i]->src, entry->dest, exit->src))
	    {
	      DEBUG_PRINT (dp << "[scop-detection-fail] cannot merge seses.\n");
	      exits.release ();
	      return invalid_sese;
	    }
	}
      exits.release ();
    }

  /* For now we just want to bail out when exit does not post-dominate entry.
     TODO: We might just add a basic_block at the exit to make exit
     post-dominate entry (the entire region).  */
  if (!dominated_by_p (CDI_POST_DOMINATORS, get_entry_bb (combined),
		       get_exit_bb (combined))
      || !dominated_by_p (CDI_DOMINATORS, get_exit_bb (combined),
			  get_entry_bb (combined)))
    {
      DEBUG_PRINT (dp << "[scop-detection-fail] cannot merge seses.\n");
      return invalid_sese;
    }

  /* FIXME: We should remove this piece of code once
     canonicalize_loop_closed_ssa has been removed, because that function
     adds a BB with single exit.  */
  if (!trivially_empty_bb_p (get_exit_bb (combined)))
    {
      /* Find the first empty succ (with single exit) of combined.exit.  */
      basic_block imm_succ = combined.exit->dest;
      if (single_succ_p (imm_succ)
	  && single_pred_p (imm_succ)
	  && trivially_empty_bb_p (imm_succ))
	combined.exit = single_succ_edge (imm_succ);
      else
	{
	  DEBUG_PRINT (dp << "[scop-detection-fail] Discarding SCoP because "
			  << "no single exit (empty succ) for sese exit";
		       print_sese (dump_file, combined));
	  return invalid_sese;
	}
    }

  /* Analyze all the BBs in new sese.  */
  if (harmful_loop_in_region (combined))
    return invalid_sese;

  DEBUG_PRINT (dp << "[merged-sese] s1: "; print_sese (dump_file, combined));

  return combined;
}

/* Build scop outer->inner if possible.  */

sese_l
scop_detection::build_scop_depth (sese_l s, loop_p loop)
{
  if (!loop)
    return s;

  DEBUG_PRINT (dp << "[Depth loop_" << loop->num << "]\n");
  s = build_scop_depth (s, loop->inner);

  sese_l s2 = merge_sese (s, get_sese (loop));
  if (!s2)
    {
      /* s might be a valid scop, so return it and start analyzing from the
	 adjacent loop.  */
      build_scop_depth (invalid_sese, loop->next);
      return s;
    }

  if (!loop_is_valid_in_scop (loop, s2))
    return build_scop_depth (invalid_sese, loop->next);

  return build_scop_breadth (s2, loop);
}

/* If loop and loop->next are valid scops, try to merge them.  */

sese_l
scop_detection::build_scop_breadth (sese_l s1, loop_p loop)
{
  if (!loop)
    return s1;
  DEBUG_PRINT (dp << "[Breadth loop_" << loop->num << "]\n");
  gcc_assert (s1);

  loop_p l = loop;
  sese_l s2 = build_scop_depth (invalid_sese, l->next);
  if (!s2)
    {
      if (s1)
	add_scop (s1);
      return s1;
    }

  sese_l combined = merge_sese (s1, s2);

  /* Combining adjacent loops may add unrelated loops into the
     region so we have to check all sub-loops of the outer loop
     that are in the combined region.  */
  if (combined)
    for (l = loop_outer (loop)->inner; l; l = l->next)
      if (bb_in_sese_p (l->header, combined)
	  && ! loop_is_valid_in_scop (l, combined))
	{
	  combined = invalid_sese;
	  break;
	}

  if (combined)
    s1 = combined;
  else
    add_scop (s2);

  if (s1)
    add_scop (s1);
  return s1;
}

/* Returns true when Graphite can represent LOOP in SCOP.
   FIXME: For the moment, graphite cannot be used on loops that iterate using
   induction variables that wrap.  */

bool
scop_detection::can_represent_loop_1 (loop_p loop, sese_l scop)
{
  tree niter;
  struct tree_niter_desc niter_desc;

  return single_exit (loop)
    && !(loop_preheader_edge (loop)->flags & EDGE_IRREDUCIBLE_LOOP)
    && number_of_iterations_exit (loop, single_exit (loop), &niter_desc, false)
    && niter_desc.control.no_overflow
    && (niter = number_of_latch_executions (loop))
    && !chrec_contains_undetermined (niter)
    && !chrec_contains_undetermined (scalar_evolution_in_region (scop,
								 loop, niter))
    && graphite_can_represent_expr (scop, loop, niter);
}

/* Return true when all the loops within LOOP can be represented by
   Graphite.  */

bool
scop_detection::can_represent_loop (loop_p loop, sese_l scop)
{
  if (!can_represent_loop_1 (loop, scop))
    return false;
  if (loop->inner && !can_represent_loop (loop->inner, scop))
    return false;
  if (loop->next && !can_represent_loop (loop->next, scop))
    return false;

  return true;
}

/* Return true when LOOP is a valid scop, that is a Static Control Part, a
   region of code that can be represented in the polyhedral model.  SCOP
   defines the region we analyse.  */

bool
scop_detection::loop_is_valid_in_scop (loop_p loop, sese_l scop) const
{
  if (!scop)
    return false;

  if (!optimize_loop_nest_for_speed_p (loop))
    {
      DEBUG_PRINT (dp << "[scop-detection-fail] loop_"
		      << loop->num << " is not on a hot path.\n");
      return false;
    }

  if (!can_represent_loop (loop, scop))
    {
      DEBUG_PRINT (dp << "[scop-detection-fail] cannot represent loop_"
		      << loop->num << "\n");
      return false;
    }

  if (loop_body_is_valid_scop (loop, scop))
    {
      DEBUG_PRINT (dp << "[valid-scop] loop_" << loop->num
		      << " is a valid scop.\n");
      return true;
    }
  return false;
}

/* Return true when BEGIN is the preheader edge of a loop with a single exit
   END.  */

bool
scop_detection::region_has_one_loop (sese_l s)
{
  edge begin = s.entry;
  edge end = s.exit;
  /* Check for a single perfectly nested loop.  */
  if (begin->dest->loop_father->inner)
    return false;

  /* Otherwise, check whether we have adjacent loops.  */
  return begin->dest->loop_father == end->src->loop_father;
}

/* Add to SCOPS a scop starting at SCOP_BEGIN and ending at SCOP_END.  */

void
scop_detection::add_scop (sese_l s)
{
  gcc_assert (s);

  /* Do not add scops with only one loop.  */
  if (region_has_one_loop (s))
    {
      DEBUG_PRINT (dp << "[scop-detection-fail] Discarding one loop SCoP: ";
		   print_sese (dump_file, s));
      return;
    }

  if (get_exit_bb (s) == EXIT_BLOCK_PTR_FOR_FN (cfun))
    {
      DEBUG_PRINT (dp << "[scop-detection-fail] "
		      << "Discarding SCoP exiting to return: ";
		   print_sese (dump_file, s));
      return;
    }

  /* Remove all the scops which are subsumed by s.  */
  remove_subscops (s);

  /* Remove intersecting scops. FIXME: It will be a good idea to keep
     the non-intersecting part of the scop already in the list.  */
  remove_intersecting_scops (s);

  scops.safe_push (s);
  DEBUG_PRINT (dp << "[scop-detection] Adding SCoP: "; print_sese (dump_file, s));
}

/* Return true when a statement in SCOP cannot be represented by Graphite.
   The assumptions are that L1 dominates L2, and SCOP->entry dominates L1.
   Limit the number of bbs between adjacent loops to
   PARAM_SCOP_MAX_NUM_BBS_BETWEEN_LOOPS.  */

bool
scop_detection::harmful_loop_in_region (sese_l scop) const
{
  basic_block exit_bb = get_exit_bb (scop);
  basic_block entry_bb = get_entry_bb (scop);

  DEBUG_PRINT (dp << "[checking-harmful-bbs] ";
	       print_sese (dump_file, scop));
  gcc_assert (dominated_by_p (CDI_DOMINATORS, exit_bb, entry_bb));

  auto_vec<basic_block> worklist;
  auto_bitmap loops;

  worklist.safe_push (entry_bb);
  while (! worklist.is_empty ())
    {
      basic_block bb = worklist.pop ();
      DEBUG_PRINT (dp << "Visiting bb_" << bb->index << "\n");

      /* The basic block should not be part of an irreducible loop.  */
      if (bb->flags & BB_IRREDUCIBLE_LOOP)
	return true;

      /* Check for unstructured control flow: CFG not generated by structured
	 if-then-else.  */
      if (bb->succs->length () > 1)
	{
	  edge e;
	  edge_iterator ei;
	  FOR_EACH_EDGE (e, ei, bb->succs)
	    if (!dominated_by_p (CDI_POST_DOMINATORS, bb, e->dest)
		&& !dominated_by_p (CDI_DOMINATORS, e->dest, bb))
	      return true;
	}

      /* Collect all loops in the current region.  */
      loop_p loop = bb->loop_father;
      if (loop_in_sese_p (loop, scop))
	bitmap_set_bit (loops, loop->num);
      else
	{
	  /* We only check for harmful statements in basic blocks not part of
	     any loop fully contained in the scop: other bbs are checked below
	     in loop_is_valid_in_scop.  */
	  if (harmful_stmt_in_bb (scop, bb))
	    return true;
	}

      if (bb != exit_bb)
	for (basic_block dom = first_dom_son (CDI_DOMINATORS, bb);
	     dom;
	     dom = next_dom_son (CDI_DOMINATORS, dom))
	  worklist.safe_push (dom);
    }

  /* Go through all loops and check that they are still valid in the combined
     scop.  */
  unsigned j;
  bitmap_iterator bi;
  EXECUTE_IF_SET_IN_BITMAP (loops, 0, j, bi)
    {
      loop_p loop = (*current_loops->larray)[j];
      gcc_assert (loop->num == (int) j);

      if (!loop_is_valid_in_scop (loop, scop))
	return true;
    }

  return false;
}

/* Returns true if S1 subsumes/surrounds S2.  */
bool
scop_detection::subsumes (sese_l s1, sese_l s2)
{
  if (dominated_by_p (CDI_DOMINATORS, get_entry_bb (s2),
		      get_entry_bb (s1))
      && dominated_by_p (CDI_POST_DOMINATORS, s2.exit->dest,
			 s1.exit->dest))
    return true;
  return false;
}

/* Remove a SCoP which is subsumed by S1.  */
void
scop_detection::remove_subscops (sese_l s1)
{
  int j;
  sese_l *s2;
  FOR_EACH_VEC_ELT_REVERSE (scops, j, s2)
    {
      if (subsumes (s1, *s2))
	{
	  DEBUG_PRINT (dp << "Removing sub-SCoP";
		       print_sese (dump_file, *s2));
	  scops.unordered_remove (j);
	}
    }
}

/* Returns true if S1 intersects with S2.  Since we already know that S1 does
   not subsume S2 or vice-versa, we only check for entry bbs.  */

bool
scop_detection::intersects (sese_l s1, sese_l s2)
{
  if (dominated_by_p (CDI_DOMINATORS, get_entry_bb (s2),
		      get_entry_bb (s1))
      && !dominated_by_p (CDI_DOMINATORS, get_entry_bb (s2),
			  get_exit_bb (s1)))
    return true;
  if ((s1.exit == s2.entry) || (s2.exit == s1.entry))
    return true;

  return false;
}

/* Remove one of the scops when it intersects with any other.  */

void
scop_detection::remove_intersecting_scops (sese_l s1)
{
  int j;
  sese_l *s2;
  FOR_EACH_VEC_ELT_REVERSE (scops, j, s2)
    {
      if (intersects (s1, *s2))
	{
	  DEBUG_PRINT (dp << "Removing intersecting SCoP";
		       print_sese (dump_file, *s2);
		       dp << "Intersects with:";
		       print_sese (dump_file, s1));
	  scops.unordered_remove (j);
	}
    }
}

/* Something like "n * m" is not allowed.  */

bool
scop_detection::graphite_can_represent_init (tree e)
{
  switch (TREE_CODE (e))
    {
    case POLYNOMIAL_CHREC:
      return graphite_can_represent_init (CHREC_LEFT (e))
	&& graphite_can_represent_init (CHREC_RIGHT (e));

    case MULT_EXPR:
      if (chrec_contains_symbols (TREE_OPERAND (e, 0)))
	return graphite_can_represent_init (TREE_OPERAND (e, 0))
	  && tree_fits_shwi_p (TREE_OPERAND (e, 1));
      else
	return graphite_can_represent_init (TREE_OPERAND (e, 1))
	  && tree_fits_shwi_p (TREE_OPERAND (e, 0));

    case PLUS_EXPR:
    case POINTER_PLUS_EXPR:
    case MINUS_EXPR:
      return graphite_can_represent_init (TREE_OPERAND (e, 0))
	&& graphite_can_represent_init (TREE_OPERAND (e, 1));

    case NEGATE_EXPR:
    case BIT_NOT_EXPR:
    CASE_CONVERT:
    case NON_LVALUE_EXPR:
      return graphite_can_represent_init (TREE_OPERAND (e, 0));

    default:
      break;
    }

  return true;
}

/* Return true when SCEV can be represented in the polyhedral model.

   An expression can be represented, if it can be expressed as an
   affine expression.  For loops (i, j) and parameters (m, n) all
   affine expressions are of the form:

   x1 * i + x2 * j + x3 * m + x4 * n + x5 * 1 where x1..x5 element of Z

   1 i + 20 j + (-2) m + 25

   Something like "i * n" or "n * m" is not allowed.  */

bool
scop_detection::graphite_can_represent_scev (tree scev)
{
  if (chrec_contains_undetermined (scev))
    return false;

  /* We disable the handling of pointer types, because it’s currently not
     supported by Graphite with the isl AST generator. SSA_NAME nodes are
     the only nodes, which are disabled in case they are pointers to object
     types, but this can be changed.  */

  if (POINTER_TYPE_P (TREE_TYPE (scev)) && TREE_CODE (scev) == SSA_NAME)
    return false;

  switch (TREE_CODE (scev))
    {
    case NEGATE_EXPR:
    case BIT_NOT_EXPR:
    CASE_CONVERT:
    case NON_LVALUE_EXPR:
      return graphite_can_represent_scev (TREE_OPERAND (scev, 0));

    case PLUS_EXPR:
    case POINTER_PLUS_EXPR:
    case MINUS_EXPR:
      return graphite_can_represent_scev (TREE_OPERAND (scev, 0))
	&& graphite_can_represent_scev (TREE_OPERAND (scev, 1));

    case MULT_EXPR:
      return !CONVERT_EXPR_CODE_P (TREE_CODE (TREE_OPERAND (scev, 0)))
	&& !CONVERT_EXPR_CODE_P (TREE_CODE (TREE_OPERAND (scev, 1)))
	&& !(chrec_contains_symbols (TREE_OPERAND (scev, 0))
	     && chrec_contains_symbols (TREE_OPERAND (scev, 1)))
	&& graphite_can_represent_init (scev)
	&& graphite_can_represent_scev (TREE_OPERAND (scev, 0))
	&& graphite_can_represent_scev (TREE_OPERAND (scev, 1));

    case POLYNOMIAL_CHREC:
      /* Check for constant strides.  With a non constant stride of
	 'n' we would have a value of 'iv * n'.  Also check that the
	 initial value can represented: for example 'n * m' cannot be
	 represented.  */
      if (!evolution_function_right_is_integer_cst (scev)
	  || !graphite_can_represent_init (scev))
	return false;
      return graphite_can_represent_scev (CHREC_LEFT (scev));

    default:
      break;
    }

  /* Only affine functions can be represented.  */
  if (tree_contains_chrecs (scev, NULL) || !scev_is_linear_expression (scev))
    return false;

  return true;
}

/* Return true when EXPR can be represented in the polyhedral model.

   This means an expression can be represented, if it is linear with respect to
   the loops and the strides are non parametric.  LOOP is the place where the
   expr will be evaluated.  SCOP defines the region we analyse.  */

bool
scop_detection::graphite_can_represent_expr (sese_l scop, loop_p loop,
					     tree expr)
{
  tree scev = scalar_evolution_in_region (scop, loop, expr);
  return graphite_can_represent_scev (scev);
}

/* Return true if the data references of STMT can be represented by Graphite.
   We try to analyze the data references in a loop contained in the SCOP.  */

bool
scop_detection::stmt_has_simple_data_refs_p (sese_l scop, gimple *stmt)
{
  loop_p nest = outermost_loop_in_sese (scop, gimple_bb (stmt));
  loop_p loop = loop_containing_stmt (stmt);
  vec<data_reference_p> drs = vNULL;

  graphite_find_data_references_in_stmt (nest, loop, stmt, &drs);

  int j;
  data_reference_p dr;
  FOR_EACH_VEC_ELT (drs, j, dr)
    {
      int nb_subscripts = DR_NUM_DIMENSIONS (dr);

      if (nb_subscripts < 1)
	{
	  free_data_refs (drs);
	  return false;
	}

      tree ref = DR_REF (dr);

      for (int i = nb_subscripts - 1; i >= 0; i--)
	{
	  if (!graphite_can_represent_scev (DR_ACCESS_FN (dr, i))
	      || (TREE_CODE (ref) != ARRAY_REF && TREE_CODE (ref) != MEM_REF
		  && TREE_CODE (ref) != COMPONENT_REF))
	    {
	      free_data_refs (drs);
	      return false;
	    }

	  ref = TREE_OPERAND (ref, 0);
	}
    }

    free_data_refs (drs);
    return true;
}

/* GIMPLE_ASM and GIMPLE_CALL may embed arbitrary side effects.
   Calls have side-effects, except those to const or pure
   functions.  */

static bool
stmt_has_side_effects (gimple *stmt)
{
  if (gimple_has_volatile_ops (stmt)
      || (gimple_code (stmt) == GIMPLE_CALL
	  && !(gimple_call_flags (stmt) & (ECF_CONST | ECF_PURE)))
      || (gimple_code (stmt) == GIMPLE_ASM))
    {
      DEBUG_PRINT (dp << "[scop-detection-fail] "
		      << "Statement has side-effects:\n";
	print_gimple_stmt (dump_file, stmt, 0, TDF_VOPS | TDF_MEMSYMS));
      return true;
    }
  return false;
}

/* Returns true if STMT can be represented in polyhedral model. LABEL,
   simple COND stmts, pure calls, and assignments can be repesented.  */

bool
scop_detection::graphite_can_represent_stmt (sese_l scop, gimple *stmt,
					     basic_block bb)
{
  loop_p loop = bb->loop_father;
  switch (gimple_code (stmt))
    {
    case GIMPLE_LABEL:
      return true;

    case GIMPLE_COND:
      {
	/* We can handle all binary comparisons.  Inequalities are
	   also supported as they can be represented with union of
	   polyhedra.  */
	enum tree_code code = gimple_cond_code (stmt);
	if (!(code == LT_EXPR
	      || code == GT_EXPR
	      || code == LE_EXPR
	      || code == GE_EXPR
	      || code == EQ_EXPR
	      || code == NE_EXPR))
	  {
	    DEBUG_PRINT (dp << "[scop-detection-fail] "
			    << "Graphite cannot handle cond stmt:\n";
			 print_gimple_stmt (dump_file, stmt, 0,
					    TDF_VOPS | TDF_MEMSYMS));
	    return false;
	  }

	for (unsigned i = 0; i < 2; ++i)
	  {
	    tree op = gimple_op (stmt, i);
	    if (!graphite_can_represent_expr (scop, loop, op)
		/* We can only constrain on integer type.  */
		|| (TREE_CODE (TREE_TYPE (op)) != INTEGER_TYPE))
	      {
		DEBUG_PRINT (dp << "[scop-detection-fail] "
				<< "Graphite cannot represent stmt:\n";
			     print_gimple_stmt (dump_file, stmt, 0,
						TDF_VOPS | TDF_MEMSYMS));
		return false;
	      }
	  }

	return true;
      }

    case GIMPLE_ASSIGN:
    case GIMPLE_CALL:
      return true;

    default:
      /* These nodes cut a new scope.  */
      DEBUG_PRINT (
	  dp << "[scop-detection-fail] "
	     << "Gimple stmt not handled in Graphite:\n";
	  print_gimple_stmt (dump_file, stmt, 0, TDF_VOPS | TDF_MEMSYMS));
      return false;
    }
}

/* Return true only when STMT is simple enough for being handled by Graphite.
   This depends on SCOP, as the parameters are initialized relatively to
   this basic block, the linear functions are initialized based on the outermost
   loop containing STMT inside the SCOP.  BB is the place where we try to
   evaluate the STMT.  */

bool
scop_detection::stmt_simple_for_scop_p (sese_l scop, gimple *stmt,
					basic_block bb) const
{
  gcc_assert (scop);

  if (is_gimple_debug (stmt))
    return true;

  if (stmt_has_side_effects (stmt))
    return false;

  if (!stmt_has_simple_data_refs_p (scop, stmt))
    {
      DEBUG_PRINT (dp << "[scop-detection-fail] "
		      << "Graphite cannot handle data-refs in stmt:\n";
	print_gimple_stmt (dump_file, stmt, 0, TDF_VOPS|TDF_MEMSYMS););
      return false;
    }

  return graphite_can_represent_stmt (scop, stmt, bb);
}

/* Return true when BB contains a harmful operation for a scop: that
   can be a function call with side effects, the induction variables
   are not linear with respect to SCOP, etc.  The current open
   scop should end before this statement.  */

bool
scop_detection::harmful_stmt_in_bb (sese_l scop, basic_block bb) const
{
  gimple_stmt_iterator gsi;

  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    if (!stmt_simple_for_scop_p (scop, gsi_stmt (gsi), bb))
      return true;

  return false;
}

/* Return true when the body of LOOP has statements that can be represented as a
   valid scop.  */

bool
scop_detection::loop_body_is_valid_scop (loop_p loop, sese_l scop) const
{
  if (!loop_ivs_can_be_represented (loop))
    {
      DEBUG_PRINT (dp << "[scop-detection-fail] loop_" << loop->num
		      << "IV cannot be represented.\n");
      return false;
    }

  if (!loop_nest_has_data_refs (loop))
    {
      DEBUG_PRINT (dp << "[scop-detection-fail] loop_" << loop->num
		      << "does not have any data reference.\n");
      return false;
    }

  basic_block *bbs = get_loop_body (loop);
  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      basic_block bb = bbs[i];

      if (harmful_stmt_in_bb (scop, bb))
	{
	  free (bbs);
	  return false;
	}
    }
  free (bbs);

  if (loop->inner)
    {
      loop = loop->inner;
      while (loop)
	{
	  if (!loop_body_is_valid_scop (loop, scop))
	    return false;
	  loop = loop->next;
	}
    }

  return true;
}

/* Returns the number of pbbs that are in loops contained in SCOP.  */

int
scop_detection::nb_pbbs_in_loops (scop_p scop)
{
  int i;
  poly_bb_p pbb;
  int res = 0;

  FOR_EACH_VEC_ELT (scop->pbbs, i, pbb)
    if (loop_in_sese_p (gbb_loop (PBB_BLACK_BOX (pbb)), scop->scop_info->region))
      res++;

  return res;
}

/* When parameter NAME is in REGION, returns its index in SESE_PARAMS.
   Otherwise returns -1.  */

static inline int
parameter_index_in_region_1 (tree name, sese_info_p region)
{
  int i;
  tree p;

  gcc_assert (TREE_CODE (name) == SSA_NAME);

  FOR_EACH_VEC_ELT (region->params, i, p)
    if (p == name)
      return i;

  return -1;
}

/* When the parameter NAME is in REGION, returns its index in
   SESE_PARAMS.  Otherwise this function inserts NAME in SESE_PARAMS
   and returns the index of NAME.  */

static int
parameter_index_in_region (tree name, sese_info_p region)
{
  int i;

  gcc_assert (TREE_CODE (name) == SSA_NAME);

  /* Cannot constrain on anything else than INTEGER_TYPE parameters.  */
  if (TREE_CODE (TREE_TYPE (name)) != INTEGER_TYPE)
    return -1;

  if (!invariant_in_sese_p_rec (name, region->region, NULL))
    return -1;

  i = parameter_index_in_region_1 (name, region);
  if (i != -1)
    return i;

  i = region->params.length ();
  region->params.safe_push (name);
  return i;
}

/* In the context of sese S, scan the expression E and translate it to
   a linear expression C.  When parsing a symbolic multiplication, K
   represents the constant multiplier of an expression containing
   parameters.  */

static void
scan_tree_for_params (sese_info_p s, tree e)
{
  if (e == chrec_dont_know)
    return;

  switch (TREE_CODE (e))
    {
    case POLYNOMIAL_CHREC:
      scan_tree_for_params (s, CHREC_LEFT (e));
      break;

    case MULT_EXPR:
      if (chrec_contains_symbols (TREE_OPERAND (e, 0)))
	scan_tree_for_params (s, TREE_OPERAND (e, 0));
      else
	scan_tree_for_params (s, TREE_OPERAND (e, 1));
      break;

    case PLUS_EXPR:
    case POINTER_PLUS_EXPR:
    case MINUS_EXPR:
      scan_tree_for_params (s, TREE_OPERAND (e, 0));
      scan_tree_for_params (s, TREE_OPERAND (e, 1));
      break;

    case NEGATE_EXPR:
    case BIT_NOT_EXPR:
    CASE_CONVERT:
    case NON_LVALUE_EXPR:
      scan_tree_for_params (s, TREE_OPERAND (e, 0));
      break;

    case SSA_NAME:
      parameter_index_in_region (e, s);
      break;

    case INTEGER_CST:
    case ADDR_EXPR:
    case REAL_CST:
    case COMPLEX_CST:
    case VECTOR_CST:
      break;

   default:
      gcc_unreachable ();
      break;
    }
}

/* Find parameters with respect to REGION in BB. We are looking in memory
   access functions, conditions and loop bounds.  */

static void
find_params_in_bb (sese_info_p region, gimple_poly_bb_p gbb)
{
  /* Find parameters in the access functions of data references.  */
  int i;
  data_reference_p dr;
  FOR_EACH_VEC_ELT (GBB_DATA_REFS (gbb), i, dr)
    for (unsigned j = 0; j < DR_NUM_DIMENSIONS (dr); j++)
      scan_tree_for_params (region, DR_ACCESS_FN (dr, j));

  /* Find parameters in conditional statements.  */
  gimple *stmt;
  loop_p loop = GBB_BB (gbb)->loop_father;
  FOR_EACH_VEC_ELT (GBB_CONDITIONS (gbb), i, stmt)
    {
      tree lhs = scalar_evolution_in_region (region->region, loop,
					     gimple_cond_lhs (stmt));
      tree rhs = scalar_evolution_in_region (region->region, loop,
					     gimple_cond_rhs (stmt));

      scan_tree_for_params (region, lhs);
      scan_tree_for_params (region, rhs);
    }
}

/* Record the parameters used in the SCOP.  A variable is a parameter
   in a scop if it does not vary during the execution of that scop.  */

static void
find_scop_parameters (scop_p scop)
{
  unsigned i;
  sese_info_p region = scop->scop_info;
  struct loop *loop;

  /* Find the parameters used in the loop bounds.  */
  FOR_EACH_VEC_ELT (region->loop_nest, i, loop)
    {
      tree nb_iters = number_of_latch_executions (loop);

      if (!chrec_contains_symbols (nb_iters))
	continue;

      nb_iters = scalar_evolution_in_region (region->region, loop, nb_iters);
      scan_tree_for_params (region, nb_iters);
    }

  /* Find the parameters used in data accesses.  */
  poly_bb_p pbb;
  FOR_EACH_VEC_ELT (scop->pbbs, i, pbb)
    find_params_in_bb (region, PBB_BLACK_BOX (pbb));

  int nbp = sese_nb_params (region);
  scop_set_nb_params (scop, nbp);
}

/* Record DEF if it is used in other bbs different than DEF_BB in the SCOP.  */

static void
build_cross_bb_scalars_def (scop_p scop, tree def, basic_block def_bb,
			     vec<tree> *writes)
{
  if (!def || !is_gimple_reg (def))
    return;

  /* Do not gather scalar variables that can be analyzed by SCEV as they can be
     generated out of the induction variables.  */
  if (scev_analyzable_p (def, scop->scop_info->region))
    return;

  gimple *use_stmt;
  imm_use_iterator imm_iter;
  FOR_EACH_IMM_USE_STMT (use_stmt, imm_iter, def)
    if (def_bb != gimple_bb (use_stmt) && !is_gimple_debug (use_stmt))
      {
	writes->safe_push (def);
	DEBUG_PRINT (dp << "Adding scalar write: ";
		     print_generic_expr (dump_file, def, 0);
		     dp << "\nFrom stmt: ";
		     print_gimple_stmt (dump_file,
					SSA_NAME_DEF_STMT (def), 0, 0));
	/* This is required by the FOR_EACH_IMM_USE_STMT when we want to break
	   before all the uses have been visited.  */
	BREAK_FROM_IMM_USE_STMT (imm_iter);
      }
}

/* Record DEF if it is used in other bbs different than DEF_BB in the SCOP.  */

static void
build_cross_bb_scalars_use (scop_p scop, tree use, gimple *use_stmt,
			    vec<scalar_use> *reads)
{
  gcc_assert (use);
  if (!is_gimple_reg (use))
    return;

  /* Do not gather scalar variables that can be analyzed by SCEV as they can be
     generated out of the induction variables.  */
  if (scev_analyzable_p (use, scop->scop_info->region))
    return;

  gimple *def_stmt = SSA_NAME_DEF_STMT (use);
  if (gimple_bb (def_stmt) != gimple_bb (use_stmt))
    {
      DEBUG_PRINT (dp << "Adding scalar read: ";
		   print_generic_expr (dump_file, use, 0);
		   dp << "\nFrom stmt: ";
		   print_gimple_stmt (dump_file, use_stmt, 0, 0));
      reads->safe_push (std::make_pair (use_stmt, use));
    }
}

/* Record all scalar variables that are defined and used in different BBs of the
   SCOP.  */

static void
graphite_find_cross_bb_scalar_vars (scop_p scop, gimple *stmt,
				    vec<scalar_use> *reads, vec<tree> *writes)
{
  tree def;

  if (gimple_code (stmt) == GIMPLE_ASSIGN)
    def = gimple_assign_lhs (stmt);
  else if (gimple_code (stmt) == GIMPLE_CALL)
    def = gimple_call_lhs (stmt);
  else if (gimple_code (stmt) == GIMPLE_PHI)
    def = gimple_phi_result (stmt);
  else
    return;


  build_cross_bb_scalars_def (scop, def, gimple_bb (stmt), writes);

  ssa_op_iter iter;
  use_operand_p use_p;
  FOR_EACH_PHI_OR_STMT_USE (use_p, stmt, iter, SSA_OP_USE)
    {
      tree use = USE_FROM_PTR (use_p);
      build_cross_bb_scalars_use (scop, use, stmt, reads);
    }
}

/* Generates a polyhedral black box only if the bb contains interesting
   information.  */

static gimple_poly_bb_p
try_generate_gimple_bb (scop_p scop, basic_block bb)
{
  vec<data_reference_p> drs = vNULL;
  vec<tree> writes = vNULL;
  vec<scalar_use> reads = vNULL;

  sese_l region = scop->scop_info->region;
  loop_p nest = outermost_loop_in_sese (region, bb);

  loop_p loop = bb->loop_father;
  if (!loop_in_sese_p (loop, region))
    loop = nest;

  for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
       gsi_next (&gsi))
    {
      gimple *stmt = gsi_stmt (gsi);
      if (is_gimple_debug (stmt))
	continue;

      graphite_find_data_references_in_stmt (nest, loop, stmt, &drs);
      graphite_find_cross_bb_scalar_vars (scop, stmt, &reads, &writes);
    }

  for (gphi_iterator psi = gsi_start_phis (bb); !gsi_end_p (psi);
       gsi_next (&psi))
    if (!virtual_operand_p (gimple_phi_result (psi.phi ())))
      graphite_find_cross_bb_scalar_vars (scop, psi.phi (), &reads, &writes);

  if (drs.is_empty () && writes.is_empty () && reads.is_empty ())
    return NULL;

  return new_gimple_poly_bb (bb, drs, reads, writes);
}

/* Compute alias-sets for all data references in DRS.  */

static void
build_alias_set (scop_p scop)
{
  int num_vertices = scop->drs.length ();
  struct graph *g = new_graph (num_vertices);
  dr_info *dr1, *dr2;
  int i, j;
  int *all_vertices;

  FOR_EACH_VEC_ELT (scop->drs, i, dr1)
    for (j = i+1; scop->drs.iterate (j, &dr2); j++)
      if (dr_may_alias_p (dr1->dr, dr2->dr, true))
	{
	  add_edge (g, i, j);
	  add_edge (g, j, i);
	}

  all_vertices = XNEWVEC (int, num_vertices);
  for (i = 0; i < num_vertices; i++)
    all_vertices[i] = i;

  graphds_dfs (g, all_vertices, num_vertices, NULL, true, NULL);
  free (all_vertices);

  for (i = 0; i < g->n_vertices; i++)
    scop->drs[i].alias_set = g->vertices[i].component + 1;

  free_graph (g);
}

/* Gather BBs and conditions for a SCOP.  */
class gather_bbs : public dom_walker
{
public:
  gather_bbs (cdi_direction, scop_p);

  virtual edge before_dom_children (basic_block);
  virtual void after_dom_children (basic_block);

private:
  auto_vec<gimple *, 3> conditions, cases;
  scop_p scop;
};
}
gather_bbs::gather_bbs (cdi_direction direction, scop_p scop)
  : dom_walker (direction), scop (scop)
{
}

/* Record in execution order the loops fully contained in the region.  */

static void
record_loop_in_sese (basic_block bb, sese_info_p region)
{
  loop_p father = bb->loop_father;
  if (loop_in_sese_p (father, region->region))
    {
      bool found = false;
      loop_p loop0;
      int j;
      FOR_EACH_VEC_ELT (region->loop_nest, j, loop0)
	if (father == loop0)
	  {
	    found = true;
	    break;
	  }
      if (!found)
	region->loop_nest.safe_push (father);
    }
}

/* Call-back for dom_walk executed before visiting the dominated
   blocks.  */

edge
gather_bbs::before_dom_children (basic_block bb)
{
  sese_info_p region = scop->scop_info;
  if (!bb_in_sese_p (bb, region->region))
    return NULL;

  record_loop_in_sese (bb, region);

  gcond *stmt = single_pred_cond_non_loop_exit (bb);

  if (stmt)
    {
      edge e = single_pred_edge (bb);

      conditions.safe_push (stmt);

      if (e->flags & EDGE_TRUE_VALUE)
	cases.safe_push (stmt);
      else
	cases.safe_push (NULL);
    }

  scop->scop_info->bbs.safe_push (bb);

  gimple_poly_bb_p gbb = try_generate_gimple_bb (scop, bb);
  if (!gbb)
    return NULL;

  GBB_CONDITIONS (gbb) = conditions.copy ();
  GBB_CONDITION_CASES (gbb) = cases.copy ();

  poly_bb_p pbb = new_poly_bb (scop, gbb);
  scop->pbbs.safe_push (pbb);

  int i;
  data_reference_p dr;
  FOR_EACH_VEC_ELT (gbb->data_refs, i, dr)
    {
      DEBUG_PRINT (dp << "Adding memory ";
		   if (dr->is_read)
		     dp << "read: ";
		   else
		     dp << "write: ";
		   print_generic_expr (dump_file, dr->ref, 0);
		   dp << "\nFrom stmt: ";
		   print_gimple_stmt (dump_file, dr->stmt, 0, 0));

      scop->drs.safe_push (dr_info (dr, pbb));
    }

  return NULL;
}

/* Call-back for dom_walk executed after visiting the dominated
   blocks.  */

void
gather_bbs::after_dom_children (basic_block bb)
{
  if (!bb_in_sese_p (bb, scop->scop_info->region))
    return;

  if (single_pred_cond_non_loop_exit (bb))
    {
      conditions.pop ();
      cases.pop ();
    }
}

/* Find Static Control Parts (SCoP) in the current function and pushes
   them to SCOPS.  */

void
build_scops (vec<scop_p> *scops)
{
  if (dump_file)
    dp.set_dump_file (dump_file);

  canonicalize_loop_closed_ssa_form ();

  scop_detection sb;
  sb.build_scop_depth (scop_detection::invalid_sese, current_loops->tree_root);

  /* Now create scops from the lightweight SESEs.  */
  vec<sese_l> scops_l = sb.get_scops ();
  int i;
  sese_l *s;
  FOR_EACH_VEC_ELT (scops_l, i, s)
    {
      scop_p scop = new_scop (s->entry, s->exit);

      /* Record all basic blocks and their conditions in REGION.  */
      gather_bbs (CDI_DOMINATORS, scop).walk (cfun->cfg->x_entry_block_ptr);

      build_alias_set (scop);

      /* Do not optimize a scop containing only PBBs that do not belong
	 to any loops.  */
      if (sb.nb_pbbs_in_loops (scop) == 0)
	{
	  DEBUG_PRINT (dp << "[scop-detection-fail] no data references.\n");
	  free_scop (scop);
	  continue;
	}

      unsigned max_arrays = PARAM_VALUE (PARAM_GRAPHITE_MAX_ARRAYS_PER_SCOP);
      if (scop->drs.length () >= max_arrays)
	{
	  DEBUG_PRINT (dp << "[scop-detection-fail] too many data references: "
		       << scop->drs.length ()
		       << " is larger than --param graphite-max-arrays-per-scop="
		       << max_arrays << ".\n");
	  free_scop (scop);
	  continue;
	}

      find_scop_parameters (scop);
      graphite_dim_t max_dim = PARAM_VALUE (PARAM_GRAPHITE_MAX_NB_SCOP_PARAMS);

      if (scop_nb_params (scop) > max_dim)
	{
	  DEBUG_PRINT (dp << "[scop-detection-fail] too many parameters: "
			  << scop_nb_params (scop)
			  << " larger than --param graphite-max-nb-scop-params="
			  << max_dim << ".\n");
	  free_scop (scop);
	  continue;
	}

      scops->safe_push (scop);
    }

  DEBUG_PRINT (dp << "number of SCoPs: " << (scops ? scops->length () : 0););
}

#endif /* HAVE_isl */
