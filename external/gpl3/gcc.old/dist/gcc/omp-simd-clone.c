/* OMP constructs' SIMD clone supporting code.

Copyright (C) 2005-2017 Free Software Foundation, Inc.

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
#include "backend.h"
#include "target.h"
#include "tree.h"
#include "gimple.h"
#include "cfghooks.h"
#include "alloc-pool.h"
#include "tree-pass.h"
#include "ssa.h"
#include "cgraph.h"
#include "pretty-print.h"
#include "diagnostic-core.h"
#include "fold-const.h"
#include "stor-layout.h"
#include "cfganal.h"
#include "gimplify.h"
#include "gimple-iterator.h"
#include "gimplify-me.h"
#include "gimple-walk.h"
#include "langhooks.h"
#include "tree-cfg.h"
#include "tree-into-ssa.h"
#include "tree-dfa.h"
#include "cfgloop.h"
#include "symbol-summary.h"
#include "ipa-prop.h"
#include "tree-eh.h"
#include "varasm.h"


/* Allocate a fresh `simd_clone' and return it.  NARGS is the number
   of arguments to reserve space for.  */

static struct cgraph_simd_clone *
simd_clone_struct_alloc (int nargs)
{
  struct cgraph_simd_clone *clone_info;
  size_t len = (sizeof (struct cgraph_simd_clone)
		+ nargs * sizeof (struct cgraph_simd_clone_arg));
  clone_info = (struct cgraph_simd_clone *)
	       ggc_internal_cleared_alloc (len);
  return clone_info;
}

/* Make a copy of the `struct cgraph_simd_clone' in FROM to TO.  */

static inline void
simd_clone_struct_copy (struct cgraph_simd_clone *to,
			struct cgraph_simd_clone *from)
{
  memcpy (to, from, (sizeof (struct cgraph_simd_clone)
		     + ((from->nargs - from->inbranch)
			* sizeof (struct cgraph_simd_clone_arg))));
}

/* Return vector of parameter types of function FNDECL.  This uses
   TYPE_ARG_TYPES if available, otherwise falls back to types of
   DECL_ARGUMENTS types.  */

static vec<tree>
simd_clone_vector_of_formal_parm_types (tree fndecl)
{
  if (TYPE_ARG_TYPES (TREE_TYPE (fndecl)))
    return ipa_get_vector_of_formal_parm_types (TREE_TYPE (fndecl));
  vec<tree> args = ipa_get_vector_of_formal_parms (fndecl);
  unsigned int i;
  tree arg;
  FOR_EACH_VEC_ELT (args, i, arg)
    args[i] = TREE_TYPE (args[i]);
  return args;
}

/* Given a simd function in NODE, extract the simd specific
   information from the OMP clauses passed in CLAUSES, and return
   the struct cgraph_simd_clone * if it should be cloned.  *INBRANCH_SPECIFIED
   is set to TRUE if the `inbranch' or `notinbranch' clause specified,
   otherwise set to FALSE.  */

