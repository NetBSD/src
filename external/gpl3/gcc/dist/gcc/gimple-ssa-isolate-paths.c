/* Detect paths through the CFG which can never be executed in a conforming
   program and isolate them.

   Copyright (C) 2013-2015 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "hash-set.h"
#include "machmode.h"
#include "vec.h"
#include "double-int.h"
#include "input.h"
#include "alias.h"
#include "symtab.h"
#include "options.h"
#include "wide-int.h"
#include "inchash.h"
#include "tree.h"
#include "fold-const.h"
#include "flags.h"
#include "predict.h"
#include "tm.h"
#include "hard-reg-set.h"
#include "input.h"
#include "function.h"
#include "dominance.h"
#include "cfg.h"
#include "basic-block.h"
#include "tree-ssa-alias.h"
#include "internal-fn.h"
#include "gimple-expr.h"
#include "is-a.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "gimple-walk.h"
#include "tree-ssa.h"
#include "stringpool.h"
#include "tree-ssanames.h"
#include "gimple-ssa.h"
#include "tree-ssa-operands.h"
#include "tree-phinodes.h"
#include "ssa-iterators.h"
#include "cfgloop.h"
#include "tree-pass.h"
#include "tree-cfg.h"
#include "diagnostic-core.h"
#include "intl.h"


static bool cfg_altered;

/* Callback for walk_stmt_load_store_ops.

   Return TRUE if OP will dereference the tree stored in DATA, FALSE
   otherwise.

   This routine only makes a superficial check for a dereference.  Thus,
   it must only be used if it is safe to return a false negative.  */
static bool
check_loadstore (gimple stmt, tree op, tree, void *data)
{
  if ((TREE_CODE (op) == MEM_REF || TREE_CODE (op) == TARGET_MEM_REF)
      && operand_equal_p (TREE_OPERAND (op, 0), (tree)data, 0))
    {
      TREE_THIS_VOLATILE (op) = 1;
      TREE_SIDE_EFFECTS (op) = 1;
      update_stmt (stmt);
      return true;
    }
  return false;
}

/* Insert a trap after SI and remove SI and all statements after the trap.  */

static void
insert_trap_and_remove_trailing_statements (gimple_stmt_iterator *si_p, tree op)
{
  /* We want the NULL pointer dereference to actually occur so that
     code that wishes to catch the signal can do so.

     If the dereference is a load, then there's nothing to do as the
     LHS will be a throw-away SSA_NAME and the RHS is the NULL dereference.

     If the dereference is a store and we can easily transform the RHS,
     then simplify the RHS to enable more DCE.   Note that we require the
     statement to be a GIMPLE_ASSIGN which filters out calls on the RHS.  */
  gimple stmt = gsi_stmt (*si_p);
  if (walk_stmt_load_store_ops (stmt, (void *)op, NULL, check_loadstore)
      && is_gimple_assign (stmt)
      && INTEGRAL_TYPE_P (TREE_TYPE (gimple_assign_lhs (stmt))))
    {
      /* We just need to turn the RHS into zero converted to the proper
         type.  */
      tree type = TREE_TYPE (gimple_assign_lhs (stmt));
      gimple_assign_set_rhs_code (stmt, INTEGER_CST);
      gimple_assign_set_rhs1 (stmt, fold_convert (type, integer_zero_node));
      update_stmt (stmt);
    }

  gcall *new_stmt
    = gimple_build_call (builtin_decl_explicit (BUILT_IN_TRAP), 0);
  gimple_seq seq = NULL;
  gimple_seq_add_stmt (&seq, new_stmt);

  /* If we had a NULL pointer dereference, then we want to insert the
     __builtin_trap after the statement, for the other cases we want
     to insert before the statement.  */
  if (walk_stmt_load_store_ops (stmt, (void *)op,
			        check_loadstore,
				check_loadstore))
    {
      gsi_insert_after (si_p, seq, GSI_NEW_STMT);
      if (stmt_ends_bb_p (stmt))
	{
	  split_block (gimple_bb (stmt), stmt);
	  return;
	}
    }
  else
    gsi_insert_before (si_p, seq, GSI_NEW_STMT);

  /* We must remove statements from the end of the block so that we
     never reference a released SSA_NAME.  */
  basic_block bb = gimple_bb (gsi_stmt (*si_p));
  for (gimple_stmt_iterator si = gsi_last_bb (bb);
       gsi_stmt (si) != gsi_stmt (*si_p);
       si = gsi_last_bb (bb))
    {
      stmt = gsi_stmt (si);
      unlink_stmt_vdef (stmt);
      gsi_remove (&si, true);
      release_defs (stmt);
    }
}

