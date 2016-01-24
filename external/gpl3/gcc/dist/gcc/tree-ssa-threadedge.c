/* SSA Jump Threading
   Copyright (C) 2005-2015 Free Software Foundation, Inc.
   Contributed by Jeff Law  <law@redhat.com>

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
#include "tm.h"
#include "hash-set.h"
#include "machmode.h"
#include "vec.h"
#include "double-int.h"
#include "input.h"
#include "alias.h"
#include "symtab.h"
#include "wide-int.h"
#include "inchash.h"
#include "tree.h"
#include "fold-const.h"
#include "flags.h"
#include "tm_p.h"
#include "predict.h"
#include "hard-reg-set.h"
#include "input.h"
#include "function.h"
#include "dominance.h"
#include "basic-block.h"
#include "cfgloop.h"
#include "timevar.h"
#include "dumpfile.h"
#include "tree-ssa-alias.h"
#include "internal-fn.h"
#include "gimple-expr.h"
#include "is-a.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "gimple-ssa.h"
#include "tree-cfg.h"
#include "tree-phinodes.h"
#include "ssa-iterators.h"
#include "stringpool.h"
#include "tree-ssanames.h"
#include "tree-ssa-propagate.h"
#include "tree-ssa-threadupdate.h"
#include "langhooks.h"
#include "params.h"
#include "tree-ssa-threadedge.h"
#include "tree-ssa-loop.h"
#include "builtins.h"
#include "cfg.h"
#include "cfganal.h"

/* To avoid code explosion due to jump threading, we limit the
   number of statements we are going to copy.  This variable
   holds the number of statements currently seen that we'll have
   to copy as part of the jump threading process.  */
static int stmt_count;

/* Array to record value-handles per SSA_NAME.  */
vec<tree> ssa_name_values;

/* Set the value for the SSA name NAME to VALUE.  */

void
set_ssa_name_value (tree name, tree value)
{
  if (SSA_NAME_VERSION (name) >= ssa_name_values.length ())
    ssa_name_values.safe_grow_cleared (SSA_NAME_VERSION (name) + 1);
  if (value && TREE_OVERFLOW_P (value))
    value = drop_tree_overflow (value);
  ssa_name_values[SSA_NAME_VERSION (name)] = value;
}

/* Initialize the per SSA_NAME value-handles array.  Returns it.  */
void
threadedge_initialize_values (void)
{
  gcc_assert (!ssa_name_values.exists ());
  ssa_name_values.create (num_ssa_names);
}

/* Free the per SSA_NAME value-handle array.  */
void
threadedge_finalize_values (void)
{
  ssa_name_values.release ();
}

/* Return TRUE if we may be able to thread an incoming edge into
   BB to an outgoing edge from BB.  Return FALSE otherwise.  */

bool
potentially_threadable_block (basic_block bb)
{
  gimple_stmt_iterator gsi;

  /* Special case.  We can get blocks that are forwarders, but are
     not optimized away because they forward from outside a loop
     to the loop header.   We want to thread through them as we can
     sometimes thread to the loop exit, which is obviously profitable. 
     the interesting case here is when the block has PHIs.  */
  if (gsi_end_p (gsi_start_nondebug_bb (bb))
      && !gsi_end_p (gsi_start_phis (bb)))
    return true;
  
  /* If BB has a single successor or a single predecessor, then
     there is no threading opportunity.  */
  if (single_succ_p (bb) || single_pred_p (bb))
    return false;

  /* If BB does not end with a conditional, switch or computed goto,
     then there is no threading opportunity.  */
  gsi = gsi_last_bb (bb);
  if (gsi_end_p (gsi)
      || ! gsi_stmt (gsi)
      || (gimple_code (gsi_stmt (gsi)) != GIMPLE_COND
	  && gimple_code (gsi_stmt (gsi)) != GIMPLE_GOTO
	  && gimple_code (gsi_stmt (gsi)) != GIMPLE_SWITCH))
    return false;

  return true;
}

/* Return the LHS of any ASSERT_EXPR where OP appears as the first
   argument to the ASSERT_EXPR and in which the ASSERT_EXPR dominates
   BB.  If no such ASSERT_EXPR is found, return OP.  */

static tree
lhs_of_dominating_assert (tree op, basic_block bb, gimple stmt)
{
  imm_use_iterator imm_iter;
  gimple use_stmt;
  use_operand_p use_p;

  FOR_EACH_IMM_USE_FAST (use_p, imm_iter, op)
    {
      use_stmt = USE_STMT (use_p);
      if (use_stmt != stmt
          && gimple_assign_single_p (use_stmt)
          && TREE_CODE (gimple_assign_rhs1 (use_stmt)) == ASSERT_EXPR
          && TREE_OPERAND (gimple_assign_rhs1 (use_stmt), 0) == op
	  && dominated_by_p (CDI_DOMINATORS, bb, gimple_bb (use_stmt)))
	{
	  return gimple_assign_lhs (use_stmt);
	}
    }
  return op;
}

/* We record temporary equivalences created by PHI nodes or
   statements within the target block.  Doing so allows us to
   identify more jump threading opportunities, even in blocks
   with side effects.

   We keep track of those temporary equivalences in a stack
   structure so that we can unwind them when we're done processing
   a particular edge.  This routine handles unwinding the data
   structures.  */

static void
remove_temporary_equivalences (vec<tree> *stack)
{
  while (stack->length () > 0)
    {
      tree prev_value, dest;

      dest = stack->pop ();

      /* A NULL value indicates we should stop unwinding, otherwise
	 pop off the next entry as they're recorded in pairs.  */
      if (dest == NULL)
	break;

      prev_value = stack->pop ();
      set_ssa_name_value (dest, prev_value);
    }
}

/* Record a temporary equivalence, saving enough information so that
   we can restore the state of recorded equivalences when we're
   done processing the current edge.  */

static void
record_temporary_equivalence (tree x, tree y, vec<tree> *stack)
{
  tree prev_x = SSA_NAME_VALUE (x);

  /* Y may be NULL if we are invalidating entries in the table.  */
  if (y && TREE_CODE (y) == SSA_NAME)
    {
      tree tmp = SSA_NAME_VALUE (y);
      y = tmp ? tmp : y;
    }

  set_ssa_name_value (x, y);
  stack->reserve (2);
  stack->quick_push (prev_x);
  stack->quick_push (x);
}

/* Record temporary equivalences created by PHIs at the target of the
   edge E.  Record unwind information for the equivalences onto STACK.

   If a PHI which prevents threading is encountered, then return FALSE
   indicating we should not thread this edge, else return TRUE. 

   If SRC_MAP/DST_MAP exist, then mark the source and destination SSA_NAMEs
   of any equivalences recorded.  We use this to make invalidation after
   traversing back edges less painful.  */

