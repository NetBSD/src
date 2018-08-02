/* Operations with affine combinations of trees.
   Copyright (C) 2005-2016 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

GCC is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

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
#include "tree-pretty-print.h"
#include "fold-const.h"
#include "tree-affine.h"
#include "gimplify.h"
#include "dumpfile.h"
#include "cfgexpand.h"

/* Extends CST as appropriate for the affine combinations COMB.  */

widest_int
wide_int_ext_for_comb (const widest_int &cst, aff_tree *comb)
{
  return wi::sext (cst, TYPE_PRECISION (comb->type));
}

/* Initializes affine combination COMB so that its value is zero in TYPE.  */

static void
aff_combination_zero (aff_tree *comb, tree type)
{
  int i;
  comb->type = type;
  comb->offset = 0;
  comb->n = 0;
  for (i = 0; i < MAX_AFF_ELTS; i++)
    comb->elts[i].coef = 0;
  comb->rest = NULL_TREE;
}

/* Sets COMB to CST.  */

void
aff_combination_const (aff_tree *comb, tree type, const widest_int &cst)
{
  aff_combination_zero (comb, type);
  comb->offset = wide_int_ext_for_comb (cst, comb);;
}

/* Sets COMB to single element ELT.  */

void
aff_combination_elt (aff_tree *comb, tree type, tree elt)
{
  aff_combination_zero (comb, type);

  comb->n = 1;
  comb->elts[0].val = elt;
  comb->elts[0].coef = 1;
}

/* Scales COMB by SCALE.  */

void
aff_combination_scale (aff_tree *comb, const widest_int &scale_in)
{
  unsigned i, j;

  widest_int scale = wide_int_ext_for_comb (scale_in, comb);
  if (scale == 1)
    return;

  if (scale == 0)
    {
      aff_combination_zero (comb, comb->type);
      return;
    }

  comb->offset = wide_int_ext_for_comb (scale * comb->offset, comb);
  for (i = 0, j = 0; i < comb->n; i++)
    {
      widest_int new_coef
	= wide_int_ext_for_comb (scale * comb->elts[i].coef, comb);
      /* A coefficient may become zero due to overflow.  Remove the zero
	 elements.  */
      if (new_coef == 0)
	continue;
      comb->elts[j].coef = new_coef;
      comb->elts[j].val = comb->elts[i].val;
      j++;
    }
  comb->n = j;

  if (comb->rest)
    {
      tree type = comb->type;
      if (POINTER_TYPE_P (type))
	type = sizetype;
      if (comb->n < MAX_AFF_ELTS)
	{
	  comb->elts[comb->n].coef = scale;
	  comb->elts[comb->n].val = comb->rest;
	  comb->rest = NULL_TREE;
	  comb->n++;
	}
      else
	comb->rest = fold_build2 (MULT_EXPR, type, comb->rest,
				  wide_int_to_tree (type, scale));
    }
}

/* Adds ELT * SCALE to COMB.  */

void
aff_combination_add_elt (aff_tree *comb, tree elt, const widest_int &scale_in)
{
  unsigned i;
  tree type;

  widest_int scale = wide_int_ext_for_comb (scale_in, comb);
  if (scale == 0)
    return;

  for (i = 0; i < comb->n; i++)
    if (operand_equal_p (comb->elts[i].val, elt, 0))
      {
	widest_int new_coef
	  = wide_int_ext_for_comb (comb->elts[i].coef + scale, comb);
	if (new_coef != 0)
	  {
	    comb->elts[i].coef = new_coef;
	    return;
	  }

	comb->n--;
	comb->elts[i] = comb->elts[comb->n];

	if (comb->rest)
	  {
	    gcc_assert (comb->n == MAX_AFF_ELTS - 1);
	    comb->elts[comb->n].coef = 1;
	    comb->elts[comb->n].val = comb->rest;
	    comb->rest = NULL_TREE;
	    comb->n++;
	  }
	return;
      }
  if (comb->n < MAX_AFF_ELTS)
    {
      comb->elts[comb->n].coef = scale;
      comb->elts[comb->n].val = elt;
      comb->n++;
      return;
    }

  type = comb->type;
  if (POINTER_TYPE_P (type))
    type = sizetype;

  if (scale == 1)
    elt = fold_convert (type, elt);
  else
    elt = fold_build2 (MULT_EXPR, type,
		       fold_convert (type, elt),
		       wide_int_to_tree (type, scale));

  if (comb->rest)
    comb->rest = fold_build2 (PLUS_EXPR, type, comb->rest,
			      elt);
  else
    comb->rest = elt;
}

