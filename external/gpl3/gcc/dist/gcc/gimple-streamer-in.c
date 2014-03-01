/* Routines for reading GIMPLE from a file stream.

   Copyright (C) 2011-2013 Free Software Foundation, Inc.
   Contributed by Diego Novillo <dnovillo@google.com>

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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "diagnostic.h"
#include "tree.h"
#include "tree-flow.h"
#include "data-streamer.h"
#include "tree-streamer.h"
#include "gimple-streamer.h"

/* Read a PHI function for basic block BB in function FN.  DATA_IN is
   the file being read.  IB is the input block to use for reading.  */

static gimple
input_phi (struct lto_input_block *ib, basic_block bb, struct data_in *data_in,
	   struct function *fn)
{
  unsigned HOST_WIDE_INT ix;
  tree phi_result;
  int i, len;
  gimple result;

  ix = streamer_read_uhwi (ib);
  phi_result = (*SSANAMES (fn))[ix];
  len = EDGE_COUNT (bb->preds);
  result = create_phi_node (phi_result, bb);

  /* We have to go through a lookup process here because the preds in the
     reconstructed graph are generally in a different order than they
     were in the original program.  */
  for (i = 0; i < len; i++)
    {
      tree def = stream_read_tree (ib, data_in);
      int src_index = streamer_read_uhwi (ib);
      bitpack_d bp = streamer_read_bitpack (ib);
      location_t arg_loc = stream_input_location (&bp, data_in);
      basic_block sbb = BASIC_BLOCK_FOR_FUNCTION (fn, src_index);

      edge e = NULL;
      int j;

      for (j = 0; j < len; j++)
	if (EDGE_PRED (bb, j)->src == sbb)
	  {
	    e = EDGE_PRED (bb, j);
	    break;
	  }

      add_phi_arg (result, def, e, arg_loc);
    }

  return result;
}


/* Read a statement with tag TAG in function FN from block IB using
   descriptors in DATA_IN.  */

static gimple
input_gimple_stmt (struct lto_input_block *ib, struct data_in *data_in,
		   struct function *fn, enum LTO_tags tag)
{
  gimple stmt;
  enum gimple_code code;
  unsigned HOST_WIDE_INT num_ops;
  size_t i;
  struct bitpack_d bp;

  code = lto_tag_to_gimple_code (tag);

  /* Read the tuple header.  */
  bp = streamer_read_bitpack (ib);
  num_ops = bp_unpack_var_len_unsigned (&bp);
  stmt = gimple_alloc (code, num_ops);
  stmt->gsbase.no_warning = bp_unpack_value (&bp, 1);
  if (is_gimple_assign (stmt))
    stmt->gsbase.nontemporal_move = bp_unpack_value (&bp, 1);
  stmt->gsbase.has_volatile_ops = bp_unpack_value (&bp, 1);
  stmt->gsbase.subcode = bp_unpack_var_len_unsigned (&bp);

  /* Read location information.  */
  gimple_set_location (stmt, stream_input_location (&bp, data_in));

  /* Read lexical block reference.  */
  gimple_set_block (stmt, stream_read_tree (ib, data_in));