static bool
record_temporary_equivalences_from_phis (edge e, vec<tree> *stack)
{
  gphi_iterator gsi;

  /* Each PHI creates a temporary equivalence, record them.
     These are context sensitive equivalences and will be removed
     later.  */
  for (gsi = gsi_start_phis (e->dest); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      gphi *phi = gsi.phi ();
      tree src = PHI_ARG_DEF_FROM_EDGE (phi, e);
      tree dst = gimple_phi_result (phi);

      /* If the desired argument is not the same as this PHI's result
	 and it is set by a PHI in E->dest, then we can not thread
	 through E->dest.  */
      if (src != dst
	  && TREE_CODE (src) == SSA_NAME
	  && gimple_code (SSA_NAME_DEF_STMT (src)) == GIMPLE_PHI
	  && gimple_bb (SSA_NAME_DEF_STMT (src)) == e->dest)
	return false;

      /* We consider any non-virtual PHI as a statement since it
	 count result in a constant assignment or copy operation.  */
      if (!virtual_operand_p (dst))
	stmt_count++;

      record_temporary_equivalence (dst, src, stack);
    }
  return true;
}

/* Fold the RHS of an assignment statement and return it as a tree.
   May return NULL_TREE if no simplification is possible.  */

static tree
fold_assignment_stmt (gimple stmt)
{
  enum tree_code subcode = gimple_assign_rhs_code (stmt);

  switch (get_gimple_rhs_class (subcode))
    {
    case GIMPLE_SINGLE_RHS:
      return fold (gimple_assign_rhs1 (stmt));

    case GIMPLE_UNARY_RHS:
      {
        tree lhs = gimple_assign_lhs (stmt);
        tree op0 = gimple_assign_rhs1 (stmt);
        return fold_unary (subcode, TREE_TYPE (lhs), op0);
      }

    case GIMPLE_BINARY_RHS:
      {
        tree lhs = gimple_assign_lhs (stmt);
        tree op0 = gimple_assign_rhs1 (stmt);
        tree op1 = gimple_assign_rhs2 (stmt);
        return fold_binary (subcode, TREE_TYPE (lhs), op0, op1);
      }

    case GIMPLE_TERNARY_RHS:
      {
        tree lhs = gimple_assign_lhs (stmt);
        tree op0 = gimple_assign_rhs1 (stmt);
        tree op1 = gimple_assign_rhs2 (stmt);
        tree op2 = gimple_assign_rhs3 (stmt);

	/* Sadly, we have to handle conditional assignments specially
	   here, because fold expects all the operands of an expression
	   to be folded before the expression itself is folded, but we
	   can't just substitute the folded condition here.  */
        if (gimple_assign_rhs_code (stmt) == COND_EXPR)
	  op0 = fold (op0);

        return fold_ternary (subcode, TREE_TYPE (lhs), op0, op1, op2);
      }

    default:
      gcc_unreachable ();
    }
}

/* A new value has been assigned to LHS.  If necessary, invalidate any
   equivalences that are no longer valid.   This includes invaliding
   LHS and any objects that are currently equivalent to LHS.

   Finding the objects that are currently marked as equivalent to LHS
   is a bit tricky.  We could walk the ssa names and see if any have
   SSA_NAME_VALUE that is the same as LHS.  That's expensive.

   However, it's far more efficient to look at the unwinding stack as
   that will have all context sensitive equivalences which are the only
   ones that we really have to worry about here.   */
static void
invalidate_equivalences (tree lhs, vec<tree> *stack)
{

  /* The stack is an unwinding stack.  If the current element is NULL
     then it's a "stop unwinding" marker.  Else the current marker is
     the SSA_NAME with an equivalence and the prior entry in the stack
     is what the current element is equivalent to.  */
  for (int i = stack->length() - 1; i >= 0; i--)
    {
      /* Ignore the stop unwinding markers.  */
      if ((*stack)[i] == NULL)
	continue;

      /* We want to check the current value of stack[i] to see if
	 it matches LHS.  If so, then invalidate.  */
      if (SSA_NAME_VALUE ((*stack)[i]) == lhs)
	record_temporary_equivalence ((*stack)[i], NULL_TREE, stack);

      /* Remember, we're dealing with two elements in this case.  */
      i--;
    }

  /* And invalidate any known value for LHS itself.  */
  if (SSA_NAME_VALUE (lhs))
    record_temporary_equivalence (lhs, NULL_TREE, stack);
}

/* Try to simplify each statement in E->dest, ultimately leading to
   a simplification of the COND_EXPR at the end of E->dest.

   Record unwind information for temporary equivalences onto STACK.

   Use SIMPLIFY (a pointer to a callback function) to further simplify
   statements using pass specific information.

   We might consider marking just those statements which ultimately
   feed the COND_EXPR.  It's not clear if the overhead of bookkeeping
   would be recovered by trying to simplify fewer statements.

   If we are able to simplify a statement into the form
   SSA_NAME = (SSA_NAME | gimple invariant), then we can record
   a context sensitive equivalence which may help us simplify
   later statements in E->dest.  */

