/* Dead store elimination
   Copyright (C) 2004-2016 Free Software Foundation, Inc.

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
#include "backend.h"
#include "rtl.h"
#include "tree.h"
#include "gimple.h"
#include "tree-pass.h"
#include "ssa.h"
#include "gimple-pretty-print.h"
#include "fold-const.h"
#include "gimple-iterator.h"
#include "tree-cfg.h"
#include "tree-dfa.h"
#include "domwalk.h"
#include "tree-cfgcleanup.h"

/* This file implements dead store elimination.

   A dead store is a store into a memory location which will later be
   overwritten by another store without any intervening loads.  In this
   case the earlier store can be deleted.

   In our SSA + virtual operand world we use immediate uses of virtual
   operands to detect dead stores.  If a store's virtual definition
   is used precisely once by a later store to the same location which
   post dominates the first store, then the first store is dead.

   The single use of the store's virtual definition ensures that
   there are no intervening aliased loads and the requirement that
   the second load post dominate the first ensures that if the earlier
   store executes, then the later stores will execute before the function
   exits.

   It may help to think of this as first moving the earlier store to
   the point immediately before the later store.  Again, the single
   use of the virtual definition and the post-dominance relationship
   ensure that such movement would be safe.  Clearly if there are
   back to back stores, then the second is redundant.

   Reviewing section 10.7.2 in Morgan's "Building an Optimizing Compiler"
   may also help in understanding this code since it discusses the
   relationship between dead store and redundant load elimination.  In
   fact, they are the same transformation applied to different views of
   the CFG.  */


/* Bitmap of blocks that have had EH statements cleaned.  We should
   remove their dead edges eventually.  */
static bitmap need_eh_cleanup;


/* A helper of dse_optimize_stmt.
   Given a GIMPLE_ASSIGN in STMT that writes to REF, find a candidate
   statement *USE_STMT that may prove STMT to be dead.
   Return TRUE if the above conditions are met, otherwise FALSE.  */

static bool
dse_possible_dead_store_p (ao_ref *ref, gimple *stmt, gimple **use_stmt)
{
  gimple *temp;
  unsigned cnt = 0;

  *use_stmt = NULL;

  /* Find the first dominated statement that clobbers (part of) the
     memory stmt stores to with no intermediate statement that may use
     part of the memory stmt stores.  That is, find a store that may
     prove stmt to be a dead store.  */
  temp = stmt;
  do
    {
      gimple *use_stmt, *defvar_def;
      imm_use_iterator ui;
      bool fail = false;
      tree defvar;

      /* Limit stmt walking to be linear in the number of possibly
         dead stores.  */
      if (++cnt > 256)
	return false;

      if (gimple_code (temp) == GIMPLE_PHI)
	defvar = PHI_RESULT (temp);
      else
	defvar = gimple_vdef (temp);
      defvar_def = temp;
      temp = NULL;
      FOR_EACH_IMM_USE_STMT (use_stmt, ui, defvar)
	{
	  cnt++;

	  /* If we ever reach our DSE candidate stmt again fail.  We
	     cannot handle dead stores in loops.  */
	  if (use_stmt == stmt)
	    {
	      fail = true;
	      BREAK_FROM_IMM_USE_STMT (ui);
	    }
	  /* In simple cases we can look through PHI nodes, but we
	     have to be careful with loops and with memory references
	     containing operands that are also operands of PHI nodes.
	     See gcc.c-torture/execute/20051110-*.c.  */
	  else if (gimple_code (use_stmt) == GIMPLE_PHI)
	    {
	      if (temp
		  /* Make sure we are not in a loop latch block.  */
		  || gimple_bb (stmt) == gimple_bb (use_stmt)
		  || dominated_by_p (CDI_DOMINATORS,
				     gimple_bb (stmt), gimple_bb (use_stmt))
		  /* We can look through PHIs to regions post-dominating
		     the DSE candidate stmt.  */
		  || !dominated_by_p (CDI_POST_DOMINATORS,
				      gimple_bb (stmt), gimple_bb (use_stmt)))
		{
		  fail = true;
		  BREAK_FROM_IMM_USE_STMT (ui);
		}
	      /* Do not consider the PHI as use if it dominates the 
	         stmt defining the virtual operand we are processing,
		 we have processed it already in this case.  */
	      if (gimple_bb (defvar_def) != gimple_bb (use_stmt)
		  && !dominated_by_p (CDI_DOMINATORS,
				      gimple_bb (defvar_def),
				      gimple_bb (use_stmt)))
		temp = use_stmt;
	    }
	  /* If the statement is a use the store is not dead.  */
	  else if (ref_maybe_used_by_stmt_p (use_stmt, ref))
	    {
	      fail = true;
	      BREAK_FROM_IMM_USE_STMT (ui);
	    }
	  /* If this is a store, remember it or bail out if we have
	     multiple ones (the will be in different CFG parts then).  */
	  else if (gimple_vdef (use_stmt))
	    {
	      if (temp)
		{
		  fail = true;
		  BREAK_FROM_IMM_USE_STMT (ui);
		}
	      temp = use_stmt;
	    }
	}

      if (fail)
	return false;

      /* If we didn't find any definition this means the store is dead
         if it isn't a store to global reachable memory.  In this case
	 just pretend the stmt makes itself dead.  Otherwise fail.  */
      if (!temp)
	{
	  if (ref_may_alias_global_p (ref))
	    return false;

	  temp = stmt;
	  break;
	}
    }
  /* Continue walking until we reach a kill.  */
  while (!stmt_kills_ref_p (temp, ref));

  *use_stmt = temp;

  return true;
}