/* Adds CST to C.  */

static void
aff_combination_add_cst (aff_tree *c, const widest_int &cst)
{
  c->offset = wide_int_ext_for_comb (c->offset + cst, c);
}

/* Adds COMB2 to COMB1.  */

void
aff_combination_add (aff_tree *comb1, aff_tree *comb2)
{
  unsigned i;

  aff_combination_add_cst (comb1, comb2->offset);
  for (i = 0; i < comb2->n; i++)
    aff_combination_add_elt (comb1, comb2->elts[i].val, comb2->elts[i].coef);
  if (comb2->rest)
    aff_combination_add_elt (comb1, comb2->rest, 1);
}

/* Converts affine combination COMB to TYPE.  */

void
aff_combination_convert (aff_tree *comb, tree type)
{
  unsigned i, j;
  tree comb_type = comb->type;

  if  (TYPE_PRECISION (type) > TYPE_PRECISION (comb_type))
    {
      tree val = fold_convert (type, aff_combination_to_tree (comb));
      tree_to_aff_combination (val, type, comb);
      return;
    }

  comb->type = type;
  if (comb->rest && !POINTER_TYPE_P (type))
    comb->rest = fold_convert (type, comb->rest);

  if (TYPE_PRECISION (type) == TYPE_PRECISION (comb_type))
    return;

  comb->offset = wide_int_ext_for_comb (comb->offset, comb);
  for (i = j = 0; i < comb->n; i++)
    {
      if (comb->elts[i].coef == 0)
	continue;
      comb->elts[j].coef = comb->elts[i].coef;
      comb->elts[j].val = fold_convert (type, comb->elts[i].val);
      j++;
    }

  comb->n = j;
  if (comb->n < MAX_AFF_ELTS && comb->rest)
    {
      comb->elts[comb->n].coef = 1;
      comb->elts[comb->n].val = comb->rest;
      comb->rest = NULL_TREE;
      comb->n++;
    }
}

/* Splits EXPR into an affine combination of parts.  */