static gimple
record_temporary_equivalences_from_stmts_at_dest (edge e,
						  vec<tree> *stack,
						  tree (*simplify) (gimple,
								    gimple),
						  bool backedge_seen)
{
  gimple stmt = NULL;
  gimple_stmt_iterator gsi;
  int max_stmt_count;

  max_stmt_count = PARAM_VALUE (PARAM_MAX_JUMP_THREAD_DUPLICATION_STMTS);

  /* Walk through each statement in the block recording equivalences
     we discover.  Note any equivalences we discover are context
     sensitive (ie, are dependent on traversing E) and must be unwound
     when we're finished processing E.  */
  for (gsi = gsi_start_bb (e->dest); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      tree cached_lhs = NULL;

      stmt = gsi_stmt (gsi);

      /* Ignore empty statements and labels.  */
      if (gimple_code (stmt) == GIMPLE_NOP
	  || gimple_code (stmt) == GIMPLE_LABEL
	  || is_gimple_debug (stmt))
	continue;

      /* If the statement has volatile operands, then we assume we
	 can not thread through this block.  This is overly
	 conservative in some ways.  */
      if (gimple_code (stmt) == GIMPLE_ASM
	  && gimple_asm_volatile_p (as_a <gasm *> (stmt)))
	return NULL;

      /* If duplicating this block is going to cause too much code
	 expansion, then do not thread through this block.  */
      stmt_count++;
      if (stmt_count > max_stmt_count)
	return NULL;

      /* If this is not a statement that sets an SSA_NAME to a new
	 value, then do not try to simplify this statement as it will
	 not simplify in any way that is helpful for jump threading.  */
      if ((gimple_code (stmt) != GIMPLE_ASSIGN
           || TREE_CODE (gimple_assign_lhs (stmt)) != SSA_NAME)
          && (gimple_code (stmt) != GIMPLE_CALL
              || gimple_call_lhs (stmt) == NULL_TREE
              || TREE_CODE (gimple_call_lhs (stmt)) != SSA_NAME))
	{
	  /* STMT might still have DEFS and we need to invalidate any known
	     equivalences for them.

	     Consider if STMT is a GIMPLE_ASM with one or more outputs that
	     feeds a conditional inside a loop.  We might derive an equivalence
	     due to the conditional.  */
	  tree op;
	  ssa_op_iter iter;

	  if (backedge_seen)
	    FOR_EACH_SSA_TREE_OPERAND (op, stmt, iter, SSA_OP_DEF)
	      invalidate_equivalences (op, stack);

	  continue;
	}

      /* The result of __builtin_object_size depends on all the arguments
	 of a phi node. Temporarily using only one edge produces invalid
	 results. For example

	 if (x < 6)
	   goto l;
	 else
	   goto l;

	 l:
	 r = PHI <&w[2].a[1](2), &a.a[6](3)>
	 __builtin_object_size (r, 0)

	 The result of __builtin_object_size is defined to be the maximum of
	 remaining bytes. If we use only one edge on the phi, the result will
	 change to be the remaining bytes for the corresponding phi argument.

	 Similarly for __builtin_constant_p:

	 r = PHI <1(2), 2(3)>
	 __builtin_constant_p (r)

	 Both PHI arguments are constant, but x ? 1 : 2 is still not
	 constant.  */

      if (is_gimple_call (stmt))
	{
	  tree fndecl = gimple_call_fndecl (stmt);
	  if (fndecl
	      && (DECL_FUNCTION_CODE (fndecl) == BUILT_IN_OBJECT_SIZE
		  || DECL_FUNCTION_CODE (fndecl) == BUILT_IN_CONSTANT_P))
	    {
	      if (backedge_seen)
		{
		  tree lhs = gimple_get_lhs (stmt);
		  invalidate_equivalences (lhs, stack);
		}
	      continue;
	    }
	}

      /* At this point we have a statement which assigns an RHS to an
	 SSA_VAR on the LHS.  We want to try and simplify this statement
	 to expose more context sensitive equivalences which in turn may
	 allow us to simplify the condition at the end of the loop.

	 Handle simple copy operations as well as implied copies from
	 ASSERT_EXPRs.  */
      if (gimple_assign_single_p (stmt)
          && TREE_CODE (gimple_assign_rhs1 (stmt)) == SSA_NAME)
	cached_lhs = gimple_assign_rhs1 (stmt);
      else if (gimple_assign_single_p (stmt)
               && TREE_CODE (gimple_assign_rhs1 (stmt)) == ASSERT_EXPR)
	cached_lhs = TREE_OPERAND (gimple_assign_rhs1 (stmt), 0);
      else
	{
	  /* A statement that is not a trivial copy or ASSERT_EXPR.
	     We're going to temporarily copy propagate the operands
	     and see if that allows us to simplify this statement.  */
	  tree *copy;
	  ssa_op_iter iter;
	  use_operand_p use_p;
	  unsigned int num, i = 0;

	  num = NUM_SSA_OPERANDS (stmt, (SSA_OP_USE | SSA_OP_VUSE));
	  copy = XCNEWVEC (tree, num);

	  /* Make a copy of the uses & vuses into USES_COPY, then cprop into
	     the operands.  */
	  FOR_EACH_SSA_USE_OPERAND (use_p, stmt, iter, SSA_OP_USE | SSA_OP_VUSE)
	    {
	      tree tmp = NULL;
	      tree use = USE_FROM_PTR (use_p);

	      copy[i++] = use;
	      if (TREE_CODE (use) == SSA_NAME)
		tmp = SSA_NAME_VALUE (use);
	      if (tmp)
		SET_USE (use_p, tmp);
	    }

	  /* Try to fold/lookup the new expression.  Inserting the
	     expression into the hash table is unlikely to help.  */
          if (is_gimple_call (stmt))
            cached_lhs = fold_call_stmt (as_a <gcall *> (stmt), false);
	  else
            cached_lhs = fold_assignment_stmt (stmt);

          if (!cached_lhs
              || (TREE_CODE (cached_lhs) != SSA_NAME
                  && !is_gimple_min_invariant (cached_lhs)))
            cached_lhs = (*simplify) (stmt, stmt);

	  /* Restore the statement's original uses/defs.  */
	  i = 0;
	  FOR_EACH_SSA_USE_OPERAND (use_p, stmt, iter, SSA_OP_USE | SSA_OP_VUSE)
	    SET_USE (use_p, copy[i++]);

	  free (copy);
	}

      /* Record the context sensitive equivalence if we were able
	 to simplify this statement. 

	 If we have traversed a backedge at some point during threading,
	 then always enter something here.  Either a real equivalence, 
	 or a NULL_TREE equivalence which is effectively invalidation of
	 prior equivalences.  */
      if (cached_lhs
	  && (TREE_CODE (cached_lhs) == SSA_NAME
	      || is_gimple_min_invariant (cached_lhs)))
	record_temporary_equivalence (gimple_get_lhs (stmt), cached_lhs, stack);
      else if (backedge_seen)
	invalidate_equivalences (gimple_get_lhs (stmt), stack);
    }
  return stmt;
}

/* Once we have passed a backedge in the CFG when threading, we do not want to
   utilize edge equivalences for simplification purpose.  They are no longer
   necessarily valid.  We use this callback rather than the ones provided by
   DOM/VRP to achieve that effect.  */
static tree
dummy_simplify (gimple stmt1 ATTRIBUTE_UNUSED, gimple stmt2 ATTRIBUTE_UNUSED)
{
  return NULL_TREE;
}

/* Simplify the control statement at the end of the block E->dest.

   To avoid allocating memory unnecessarily, a scratch GIMPLE_COND
   is available to use/clobber in DUMMY_COND.

   Use SIMPLIFY (a pointer to a callback function) to further simplify
   a condition using pass specific information.

   Return the simplified condition or NULL if simplification could
   not be performed.  */