  /* Read in all the operands.  */
  switch (code)
    {
    case GIMPLE_RESX:
      gimple_resx_set_region (stmt, streamer_read_hwi (ib));
      break;

    case GIMPLE_EH_MUST_NOT_THROW:
      gimple_eh_must_not_throw_set_fndecl (stmt, stream_read_tree (ib, data_in));
      break;

    case GIMPLE_EH_DISPATCH:
      gimple_eh_dispatch_set_region (stmt, streamer_read_hwi (ib));
      break;

    case GIMPLE_ASM:
      {
	/* FIXME lto.  Move most of this into a new gimple_asm_set_string().  */
	tree str;
	stmt->gimple_asm.ni = streamer_read_uhwi (ib);
	stmt->gimple_asm.no = streamer_read_uhwi (ib);
	stmt->gimple_asm.nc = streamer_read_uhwi (ib);
	stmt->gimple_asm.nl = streamer_read_uhwi (ib);
	str = streamer_read_string_cst (data_in, ib);
	stmt->gimple_asm.string = TREE_STRING_POINTER (str);
      }
      /* Fallthru  */

    case GIMPLE_ASSIGN:
    case GIMPLE_CALL:
    case GIMPLE_RETURN:
    case GIMPLE_SWITCH:
    case GIMPLE_LABEL:
    case GIMPLE_COND:
    case GIMPLE_GOTO:
    case GIMPLE_DEBUG:
      for (i = 0; i < num_ops; i++)
	{
	  tree *opp, op = stream_read_tree (ib, data_in);
	  gimple_set_op (stmt, i, op);
	  if (!op)
	    continue;

	  opp = gimple_op_ptr (stmt, i);
	  if (TREE_CODE (*opp) == ADDR_EXPR)
	    opp = &TREE_OPERAND (*opp, 0);
	  while (handled_component_p (*opp))
	    {
	      if (TREE_CODE (*opp) == COMPONENT_REF)
		{
		  /* Fixup FIELD_DECLs in COMPONENT_REFs, they are not handled
		     by decl merging.  */
		  tree field, type, tem;
		  tree closest_match = NULL_TREE;
		  field = TREE_OPERAND (*opp, 1);
		  type = DECL_CONTEXT (field);
		  for (tem = TYPE_FIELDS (type); tem; tem = TREE_CHAIN (tem))
		    {
		      if (TREE_CODE (tem) != FIELD_DECL)
			continue;
		      if (tem == field)
			break;
		      if (DECL_NONADDRESSABLE_P (tem)
			  == DECL_NONADDRESSABLE_P (field)
			  && gimple_compare_field_offset (tem, field))
			{
			  if (types_compatible_p (TREE_TYPE (tem),
						  TREE_TYPE (field)))
			    break;
			  else
			    closest_match = tem;
			}
		    }
		  /* In case of type mismatches across units we can fail
		     to unify some types and thus not find a proper
		     field-decl here.  */
		  if (tem == NULL_TREE)
		    {
		      /* Thus, emit a ODR violation warning.  */
		      if (warning_at (gimple_location (stmt), 0,
				      "use of type %<%E%> with two mismatching "
				      "declarations at field %<%E%>",
				      type, TREE_OPERAND (*opp, 1)))
			{
			  if (TYPE_FIELDS (type))
			    inform (DECL_SOURCE_LOCATION (TYPE_FIELDS (type)),
				    "original type declared here");
			  inform (DECL_SOURCE_LOCATION (TREE_OPERAND (*opp, 1)),
				  "field in mismatching type declared here");
			  if (TYPE_NAME (TREE_TYPE (field))
			      && (TREE_CODE (TYPE_NAME (TREE_TYPE (field)))
				  == TYPE_DECL))
			    inform (DECL_SOURCE_LOCATION
				      (TYPE_NAME (TREE_TYPE (field))),
				    "type of field declared here");
			  if (closest_match
			      && TYPE_NAME (TREE_TYPE (closest_match))
			      && (TREE_CODE (TYPE_NAME
				   (TREE_TYPE (closest_match))) == TYPE_DECL))
			    inform (DECL_SOURCE_LOCATION
				      (TYPE_NAME (TREE_TYPE (closest_match))),
				    "type of mismatching field declared here");
			}
		      /* And finally fixup the types.  */
		      TREE_OPERAND (*opp, 0)
			= build1 (VIEW_CONVERT_EXPR, type,
				  TREE_OPERAND (*opp, 0));
		    }
		  else
		    TREE_OPERAND (*opp, 1) = tem;
		}
	      else if ((TREE_CODE (*opp) == ARRAY_REF
			|| TREE_CODE (*opp) == ARRAY_RANGE_REF)
		       && (TREE_CODE (TREE_TYPE (TREE_OPERAND (*opp, 0)))
			   != ARRAY_TYPE))
		{
		  /* And ARRAY_REFs to objects that had mismatched types
		     during symbol merging to avoid ICEs.  */
		  TREE_OPERAND (*opp, 0)
		    = build1 (VIEW_CONVERT_EXPR,
			      build_array_type (TREE_TYPE (*opp), NULL_TREE),
			      TREE_OPERAND (*opp, 0));
		}

	      opp = &TREE_OPERAND (*opp, 0);
	    }
	  /* At LTO output time we wrap all global decls in MEM_REFs to
	     allow seamless replacement with prevailing decls.  Undo this
	     here if the prevailing decl allows for this.
	     ???  Maybe we should simply fold all stmts.  */
	  if (TREE_CODE (*opp) == MEM_REF
	      && TREE_CODE (TREE_OPERAND (*opp, 0)) == ADDR_EXPR
	      && integer_zerop (TREE_OPERAND (*opp, 1))
	      && (TREE_THIS_VOLATILE (*opp)
		  == TREE_THIS_VOLATILE
		       (TREE_OPERAND (TREE_OPERAND (*opp, 0), 0)))
	      && !TYPE_REF_CAN_ALIAS_ALL (TREE_TYPE (TREE_OPERAND (*opp, 1)))
	      && (TREE_TYPE (*opp)
		  == TREE_TYPE (TREE_TYPE (TREE_OPERAND (*opp, 1))))
	      && (TREE_TYPE (*opp)
		  == TREE_TYPE (TREE_OPERAND (TREE_OPERAND (*opp, 0), 0))))
	    *opp = TREE_OPERAND (TREE_OPERAND (*opp, 0), 0);
	}
      if (is_gimple_call (stmt))
	{
	  if (gimple_call_internal_p (stmt))
	    gimple_call_set_internal_fn
	      (stmt, streamer_read_enum (ib, internal_fn, IFN_LAST));
	  else
	    gimple_call_set_fntype (stmt, stream_read_tree (ib, data_in));
	}
      break;

    case GIMPLE_NOP:
    case GIMPLE_PREDICT:
      break;

    case GIMPLE_TRANSACTION:
      gimple_transaction_set_label (stmt, stream_read_tree (ib, data_in));
      break;

    default:
      internal_error ("bytecode stream: unknown GIMPLE statement tag %s",
		      lto_tag_name (tag));
    }