static struct cgraph_simd_clone *
simd_clone_clauses_extract (struct cgraph_node *node, tree clauses,
			    bool *inbranch_specified)
{
  vec<tree> args = simd_clone_vector_of_formal_parm_types (node->decl);
  tree t;
  int n;
  *inbranch_specified = false;

  n = args.length ();
  if (n > 0 && args.last () == void_type_node)
    n--;

  /* To distinguish from an OpenMP simd clone, Cilk Plus functions to
     be cloned have a distinctive artificial label in addition to "omp
     declare simd".  */
  bool cilk_clone
    = (flag_cilkplus
       && lookup_attribute ("cilk simd function",
			    DECL_ATTRIBUTES (node->decl)));

  /* Allocate one more than needed just in case this is an in-branch
     clone which will require a mask argument.  */
  struct cgraph_simd_clone *clone_info = simd_clone_struct_alloc (n + 1);
  clone_info->nargs = n;
  clone_info->cilk_elemental = cilk_clone;

  if (!clauses)
    goto out;

  clauses = TREE_VALUE (clauses);
  if (!clauses || TREE_CODE (clauses) != OMP_CLAUSE)
    goto out;

  for (t = clauses; t; t = OMP_CLAUSE_CHAIN (t))
    {
      switch (OMP_CLAUSE_CODE (t))
	{
	case OMP_CLAUSE_INBRANCH:
	  clone_info->inbranch = 1;
	  *inbranch_specified = true;
	  break;
	case OMP_CLAUSE_NOTINBRANCH:
	  clone_info->inbranch = 0;
	  *inbranch_specified = true;
	  break;
	case OMP_CLAUSE_SIMDLEN:
	  clone_info->simdlen
	    = TREE_INT_CST_LOW (OMP_CLAUSE_SIMDLEN_EXPR (t));
	  break;
	case OMP_CLAUSE_LINEAR:
	  {
	    tree decl = OMP_CLAUSE_DECL (t);
	    tree step = OMP_CLAUSE_LINEAR_STEP (t);
	    int argno = TREE_INT_CST_LOW (decl);
	    if (OMP_CLAUSE_LINEAR_VARIABLE_STRIDE (t))
	      {
		enum cgraph_simd_clone_arg_type arg_type;
		if (TREE_CODE (args[argno]) == REFERENCE_TYPE)
		  switch (OMP_CLAUSE_LINEAR_KIND (t))
		    {
		    case OMP_CLAUSE_LINEAR_REF:
		      arg_type
			= SIMD_CLONE_ARG_TYPE_LINEAR_REF_VARIABLE_STEP;
		      break;
		    case OMP_CLAUSE_LINEAR_UVAL:
		      arg_type
			= SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_VARIABLE_STEP;
		      break;
		    case OMP_CLAUSE_LINEAR_VAL:
		    case OMP_CLAUSE_LINEAR_DEFAULT:
		      arg_type
			= SIMD_CLONE_ARG_TYPE_LINEAR_VAL_VARIABLE_STEP;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		else
		  arg_type = SIMD_CLONE_ARG_TYPE_LINEAR_VARIABLE_STEP;
		clone_info->args[argno].arg_type = arg_type;
		clone_info->args[argno].linear_step = tree_to_shwi (step);
		gcc_assert (clone_info->args[argno].linear_step >= 0
			    && clone_info->args[argno].linear_step < n);
	      }
	    else
	      {
		if (POINTER_TYPE_P (args[argno]))
		  step = fold_convert (ssizetype, step);
		if (!tree_fits_shwi_p (step))
		  {
		    warning_at (OMP_CLAUSE_LOCATION (t), 0,
				"ignoring large linear step");
		    args.release ();
		    return NULL;
		  }
		else if (integer_zerop (step))
		  {
		    warning_at (OMP_CLAUSE_LOCATION (t), 0,
				"ignoring zero linear step");
		    args.release ();
		    return NULL;
		  }
		else
		  {
		    enum cgraph_simd_clone_arg_type arg_type;
		    if (TREE_CODE (args[argno]) == REFERENCE_TYPE)
		      switch (OMP_CLAUSE_LINEAR_KIND (t))
			{
			case OMP_CLAUSE_LINEAR_REF:
			  arg_type
			    = SIMD_CLONE_ARG_TYPE_LINEAR_REF_CONSTANT_STEP;
			  break;
			case OMP_CLAUSE_LINEAR_UVAL:
			  arg_type
			    = SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_CONSTANT_STEP;
			  break;
			case OMP_CLAUSE_LINEAR_VAL:
			case OMP_CLAUSE_LINEAR_DEFAULT:
			  arg_type
			    = SIMD_CLONE_ARG_TYPE_LINEAR_VAL_CONSTANT_STEP;
			  break;
			default:
			  gcc_unreachable ();
			}
		    else
		      arg_type = SIMD_CLONE_ARG_TYPE_LINEAR_CONSTANT_STEP;
		    clone_info->args[argno].arg_type = arg_type;
		    clone_info->args[argno].linear_step = tree_to_shwi (step);
		  }
	      }
	    break;
	  }
	case OMP_CLAUSE_UNIFORM:
	  {
	    tree decl = OMP_CLAUSE_DECL (t);
	    int argno = tree_to_uhwi (decl);
	    clone_info->args[argno].arg_type
	      = SIMD_CLONE_ARG_TYPE_UNIFORM;
	    break;
	  }
	case OMP_CLAUSE_ALIGNED:
	  {
	    tree decl = OMP_CLAUSE_DECL (t);
	    int argno = tree_to_uhwi (decl);
	    clone_info->args[argno].alignment
	      = TREE_INT_CST_LOW (OMP_CLAUSE_ALIGNED_ALIGNMENT (t));
	    break;
	  }
	default:
	  break;
	}
    }

 out:
  if (TYPE_ATOMIC (TREE_TYPE (TREE_TYPE (node->decl))))
    {
      warning_at (DECL_SOURCE_LOCATION (node->decl), 0,
		  "ignoring %<#pragma omp declare simd%> on function "
		  "with %<_Atomic%> qualified return type");
      args.release ();
      return NULL;
    }

  for (unsigned int argno = 0; argno < clone_info->nargs; argno++)
    if (TYPE_ATOMIC (args[argno])
	&& clone_info->args[argno].arg_type != SIMD_CLONE_ARG_TYPE_UNIFORM)
      {
	warning_at (DECL_SOURCE_LOCATION (node->decl), 0,
		    "ignoring %<#pragma omp declare simd%> on function "
		    "with %<_Atomic%> qualified non-%<uniform%> argument");
	args.release ();
	return NULL;
      }

  args.release ();
  return clone_info;
}

/* Given a SIMD clone in NODE, calculate the characteristic data
   type and return the coresponding type.  The characteristic data
   type is computed as described in the Intel Vector ABI.  */

static tree
simd_clone_compute_base_data_type (struct cgraph_node *node,
				   struct cgraph_simd_clone *clone_info)
{
  tree type = integer_type_node;
  tree fndecl = node->decl;

  /* a) For non-void function, the characteristic data type is the
        return type.  */
  if (TREE_CODE (TREE_TYPE (TREE_TYPE (fndecl))) != VOID_TYPE)
    type = TREE_TYPE (TREE_TYPE (fndecl));

  /* b) If the function has any non-uniform, non-linear parameters,
        then the characteristic data type is the type of the first
        such parameter.  */
  else
    {
      vec<tree> map = simd_clone_vector_of_formal_parm_types (fndecl);
      for (unsigned int i = 0; i < clone_info->nargs; ++i)
	if (clone_info->args[i].arg_type == SIMD_CLONE_ARG_TYPE_VECTOR)
	  {
	    type = map[i];
	    break;
	  }
      map.release ();
    }

  /* c) If the characteristic data type determined by a) or b) above
        is struct, union, or class type which is pass-by-value (except
        for the type that maps to the built-in complex data type), the
        characteristic data type is int.  */
  if (RECORD_OR_UNION_TYPE_P (type)
      && !aggregate_value_p (type, NULL)
      && TREE_CODE (type) != COMPLEX_TYPE)
    return integer_type_node;

  /* d) If none of the above three classes is applicable, the
        characteristic data type is int.  */

  return type;

  /* e) For Intel Xeon Phi native and offload compilation, if the
        resulting characteristic data type is 8-bit or 16-bit integer
        data type, the characteristic data type is int.  */
  /* Well, we don't handle Xeon Phi yet.  */
}

static tree
simd_clone_mangle (struct cgraph_node *node,
		   struct cgraph_simd_clone *clone_info)
{
  char vecsize_mangle = clone_info->vecsize_mangle;
  char mask = clone_info->inbranch ? 'M' : 'N';
  unsigned int simdlen = clone_info->simdlen;
  unsigned int n;
  pretty_printer pp;

  gcc_assert (vecsize_mangle && simdlen);

  pp_string (&pp, "_ZGV");
  pp_character (&pp, vecsize_mangle);
  pp_character (&pp, mask);
  pp_decimal_int (&pp, simdlen);

  for (n = 0; n < clone_info->nargs; ++n)
    {
      struct cgraph_simd_clone_arg arg = clone_info->args[n];

      switch (arg.arg_type)
	{
	case SIMD_CLONE_ARG_TYPE_UNIFORM:
	  pp_character (&pp, 'u');
	  break;
	case SIMD_CLONE_ARG_TYPE_LINEAR_CONSTANT_STEP:
	  pp_character (&pp, 'l');
	  goto mangle_linear;
	case SIMD_CLONE_ARG_TYPE_LINEAR_REF_CONSTANT_STEP:
	  pp_character (&pp, 'R');
	  goto mangle_linear;
	case SIMD_CLONE_ARG_TYPE_LINEAR_VAL_CONSTANT_STEP:
	  pp_character (&pp, 'L');
	  goto mangle_linear;
	case SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_CONSTANT_STEP:
	  pp_character (&pp, 'U');
	  goto mangle_linear;
	mangle_linear:
	  gcc_assert (arg.linear_step != 0);
	  if (arg.linear_step > 1)
	    pp_unsigned_wide_integer (&pp, arg.linear_step);
	  else if (arg.linear_step < 0)
	    {
	      pp_character (&pp, 'n');
	      pp_unsigned_wide_integer (&pp, (-(unsigned HOST_WIDE_INT)
					      arg.linear_step));
	    }
	  break;
	case SIMD_CLONE_ARG_TYPE_LINEAR_VARIABLE_STEP:
	  pp_string (&pp, "ls");
	  pp_unsigned_wide_integer (&pp, arg.linear_step);
	  break;
	case SIMD_CLONE_ARG_TYPE_LINEAR_REF_VARIABLE_STEP:
	  pp_string (&pp, "Rs");
	  pp_unsigned_wide_integer (&pp, arg.linear_step);
	  break;
	case SIMD_CLONE_ARG_TYPE_LINEAR_VAL_VARIABLE_STEP:
	  pp_string (&pp, "Ls");
	  pp_unsigned_wide_integer (&pp, arg.linear_step);
	  break;
	case SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_VARIABLE_STEP:
	  pp_string (&pp, "Us");
	  pp_unsigned_wide_integer (&pp, arg.linear_step);
	  break;
	default:
	  pp_character (&pp, 'v');
	}
      if (arg.alignment)
	{
	  pp_character (&pp, 'a');
	  pp_decimal_int (&pp, arg.alignment);
	}
    }

  pp_underscore (&pp);
  const char *str = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (node->decl));
  if (*str == '*')
    ++str;
  pp_string (&pp, str);
  str = pp_formatted_text (&pp);

  /* If there already is a SIMD clone with the same mangled name, don't
     add another one.  This can happen e.g. for
     #pragma omp declare simd
     #pragma omp declare simd simdlen(8)
     int foo (int, int);
     if the simdlen is assumed to be 8 for the first one, etc.  */
  for (struct cgraph_node *clone = node->simd_clones; clone;
       clone = clone->simdclone->next_clone)
    if (strcmp (IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (clone->decl)),
		str) == 0)
      return NULL_TREE;

  return get_identifier (str);
}

/* Create a simd clone of OLD_NODE and return it.  */

static struct cgraph_node *
simd_clone_create (struct cgraph_node *old_node)
{
  struct cgraph_node *new_node;
  if (old_node->definition)
    {
      if (!old_node->has_gimple_body_p ())
	return NULL;
      old_node->get_body ();
      new_node = old_node->create_version_clone_with_body (vNULL, NULL, NULL,
							   false, NULL, NULL,
							   "simdclone");
    }
  else
    {
      tree old_decl = old_node->decl;
      tree new_decl = copy_node (old_node->decl);
      DECL_NAME (new_decl) = clone_function_name (old_decl, "simdclone");
      SET_DECL_ASSEMBLER_NAME (new_decl, DECL_NAME (new_decl));
      SET_DECL_RTL (new_decl, NULL);
      DECL_STATIC_CONSTRUCTOR (new_decl) = 0;
      DECL_STATIC_DESTRUCTOR (new_decl) = 0;
      new_node = old_node->create_version_clone (new_decl, vNULL, NULL);
      if (old_node->in_other_partition)
	new_node->in_other_partition = 1;
    }
  if (new_node == NULL)
    return new_node;

  TREE_PUBLIC (new_node->decl) = TREE_PUBLIC (old_node->decl);
  DECL_COMDAT (new_node->decl) = DECL_COMDAT (old_node->decl);
  DECL_WEAK (new_node->decl) = DECL_WEAK (old_node->decl);
  DECL_EXTERNAL (new_node->decl) = DECL_EXTERNAL (old_node->decl);
  DECL_VISIBILITY_SPECIFIED (new_node->decl)
    = DECL_VISIBILITY_SPECIFIED (old_node->decl);
  DECL_VISIBILITY (new_node->decl) = DECL_VISIBILITY (old_node->decl);
  DECL_DLLIMPORT_P (new_node->decl) = DECL_DLLIMPORT_P (old_node->decl);
  if (DECL_ONE_ONLY (old_node->decl))
    make_decl_one_only (new_node->decl, DECL_ASSEMBLER_NAME (new_node->decl));

  /* The method cgraph_version_clone_with_body () will force the new
     symbol local.  Undo this, and inherit external visibility from
     the old node.  */
  new_node->local.local = old_node->local.local;
  new_node->externally_visible = old_node->externally_visible;

  return new_node;
}