static tree
simplify_control_stmt_condition (edge e,
				 gimple stmt,
				 gcond *dummy_cond,
				 tree (*simplify) (gimple, gimple),
				 bool handle_dominating_asserts)
{
  tree cond, cached_lhs;
  enum gimple_code code = gimple_code (stmt);

  /* For comparisons, we have to update both operands, then try
     to simplify the comparison.  */
  if (code == GIMPLE_COND)
    {
      tree op0, op1;
      enum tree_code cond_code;

      op0 = gimple_cond_lhs (stmt);
      op1 = gimple_cond_rhs (stmt);
      cond_code = gimple_cond_code (stmt);

      /* Get the current value of both operands.  */
      if (TREE_CODE (op0) == SSA_NAME)
	{
	  for (int i = 0; i < 2; i++)
	    {
	      if (TREE_CODE (op0) == SSA_NAME
		  && SSA_NAME_VALUE (op0))
		op0 = SSA_NAME_VALUE (op0);
	      else
		break;
	    }
	}

      if (TREE_CODE (op1) == SSA_NAME)
	{
	  for (int i = 0; i < 2; i++)
	    {
	      if (TREE_CODE (op1) == SSA_NAME
		  && SSA_NAME_VALUE (op1))
		op1 = SSA_NAME_VALUE (op1);
	      else
		break;
	    }
	}

      if (handle_dominating_asserts)
	{
	  /* Now see if the operand was consumed by an ASSERT_EXPR
	     which dominates E->src.  If so, we want to replace the
	     operand with the LHS of the ASSERT_EXPR.  */
	  if (TREE_CODE (op0) == SSA_NAME)
	    op0 = lhs_of_dominating_assert (op0, e->src, stmt);

	  if (TREE_CODE (op1) == SSA_NAME)
	    op1 = lhs_of_dominating_assert (op1, e->src, stmt);
	}

      /* We may need to canonicalize the comparison.  For
	 example, op0 might be a constant while op1 is an
	 SSA_NAME.  Failure to canonicalize will cause us to
	 miss threading opportunities.  */
      if (tree_swap_operands_p (op0, op1, false))
	{
	  tree tmp;
	  cond_code = swap_tree_comparison (cond_code);
	  tmp = op0;
	  op0 = op1;
	  op1 = tmp;
	}

      /* Stuff the operator and operands into our dummy conditional
	 expression.  */
      gimple_cond_set_code (dummy_cond, cond_code);
      gimple_cond_set_lhs (dummy_cond, op0);
      gimple_cond_set_rhs (dummy_cond, op1);

      /* We absolutely do not care about any type conversions
         we only care about a zero/nonzero value.  */
      fold_defer_overflow_warnings ();

      cached_lhs = fold_binary (cond_code, boolean_type_node, op0, op1);
      if (cached_lhs)
	while (CONVERT_EXPR_P (cached_lhs))
          cached_lhs = TREE_OPERAND (cached_lhs, 0);

      fold_undefer_overflow_warnings ((cached_lhs
                                       && is_gimple_min_invariant (cached_lhs)),
				      stmt, WARN_STRICT_OVERFLOW_CONDITIONAL);

      /* If we have not simplified the condition down to an invariant,
	 then use the pass specific callback to simplify the condition.  */
      if (!cached_lhs
          || !is_gimple_min_invariant (cached_lhs))
        cached_lhs = (*simplify) (dummy_cond, stmt);

      return cached_lhs;
    }

  if (code == GIMPLE_SWITCH)
    cond = gimple_switch_index (as_a <gswitch *> (stmt));
  else if (code == GIMPLE_GOTO)
    cond = gimple_goto_dest (stmt);
  else
    gcc_unreachable ();

  /* We can have conditionals which just test the state of a variable
     rather than use a relational operator.  These are simpler to handle.  */
  if (TREE_CODE (cond) == SSA_NAME)
    {
      tree original_lhs = cond;
      cached_lhs = cond;

      /* Get the variable's current value from the equivalence chains.

	 It is possible to get loops in the SSA_NAME_VALUE chains
	 (consider threading the backedge of a loop where we have
	 a loop invariant SSA_NAME used in the condition.  */
      if (cached_lhs)
	{
	  for (int i = 0; i < 2; i++)
	    {
	      if (TREE_CODE (cached_lhs) == SSA_NAME
		  && SSA_NAME_VALUE (cached_lhs))
		cached_lhs = SSA_NAME_VALUE (cached_lhs);
	      else
		break;
	    }
	}

      /* If we're dominated by a suitable ASSERT_EXPR, then
	 update CACHED_LHS appropriately.  */
      if (handle_dominating_asserts && TREE_CODE (cached_lhs) == SSA_NAME)
	cached_lhs = lhs_of_dominating_assert (cached_lhs, e->src, stmt);

      /* If we haven't simplified to an invariant yet, then use the
	 pass specific callback to try and simplify it further.  */
      if (cached_lhs && ! is_gimple_min_invariant (cached_lhs))
        cached_lhs = (*simplify) (stmt, stmt);

      /* We couldn't find an invariant.  But, callers of this
	 function may be able to do something useful with the
	 unmodified destination.  */
      if (!cached_lhs)
	cached_lhs = original_lhs;
    }
  else
    cached_lhs = NULL;

  return cached_lhs;
}

/* Copy debug stmts from DEST's chain of single predecessors up to
   SRC, so that we don't lose the bindings as PHI nodes are introduced
   when DEST gains new predecessors.  */