void
tree_to_aff_combination (tree expr, tree type, aff_tree *comb)
{
  aff_tree tmp;
  enum tree_code code;
  tree cst, core, toffset;
  HOST_WIDE_INT bitpos, bitsize;
  machine_mode mode;
  int unsignedp, reversep, volatilep;

  STRIP_NOPS (expr);

  code = TREE_CODE (expr);
  switch (code)
    {
    case INTEGER_CST:
      aff_combination_const (comb, type, wi::to_widest (expr));
      return;

    case POINTER_PLUS_EXPR:
      tree_to_aff_combination (TREE_OPERAND (expr, 0), type, comb);
      tree_to_aff_combination (TREE_OPERAND (expr, 1), sizetype, &tmp);
      aff_combination_add (comb, &tmp);
      return;

    case PLUS_EXPR:
    case MINUS_EXPR:
      tree_to_aff_combination (TREE_OPERAND (expr, 0), type, comb);
      tree_to_aff_combination (TREE_OPERAND (expr, 1), type, &tmp);
      if (code == MINUS_EXPR)
	aff_combination_scale (&tmp, -1);
      aff_combination_add (comb, &tmp);
      return;

    case MULT_EXPR:
      cst = TREE_OPERAND (expr, 1);
      if (TREE_CODE (cst) != INTEGER_CST)
	break;
      tree_to_aff_combination (TREE_OPERAND (expr, 0), type, comb);
      aff_combination_scale (comb, wi::to_widest (cst));
      return;

    case NEGATE_EXPR:
      tree_to_aff_combination (TREE_OPERAND (expr, 0), type, comb);
      aff_combination_scale (comb, -1);
      return;

    case BIT_NOT_EXPR:
      /* ~x = -x - 1 */
      tree_to_aff_combination (TREE_OPERAND (expr, 0), type, comb);
      aff_combination_scale (comb, -1);
      aff_combination_add_cst (comb, -1);
      return;

    case ADDR_EXPR:
      /* Handle &MEM[ptr + CST] which is equivalent to POINTER_PLUS_EXPR.  */
      if (TREE_CODE (TREE_OPERAND (expr, 0)) == MEM_REF)
	{
	  expr = TREE_OPERAND (expr, 0);
	  tree_to_aff_combination (TREE_OPERAND (expr, 0), type, comb);
	  tree_to_aff_combination (TREE_OPERAND (expr, 1), sizetype, &tmp);
	  aff_combination_add (comb, &tmp);
	  return;
	}
      core = get_inner_reference (TREE_OPERAND (expr, 0), &bitsize, &bitpos,
				  &toffset, &mode, &unsignedp, &reversep,
				  &volatilep, false);
      if (bitpos % BITS_PER_UNIT != 0)
	break;
      aff_combination_const (comb, type, bitpos / BITS_PER_UNIT);
      if (TREE_CODE (core) == MEM_REF)
	{
	  aff_combination_add_cst (comb, wi::to_widest (TREE_OPERAND (core, 1)));
	  core = TREE_OPERAND (core, 0);
	}
      else
	core = build_fold_addr_expr (core);

      if (TREE_CODE (core) == ADDR_EXPR)
	aff_combination_add_elt (comb, core, 1);
      else
	{
	  tree_to_aff_combination (core, type, &tmp);
	  aff_combination_add (comb, &tmp);
	}
      if (toffset)
	{
	  tree_to_aff_combination (toffset, type, &tmp);
	  aff_combination_add (comb, &tmp);
	}
      return;

    case MEM_REF:
      if (TREE_CODE (TREE_OPERAND (expr, 0)) == ADDR_EXPR)
	tree_to_aff_combination (TREE_OPERAND (TREE_OPERAND (expr, 0), 0),
				 type, comb);
      else if (integer_zerop (TREE_OPERAND (expr, 1)))
	{
	  aff_combination_elt (comb, type, expr);
	  return;
	}
      else
	aff_combination_elt (comb, type,
			     build2 (MEM_REF, TREE_TYPE (expr),
				     TREE_OPERAND (expr, 0),
				     build_int_cst
				      (TREE_TYPE (TREE_OPERAND (expr, 1)), 0)));
      tree_to_aff_combination (TREE_OPERAND (expr, 1), sizetype, &tmp);
      aff_combination_add (comb, &tmp);
      return;

    default:
      break;
    }

  aff_combination_elt (comb, type, expr);
}

/* Creates EXPR + ELT * SCALE in TYPE.  EXPR is taken from affine
   combination COMB.  */