/* Adjust the return type of the given function to its appropriate
   vector counterpart.  Returns a simd array to be used throughout the
   function as a return value.  */

static tree
simd_clone_adjust_return_type (struct cgraph_node *node)
{
  tree fndecl = node->decl;
  tree orig_rettype = TREE_TYPE (TREE_TYPE (fndecl));
  unsigned int veclen;
  tree t;

  /* Adjust the function return type.  */
  if (orig_rettype == void_type_node)
    return NULL_TREE;
  TREE_TYPE (fndecl) = build_distinct_type_copy (TREE_TYPE (fndecl));
  t = TREE_TYPE (TREE_TYPE (fndecl));
  if (INTEGRAL_TYPE_P (t) || POINTER_TYPE_P (t))
    veclen = node->simdclone->vecsize_int;
  else
    veclen = node->simdclone->vecsize_float;
  veclen /= GET_MODE_BITSIZE (TYPE_MODE (t));
  if (veclen > node->simdclone->simdlen)
    veclen = node->simdclone->simdlen;
  if (POINTER_TYPE_P (t))
    t = pointer_sized_int_node;
  if (veclen == node->simdclone->simdlen)
    t = build_vector_type (t, node->simdclone->simdlen);
  else
    {
      t = build_vector_type (t, veclen);
      t = build_array_type_nelts (t, node->simdclone->simdlen / veclen);
    }
  TREE_TYPE (TREE_TYPE (fndecl)) = t;
  if (!node->definition)
    return NULL_TREE;

  t = DECL_RESULT (fndecl);
  /* Adjust the DECL_RESULT.  */
  gcc_assert (TREE_TYPE (t) != void_type_node);
  TREE_TYPE (t) = TREE_TYPE (TREE_TYPE (fndecl));
  relayout_decl (t);

  tree atype = build_array_type_nelts (orig_rettype,
				       node->simdclone->simdlen);
  if (veclen != node->simdclone->simdlen)
    return build1 (VIEW_CONVERT_EXPR, atype, t);

  /* Set up a SIMD array to use as the return value.  */
  tree retval = create_tmp_var_raw (atype, "retval");
  gimple_add_tmp_var (retval);
  return retval;
}

/* Each vector argument has a corresponding array to be used locally
   as part of the eventual loop.  Create such temporary array and
   return it.

   PREFIX is the prefix to be used for the temporary.

   TYPE is the inner element type.

   SIMDLEN is the number of elements.  */

static tree
create_tmp_simd_array (const char *prefix, tree type, int simdlen)
{
  tree atype = build_array_type_nelts (type, simdlen);
  tree avar = create_tmp_var_raw (atype, prefix);
  gimple_add_tmp_var (avar);
  return avar;
}

/* Modify the function argument types to their corresponding vector
   counterparts if appropriate.  Also, create one array for each simd
   argument to be used locally when using the function arguments as
   part of the loop.

   NODE is the function whose arguments are to be adjusted.

   Returns an adjustment vector that will be filled describing how the
   argument types will be adjusted.  */

static ipa_parm_adjustment_vec
simd_clone_adjust_argument_types (struct cgraph_node *node)
{
  vec<tree> args;
  ipa_parm_adjustment_vec adjustments;

  if (node->definition)
    args = ipa_get_vector_of_formal_parms (node->decl);
  else
    args = simd_clone_vector_of_formal_parm_types (node->decl);
  adjustments.create (args.length ());
  unsigned i, j, veclen;
  struct ipa_parm_adjustment adj;
  struct cgraph_simd_clone *sc = node->simdclone;

  for (i = 0; i < sc->nargs; ++i)
    {
      memset (&adj, 0, sizeof (adj));
      tree parm = args[i];
      tree parm_type = node->definition ? TREE_TYPE (parm) : parm;
      adj.base_index = i;
      adj.base = parm;

      sc->args[i].orig_arg = node->definition ? parm : NULL_TREE;
      sc->args[i].orig_type = parm_type;

      switch (sc->args[i].arg_type)
	{
	default:
	  /* No adjustment necessary for scalar arguments.  */
	  adj.op = IPA_PARM_OP_COPY;
	  break;
	case SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_CONSTANT_STEP:
	case SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_VARIABLE_STEP:
	  if (node->definition)
	    sc->args[i].simd_array
	      = create_tmp_simd_array (IDENTIFIER_POINTER (DECL_NAME (parm)),
				       TREE_TYPE (parm_type),
				       sc->simdlen);
	  adj.op = IPA_PARM_OP_COPY;
	  break;
	case SIMD_CLONE_ARG_TYPE_LINEAR_VAL_CONSTANT_STEP:
	case SIMD_CLONE_ARG_TYPE_LINEAR_VAL_VARIABLE_STEP:
	case SIMD_CLONE_ARG_TYPE_VECTOR:
	  if (INTEGRAL_TYPE_P (parm_type) || POINTER_TYPE_P (parm_type))
	    veclen = sc->vecsize_int;
	  else
	    veclen = sc->vecsize_float;
	  veclen /= GET_MODE_BITSIZE (TYPE_MODE (parm_type));
	  if (veclen > sc->simdlen)
	    veclen = sc->simdlen;
	  adj.arg_prefix = "simd";
	  if (POINTER_TYPE_P (parm_type))
	    adj.type = build_vector_type (pointer_sized_int_node, veclen);
	  else
	    adj.type = build_vector_type (parm_type, veclen);
	  sc->args[i].vector_type = adj.type;
	  for (j = veclen; j < sc->simdlen; j += veclen)
	    {
	      adjustments.safe_push (adj);
	      if (j == veclen)
		{
		  memset (&adj, 0, sizeof (adj));
		  adj.op = IPA_PARM_OP_NEW;
		  adj.arg_prefix = "simd";
		  adj.base_index = i;
		  adj.type = sc->args[i].vector_type;
		}
	    }

	  if (node->definition)
	    sc->args[i].simd_array
	      = create_tmp_simd_array (DECL_NAME (parm)
				       ? IDENTIFIER_POINTER (DECL_NAME (parm))
				       : NULL, parm_type, sc->simdlen);
	}
      adjustments.safe_push (adj);
    }

  if (sc->inbranch)
    {
      tree base_type = simd_clone_compute_base_data_type (sc->origin, sc);

      memset (&adj, 0, sizeof (adj));
      adj.op = IPA_PARM_OP_NEW;
      adj.arg_prefix = "mask";

      adj.base_index = i;
      if (INTEGRAL_TYPE_P (base_type) || POINTER_TYPE_P (base_type))
	veclen = sc->vecsize_int;
      else
	veclen = sc->vecsize_float;
      veclen /= GET_MODE_BITSIZE (TYPE_MODE (base_type));
      if (veclen > sc->simdlen)
	veclen = sc->simdlen;
      if (sc->mask_mode != VOIDmode)
	adj.type
	  = lang_hooks.types.type_for_mode (sc->mask_mode, 1);
      else if (POINTER_TYPE_P (base_type))
	adj.type = build_vector_type (pointer_sized_int_node, veclen);
      else
	adj.type = build_vector_type (base_type, veclen);
      adjustments.safe_push (adj);

      for (j = veclen; j < sc->simdlen; j += veclen)
	adjustments.safe_push (adj);

      /* We have previously allocated one extra entry for the mask.  Use
	 it and fill it.  */
      sc->nargs++;
      if (sc->mask_mode != VOIDmode)
	base_type = boolean_type_node;
      if (node->definition)
	{
	  sc->args[i].orig_arg
	    = build_decl (UNKNOWN_LOCATION, PARM_DECL, NULL, base_type);
	  if (sc->mask_mode == VOIDmode)
	    sc->args[i].simd_array
	      = create_tmp_simd_array ("mask", base_type, sc->simdlen);
	  else if (veclen < sc->simdlen)
	    sc->args[i].simd_array
	      = create_tmp_simd_array ("mask", adj.type, sc->simdlen / veclen);
	  else
	    sc->args[i].simd_array = NULL_TREE;
	}
      sc->args[i].orig_type = base_type;
      sc->args[i].arg_type = SIMD_CLONE_ARG_TYPE_MASK;
    }

  if (node->definition)
    ipa_modify_formal_parameters (node->decl, adjustments);
  else
    {
      tree new_arg_types = NULL_TREE, new_reversed;
      bool last_parm_void = false;
      if (args.length () > 0 && args.last () == void_type_node)
	last_parm_void = true;

      gcc_assert (TYPE_ARG_TYPES (TREE_TYPE (node->decl)));
      j = adjustments.length ();
      for (i = 0; i < j; i++)
	{
	  struct ipa_parm_adjustment *adj = &adjustments[i];
	  tree ptype;
	  if (adj->op == IPA_PARM_OP_COPY)
	    ptype = args[adj->base_index];
	  else
	    ptype = adj->type;
	  new_arg_types = tree_cons (NULL_TREE, ptype, new_arg_types);
	}
      new_reversed = nreverse (new_arg_types);
      if (last_parm_void)
	{
	  if (new_reversed)
	    TREE_CHAIN (new_arg_types) = void_list_node;
	  else
	    new_reversed = void_list_node;
	}

      tree new_type = build_distinct_type_copy (TREE_TYPE (node->decl));
      TYPE_ARG_TYPES (new_type) = new_reversed;
      TREE_TYPE (node->decl) = new_type;

      adjustments.release ();
    }
  args.release ();
  return adjustments;
}