void
propagate_threaded_block_debug_into (basic_block dest, basic_block src)
{
  if (!MAY_HAVE_DEBUG_STMTS)
    return;

  if (!single_pred_p (dest))
    return;

  gcc_checking_assert (dest != src);

  gimple_stmt_iterator gsi = gsi_after_labels (dest);
  int i = 0;
  const int alloc_count = 16; // ?? Should this be a PARAM?

  /* Estimate the number of debug vars overridden in the beginning of
     DEST, to tell how many we're going to need to begin with.  */
  for (gimple_stmt_iterator si = gsi;
       i * 4 <= alloc_count * 3 && !gsi_end_p (si); gsi_next (&si))
    {
      gimple stmt = gsi_stmt (si);
      if (!is_gimple_debug (stmt))
	break;
      i++;
    }

  auto_vec<tree, alloc_count> fewvars;
  hash_set<tree> *vars = NULL;

  /* If we're already starting with 3/4 of alloc_count, go for a
     hash_set, otherwise start with an unordered stack-allocated
     VEC.  */
  if (i * 4 > alloc_count * 3)
    vars = new hash_set<tree>;

  /* Now go through the initial debug stmts in DEST again, this time
     actually inserting in VARS or FEWVARS.  Don't bother checking for
     duplicates in FEWVARS.  */
  for (gimple_stmt_iterator si = gsi; !gsi_end_p (si); gsi_next (&si))
    {
      gimple stmt = gsi_stmt (si);
      if (!is_gimple_debug (stmt))
	break;

      tree var;

      if (gimple_debug_bind_p (stmt))
	var = gimple_debug_bind_get_var (stmt);
      else if (gimple_debug_source_bind_p (stmt))
	var = gimple_debug_source_bind_get_var (stmt);
      else
	gcc_unreachable ();

      if (vars)
	vars->add (var);
      else
	fewvars.quick_push (var);
    }

  basic_block bb = dest;

  do
    {
      bb = single_pred (bb);
      for (gimple_stmt_iterator si = gsi_last_bb (bb);
	   !gsi_end_p (si); gsi_prev (&si))
	{
	  gimple stmt = gsi_stmt (si);
	  if (!is_gimple_debug (stmt))
	    continue;

	  tree var;

	  if (gimple_debug_bind_p (stmt))
	    var = gimple_debug_bind_get_var (stmt);
	  else if (gimple_debug_source_bind_p (stmt))
	    var = gimple_debug_source_bind_get_var (stmt);
	  else
	    gcc_unreachable ();

	  /* Discard debug bind overlaps.  ??? Unlike stmts from src,
	     copied into a new block that will precede BB, debug bind
	     stmts in bypassed BBs may actually be discarded if
	     they're overwritten by subsequent debug bind stmts, which
	     might be a problem once we introduce stmt frontier notes
	     or somesuch.  Adding `&& bb == src' to the condition
	     below will preserve all potentially relevant debug
	     notes.  */
	  if (vars && vars->add (var))
	    continue;
	  else if (!vars)
	    {
	      int i = fewvars.length ();
	      while (i--)
		if (fewvars[i] == var)
		  break;
	      if (i >= 0)
		continue;

	      if (fewvars.length () < (unsigned) alloc_count)
		fewvars.quick_push (var);
	      else
		{
		  vars = new hash_set<tree>;
		  for (i = 0; i < alloc_count; i++)
		    vars->add (fewvars[i]);
		  fewvars.release ();
		  vars->add (var);
		}
	    }

	  stmt = gimple_copy (stmt);
	  /* ??? Should we drop the location of the copy to denote
	     they're artificial bindings?  */
	  gsi_insert_before (&gsi, stmt, GSI_NEW_STMT);
	}
    }
  while (bb != src && single_pred_p (bb));

  if (vars)
    delete vars;
  else if (fewvars.exists ())
    fewvars.release ();
}

/* See if TAKEN_EDGE->dest is a threadable block with no side effecs (ie, it
   need not be duplicated as part of the CFG/SSA updating process).

   If it is threadable, add it to PATH and VISITED and recurse, ultimately
   returning TRUE from the toplevel call.   Otherwise do nothing and
   return false.

   DUMMY_COND, HANDLE_DOMINATING_ASSERTS and SIMPLIFY are used to
   try and simplify the condition at the end of TAKEN_EDGE->dest.  */
static bool
thread_around_empty_blocks (edge taken_edge,
			    gcond *dummy_cond,
			    bool handle_dominating_asserts,
			    tree (*simplify) (gimple, gimple),
			    bitmap visited,
			    vec<jump_thread_edge *> *path,
			    bool *backedge_seen_p)
{
  basic_block bb = taken_edge->dest;
  gimple_stmt_iterator gsi;
  gimple stmt;
  tree cond;

  /* The key property of these blocks is that they need not be duplicated
     when threading.  Thus they can not have visible side effects such
     as PHI nodes.  */
  if (!gsi_end_p (gsi_start_phis (bb)))
    return false;

  /* Skip over DEBUG statements at the start of the block.  */
  gsi = gsi_start_nondebug_bb (bb);

  /* If the block has no statements, but does have a single successor, then
     it's just a forwarding block and we can thread through it trivially.

     However, note that just threading through empty blocks with single
     successors is not inherently profitable.  For the jump thread to
     be profitable, we must avoid a runtime conditional.

     By taking the return value from the recursive call, we get the
     desired effect of returning TRUE when we found a profitable jump
     threading opportunity and FALSE otherwise.

     This is particularly important when this routine is called after
     processing a joiner block.  Returning TRUE too aggressively in
     that case results in pointless duplication of the joiner block.  */
  if (gsi_end_p (gsi))
    {
      if (single_succ_p (bb))
	{
	  taken_edge = single_succ_edge (bb);
	  if (!bitmap_bit_p (visited, taken_edge->dest->index))
	    {
	      jump_thread_edge *x
		= new jump_thread_edge (taken_edge, EDGE_NO_COPY_SRC_BLOCK);
	      path->safe_push (x);
	      bitmap_set_bit (visited, taken_edge->dest->index);
	      *backedge_seen_p |= ((taken_edge->flags & EDGE_DFS_BACK) != 0);
	      if (*backedge_seen_p)
		simplify = dummy_simplify;
	      return thread_around_empty_blocks (taken_edge,
						 dummy_cond,
						 handle_dominating_asserts,
						 simplify,
						 visited,
						 path,
						 backedge_seen_p);
	    }
	}

      /* We have a block with no statements, but multiple successors?  */
      return false;
    }

  /* The only real statements this block can have are a control
     flow altering statement.  Anything else stops the thread.  */
  stmt = gsi_stmt (gsi);
  if (gimple_code (stmt) != GIMPLE_COND
      && gimple_code (stmt) != GIMPLE_GOTO
      && gimple_code (stmt) != GIMPLE_SWITCH)
    return false;

  /* If we have traversed a backedge, then we do not want to look
     at certain expressions in the table that can not be relied upon.
     Luckily the only code that looked at those expressions is the
     SIMPLIFY callback, which we replace if we can no longer use it.  */
  if (*backedge_seen_p)
    simplify = dummy_simplify;

  /* Extract and simplify the condition.  */
  cond = simplify_control_stmt_condition (taken_edge, stmt, dummy_cond,
					  simplify, handle_dominating_asserts);

  /* If the condition can be statically computed and we have not already
     visited the destination edge, then add the taken edge to our thread
     path.  */
  if (cond && is_gimple_min_invariant (cond))
    {
      taken_edge = find_taken_edge (bb, cond);

      if (bitmap_bit_p (visited, taken_edge->dest->index))
	return false;
      bitmap_set_bit (visited, taken_edge->dest->index);

      jump_thread_edge *x
	= new jump_thread_edge (taken_edge, EDGE_NO_COPY_SRC_BLOCK);
      path->safe_push (x);
      *backedge_seen_p |= ((taken_edge->flags & EDGE_DFS_BACK) != 0);
      if (*backedge_seen_p)
	simplify = dummy_simplify;

      thread_around_empty_blocks (taken_edge,
				  dummy_cond,
				  handle_dominating_asserts,
				  simplify,
				  visited,
				  path,
				  backedge_seen_p);
      return true;
    }

  return false;
}