/* BB when reached via incoming edge E will exhibit undefined behaviour
   at STMT.  Isolate and optimize the path which exhibits undefined
   behaviour.

   Isolation is simple.  Duplicate BB and redirect E to BB'.

   Optimization is simple as well.  Replace STMT in BB' with an
   unconditional trap and remove all outgoing edges from BB'.

   If RET_ZERO, do not trap, only return NULL.

   DUPLICATE is a pre-existing duplicate, use it as BB' if it exists.

   Return BB'.  */

basic_block
isolate_path (basic_block bb, basic_block duplicate,
	      edge e, gimple stmt, tree op, bool ret_zero)
{
  gimple_stmt_iterator si, si2;
  edge_iterator ei;
  edge e2;

  /* First duplicate BB if we have not done so already and remove all
     the duplicate's outgoing edges as duplicate is going to unconditionally
     trap.  Removing the outgoing edges is both an optimization and ensures
     we don't need to do any PHI node updates.  */
  if (!duplicate)
    {
      duplicate = duplicate_block (bb, NULL, NULL);
      if (!ret_zero)
	for (ei = ei_start (duplicate->succs); (e2 = ei_safe_edge (ei)); )
	  remove_edge (e2);
    }

  /* Complete the isolation step by redirecting E to reach DUPLICATE.  */
  e2 = redirect_edge_and_branch (e, duplicate);
  if (e2)
    flush_pending_stmts (e2);


  /* There may be more than one statement in DUPLICATE which exhibits
     undefined behaviour.  Ultimately we want the first such statement in
     DUPLCIATE so that we're able to delete as much code as possible.

     So each time we discover undefined behaviour in DUPLICATE, search for
     the statement which triggers undefined behaviour.  If found, then
     transform the statement into a trap and delete everything after the
     statement.  If not found, then this particular instance was subsumed by
     an earlier instance of undefined behaviour and there's nothing to do.

     This is made more complicated by the fact that we have STMT, which is in
     BB rather than in DUPLICATE.  So we set up two iterators, one for each
     block and walk forward looking for STMT in BB, advancing each iterator at
     each step.

     When we find STMT the second iterator should point to STMT's equivalent in
     duplicate.  If DUPLICATE ends before STMT is found in BB, then there's
     nothing to do.

     Ignore labels and debug statements.  */
  si = gsi_start_nondebug_after_labels_bb (bb);
  si2 = gsi_start_nondebug_after_labels_bb (duplicate);
  while (!gsi_end_p (si) && !gsi_end_p (si2) && gsi_stmt (si) != stmt)
    {
      gsi_next_nondebug (&si);
      gsi_next_nondebug (&si2);
    }

  /* This would be an indicator that we never found STMT in BB, which should
     never happen.  */
  gcc_assert (!gsi_end_p (si));

  /* If we did not run to the end of DUPLICATE, then SI points to STMT and
     SI2 points to the duplicate of STMT in DUPLICATE.  Insert a trap
     before SI2 and remove SI2 and all trailing statements.  */
  if (!gsi_end_p (si2))
    {
      if (ret_zero)
	{
	  greturn *ret = as_a <greturn *> (gsi_stmt (si2));
	  tree zero = build_zero_cst (TREE_TYPE (gimple_return_retval (ret)));
	  gimple_return_set_retval (ret, zero);
	  update_stmt (ret);
	}
      else
	insert_trap_and_remove_trailing_statements (&si2, op);
    }

  return duplicate;
}

/* Look for PHI nodes which feed statements in the same block where
   the value of the PHI node implies the statement is erroneous.

   For example, a NULL PHI arg value which then feeds a pointer
   dereference.

   When found isolate and optimize the path associated with the PHI
   argument feeding the erroneous statement.  */