/* Initialize and copy the function arguments in NODE to their
   corresponding local simd arrays.  Returns a fresh gimple_seq with
   the instruction sequence generated.  */

static gimple_seq
simd_clone_init_simd_arrays (struct cgraph_node *node,
			     ipa_parm_adjustment_vec adjustments)
{
  gimple_seq seq = NULL;
  unsigned i = 0, j = 0, k;

  for (tree arg = DECL_ARGUMENTS (node->decl);
       arg;
       arg = DECL_CHAIN (arg), i++, j++)
    {
      if (adjustments[j].op == IPA_PARM_OP_COPY
	  || POINTER_TYPE_P (TREE_TYPE (arg)))
	continue;

      node->simdclone->args[i].vector_arg = arg;

      tree array = node->simdclone->args[i].simd_array;
      if (node->simdclone->mask_mode != VOIDmode
	  && node->simdclone->args[i].arg_type == SIMD_CLONE_ARG_TYPE_MASK)
	{
	  if (array == NULL_TREE)
	    continue;
	  unsigned int l
	    = tree_to_uhwi (TYPE_MAX_VALUE (TYPE_DOMAIN (TREE_TYPE (array))));
	  for (k = 0; k <= l; k++)
	    {
	      if (k)
		{
		  arg = DECL_CHAIN (arg);
		  j++;
		}
	      tree t = build4 (ARRAY_REF, TREE_TYPE (TREE_TYPE (array)),
			       array, size_int (k), NULL, NULL);
	      t = build2 (MODIFY_EXPR, TREE_TYPE (t), t, arg);
	      gimplify_and_add (t, &seq);
	    }
	  continue;
	}
      if (TYPE_VECTOR_SUBPARTS (TREE_TYPE (arg)) == node->simdclone->simdlen)
	{
	  tree ptype = build_pointer_type (TREE_TYPE (TREE_TYPE (array)));
	  tree ptr = build_fold_addr_expr (array);
	  tree t = build2 (MEM_REF, TREE_TYPE (arg), ptr,
			   build_int_cst (ptype, 0));
	  t = build2 (MODIFY_EXPR, TREE_TYPE (t), t, arg);
	  gimplify_and_add (t, &seq);
	}
      else
	{
	  unsigned int simdlen = TYPE_VECTOR_SUBPARTS (TREE_TYPE (arg));
	  tree ptype = build_pointer_type (TREE_TYPE (TREE_TYPE (array)));
	  for (k = 0; k < node->simdclone->simdlen; k += simdlen)
	    {
	      tree ptr = build_fold_addr_expr (array);
	      int elemsize;
	      if (k)
		{
		  arg = DECL_CHAIN (arg);
		  j++;
		}
	      elemsize
		= GET_MODE_SIZE (TYPE_MODE (TREE_TYPE (TREE_TYPE (arg))));
	      tree t = build2 (MEM_REF, TREE_TYPE (arg), ptr,
			       build_int_cst (ptype, k * elemsize));
	      t = build2 (MODIFY_EXPR, TREE_TYPE (t), t, arg);
	      gimplify_and_add (t, &seq);
	    }
	}
    }
  return seq;
}

/* Callback info for ipa_simd_modify_stmt_ops below.  */

struct modify_stmt_info {
  ipa_parm_adjustment_vec adjustments;
  gimple *stmt;
  /* True if the parent statement was modified by
     ipa_simd_modify_stmt_ops.  */
  bool modified;
};

/* Callback for walk_gimple_op.

   Adjust operands from a given statement as specified in the
   adjustments vector in the callback data.  */

static tree
ipa_simd_modify_stmt_ops (tree *tp, int *walk_subtrees, void *data)
{
  struct walk_stmt_info *wi = (struct walk_stmt_info *) data;
  struct modify_stmt_info *info = (struct modify_stmt_info *) wi->info;
  tree *orig_tp = tp;
  if (TREE_CODE (*tp) == ADDR_EXPR)
    tp = &TREE_OPERAND (*tp, 0);
  struct ipa_parm_adjustment *cand = NULL;
  if (TREE_CODE (*tp) == PARM_DECL)
    cand = ipa_get_adjustment_candidate (&tp, NULL, info->adjustments, true);
  else
    {
      if (TYPE_P (*tp))
	*walk_subtrees = 0;
    }

  tree repl = NULL_TREE;
  if (cand)
    repl = unshare_expr (cand->new_decl);
  else
    {
      if (tp != orig_tp)
	{
	  *walk_subtrees = 0;
	  bool modified = info->modified;
	  info->modified = false;
	  walk_tree (tp, ipa_simd_modify_stmt_ops, wi, wi->pset);
	  if (!info->modified)
	    {
	      info->modified = modified;
	      return NULL_TREE;
	    }
	  info->modified = modified;
	  repl = *tp;
	}
      else
	return NULL_TREE;
    }

  if (tp != orig_tp)
    {
      repl = build_fold_addr_expr (repl);
      gimple *stmt;
      if (is_gimple_debug (info->stmt))
	{
	  tree vexpr = make_node (DEBUG_EXPR_DECL);
	  stmt = gimple_build_debug_source_bind (vexpr, repl, NULL);
	  DECL_ARTIFICIAL (vexpr) = 1;
	  TREE_TYPE (vexpr) = TREE_TYPE (repl);
	  SET_DECL_MODE (vexpr, TYPE_MODE (TREE_TYPE (repl)));
	  repl = vexpr;
	}
      else
	{
	  stmt = gimple_build_assign (make_ssa_name (TREE_TYPE (repl)), repl);
	  repl = gimple_assign_lhs (stmt);
	}
      gimple_stmt_iterator gsi = gsi_for_stmt (info->stmt);
      gsi_insert_before (&gsi, stmt, GSI_SAME_STMT);
      *orig_tp = repl;
    }
  else if (!useless_type_conversion_p (TREE_TYPE (*tp), TREE_TYPE (repl)))
    {
      tree vce = build1 (VIEW_CONVERT_EXPR, TREE_TYPE (*tp), repl);
      *tp = vce;
    }
  else
    *tp = repl;

  info->modified = true;
  return NULL_TREE;
}

/* Traverse the function body and perform all modifications as
   described in ADJUSTMENTS.  At function return, ADJUSTMENTS will be
   modified such that the replacement/reduction value will now be an
   offset into the corresponding simd_array.

   This function will replace all function argument uses with their
   corresponding simd array elements, and ajust the return values
   accordingly.  */