/* Return true if the CFG contains at least one path from START_BB to END_BB.
   When a path is found, record in PATH the blocks from END_BB to START_BB.
   VISITED_BBS is used to make sure we don't fall into an infinite loop.  Bound
   the recursion to basic blocks belonging to LOOP.  */

static bool
fsm_find_thread_path (basic_block start_bb, basic_block end_bb,
		      vec<basic_block, va_gc> *&path,
		      hash_set<basic_block> *visited_bbs, loop_p loop)
{
  if (loop != start_bb->loop_father)
    return false;

  if (start_bb == end_bb)
    {
      vec_safe_push (path, start_bb);
      return true;
    }

  if (!visited_bbs->add (start_bb))
    {
      edge e;
      edge_iterator ei;
      FOR_EACH_EDGE (e, ei, start_bb->succs)
	if (fsm_find_thread_path (e->dest, end_bb, path, visited_bbs, loop))
	  {
	    vec_safe_push (path, start_bb);
	    return true;
	  }
    }

  return false;
}

static int max_threaded_paths;

/* We trace the value of the variable EXPR back through any phi nodes looking
   for places where it gets a constant value and save the path.  Stop after
   having recorded MAX_PATHS jump threading paths.  */

static void
fsm_find_control_statement_thread_paths (tree expr,
					 hash_set<basic_block> *visited_bbs,
					 vec<basic_block, va_gc> *&path,
					 bool seen_loop_phi)
{
  tree var = SSA_NAME_VAR (expr);
  gimple def_stmt = SSA_NAME_DEF_STMT (expr);
  basic_block var_bb = gimple_bb (def_stmt);

  if (var == NULL || var_bb == NULL)
    return;

  /* For the moment we assume that an SSA chain only contains phi nodes, and
     eventually one of the phi arguments will be an integer constant.  In the
     future, this could be extended to also handle simple assignments of
     arithmetic operations.  */
  if (gimple_code (def_stmt) != GIMPLE_PHI)
    return;

  /* Avoid infinite recursion.  */
  if (visited_bbs->add (var_bb))
    return;

  gphi *phi = as_a <gphi *> (def_stmt);
  int next_path_length = 0;
  basic_block last_bb_in_path = path->last ();

  if (loop_containing_stmt (phi)->header == gimple_bb (phi))
    {
      /* Do not walk through more than one loop PHI node.  */
      if (seen_loop_phi)
	return;
      seen_loop_phi = true;
    }

  /* Following the chain of SSA_NAME definitions, we jumped from a definition in
     LAST_BB_IN_PATH to a definition in VAR_BB.  When these basic blocks are
     different, append to PATH the blocks from LAST_BB_IN_PATH to VAR_BB.  */
  if (var_bb != last_bb_in_path)
    {
      edge e;
      int e_count = 0;
      edge_iterator ei;
      vec<basic_block, va_gc> *next_path;
      vec_alloc (next_path, n_basic_blocks_for_fn (cfun));

      FOR_EACH_EDGE (e, ei, last_bb_in_path->preds)
	{
	  hash_set<basic_block> *visited_bbs = new hash_set<basic_block>;

	  if (fsm_find_thread_path (var_bb, e->src, next_path, visited_bbs,
				    e->src->loop_father))
	    ++e_count;

	  delete visited_bbs;

	  /* If there is more than one path, stop.  */
	  if (e_count > 1)
	    {
	      vec_free (next_path);
	      return;
	    }
	}

      /* Stop if we have not found a path: this could occur when the recursion
	 is stopped by one of the bounds.  */
      if (e_count == 0)
	{
	  vec_free (next_path);
	  return;
	}

      /* Append all the nodes from NEXT_PATH to PATH.  */
      vec_safe_splice (path, next_path);
      next_path_length = next_path->length ();
      vec_free (next_path);
    }

  gcc_assert (path->last () == var_bb);

  /* Iterate over the arguments of PHI.  */
  unsigned int i;
  for (i = 0; i < gimple_phi_num_args (phi); i++)
    {
      tree arg = gimple_phi_arg_def (phi, i);
      basic_block bbi = gimple_phi_arg_edge (phi, i)->src;

      /* Skip edges pointing outside the current loop.  */
      if (!arg || var_bb->loop_father != bbi->loop_father)
	continue;

      if (TREE_CODE (arg) == SSA_NAME)
	{
	  vec_safe_push (path, bbi);
	  /* Recursively follow SSA_NAMEs looking for a constant definition.  */
	  fsm_find_control_statement_thread_paths (arg, visited_bbs, path,
						   seen_loop_phi);

	  path->pop ();
	  continue;
	}

      if (TREE_CODE (arg) != INTEGER_CST)
	continue;

      int path_length = path->length ();
      /* A path with less than 2 basic blocks should not be jump-threaded.  */
      if (path_length < 2)
	continue;

      if (path_length > PARAM_VALUE (PARAM_MAX_FSM_THREAD_LENGTH))
	{
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file, "FSM jump-thread path not considered: "
		     "the number of basic blocks on the path "
		     "exceeds PARAM_MAX_FSM_THREAD_LENGTH.\n");
	  continue;
	}

      if (max_threaded_paths <= 0)
	{
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file, "FSM jump-thread path not considered: "
		     "the number of previously recorded FSM paths to thread "
		     "exceeds PARAM_MAX_FSM_THREAD_PATHS.\n");
	  continue;
	}

      /* Add BBI to the path.  */
      vec_safe_push (path, bbi);
      ++path_length;

      int n_insns = 0;
      gimple_stmt_iterator gsi;
      int j;
      loop_p loop = (*path)[0]->loop_father;
      bool path_crosses_loops = false;

      /* Count the number of instructions on the path: as these instructions
	 will have to be duplicated, we will not record the path if there are
	 too many instructions on the path.  Also check that all the blocks in
	 the path belong to a single loop.  */
      for (j = 1; j < path_length - 1; j++)
	{
	  basic_block bb = (*path)[j];

	  if (bb->loop_father != loop)
	    {
	      path_crosses_loops = true;
	      break;
	    }

	  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
	    {
	      gimple stmt = gsi_stmt (gsi);
	      /* Do not count empty statements and labels.  */
	      if (gimple_code (stmt) != GIMPLE_NOP
		  && gimple_code (stmt) != GIMPLE_LABEL
		  && !is_gimple_debug (stmt))
		++n_insns;
	    }
	}

      if (path_crosses_loops)
	{
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file, "FSM jump-thread path not considered: "
		     "the path crosses loops.\n");
	  path->pop ();
	  continue;
	}

      if (n_insns >= PARAM_VALUE (PARAM_MAX_FSM_THREAD_PATH_INSNS))
	{
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file, "FSM jump-thread path not considered: "
		     "the number of instructions on the path "
		     "exceeds PARAM_MAX_FSM_THREAD_PATH_INSNS.\n");
	  path->pop ();
	  continue;
	}

      vec<jump_thread_edge *> *jump_thread_path
	= new vec<jump_thread_edge *> ();

      /* Record the edges between the blocks in PATH.  */
      for (j = 0; j < path_length - 1; j++)
	{
	  edge e = find_edge ((*path)[path_length - j - 1],
			      (*path)[path_length - j - 2]);
	  gcc_assert (e);
	  jump_thread_edge *x = new jump_thread_edge (e, EDGE_FSM_THREAD);
	  jump_thread_path->safe_push (x);
	}

      /* Add the edge taken when the control variable has value ARG.  */
      edge taken_edge = find_taken_edge ((*path)[0], arg);
      jump_thread_edge *x
	= new jump_thread_edge (taken_edge, EDGE_NO_COPY_SRC_BLOCK);
      jump_thread_path->safe_push (x);

      register_jump_thread (jump_thread_path);
      --max_threaded_paths;

      /* Remove BBI from the path.  */
      path->pop ();
    }

  /* Remove all the nodes that we added from NEXT_PATH.  */
  if (next_path_length)
    vec_safe_truncate (path, (path->length () - next_path_length));
}