/* Attempt to eliminate dead stores in the statement referenced by BSI.

   A dead store is a store into a memory location which will later be
   overwritten by another store without any intervening loads.  In this
   case the earlier store can be deleted.

   In our SSA + virtual operand world we use immediate uses of virtual
   operands to detect dead stores.  If a store's virtual definition
   is used precisely once by a later store to the same location which
   post dominates the first store, then the first store is dead.  */

static void
dse_optimize_stmt (gimple_stmt_iterator *gsi)
{
  gimple *stmt = gsi_stmt (*gsi);

  /* If this statement has no virtual defs, then there is nothing
     to do.  */
  if (!gimple_vdef (stmt))
    return;

  /* Don't return early on *this_2(D) ={v} {CLOBBER}.  */
  if (gimple_has_volatile_ops (stmt)
      && (!gimple_clobber_p (stmt)
	  || TREE_CODE (gimple_assign_lhs (stmt)) != MEM_REF))
    return;

  /* We know we have virtual definitions.  We can handle assignments and
     some builtin calls.  */
  if (gimple_call_builtin_p (stmt, BUILT_IN_NORMAL))
    {
      switch (DECL_FUNCTION_CODE (gimple_call_fndecl (stmt)))
	{
	  case BUILT_IN_MEMCPY:
	  case BUILT_IN_MEMMOVE:
	  case BUILT_IN_MEMSET:
	    {
	      gimple *use_stmt;
	      ao_ref ref;
	      tree size = NULL_TREE;
	      if (gimple_call_num_args (stmt) == 3)
		size = gimple_call_arg (stmt, 2);
	      tree ptr = gimple_call_arg (stmt, 0);
	      ao_ref_init_from_ptr_and_size (&ref, ptr, size);
	      if (!dse_possible_dead_store_p (&ref, stmt, &use_stmt))
		return;

	      if (dump_file && (dump_flags & TDF_DETAILS))
		{
		  fprintf (dump_file, "  Deleted dead call '");
		  print_gimple_stmt (dump_file, gsi_stmt (*gsi), dump_flags, 0);
		  fprintf (dump_file, "'\n");
		}

	      tree lhs = gimple_call_lhs (stmt);
	      if (lhs)
		{
		  gimple *new_stmt = gimple_build_assign (lhs, ptr);
		  unlink_stmt_vdef (stmt);
		  if (gsi_replace (gsi, new_stmt, true))
		    bitmap_set_bit (need_eh_cleanup, gimple_bb (stmt)->index);
		}
	      else
		{
		  /* Then we need to fix the operand of the consuming stmt.  */
		  unlink_stmt_vdef (stmt);

		  /* Remove the dead store.  */
		  if (gsi_remove (gsi, true))
		    bitmap_set_bit (need_eh_cleanup, gimple_bb (stmt)->index);
		  release_defs (stmt);
		}
	      break;
	    }
	  default:
	    return;
	}
    }

  if (is_gimple_assign (stmt))
    {
      gimple *use_stmt;

      /* Self-assignments are zombies.  */
      if (operand_equal_p (gimple_assign_rhs1 (stmt),
			   gimple_assign_lhs (stmt), 0))
	use_stmt = stmt;
      else
	{
	  ao_ref ref;
	  ao_ref_init (&ref, gimple_assign_lhs (stmt));
  	  if (!dse_possible_dead_store_p (&ref, stmt, &use_stmt))
	    return;
	}

      /* Now we know that use_stmt kills the LHS of stmt.  */

      /* But only remove *this_2(D) ={v} {CLOBBER} if killed by
	 another clobber stmt.  */
      if (gimple_clobber_p (stmt)
	  && !gimple_clobber_p (use_stmt))
	return;

      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "  Deleted dead store '");
	  print_gimple_stmt (dump_file, gsi_stmt (*gsi), dump_flags, 0);
	  fprintf (dump_file, "'\n");
	}

      /* Then we need to fix the operand of the consuming stmt.  */
      unlink_stmt_vdef (stmt);

      /* Remove the dead store.  */
      basic_block bb = gimple_bb (stmt);
      if (gsi_remove (gsi, true))
	bitmap_set_bit (need_eh_cleanup, bb->index);

      /* And release any SSA_NAMEs set in this statement back to the
	 SSA_NAME manager.  */
      release_defs (stmt);
    }
}