static void
find_implicit_erroneous_behaviour (void)
{
  basic_block bb;

  FOR_EACH_BB_FN (bb, cfun)
    {
      gphi_iterator si;

      /* Out of an abundance of caution, do not isolate paths to a
	 block where the block has any abnormal outgoing edges.

	 We might be able to relax this in the future.  We have to detect
	 when we have to split the block with the NULL dereference and
	 the trap we insert.  We have to preserve abnormal edges out
	 of the isolated block which in turn means updating PHIs at
	 the targets of those abnormal outgoing edges.  */
      if (has_abnormal_or_eh_outgoing_edge_p (bb))
	continue;

      /* First look for a PHI which sets a pointer to NULL and which
 	 is then dereferenced within BB.  This is somewhat overly
	 conservative, but probably catches most of the interesting
	 cases.   */
      for (si = gsi_start_phis (bb); !gsi_end_p (si); gsi_next (&si))
	{
	  gphi *phi = si.phi ();
	  tree lhs = gimple_phi_result (phi);

	  /* If the result is not a pointer, then there is no need to
 	     examine the arguments.  */
	  if (!POINTER_TYPE_P (TREE_TYPE (lhs)))
	    continue;

	  /* PHI produces a pointer result.  See if any of the PHI's
	     arguments are NULL.

	     When we remove an edge, we want to reprocess the current
	     index, hence the ugly way we update I for each iteration.  */
	  basic_block duplicate = NULL;
	  for (unsigned i = 0, next_i = 0;
	       i < gimple_phi_num_args (phi);
	       i = next_i)
	    {
	      tree op = gimple_phi_arg_def (phi, i);
	      edge e = gimple_phi_arg_edge (phi, i);
	      imm_use_iterator iter;
	      gimple use_stmt;

	      next_i = i + 1;

	      if (TREE_CODE (op) == ADDR_EXPR)
		{
		  tree valbase = get_base_address (TREE_OPERAND (op, 0));
		  if ((TREE_CODE (valbase) == VAR_DECL
		       && !is_global_var (valbase))
		      || TREE_CODE (valbase) == PARM_DECL)
		    {
		      FOR_EACH_IMM_USE_STMT (use_stmt, iter, lhs)
			{
			  greturn *return_stmt
			    = dyn_cast <greturn *> (use_stmt);
			  if (!return_stmt)
			    continue;

			  if (gimple_return_retval (return_stmt) != lhs)
			    continue;

			  if (warning_at (gimple_location (use_stmt),
					  OPT_Wreturn_local_addr,
					  "function may return address "
					  "of local variable"))
			    inform (DECL_SOURCE_LOCATION(valbase),
				    "declared here");

			  if (gimple_bb (use_stmt) == bb)
			    {
			      duplicate = isolate_path (bb, duplicate, e,
							use_stmt, lhs, true);

			      /* When we remove an incoming edge, we need to
				 reprocess the Ith element.  */
			      next_i = i;
			      cfg_altered = true;
			    }
			}
		    }
		}

	      if (!integer_zerop (op))
		continue;

	      /* We've got a NULL PHI argument.  Now see if the
 	         PHI's result is dereferenced within BB.  */
	      FOR_EACH_IMM_USE_STMT (use_stmt, iter, lhs)
	        {
	          /* We only care about uses in BB.  Catching cases in
		     in other blocks would require more complex path
		     isolation code.   */
		  if (gimple_bb (use_stmt) != bb)
		    continue;

		  if (infer_nonnull_range (use_stmt, lhs,
					   flag_isolate_erroneous_paths_dereference,
					   flag_isolate_erroneous_paths_attribute))

		    {
		      duplicate = isolate_path (bb, duplicate, e,
						use_stmt, lhs, false);

		      /* When we remove an incoming edge, we need to
			 reprocess the Ith element.  */
		      next_i = i;
		      cfg_altered = true;
		    }
		}
	    }
	}
    }
}

/* Look for statements which exhibit erroneous behaviour.  For example
   a NULL pointer dereference.

   When found, optimize the block containing the erroneous behaviour.  */
static void
find_explicit_erroneous_behaviour (void)
{
  basic_block bb;

  FOR_EACH_BB_FN (bb, cfun)
    {
      gimple_stmt_iterator si;

      /* Out of an abundance of caution, do not isolate paths to a
	 block where the block has any abnormal outgoing edges.

	 We might be able to relax this in the future.  We have to detect
	 when we have to split the block with the NULL dereference and
	 the trap we insert.  We have to preserve abnormal edges out
	 of the isolated block which in turn means updating PHIs at
	 the targets of those abnormal outgoing edges.  */
      if (has_abnormal_or_eh_outgoing_edge_p (bb))
	continue;

      /* Now look at the statements in the block and see if any of
	 them explicitly dereference a NULL pointer.  This happens
	 because of jump threading and constant propagation.  */
      for (si = gsi_start_bb (bb); !gsi_end_p (si); gsi_next (&si))
	{
	  gimple stmt = gsi_stmt (si);

	  /* By passing null_pointer_node, we can use infer_nonnull_range
	     to detect explicit NULL pointer dereferences and other uses
	     where a non-NULL value is required.  */
	  if (infer_nonnull_range (stmt, null_pointer_node,
				   flag_isolate_erroneous_paths_dereference,
				   flag_isolate_erroneous_paths_attribute))
	    {
	      insert_trap_and_remove_trailing_statements (&si,
							  null_pointer_node);

	      /* And finally, remove all outgoing edges from BB.  */
	      edge e;
	      for (edge_iterator ei = ei_start (bb->succs);
		   (e = ei_safe_edge (ei)); )
		remove_edge (e);

	      /* Ignore any more operands on this statement and
		 continue the statement iterator (which should
		 terminate its loop immediately.  */
	      cfg_altered = true;
	      break;
	    }

	  /* Detect returning the address of a local variable.  This only
	     becomes undefined behavior if the result is used, so we do not
	     insert a trap and only return NULL instead.  */
	  if (greturn *return_stmt = dyn_cast <greturn *> (stmt))
	    {
	      tree val = gimple_return_retval (return_stmt);
	      if (val && TREE_CODE (val) == ADDR_EXPR)
		{
		  tree valbase = get_base_address (TREE_OPERAND (val, 0));
		  if ((TREE_CODE (valbase) == VAR_DECL
		       && !is_global_var (valbase))
		      || TREE_CODE (valbase) == PARM_DECL)
		    {
		      /* We only need it for this particular case.  */
		      calculate_dominance_info (CDI_POST_DOMINATORS);
		      const char* msg;
		      bool always_executed = dominated_by_p
			(CDI_POST_DOMINATORS,
			 single_succ (ENTRY_BLOCK_PTR_FOR_FN (cfun)), bb);
		      if (always_executed)
			msg = N_("function returns address of local variable");
		      else
			msg = N_("function may return address of "
				 "local variable");

		      if (warning_at (gimple_location (stmt),
				      OPT_Wreturn_local_addr, msg))
			inform (DECL_SOURCE_LOCATION(valbase), "declared here");
		      tree zero = build_zero_cst (TREE_TYPE (val));
		      gimple_return_set_retval (return_stmt, zero);
		      update_stmt (stmt);
		    }
		}
	    }
	}
    }
}