static void
ipa_simd_modify_function_body (struct cgraph_node *node,
			       ipa_parm_adjustment_vec adjustments,
			       tree retval_array, tree iter)
{
  basic_block bb;
  unsigned int i, j, l;

  /* Re-use the adjustments array, but this time use it to replace
     every function argument use to an offset into the corresponding
     simd_array.  */
  for (i = 0, j = 0; i < node->simdclone->nargs; ++i, ++j)
    {
      if (!node->simdclone->args[i].vector_arg)
	continue;

      tree basetype = TREE_TYPE (node->simdclone->args[i].orig_arg);
      tree vectype = TREE_TYPE (node->simdclone->args[i].vector_arg);
      adjustments[j].new_decl
	= build4 (ARRAY_REF,
		  basetype,
		  node->simdclone->args[i].simd_array,
		  iter,
		  NULL_TREE, NULL_TREE);
      if (adjustments[j].op == IPA_PARM_OP_NONE
	  && TYPE_VECTOR_SUBPARTS (vectype) < node->simdclone->simdlen)
	j += node->simdclone->simdlen / TYPE_VECTOR_SUBPARTS (vectype) - 1;
    }

  l = adjustments.length ();
  tree name;

  FOR_EACH_SSA_NAME (i, name, cfun)
    {
      if (SSA_NAME_VAR (name)
	  && TREE_CODE (SSA_NAME_VAR (name)) == PARM_DECL)
	{
	  for (j = 0; j < l; j++)
	    if (SSA_NAME_VAR (name) == adjustments[j].base
		&& adjustments[j].new_decl)
	      {
		tree base_var;
		if (adjustments[j].new_ssa_base == NULL_TREE)
		  {
		    base_var
		      = copy_var_decl (adjustments[j].base,
				       DECL_NAME (adjustments[j].base),
				       TREE_TYPE (adjustments[j].base));
		    adjustments[j].new_ssa_base = base_var;
		  }
		else
		  base_var = adjustments[j].new_ssa_base;
		if (SSA_NAME_IS_DEFAULT_DEF (name))
		  {
		    bb = single_succ (ENTRY_BLOCK_PTR_FOR_FN (cfun));
		    gimple_stmt_iterator gsi = gsi_after_labels (bb);
		    tree new_decl = unshare_expr (adjustments[j].new_decl);
		    set_ssa_default_def (cfun, adjustments[j].base, NULL_TREE);
		    SET_SSA_NAME_VAR_OR_IDENTIFIER (name, base_var);
		    SSA_NAME_IS_DEFAULT_DEF (name) = 0;
		    gimple *stmt = gimple_build_assign (name, new_decl);
		    gsi_insert_before (&gsi, stmt, GSI_SAME_STMT);
		  }
		else
		  SET_SSA_NAME_VAR_OR_IDENTIFIER (name, base_var);
	      }
	}
    }

  struct modify_stmt_info info;
  info.adjustments = adjustments;

  FOR_EACH_BB_FN (bb, DECL_STRUCT_FUNCTION (node->decl))
    {
      gimple_stmt_iterator gsi;

      gsi = gsi_start_bb (bb);
      while (!gsi_end_p (gsi))
	{
	  gimple *stmt = gsi_stmt (gsi);
	  info.stmt = stmt;
	  struct walk_stmt_info wi;

	  memset (&wi, 0, sizeof (wi));
	  info.modified = false;
	  wi.info = &info;
	  walk_gimple_op (stmt, ipa_simd_modify_stmt_ops, &wi);

	  if (greturn *return_stmt = dyn_cast <greturn *> (stmt))
	    {
	      tree retval = gimple_return_retval (return_stmt);
	      if (!retval)
		{
		  gsi_remove (&gsi, true);
		  continue;
		}

	      /* Replace `return foo' with `retval_array[iter] = foo'.  */
	      tree ref = build4 (ARRAY_REF, TREE_TYPE (retval),
				 retval_array, iter, NULL, NULL);
	      stmt = gimple_build_assign (ref, retval);
	      gsi_replace (&gsi, stmt, true);
	      info.modified = true;
	    }

	  if (info.modified)
	    {
	      update_stmt (stmt);
	      if (maybe_clean_eh_stmt (stmt))
		gimple_purge_dead_eh_edges (gimple_bb (stmt));
	    }
	  gsi_next (&gsi);
	}
    }
}

/* Helper function of simd_clone_adjust, return linear step addend
   of Ith argument.  */

static tree
simd_clone_linear_addend (struct cgraph_node *node, unsigned int i,
			  tree addtype, basic_block entry_bb)
{
  tree ptype = NULL_TREE;
  switch (node->simdclone->args[i].arg_type)
    {
    case SIMD_CLONE_ARG_TYPE_LINEAR_CONSTANT_STEP:
    case SIMD_CLONE_ARG_TYPE_LINEAR_REF_CONSTANT_STEP:
    case SIMD_CLONE_ARG_TYPE_LINEAR_VAL_CONSTANT_STEP:
    case SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_CONSTANT_STEP:
      return build_int_cst (addtype, node->simdclone->args[i].linear_step);
    case SIMD_CLONE_ARG_TYPE_LINEAR_VARIABLE_STEP:
    case SIMD_CLONE_ARG_TYPE_LINEAR_REF_VARIABLE_STEP:
      ptype = TREE_TYPE (node->simdclone->args[i].orig_arg);
      break;
    case SIMD_CLONE_ARG_TYPE_LINEAR_VAL_VARIABLE_STEP:
    case SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_VARIABLE_STEP:
      ptype = TREE_TYPE (TREE_TYPE (node->simdclone->args[i].orig_arg));
      break;
    default:
      gcc_unreachable ();
    }

  unsigned int idx = node->simdclone->args[i].linear_step;
  tree arg = node->simdclone->args[idx].orig_arg;
  gcc_assert (is_gimple_reg_type (TREE_TYPE (arg)));
  gimple_stmt_iterator gsi = gsi_after_labels (entry_bb);
  gimple *g;
  tree ret;
  if (is_gimple_reg (arg))
    ret = get_or_create_ssa_default_def (cfun, arg);
  else
    {
      g = gimple_build_assign (make_ssa_name (TREE_TYPE (arg)), arg);
      gsi_insert_before (&gsi, g, GSI_SAME_STMT);
      ret = gimple_assign_lhs (g);
    }
  if (TREE_CODE (TREE_TYPE (arg)) == REFERENCE_TYPE)
    {
      g = gimple_build_assign (make_ssa_name (TREE_TYPE (TREE_TYPE (arg))),
			       build_simple_mem_ref (ret));
      gsi_insert_before (&gsi, g, GSI_SAME_STMT);
      ret = gimple_assign_lhs (g);
    }
  if (!useless_type_conversion_p (addtype, TREE_TYPE (ret)))
    {
      g = gimple_build_assign (make_ssa_name (addtype), NOP_EXPR, ret);
      gsi_insert_before (&gsi, g, GSI_SAME_STMT);
      ret = gimple_assign_lhs (g);
    }
  if (POINTER_TYPE_P (ptype))
    {
      tree size = TYPE_SIZE_UNIT (TREE_TYPE (ptype));
      if (size && TREE_CODE (size) == INTEGER_CST)
	{
	  g = gimple_build_assign (make_ssa_name (addtype), MULT_EXPR,
				   ret, fold_convert (addtype, size));
	  gsi_insert_before (&gsi, g, GSI_SAME_STMT);
	  ret = gimple_assign_lhs (g);
	}
    }
  return ret;
}

/* Adjust the argument types in NODE to their appropriate vector
   counterparts.  */