  /* Update the properties of symbols, SSA names and labels associated
     with STMT.  */
  if (code == GIMPLE_ASSIGN || code == GIMPLE_CALL)
    {
      tree lhs = gimple_get_lhs (stmt);
      if (lhs && TREE_CODE (lhs) == SSA_NAME)
	SSA_NAME_DEF_STMT (lhs) = stmt;
    }
  else if (code == GIMPLE_LABEL)
    gcc_assert (emit_label_in_global_context_p (gimple_label_label (stmt))
	        || DECL_CONTEXT (gimple_label_label (stmt)) == fn->decl);
  else if (code == GIMPLE_ASM)
    {
      unsigned i;

      for (i = 0; i < gimple_asm_noutputs (stmt); i++)
	{
	  tree op = TREE_VALUE (gimple_asm_output_op (stmt, i));
	  if (TREE_CODE (op) == SSA_NAME)
	    SSA_NAME_DEF_STMT (op) = stmt;
	}
    }

  /* Reset alias information.  */
  if (code == GIMPLE_CALL)
    gimple_call_reset_alias_info (stmt);

  /* Mark the statement modified so its operand vectors can be filled in.  */
  gimple_set_modified (stmt, true);

  return stmt;
}


/* Read a basic block with tag TAG from DATA_IN using input block IB.
   FN is the function being processed.  */

void
input_bb (struct lto_input_block *ib, enum LTO_tags tag,
	  struct data_in *data_in, struct function *fn,
	  int count_materialization_scale)
{
  unsigned int index;
  basic_block bb;
  gimple_stmt_iterator bsi;

  /* This routine assumes that CFUN is set to FN, as it needs to call
     basic GIMPLE routines that use CFUN.  */
  gcc_assert (cfun == fn);

  index = streamer_read_uhwi (ib);
  bb = BASIC_BLOCK_FOR_FUNCTION (fn, index);

  bb->count = (streamer_read_hwi (ib) * count_materialization_scale
	       + REG_BR_PROB_BASE / 2) / REG_BR_PROB_BASE;
  bb->frequency = streamer_read_hwi (ib);
  bb->flags = streamer_read_hwi (ib);

  /* LTO_bb1 has statements.  LTO_bb0 does not.  */
  if (tag == LTO_bb0)
    return;

  bsi = gsi_start_bb (bb);
  tag = streamer_read_record_start (ib);
  while (tag)
    {
      gimple stmt = input_gimple_stmt (ib, data_in, fn, tag);
      gsi_insert_after (&bsi, stmt, GSI_NEW_STMT);

      /* After the statement, expect a 0 delimiter or the EH region
	 that the previous statement belongs to.  */
      tag = streamer_read_record_start (ib);
      lto_tag_check_set (tag, 2, LTO_eh_region, LTO_null);

      if (tag == LTO_eh_region)
	{
	  HOST_WIDE_INT region = streamer_read_hwi (ib);
	  gcc_assert (region == (int) region);
	  add_stmt_to_eh_lp (stmt, region);
	}

      tag = streamer_read_record_start (ib);
    }

  tag = streamer_read_record_start (ib);
  while (tag)
    {
      input_phi (ib, bb, data_in, fn);
      tag = streamer_read_record_start (ib);
    }
}