class dse_dom_walker : public dom_walker
{
public:
  dse_dom_walker (cdi_direction direction) : dom_walker (direction) {}

  virtual edge before_dom_children (basic_block);
};

edge
dse_dom_walker::before_dom_children (basic_block bb)
{
  gimple_stmt_iterator gsi;

  for (gsi = gsi_last_bb (bb); !gsi_end_p (gsi);)
    {
      dse_optimize_stmt (&gsi);
      if (gsi_end_p (gsi))
	gsi = gsi_last_bb (bb);
      else
	gsi_prev (&gsi);
    }
  return NULL;
}

namespace {

const pass_data pass_data_dse =
{
  GIMPLE_PASS, /* type */
  "dse", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_TREE_DSE, /* tv_id */
  ( PROP_cfg | PROP_ssa ), /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

class pass_dse : public gimple_opt_pass
{
public:
  pass_dse (gcc::context *ctxt)
    : gimple_opt_pass (pass_data_dse, ctxt)
  {}

  /* opt_pass methods: */
  opt_pass * clone () { return new pass_dse (m_ctxt); }
  virtual bool gate (function *) { return flag_tree_dse != 0; }
  virtual unsigned int execute (function *);

}; // class pass_dse

unsigned int
pass_dse::execute (function *fun)
{
  need_eh_cleanup = BITMAP_ALLOC (NULL);

  renumber_gimple_stmt_uids ();

  /* We might consider making this a property of each pass so that it
     can be [re]computed on an as-needed basis.  Particularly since
     this pass could be seen as an extension of DCE which needs post
     dominators.  */
  calculate_dominance_info (CDI_POST_DOMINATORS);
  calculate_dominance_info (CDI_DOMINATORS);

  /* Dead store elimination is fundamentally a walk of the post-dominator
     tree and a backwards walk of statements within each block.  */
  dse_dom_walker (CDI_POST_DOMINATORS).walk (fun->cfg->x_exit_block_ptr);

  /* Removal of stores may make some EH edges dead.  Purge such edges from
     the CFG as needed.  */
  if (!bitmap_empty_p (need_eh_cleanup))
    {
      gimple_purge_all_dead_eh_edges (need_eh_cleanup);
      cleanup_tree_cfg ();
    }

  BITMAP_FREE (need_eh_cleanup);
    
  /* For now, just wipe the post-dominator information.  */
  free_dominance_info (CDI_POST_DOMINATORS);
  return 0;
}

} // anon namespace

gimple_opt_pass *
make_pass_dse (gcc::context *ctxt)
{
  return new pass_dse (ctxt);
}