static void
simd_clone_adjust (struct cgraph_node *node)
{
  push_cfun (DECL_STRUCT_FUNCTION (node->decl));

  targetm.simd_clone.adjust (node);

  tree retval = simd_clone_adjust_return_type (node);
  ipa_parm_adjustment_vec adjustments
    = simd_clone_adjust_argument_types (node);

  push_gimplify_context ();

  gimple_seq seq = simd_clone_init_simd_arrays (node, adjustments);

  /* Adjust all uses of vector arguments accordingly.  Adjust all
     return values accordingly.  */
  tree iter = create_tmp_var (unsigned_type_node, "iter");
  tree iter1 = make_ssa_name (iter);
  tree iter2 = NULL_TREE;
  ipa_simd_modify_function_body (node, adjustments, retval, iter1);
  adjustments.release ();

  /* Initialize the iteration variable.  */
  basic_block entry_bb = single_succ (ENTRY_BLOCK_PTR_FOR_FN (cfun));
  basic_block body_bb = split_block_after_labels (entry_bb)->dest;
  gimple_stmt_iterator gsi = gsi_after_labels (entry_bb);
  /* Insert the SIMD array and iv initialization at function
     entry.  */
  gsi_insert_seq_before (&gsi, seq, GSI_NEW_STMT);

  pop_gimplify_context (NULL);

  gimple *g;
  basic_block incr_bb = NULL;
  struct loop *loop = NULL;

  /* Create a new BB right before the original exit BB, to hold the
     iteration increment and the condition/branch.  */
  if (EDGE_COUNT (EXIT_BLOCK_PTR_FOR_FN (cfun)->preds))
    {
      basic_block orig_exit = EDGE_PRED (EXIT_BLOCK_PTR_FOR_FN (cfun), 0)->src;
      incr_bb = create_empty_bb (orig_exit);
      add_bb_to_loop (incr_bb, body_bb->loop_father);
      /* The succ of orig_exit was EXIT_BLOCK_PTR_FOR_FN (cfun), with an empty
	 flag.  Set it now to be a FALLTHRU_EDGE.  */
      gcc_assert (EDGE_COUNT (orig_exit->succs) == 1);
      EDGE_SUCC (orig_exit, 0)->flags |= EDGE_FALLTHRU;
      for (unsigned i = 0;
	   i < EDGE_COUNT (EXIT_BLOCK_PTR_FOR_FN (cfun)->preds); ++i)
	{
	  edge e = EDGE_PRED (EXIT_BLOCK_PTR_FOR_FN (cfun), i);
	  redirect_edge_succ (e, incr_bb);
	}
    }
  else if (node->simdclone->inbranch)
    {
      incr_bb = create_empty_bb (entry_bb);
      add_bb_to_loop (incr_bb, body_bb->loop_father);
    }

  if (incr_bb)
    {
      edge e = make_edge (incr_bb, EXIT_BLOCK_PTR_FOR_FN (cfun), 0);
      e->probability = REG_BR_PROB_BASE;
      gsi = gsi_last_bb (incr_bb);
      iter2 = make_ssa_name (iter);
      g = gimple_build_assign (iter2, PLUS_EXPR, iter1,
			       build_int_cst (unsigned_type_node, 1));
      gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);

      /* Mostly annotate the loop for the vectorizer (the rest is done
	 below).  */
      loop = alloc_loop ();
      cfun->has_force_vectorize_loops = true;
      loop->safelen = node->simdclone->simdlen;
      loop->force_vectorize = true;
      loop->header = body_bb;
    }

  /* Branch around the body if the mask applies.  */
  if (node->simdclone->inbranch)
    {
      gsi = gsi_last_bb (loop->header);
      tree mask_array
	= node->simdclone->args[node->simdclone->nargs - 1].simd_array;
      tree mask;
      if (node->simdclone->mask_mode != VOIDmode)
	{
	  tree shift_cnt;
	  if (mask_array == NULL_TREE)
	    {
	      tree arg = node->simdclone->args[node->simdclone->nargs
					       - 1].vector_arg;
	      mask = get_or_create_ssa_default_def (cfun, arg);
	      shift_cnt = iter1;
	    }
	  else
	    {
	      tree maskt = TREE_TYPE (mask_array);
	      int c = tree_to_uhwi (TYPE_MAX_VALUE (TYPE_DOMAIN (maskt)));
	      c = node->simdclone->simdlen / (c + 1);
	      int s = exact_log2 (c);
	      gcc_assert (s > 0);
	      c--;
	      tree idx = make_ssa_name (TREE_TYPE (iter1));
	      g = gimple_build_assign (idx, RSHIFT_EXPR, iter1,
				       build_int_cst (NULL_TREE, s));
	      gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
	      mask = make_ssa_name (TREE_TYPE (TREE_TYPE (mask_array)));
	      tree aref = build4 (ARRAY_REF,
				  TREE_TYPE (TREE_TYPE (mask_array)),
				  mask_array, idx, NULL, NULL);
	      g = gimple_build_assign (mask, aref);
	      gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
	      shift_cnt = make_ssa_name (TREE_TYPE (iter1));
	      g = gimple_build_assign (shift_cnt, BIT_AND_EXPR, iter1,
				       build_int_cst (TREE_TYPE (iter1), c));
	      gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
	    }
	  g = gimple_build_assign (make_ssa_name (TREE_TYPE (mask)),
				   RSHIFT_EXPR, mask, shift_cnt);
	  gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
	  mask = gimple_assign_lhs (g);
	  g = gimple_build_assign (make_ssa_name (TREE_TYPE (mask)),
				   BIT_AND_EXPR, mask,
				   build_int_cst (TREE_TYPE (mask), 1));
	  gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
	  mask = gimple_assign_lhs (g);
	}
      else
	{
	  mask = make_ssa_name (TREE_TYPE (TREE_TYPE (mask_array)));
	  tree aref = build4 (ARRAY_REF,
			      TREE_TYPE (TREE_TYPE (mask_array)),
			      mask_array, iter1, NULL, NULL);
	  g = gimple_build_assign (mask, aref);
	  gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
	  int bitsize = GET_MODE_BITSIZE (TYPE_MODE (TREE_TYPE (aref)));
	  if (!INTEGRAL_TYPE_P (TREE_TYPE (aref)))
	    {
	      aref = build1 (VIEW_CONVERT_EXPR,
			     build_nonstandard_integer_type (bitsize, 0),
							     mask);
	      mask = make_ssa_name (TREE_TYPE (aref));
	      g = gimple_build_assign (mask, aref);
	      gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
	    }
	}

      g = gimple_build_cond (EQ_EXPR, mask, build_zero_cst (TREE_TYPE (mask)),
			     NULL, NULL);
      gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
      make_edge (loop->header, incr_bb, EDGE_TRUE_VALUE);
      FALLTHRU_EDGE (loop->header)->flags = EDGE_FALSE_VALUE;
    }

  basic_block latch_bb = NULL;
  basic_block new_exit_bb = NULL;

  /* Generate the condition.  */
  if (incr_bb)
    {
      gsi = gsi_last_bb (incr_bb);
      g = gimple_build_cond (LT_EXPR, iter2,
			     build_int_cst (unsigned_type_node,
					    node->simdclone->simdlen),
			     NULL, NULL);
      gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
      edge e = split_block (incr_bb, gsi_stmt (gsi));
      latch_bb = e->dest;
      new_exit_bb = split_block_after_labels (latch_bb)->dest;
      loop->latch = latch_bb;

      redirect_edge_succ (FALLTHRU_EDGE (latch_bb), body_bb);

      make_edge (incr_bb, new_exit_bb, EDGE_FALSE_VALUE);
      /* The successor of incr_bb is already pointing to latch_bb; just
	 change the flags.
	 make_edge (incr_bb, latch_bb, EDGE_TRUE_VALUE);  */
      FALLTHRU_EDGE (incr_bb)->flags = EDGE_TRUE_VALUE;
    }

  gphi *phi = create_phi_node (iter1, body_bb);
  edge preheader_edge = find_edge (entry_bb, body_bb);
  edge latch_edge = NULL;
  add_phi_arg (phi, build_zero_cst (unsigned_type_node), preheader_edge,
	       UNKNOWN_LOCATION);
  if (incr_bb)
    {
      latch_edge = single_succ_edge (latch_bb);
      add_phi_arg (phi, iter2, latch_edge, UNKNOWN_LOCATION);

      /* Generate the new return.  */
      gsi = gsi_last_bb (new_exit_bb);
      if (retval
	  && TREE_CODE (retval) == VIEW_CONVERT_EXPR
	  && TREE_CODE (TREE_OPERAND (retval, 0)) == RESULT_DECL)
	retval = TREE_OPERAND (retval, 0);
      else if (retval)
	{
	  retval = build1 (VIEW_CONVERT_EXPR,
			   TREE_TYPE (TREE_TYPE (node->decl)),
			   retval);
	  retval = force_gimple_operand_gsi (&gsi, retval, true, NULL,
					     false, GSI_CONTINUE_LINKING);
	}
      g = gimple_build_return (retval);
      gsi_insert_after (&gsi, g, GSI_CONTINUE_LINKING);
    }

  /* Handle aligned clauses by replacing default defs of the aligned
     uniform args with __builtin_assume_aligned (arg_N(D), alignment)
     lhs.  Handle linear by adding PHIs.  */
  for (unsigned i = 0; i < node->simdclone->nargs; i++)
    if (node->simdclone->args[i].arg_type == SIMD_CLONE_ARG_TYPE_UNIFORM
	&& (TREE_ADDRESSABLE (node->simdclone->args[i].orig_arg)
	    || !is_gimple_reg_type
			(TREE_TYPE (node->simdclone->args[i].orig_arg))))
      {
	tree orig_arg = node->simdclone->args[i].orig_arg;
	if (is_gimple_reg_type (TREE_TYPE (orig_arg)))
	  iter1 = make_ssa_name (TREE_TYPE (orig_arg));
	else
	  {
	    iter1 = create_tmp_var_raw (TREE_TYPE (orig_arg));
	    gimple_add_tmp_var (iter1);
	  }
	gsi = gsi_after_labels (entry_bb);
	g = gimple_build_assign (iter1, orig_arg);
	gsi_insert_before (&gsi, g, GSI_NEW_STMT);
	gsi = gsi_after_labels (body_bb);
	g = gimple_build_assign (orig_arg, iter1);
	gsi_insert_before (&gsi, g, GSI_NEW_STMT);
      }
    else if (node->simdclone->args[i].arg_type == SIMD_CLONE_ARG_TYPE_UNIFORM
	     && DECL_BY_REFERENCE (node->simdclone->args[i].orig_arg)
	     && TREE_CODE (TREE_TYPE (node->simdclone->args[i].orig_arg))
		== REFERENCE_TYPE
	     && TREE_ADDRESSABLE
		  (TREE_TYPE (TREE_TYPE (node->simdclone->args[i].orig_arg))))
      {
	tree orig_arg = node->simdclone->args[i].orig_arg;
	tree def = ssa_default_def (cfun, orig_arg);
	if (def && !has_zero_uses (def))
	  {
	    iter1 = create_tmp_var_raw (TREE_TYPE (TREE_TYPE (orig_arg)));
	    gimple_add_tmp_var (iter1);
	    gsi = gsi_after_labels (entry_bb);
	    g = gimple_build_assign (iter1, build_simple_mem_ref (def));
	    gsi_insert_before (&gsi, g, GSI_NEW_STMT);
	    gsi = gsi_after_labels (body_bb);
	    g = gimple_build_assign (build_simple_mem_ref (def), iter1);
	    gsi_insert_before (&gsi, g, GSI_NEW_STMT);
	  }
      }
    else if (node->simdclone->args[i].alignment
	     && node->simdclone->args[i].arg_type
		== SIMD_CLONE_ARG_TYPE_UNIFORM
	     && (node->simdclone->args[i].alignment
		 & (node->simdclone->args[i].alignment - 1)) == 0
	     && TREE_CODE (TREE_TYPE (node->simdclone->args[i].orig_arg))
		== POINTER_TYPE)
      {
	unsigned int alignment = node->simdclone->args[i].alignment;
	tree orig_arg = node->simdclone->args[i].orig_arg;
	tree def = ssa_default_def (cfun, orig_arg);
	if (def && !has_zero_uses (def))
	  {
	    tree fn = builtin_decl_explicit (BUILT_IN_ASSUME_ALIGNED);
	    gimple_seq seq = NULL;
	    bool need_cvt = false;
	    gcall *call
	      = gimple_build_call (fn, 2, def, size_int (alignment));
	    g = call;
	    if (!useless_type_conversion_p (TREE_TYPE (orig_arg),
					    ptr_type_node))
	      need_cvt = true;
	    tree t = make_ssa_name (need_cvt ? ptr_type_node : orig_arg);
	    gimple_call_set_lhs (g, t);
	    gimple_seq_add_stmt_without_update (&seq, g);
	    if (need_cvt)
	      {
		t = make_ssa_name (orig_arg);
		g = gimple_build_assign (t, NOP_EXPR, gimple_call_lhs (g));
		gimple_seq_add_stmt_without_update (&seq, g);
	      }
	    gsi_insert_seq_on_edge_immediate
	      (single_succ_edge (ENTRY_BLOCK_PTR_FOR_FN (cfun)), seq);

	    entry_bb = single_succ (ENTRY_BLOCK_PTR_FOR_FN (cfun));
	    int freq = compute_call_stmt_bb_frequency (current_function_decl,
						       entry_bb);
	    node->create_edge (cgraph_node::get_create (fn),
			       call, entry_bb->count, freq);

	    imm_use_iterator iter;
	    use_operand_p use_p;
	    gimple *use_stmt;
	    tree repl = gimple_get_lhs (g);
	    FOR_EACH_IMM_USE_STMT (use_stmt, iter, def)
	      if (is_gimple_debug (use_stmt) || use_stmt == call)
		continue;
	      else
		FOR_EACH_IMM_USE_ON_STMT (use_p, iter)
		  SET_USE (use_p, repl);
	  }
      }
    else if ((node->simdclone->args[i].arg_type
	      == SIMD_CLONE_ARG_TYPE_LINEAR_CONSTANT_STEP)
	     || (node->simdclone->args[i].arg_type
		 == SIMD_CLONE_ARG_TYPE_LINEAR_REF_CONSTANT_STEP)
	     || (node->simdclone->args[i].arg_type
		 == SIMD_CLONE_ARG_TYPE_LINEAR_VARIABLE_STEP)
	     || (node->simdclone->args[i].arg_type
		 == SIMD_CLONE_ARG_TYPE_LINEAR_REF_VARIABLE_STEP))
      {
	tree orig_arg = node->simdclone->args[i].orig_arg;
	gcc_assert (INTEGRAL_TYPE_P (TREE_TYPE (orig_arg))
		    || POINTER_TYPE_P (TREE_TYPE (orig_arg)));
	tree def = NULL_TREE;
	if (TREE_ADDRESSABLE (orig_arg))
	  {
	    def = make_ssa_name (TREE_TYPE (orig_arg));
	    iter1 = make_ssa_name (TREE_TYPE (orig_arg));
	    if (incr_bb)
	      iter2 = make_ssa_name (TREE_TYPE (orig_arg));
	    gsi = gsi_after_labels (entry_bb);
	    g = gimple_build_assign (def, orig_arg);
	    gsi_insert_before (&gsi, g, GSI_NEW_STMT);
	  }
	else
	  {
	    def = ssa_default_def (cfun, orig_arg);
	    if (!def || has_zero_uses (def))
	      def = NULL_TREE;
	    else
	      {
		iter1 = make_ssa_name (orig_arg);
		if (incr_bb)
		  iter2 = make_ssa_name (orig_arg);
	      }
	  }
	if (def)
	  {
	    phi = create_phi_node (iter1, body_bb);
	    add_phi_arg (phi, def, preheader_edge, UNKNOWN_LOCATION);
	    if (incr_bb)
	      {
		add_phi_arg (phi, iter2, latch_edge, UNKNOWN_LOCATION);
		enum tree_code code = INTEGRAL_TYPE_P (TREE_TYPE (orig_arg))
				      ? PLUS_EXPR : POINTER_PLUS_EXPR;
		tree addtype = INTEGRAL_TYPE_P (TREE_TYPE (orig_arg))
			       ? TREE_TYPE (orig_arg) : sizetype;
		tree addcst = simd_clone_linear_addend (node, i, addtype,
							entry_bb);
		gsi = gsi_last_bb (incr_bb);
		g = gimple_build_assign (iter2, code, iter1, addcst);
		gsi_insert_before (&gsi, g, GSI_SAME_STMT);
	      }

	    imm_use_iterator iter;
	    use_operand_p use_p;
	    gimple *use_stmt;
	    if (TREE_ADDRESSABLE (orig_arg))
	      {
		gsi = gsi_after_labels (body_bb);
		g = gimple_build_assign (orig_arg, iter1);
		gsi_insert_before (&gsi, g, GSI_NEW_STMT);
	      }
	    else
	      FOR_EACH_IMM_USE_STMT (use_stmt, iter, def)
		if (use_stmt == phi)
		  continue;
		else
		  FOR_EACH_IMM_USE_ON_STMT (use_p, iter)
		    SET_USE (use_p, iter1);
	  }
      }
    else if (node->simdclone->args[i].arg_type
	     == SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_CONSTANT_STEP
	     || (node->simdclone->args[i].arg_type
		 == SIMD_CLONE_ARG_TYPE_LINEAR_UVAL_VARIABLE_STEP))
      {
	tree orig_arg = node->simdclone->args[i].orig_arg;
	tree def = ssa_default_def (cfun, orig_arg);
	gcc_assert (!TREE_ADDRESSABLE (orig_arg)
		    && TREE_CODE (TREE_TYPE (orig_arg)) == REFERENCE_TYPE);
	if (def && !has_zero_uses (def))
	  {
	    tree rtype = TREE_TYPE (TREE_TYPE (orig_arg));
	    iter1 = make_ssa_name (orig_arg);
	    if (incr_bb)
	      iter2 = make_ssa_name (orig_arg);
	    tree iter3 = make_ssa_name (rtype);
	    tree iter4 = make_ssa_name (rtype);
	    tree iter5 = incr_bb ? make_ssa_name (rtype) : NULL_TREE;
	    gsi = gsi_after_labels (entry_bb);
	    gimple *load
	      = gimple_build_assign (iter3, build_simple_mem_ref (def));
	    gsi_insert_before (&gsi, load, GSI_NEW_STMT);

	    tree array = node->simdclone->args[i].simd_array;
	    TREE_ADDRESSABLE (array) = 1;
	    tree ptr = build_fold_addr_expr (array);
	    phi = create_phi_node (iter1, body_bb);
	    add_phi_arg (phi, ptr, preheader_edge, UNKNOWN_LOCATION);
	    if (incr_bb)
	      {
		add_phi_arg (phi, iter2, latch_edge, UNKNOWN_LOCATION);
		g = gimple_build_assign (iter2, POINTER_PLUS_EXPR, iter1,
					 TYPE_SIZE_UNIT (TREE_TYPE (iter3)));
		gsi = gsi_last_bb (incr_bb);
		gsi_insert_before (&gsi, g, GSI_SAME_STMT);
	      }

	    phi = create_phi_node (iter4, body_bb);
	    add_phi_arg (phi, iter3, preheader_edge, UNKNOWN_LOCATION);
	    if (incr_bb)
	      {
		add_phi_arg (phi, iter5, latch_edge, UNKNOWN_LOCATION);
		enum tree_code code = INTEGRAL_TYPE_P (TREE_TYPE (iter3))
				      ? PLUS_EXPR : POINTER_PLUS_EXPR;
		tree addtype = INTEGRAL_TYPE_P (TREE_TYPE (iter3))
			       ? TREE_TYPE (iter3) : sizetype;
		tree addcst = simd_clone_linear_addend (node, i, addtype,
							entry_bb);
		g = gimple_build_assign (iter5, code, iter4, addcst);
		gsi = gsi_last_bb (incr_bb);
		gsi_insert_before (&gsi, g, GSI_SAME_STMT);
	      }

	    g = gimple_build_assign (build_simple_mem_ref (iter1), iter4);
	    gsi = gsi_after_labels (body_bb);
	    gsi_insert_before (&gsi, g, GSI_SAME_STMT);

	    imm_use_iterator iter;
	    use_operand_p use_p;
	    gimple *use_stmt;
	    FOR_EACH_IMM_USE_STMT (use_stmt, iter, def)
	      if (use_stmt == load)
		continue;
	      else
		FOR_EACH_IMM_USE_ON_STMT (use_p, iter)
		  SET_USE (use_p, iter1);

	    if (!TYPE_READONLY (rtype) && incr_bb)
	      {
		tree v = make_ssa_name (rtype);
		tree aref = build4 (ARRAY_REF, rtype, array,
				    size_zero_node, NULL_TREE,
				    NULL_TREE);
		gsi = gsi_after_labels (new_exit_bb);
		g = gimple_build_assign (v, aref);
		gsi_insert_before (&gsi, g, GSI_SAME_STMT);
		g = gimple_build_assign (build_simple_mem_ref (def), v);
		gsi_insert_before (&gsi, g, GSI_SAME_STMT);
	      }
	  }
      }

  calculate_dominance_info (CDI_DOMINATORS);
  if (loop)
    add_loop (loop, loop->header->loop_father);
  update_ssa (TODO_update_ssa);

  pop_cfun ();
}