/* We are exiting E->src, see if E->dest ends with a conditional
   jump which has a known value when reached via E.

   E->dest can have arbitrary side effects which, if threading is
   successful, will be maintained.

   Special care is necessary if E is a back edge in the CFG as we
   may have already recorded equivalences for E->dest into our
   various tables, including the result of the conditional at
   the end of E->dest.  Threading opportunities are severely
   limited in that case to avoid short-circuiting the loop
   incorrectly.

   DUMMY_COND is a shared cond_expr used by condition simplification as scratch,
   to avoid allocating memory.

   HANDLE_DOMINATING_ASSERTS is true if we should try to replace operands of
   the simplified condition with left-hand sides of ASSERT_EXPRs they are
   used in.

   STACK is used to undo temporary equivalences created during the walk of
   E->dest.

   SIMPLIFY is a pass-specific function used to simplify statements.

   Our caller is responsible for restoring the state of the expression
   and const_and_copies stacks.

   Positive return value is success.  Zero return value is failure, but
   the block can still be duplicated as a joiner in a jump thread path,
   negative indicates the block should not be duplicated and thus is not
   suitable for a joiner in a jump threading path.  */

static int
thread_through_normal_block (edge e,
			     gcond *dummy_cond,
			     bool handle_dominating_asserts,
			     vec<tree> *stack,
			     tree (*simplify) (gimple, gimple),
			     vec<jump_thread_edge *> *path,
			     bitmap visited,
			     bool *backedge_seen_p)
{
  /* If we have traversed a backedge, then we do not want to look
     at certain expressions in the table that can not be relied upon.
     Luckily the only code that looked at those expressions is the
     SIMPLIFY callback, which we replace if we can no longer use it.  */
  if (*backedge_seen_p)
    simplify = dummy_simplify;

  /* PHIs create temporary equivalences.
     Note that if we found a PHI that made the block non-threadable, then
     we need to bubble that up to our caller in the same manner we do
     when we prematurely stop processing statements below.  */
  if (!record_temporary_equivalences_from_phis (e, stack))
    return -1;

  /* Now walk each statement recording any context sensitive
     temporary equivalences we can detect.  */
  gimple stmt
    = record_temporary_equivalences_from_stmts_at_dest (e, stack, simplify,
							*backedge_seen_p);

  /* There's two reasons STMT might be null, and distinguishing
     between them is important.

     First the block may not have had any statements.  For example, it
     might have some PHIs and unconditionally transfer control elsewhere.
     Such blocks are suitable for jump threading, particularly as a
     joiner block.

     The second reason would be if we did not process all the statements
     in the block (because there were too many to make duplicating the
     block profitable.   If we did not look at all the statements, then
     we may not have invalidated everything needing invalidation.  Thus
     we must signal to our caller that this block is not suitable for
     use as a joiner in a threading path.  */
  if (!stmt)
    {
      /* First case.  The statement simply doesn't have any instructions, but
	 does have PHIs.  */
      if (gsi_end_p (gsi_start_nondebug_bb (e->dest))
	  && !gsi_end_p (gsi_start_phis (e->dest)))
	return 0;

      /* Second case.  */
      return -1;
    }
  
  /* If we stopped at a COND_EXPR or SWITCH_EXPR, see if we know which arm
     will be taken.  */
  if (gimple_code (stmt) == GIMPLE_COND
      || gimple_code (stmt) == GIMPLE_GOTO
      || gimple_code (stmt) == GIMPLE_SWITCH)
    {
      tree cond;

      /* Extract and simplify the condition.  */
      cond = simplify_control_stmt_condition (e, stmt, dummy_cond, simplify,
					      handle_dominating_asserts);

      if (!cond)
	return 0;

      if (is_gimple_min_invariant (cond))
	{
	  edge taken_edge = find_taken_edge (e->dest, cond);
	  basic_block dest = (taken_edge ? taken_edge->dest : NULL);

	  /* DEST could be NULL for a computed jump to an absolute
	     address.  */
	  if (dest == NULL
	      || dest == e->dest
	      || bitmap_bit_p (visited, dest->index))
	    return 0;

	  /* Only push the EDGE_START_JUMP_THREAD marker if this is
	     first edge on the path.  */
	  if (path->length () == 0)
	    {
              jump_thread_edge *x
	        = new jump_thread_edge (e, EDGE_START_JUMP_THREAD);
	      path->safe_push (x);
	      *backedge_seen_p |= ((e->flags & EDGE_DFS_BACK) != 0);
	    }

	  jump_thread_edge *x
	    = new jump_thread_edge (taken_edge, EDGE_COPY_SRC_BLOCK);
	  path->safe_push (x);
	  *backedge_seen_p |= ((taken_edge->flags & EDGE_DFS_BACK) != 0);
	  if (*backedge_seen_p)
	    simplify = dummy_simplify;

	  /* See if we can thread through DEST as well, this helps capture
	     secondary effects of threading without having to re-run DOM or
	     VRP. 

	     We don't want to thread back to a block we have already
 	     visited.  This may be overly conservative.  */
	  bitmap_set_bit (visited, dest->index);
	  bitmap_set_bit (visited, e->dest->index);
	  thread_around_empty_blocks (taken_edge,
				      dummy_cond,
				      handle_dominating_asserts,
				      simplify,
				      visited,
				      path,
				      backedge_seen_p);
	  return 1;
	}

      if (!flag_expensive_optimizations
	  || optimize_function_for_size_p (cfun)
	  || TREE_CODE (cond) != SSA_NAME
	  || e->dest->loop_father != e->src->loop_father
	  || loop_depth (e->dest->loop_father) == 0)
	return 0;

      /* When COND cannot be simplified, try to find paths from a control
	 statement back through the PHI nodes which would affect that control
	 statement.  */
      vec<basic_block, va_gc> *bb_path;
      vec_alloc (bb_path, n_basic_blocks_for_fn (cfun));
      vec_safe_push (bb_path, e->dest);
      hash_set<basic_block> *visited_bbs = new hash_set<basic_block>;

      max_threaded_paths = PARAM_VALUE (PARAM_MAX_FSM_THREAD_PATHS);
      fsm_find_control_statement_thread_paths (cond, visited_bbs, bb_path,
					       false);

      delete visited_bbs;
      vec_free (bb_path);
    }
  return 0;
}