/* Search the function for statements which, if executed, would cause
   the program to fault such as a dereference of a NULL pointer.

   Such a program can't be valid if such a statement was to execute
   according to ISO standards.

   We detect explicit NULL pointer dereferences as well as those implied
   by a PHI argument having a NULL value which unconditionally flows into
   a dereference in the same block as the PHI.

   In the former case we replace the offending statement with an
   unconditional trap and eliminate the outgoing edges from the statement's
   basic block.  This may expose secondary optimization opportunities.

   In the latter case, we isolate the path(s) with the NULL PHI
   feeding the dereference.  We can then replace the offending statement
   and eliminate the outgoing edges in the duplicate.  Again, this may
   expose secondary optimization opportunities.

   A warning for both cases may be advisable as well.

   Other statically detectable violations of the ISO standard could be
   handled in a similar way, such as out-of-bounds array indexing.  */

static unsigned int
gimple_ssa_isolate_erroneous_paths (void)
{
  initialize_original_copy_tables ();

  /* Search all the blocks for edges which, if traversed, will
     result in undefined behaviour.  */
  cfg_altered = false;

  /* First handle cases where traversal of a particular edge
     triggers undefined behaviour.  These cases require creating
     duplicate blocks and thus new SSA_NAMEs.

     We want that process complete prior to the phase where we start
     removing edges from the CFG.  Edge removal may ultimately result in
     removal of PHI nodes and thus releasing SSA_NAMEs back to the
     name manager.

     If the two processes run in parallel we could release an SSA_NAME
     back to the manager but we could still have dangling references
     to the released SSA_NAME in unreachable blocks.
     that any released names not have dangling references in the IL.  */
  find_implicit_erroneous_behaviour ();
  find_explicit_erroneous_behaviour ();

  free_original_copy_tables ();

  /* We scramble the CFG and loop structures a bit, clean up
     appropriately.  We really should incrementally update the
     loop structures, in theory it shouldn't be that hard.  */
  free_dominance_info (CDI_POST_DOMINATORS);
  if (cfg_altered)
    {
      free_dominance_info (CDI_DOMINATORS);
      loops_state_set (LOOPS_NEED_FIXUP);
      return TODO_cleanup_cfg | TODO_update_ssa;
    }
  return 0;
}

namespace {
const pass_data pass_data_isolate_erroneous_paths =
{
  GIMPLE_PASS, /* type */
  "isolate-paths", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_ISOLATE_ERRONEOUS_PATHS, /* tv_id */
  ( PROP_cfg | PROP_ssa ), /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

class pass_isolate_erroneous_paths : public gimple_opt_pass
{
public:
  pass_isolate_erroneous_paths (gcc::context *ctxt)
    : gimple_opt_pass (pass_data_isolate_erroneous_paths, ctxt)
  {}

  /* opt_pass methods: */
  opt_pass * clone () { return new pass_isolate_erroneous_paths (m_ctxt); }
  virtual bool gate (function *)
    {
      /* If we do not have a suitable builtin function for the trap statement,
	 then do not perform the optimization.  */
      return (flag_isolate_erroneous_paths_dereference != 0
	      || flag_isolate_erroneous_paths_attribute != 0);
    }

  virtual unsigned int execute (function *)
    {
      return gimple_ssa_isolate_erroneous_paths ();
    }

}; // class pass_isolate_erroneous_paths
}

gimple_opt_pass *
make_pass_isolate_erroneous_paths (gcc::context *ctxt)
{
  return new pass_isolate_erroneous_paths (ctxt);
}