/* If the function in NODE is tagged as an elemental SIMD function,
   create the appropriate SIMD clones.  */

static void
expand_simd_clones (struct cgraph_node *node)
{
  tree attr = lookup_attribute ("omp declare simd",
				DECL_ATTRIBUTES (node->decl));
  if (attr == NULL_TREE
      || node->global.inlined_to
      || lookup_attribute ("noclone", DECL_ATTRIBUTES (node->decl)))
    return;

  /* Ignore
     #pragma omp declare simd
     extern int foo ();
     in C, there we don't know the argument types at all.  */
  if (!node->definition
      && TYPE_ARG_TYPES (TREE_TYPE (node->decl)) == NULL_TREE)
    return;

  /* Call this before creating clone_info, as it might ggc_collect.  */
  if (node->definition && node->has_gimple_body_p ())
    node->get_body ();

  do
    {
      /* Start with parsing the "omp declare simd" attribute(s).  */
      bool inbranch_clause_specified;
      struct cgraph_simd_clone *clone_info
	= simd_clone_clauses_extract (node, TREE_VALUE (attr),
				      &inbranch_clause_specified);
      if (clone_info == NULL)
	continue;

      int orig_simdlen = clone_info->simdlen;
      tree base_type = simd_clone_compute_base_data_type (node, clone_info);
      /* The target can return 0 (no simd clones should be created),
	 1 (just one ISA of simd clones should be created) or higher
	 count of ISA variants.  In that case, clone_info is initialized
	 for the first ISA variant.  */
      int count
	= targetm.simd_clone.compute_vecsize_and_simdlen (node, clone_info,
							  base_type, 0);
      if (count == 0)
	continue;

      /* Loop over all COUNT ISA variants, and if !INBRANCH_CLAUSE_SPECIFIED,
	 also create one inbranch and one !inbranch clone of it.  */
      for (int i = 0; i < count * 2; i++)
	{
	  struct cgraph_simd_clone *clone = clone_info;
	  if (inbranch_clause_specified && (i & 1) != 0)
	    continue;

	  if (i != 0)
	    {
	      clone = simd_clone_struct_alloc (clone_info->nargs
					       + ((i & 1) != 0));
	      simd_clone_struct_copy (clone, clone_info);
	      /* Undo changes targetm.simd_clone.compute_vecsize_and_simdlen
		 and simd_clone_adjust_argument_types did to the first
		 clone's info.  */
	      clone->nargs -= clone_info->inbranch;
	      clone->simdlen = orig_simdlen;
	      /* And call the target hook again to get the right ISA.  */
	      targetm.simd_clone.compute_vecsize_and_simdlen (node, clone,
							      base_type,
							      i / 2);
	      if ((i & 1) != 0)
		clone->inbranch = 1;
	    }

	  /* simd_clone_mangle might fail if such a clone has been created
	     already.  */
	  tree id = simd_clone_mangle (node, clone);
	  if (id == NULL_TREE)
	    continue;

	  /* Only when we are sure we want to create the clone actually
	     clone the function (or definitions) or create another
	     extern FUNCTION_DECL (for prototypes without definitions).  */
	  struct cgraph_node *n = simd_clone_create (node);
	  if (n == NULL)
	    continue;

	  n->simdclone = clone;
	  clone->origin = node;
	  clone->next_clone = NULL;
	  if (node->simd_clones == NULL)
	    {
	      clone->prev_clone = n;
	      node->simd_clones = n;
	    }
	  else
	    {
	      clone->prev_clone = node->simd_clones->simdclone->prev_clone;
	      clone->prev_clone->simdclone->next_clone = n;
	      node->simd_clones->simdclone->prev_clone = n;
	    }
	  symtab->change_decl_assembler_name (n->decl, id);
	  /* And finally adjust the return type, parameters and for
	     definitions also function body.  */
	  if (node->definition)
	    simd_clone_adjust (n);
	  else
	    {
	      simd_clone_adjust_return_type (n);
	      simd_clone_adjust_argument_types (n);
	    }
	}
    }
  while ((attr = lookup_attribute ("omp declare simd", TREE_CHAIN (attr))));
}