static tree
add_elt_to_tree (tree expr, tree type, tree elt, const widest_int &scale_in,
		 aff_tree *comb ATTRIBUTE_UNUSED)
{
  enum tree_code code;
  tree type1 = type;
  if (POINTER_TYPE_P (type))
    type1 = sizetype;

  widest_int scale = wide_int_ext_for_comb (scale_in, comb);

  if (scale == -1
      && POINTER_TYPE_P (TREE_TYPE (elt)))
    {
      elt = convert_to_ptrofftype (elt);
      elt = fold_build1 (NEGATE_EXPR, TREE_TYPE (elt), elt);
      scale = 1;
    }

  if (scale == 1)
    {
      if (!expr)
	{
	  if (POINTER_TYPE_P (TREE_TYPE (elt)))
	    return elt;
	  else
	    return fold_convert (type1, elt);
	}

      if (POINTER_TYPE_P (TREE_TYPE (expr)))
	return fold_build_pointer_plus (expr, elt);
      if (POINTER_TYPE_P (TREE_TYPE (elt)))
	return fold_build_pointer_plus (elt, expr);
      return fold_build2 (PLUS_EXPR, type1,
			  expr, fold_convert (type1, elt));
    }

  if (scale == -1)
    {
      if (!expr)
	return fold_build1 (NEGATE_EXPR, type1,
			    fold_convert (type1, elt));

      if (POINTER_TYPE_P (TREE_TYPE (expr)))
	{
	  elt = convert_to_ptrofftype (elt);
	  elt = fold_build1 (NEGATE_EXPR, TREE_TYPE (elt), elt);
	  return fold_build_pointer_plus (expr, elt);
	}
      return fold_build2 (MINUS_EXPR, type1,
			  expr, fold_convert (type1, elt));
    }

  elt = fold_convert (type1, elt);
  if (!expr)
    return fold_build2 (MULT_EXPR, type1, elt,
			wide_int_to_tree (type1, scale));

  if (wi::neg_p (scale))
    {
      code = MINUS_EXPR;
      scale = -scale;
    }
  else
    code = PLUS_EXPR;

  elt = fold_build2 (MULT_EXPR, type1, elt,
		     wide_int_to_tree (type1, scale));
  if (POINTER_TYPE_P (TREE_TYPE (expr)))
    {
      if (code == MINUS_EXPR)
        elt = fold_build1 (NEGATE_EXPR, type1, elt);
      return fold_build_pointer_plus (expr, elt);
    }
  return fold_build2 (code, type1, expr, elt);
}

/* Makes tree from the affine combination COMB.  */

tree
aff_combination_to_tree (aff_tree *comb)
{
  tree type = comb->type;
  tree expr = NULL_TREE;
  unsigned i;
  widest_int off, sgn;
  tree type1 = type;
  if (POINTER_TYPE_P (type))
    type1 = sizetype;

  gcc_assert (comb->n == MAX_AFF_ELTS || comb->rest == NULL_TREE);

  for (i = 0; i < comb->n; i++)
    expr = add_elt_to_tree (expr, type, comb->elts[i].val, comb->elts[i].coef,
			    comb);

  if (comb->rest)
    expr = add_elt_to_tree (expr, type, comb->rest, 1, comb);

  /* Ensure that we get x - 1, not x + (-1) or x + 0xff..f if x is
     unsigned.  */
  if (wi::neg_p (comb->offset))
    {
      off = -comb->offset;
      sgn = -1;
    }
  else
    {
      off = comb->offset;
      sgn = 1;
    }
  return add_elt_to_tree (expr, type, wide_int_to_tree (type1, off), sgn,
			  comb);
}

/* Copies the tree elements of COMB to ensure that they are not shared.  */

void
unshare_aff_combination (aff_tree *comb)
{
  unsigned i;

  for (i = 0; i < comb->n; i++)
    comb->elts[i].val = unshare_expr (comb->elts[i].val);
  if (comb->rest)
    comb->rest = unshare_expr (comb->rest);
}

/* Remove M-th element from COMB.  */

void
aff_combination_remove_elt (aff_tree *comb, unsigned m)
{
  comb->n--;
  if (m <= comb->n)
    comb->elts[m] = comb->elts[comb->n];
  if (comb->rest)
    {
      comb->elts[comb->n].coef = 1;
      comb->elts[comb->n].val = comb->rest;
      comb->rest = NULL_TREE;
      comb->n++;
    }
}

/* Adds C * COEF * VAL to R.  VAL may be NULL, in that case only
   C * COEF is added to R.  */


static void
aff_combination_add_product (aff_tree *c, const widest_int &coef, tree val,
			     aff_tree *r)
{
  unsigned i;
  tree aval, type;

  for (i = 0; i < c->n; i++)
    {
      aval = c->elts[i].val;
      if (val)
	{
	  type = TREE_TYPE (aval);
	  aval = fold_build2 (MULT_EXPR, type, aval,
			      fold_convert (type, val));
	}

      aff_combination_add_elt (r, aval, coef * c->elts[i].coef);
    }

  if (c->rest)
    {
      aval = c->rest;
      if (val)
	{
	  type = TREE_TYPE (aval);
	  aval = fold_build2 (MULT_EXPR, type, aval,
			      fold_convert (type, val));
	}

      aff_combination_add_elt (r, aval, coef);
    }

  if (val)
    aff_combination_add_elt (r, val, coef * c->offset);
  else
    aff_combination_add_cst (r, coef * c->offset);
}