/* We are exiting E->src, see if E->dest ends with a conditional
   jump which has a known value when reached via E.

   Special care is necessary if E is a back edge in the CFG as we
   may have already recorded equivalences for E->dest into our
   various tables, including the result of the conditional at
   the end of E->dest.  Threading opportunities are severely
   limited in that case to avoid short-circuiting the loop
   incorrectly.

   Note it is quite common for the first block inside a loop to
   end with a conditional which is either always true or always
   false when reached via the loop backedge.  Thus we do not want
   to blindly disable threading across a loop backedge.

   DUMMY_COND is a shared cond_expr used by condition simplification as scratch,
   to avoid allocating memory.

   HANDLE_DOMINATING_ASSERTS is true if we should try to replace operands of
   the simplified condition with left-hand sides of ASSERT_EXPRs they are
   used in.

   STACK is used to undo temporary equivalences created during the walk of
   E->dest.

   SIMPLIFY is a pass-specific function used to simplify statements.  */

void
thread_across_edge (gcond *dummy_cond,
		    edge e,
		    bool handle_dominating_asserts,
		    vec<tree> *stack,
		    tree (*simplify) (gimple, gimple))
{
  bitmap visited = BITMAP_ALLOC (NULL);
  bool backedge_seen;

  stmt_count = 0;

  vec<jump_thread_edge *> *path = new vec<jump_thread_edge *> ();
  bitmap_clear (visited);
  bitmap_set_bit (visited, e->src->index);
  bitmap_set_bit (visited, e->dest->index);
  backedge_seen = ((e->flags & EDGE_DFS_BACK) != 0);
  if (backedge_seen)
    simplify = dummy_simplify;

  int threaded = thread_through_normal_block (e, dummy_cond,
					      handle_dominating_asserts,
					      stack, simplify, path,
					      visited, &backedge_seen);
  if (threaded > 0)
    {
      propagate_threaded_block_debug_into (path->last ()->e->dest,
					   e->dest);
      remove_temporary_equivalences (stack);
      BITMAP_FREE (visited);
      register_jump_thread (path);
      return;
    }
  else
    {
      /* Negative and zero return values indicate no threading was possible,
	 thus there should be no edges on the thread path and no need to walk
	 through the vector entries.  */
      gcc_assert (path->length () == 0);
      path->release ();
      delete path;

      /* A negative status indicates the target block was deemed too big to
	 duplicate.  Just quit now rather than trying to use the block as
	 a joiner in a jump threading path.

	 This prevents unnecessary code growth, but more importantly if we
	 do not look at all the statements in the block, then we may have
	 missed some invalidations if we had traversed a backedge!  */
      if (threaded < 0)
	{
	  BITMAP_FREE (visited);
	  remove_temporary_equivalences (stack);
	  return;
	}
    }

 /* We were unable to determine what out edge from E->dest is taken.  However,
    we might still be able to thread through successors of E->dest.  This
    often occurs when E->dest is a joiner block which then fans back out
    based on redundant tests.

    If so, we'll copy E->dest and redirect the appropriate predecessor to
    the copy.  Within the copy of E->dest, we'll thread one or more edges
    to points deeper in the CFG.

    This is a stopgap until we have a more structured approach to path
    isolation.  */
  {
    edge taken_edge;
    edge_iterator ei;
    bool found;

    /* If E->dest has abnormal outgoing edges, then there's no guarantee
       we can safely redirect any of the edges.  Just punt those cases.  */
    FOR_EACH_EDGE (taken_edge, ei, e->dest->succs)
      if (taken_edge->flags & EDGE_ABNORMAL)
	{
	  remove_temporary_equivalences (stack);
	  BITMAP_FREE (visited);
	  return;
	}

    /* Look at each successor of E->dest to see if we can thread through it.  */
    FOR_EACH_EDGE (taken_edge, ei, e->dest->succs)
      {
	/* Push a fresh marker so we can unwind the equivalences created
	   for each of E->dest's successors.  */
	stack->safe_push (NULL_TREE);
     
	/* Avoid threading to any block we have already visited.  */
	bitmap_clear (visited);
	bitmap_set_bit (visited, e->src->index);
	bitmap_set_bit (visited, e->dest->index);
	bitmap_set_bit (visited, taken_edge->dest->index);
        vec<jump_thread_edge *> *path = new vec<jump_thread_edge *> ();

	/* Record whether or not we were able to thread through a successor
	   of E->dest.  */
        jump_thread_edge *x = new jump_thread_edge (e, EDGE_START_JUMP_THREAD);
	path->safe_push (x);

        x = new jump_thread_edge (taken_edge, EDGE_COPY_SRC_JOINER_BLOCK);
	path->safe_push (x);
	found = false;
	backedge_seen = ((e->flags & EDGE_DFS_BACK) != 0);
	backedge_seen |= ((taken_edge->flags & EDGE_DFS_BACK) != 0);
	if (backedge_seen)
	  simplify = dummy_simplify;
	found = thread_around_empty_blocks (taken_edge,
					    dummy_cond,
					    handle_dominating_asserts,
					    simplify,
					    visited,
					    path,
					    &backedge_seen);

	if (backedge_seen)
	  simplify = dummy_simplify;

	if (!found)
	  found = thread_through_normal_block (path->last ()->e, dummy_cond,
					       handle_dominating_asserts,
					       stack, simplify, path, visited,
					       &backedge_seen) > 0;

	/* If we were able to thread through a successor of E->dest, then
	   record the jump threading opportunity.  */
	if (found)
	  {
	    propagate_threaded_block_debug_into (path->last ()->e->dest,
						 taken_edge->dest);
	    register_jump_thread (path);
	  }
	else
	  {
	    delete_jump_thread_path (path);
	  }

	/* And unwind the equivalence table.  */
	remove_temporary_equivalences (stack);
      }
    BITMAP_FREE (visited);
  }

  remove_temporary_equivalences (stack);
}