/* Entry point for IPA simd clone creation pass.  */

static unsigned int
ipa_omp_simd_clone (void)
{
  struct cgraph_node *node;
  FOR_EACH_FUNCTION (node)
    expand_simd_clones (node);
  return 0;
}

namespace {

const pass_data pass_data_omp_simd_clone =
{
  SIMPLE_IPA_PASS,		/* type */
  "simdclone",			/* name */
  OPTGROUP_OMP,			/* optinfo_flags */
  TV_NONE,			/* tv_id */
  ( PROP_ssa | PROP_cfg ),	/* properties_required */
  0,				/* properties_provided */
  0,				/* properties_destroyed */
  0,				/* todo_flags_start */
  0,				/* todo_flags_finish */
};

class pass_omp_simd_clone : public simple_ipa_opt_pass
{
public:
  pass_omp_simd_clone(gcc::context *ctxt)
    : simple_ipa_opt_pass(pass_data_omp_simd_clone, ctxt)
  {}

  /* opt_pass methods: */
  virtual bool gate (function *);
  virtual unsigned int execute (function *) { return ipa_omp_simd_clone (); }
};

bool
pass_omp_simd_clone::gate (function *)
{
  return targetm.simd_clone.compute_vecsize_and_simdlen != NULL;
}

} // anon namespace

simple_ipa_opt_pass *
make_pass_omp_simd_clone (gcc::context *ctxt)
{
  return new pass_omp_simd_clone (ctxt);
}