/* Multiplies C1 by C2, storing the result to R  */

void
aff_combination_mult (aff_tree *c1, aff_tree *c2, aff_tree *r)
{
  unsigned i;
  gcc_assert (TYPE_PRECISION (c1->type) == TYPE_PRECISION (c2->type));

  aff_combination_zero (r, c1->type);

  for (i = 0; i < c2->n; i++)
    aff_combination_add_product (c1, c2->elts[i].coef, c2->elts[i].val, r);
  if (c2->rest)
    aff_combination_add_product (c1, 1, c2->rest, r);
  aff_combination_add_product (c1, c2->offset, NULL, r);
}

/* Returns the element of COMB whose value is VAL, or NULL if no such
   element exists.  If IDX is not NULL, it is set to the index of VAL in
   COMB.  */

static struct aff_comb_elt *
aff_combination_find_elt (aff_tree *comb, tree val, unsigned *idx)
{
  unsigned i;

  for (i = 0; i < comb->n; i++)
    if (operand_equal_p (comb->elts[i].val, val, 0))
      {
	if (idx)
	  *idx = i;

	return &comb->elts[i];
      }

  return NULL;
}

/* Element of the cache that maps ssa name NAME to its expanded form
   as an affine expression EXPANSION.  */

struct name_expansion
{
  aff_tree expansion;

  /* True if the expansion for the name is just being generated.  */
  unsigned in_progress : 1;
};

/* Expands SSA names in COMB recursively.  CACHE is used to cache the
   results.  */

void
aff_combination_expand (aff_tree *comb ATTRIBUTE_UNUSED,
			hash_map<tree, name_expansion *> **cache)
{
  unsigned i;
  aff_tree to_add, current, curre;
  tree e, rhs;
  gimple *def;
  widest_int scale;
  struct name_expansion *exp;

  aff_combination_zero (&to_add, comb->type);
  for (i = 0; i < comb->n; i++)
    {
      tree type, name;
      enum tree_code code;

      e = comb->elts[i].val;
      type = TREE_TYPE (e);
      name = e;
      /* Look through some conversions.  */
      if (CONVERT_EXPR_P (e)
          && (TYPE_PRECISION (type)
	      >= TYPE_PRECISION (TREE_TYPE (TREE_OPERAND (e, 0)))))
	name = TREE_OPERAND (e, 0);
      if (TREE_CODE (name) != SSA_NAME)
	continue;
      def = SSA_NAME_DEF_STMT (name);
      if (!is_gimple_assign (def) || gimple_assign_lhs (def) != name)
	continue;

      code = gimple_assign_rhs_code (def);
      if (code != SSA_NAME
	  && !IS_EXPR_CODE_CLASS (TREE_CODE_CLASS (code))
	  && (get_gimple_rhs_class (code) != GIMPLE_SINGLE_RHS
	      || !is_gimple_min_invariant (gimple_assign_rhs1 (def))))
	continue;

      /* We do not know whether the reference retains its value at the
	 place where the expansion is used.  */
      if (TREE_CODE_CLASS (code) == tcc_reference)
	continue;

      if (!*cache)
	*cache = new hash_map<tree, name_expansion *>;
      name_expansion **slot = &(*cache)->get_or_insert (e);
      exp = *slot;

      if (!exp)
	{
	  exp = XNEW (struct name_expansion);
	  exp->in_progress = 1;
	  *slot = exp;
	  /* In principle this is a generally valid folding, but
	     it is not unconditionally an optimization, so do it
	     here and not in fold_unary.  */
	  /* Convert (T1)(X *+- CST) into (T1)X *+- (T1)CST if T1 is wider
	     than the type of X and overflow for the type of X is
	     undefined.  */
	  if (e != name
	      && INTEGRAL_TYPE_P (type)
	      && INTEGRAL_TYPE_P (TREE_TYPE (name))
	      && TYPE_OVERFLOW_UNDEFINED (TREE_TYPE (name))
	      && TYPE_PRECISION (type) > TYPE_PRECISION (TREE_TYPE (name))
	      && (code == PLUS_EXPR || code == MINUS_EXPR || code == MULT_EXPR)
	      && TREE_CODE (gimple_assign_rhs2 (def)) == INTEGER_CST)
	    rhs = fold_build2 (code, type,
			       fold_convert (type, gimple_assign_rhs1 (def)),
			       fold_convert (type, gimple_assign_rhs2 (def)));
	  else
	    {
	      rhs = gimple_assign_rhs_to_tree (def);
	      if (e != name)
		rhs = fold_convert (type, rhs);
	    }
	  tree_to_aff_combination_expand (rhs, comb->type, &current, cache);
	  exp->expansion = current;
	  exp->in_progress = 0;
	}
      else
	{
	  /* Since we follow the definitions in the SSA form, we should not
	     enter a cycle unless we pass through a phi node.  */
	  gcc_assert (!exp->in_progress);
	  current = exp->expansion;
	}

      /* Accumulate the new terms to TO_ADD, so that we do not modify
	 COMB while traversing it; include the term -coef * E, to remove
         it from COMB.  */
      scale = comb->elts[i].coef;
      aff_combination_zero (&curre, comb->type);
      aff_combination_add_elt (&curre, e, -scale);
      aff_combination_scale (&current, scale);
      aff_combination_add (&to_add, &current);
      aff_combination_add (&to_add, &curre);
    }
  aff_combination_add (comb, &to_add);
}

/* Similar to tree_to_aff_combination, but follows SSA name definitions
   and expands them recursively.  CACHE is used to cache the expansions
   of the ssa names, to avoid exponential time complexity for cases
   like

   a1 = a0 + a0;
   a2 = a1 + a1;
   a3 = a2 + a2;
   ...  */

void
tree_to_aff_combination_expand (tree expr, tree type, aff_tree *comb,
				hash_map<tree, name_expansion *> **cache)
{
  tree_to_aff_combination (expr, type, comb);
  aff_combination_expand (comb, cache);
}

/* Frees memory occupied by struct name_expansion in *VALUE.  Callback for
   hash_map::traverse.  */

bool
free_name_expansion (tree const &, name_expansion **value, void *)
{
  free (*value);
  return true;
}

/* Frees memory allocated for the CACHE used by
   tree_to_aff_combination_expand.  */

void
free_affine_expand_cache (hash_map<tree, name_expansion *> **cache)
{
  if (!*cache)
    return;

  (*cache)->traverse<void *, free_name_expansion> (NULL);
  delete (*cache);
  *cache = NULL;
}

/* If VAL != CST * DIV for any constant CST, returns false.
   Otherwise, if *MULT_SET is true, additionally compares CST and MULT,
   and if they are different, returns false.  Finally, if neither of these
   two cases occur, true is returned, and CST is stored to MULT and MULT_SET
   is set to true.  */

static bool
wide_int_constant_multiple_p (const widest_int &val, const widest_int &div,
			      bool *mult_set, widest_int *mult)
{
  widest_int rem, cst;

  if (val == 0)
    {
      if (*mult_set && mult != 0)
	return false;
      *mult_set = true;
      *mult = 0;
      return true;
    }

  if (div == 0)
    return false;

  if (!wi::multiple_of_p (val, div, SIGNED, &cst))
    return false;

  if (*mult_set && *mult != cst)
    return false;

  *mult_set = true;
  *mult = cst;
  return true;
}

/* Returns true if VAL = X * DIV for some constant X.  If this is the case,
   X is stored to MULT.  */

bool
aff_combination_constant_multiple_p (aff_tree *val, aff_tree *div,
				     widest_int *mult)
{
  bool mult_set = false;
  unsigned i;

  if (val->n == 0 && val->offset == 0)
    {
      *mult = 0;
      return true;
    }
  if (val->n != div->n)
    return false;

  if (val->rest || div->rest)
    return false;

  if (!wide_int_constant_multiple_p (val->offset, div->offset,
				     &mult_set, mult))
    return false;

  for (i = 0; i < div->n; i++)
    {
      struct aff_comb_elt *elt
	      = aff_combination_find_elt (val, div->elts[i].val, NULL);
      if (!elt)
	return false;
      if (!wide_int_constant_multiple_p (elt->coef, div->elts[i].coef,
					 &mult_set, mult))
	return false;
    }

  gcc_assert (mult_set);
  return true;
}

/* Prints the affine VAL to the FILE. */

static void
print_aff (FILE *file, aff_tree *val)
{
  unsigned i;
  signop sgn = TYPE_SIGN (val->type);
  if (POINTER_TYPE_P (val->type))
    sgn = SIGNED;
  fprintf (file, "{\n  type = ");
  print_generic_expr (file, val->type, TDF_VOPS|TDF_MEMSYMS);
  fprintf (file, "\n  offset = ");
  print_dec (val->offset, file, sgn);
  if (val->n > 0)
    {
      fprintf (file, "\n  elements = {\n");
      for (i = 0; i < val->n; i++)
	{
	  fprintf (file, "    [%d] = ", i);
	  print_generic_expr (file, val->elts[i].val, TDF_VOPS|TDF_MEMSYMS);

	  fprintf (file, " * ");
	  print_dec (val->elts[i].coef, file, sgn);
	  if (i != val->n - 1)
	    fprintf (file, ", \n");
	}
      fprintf (file, "\n  }");
  }
  if (val->rest)
    {
      fprintf (file, "\n  rest = ");
      print_generic_expr (file, val->rest, TDF_VOPS|TDF_MEMSYMS);
    }
  fprintf (file, "\n}");
}

/* Prints the affine VAL to the standard error, used for debugging.  */

DEBUG_FUNCTION void
debug_aff (aff_tree *val)
{
  print_aff (stderr, val);
  fprintf (stderr, "\n");
}

/* Computes address of the reference REF in ADDR.  The size of the accessed
   location is stored to SIZE.  Returns the ultimate containing object to
   which REF refers.  */

tree
get_inner_reference_aff (tree ref, aff_tree *addr, widest_int *size)
{
  HOST_WIDE_INT bitsize, bitpos;
  tree toff;
  machine_mode mode;
  int uns, rev, vol;
  aff_tree tmp;
  tree base = get_inner_reference (ref, &bitsize, &bitpos, &toff, &mode,
				   &uns, &rev, &vol, false);
  tree base_addr = build_fold_addr_expr (base);

  /* ADDR = &BASE + TOFF + BITPOS / BITS_PER_UNIT.  */

  tree_to_aff_combination (base_addr, sizetype, addr);

  if (toff)
    {
      tree_to_aff_combination (toff, sizetype, &tmp);
      aff_combination_add (addr, &tmp);
    }

  aff_combination_const (&tmp, sizetype, bitpos / BITS_PER_UNIT);
  aff_combination_add (addr, &tmp);

  *size = (bitsize + BITS_PER_UNIT - 1) / BITS_PER_UNIT;

  return base;
}

/* Returns true if a region of size SIZE1 at position 0 and a region of
   size SIZE2 at position DIFF cannot overlap.  */

bool
aff_comb_cannot_overlap_p (aff_tree *diff, const widest_int &size1,
			   const widest_int &size2)
{
  /* Unless the difference is a constant, we fail.  */
  if (diff->n != 0)
    return false;

  if (wi::neg_p (diff->offset))
    {
      /* The second object is before the first one, we succeed if the last
	 element of the second object is before the start of the first one.  */
      return wi::neg_p (diff->offset + size2 - 1);
    }
  else
    {
      /* We succeed if the second object starts after the first one ends.  */
      return wi::les_p (size1, diff->offset);
    }
}

