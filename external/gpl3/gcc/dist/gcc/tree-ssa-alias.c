/* Alias analysis for trees.
   Copyright (C) 2004-2013 Free Software Foundation, Inc.
   Contributed by Diego Novillo <dnovillo@redhat.com>

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
#include "tree.h"
#include "tm_p.h"
#include "target.h"
#include "basic-block.h"
#include "timevar.h"	/* for TV_ALIAS_STMT_WALK */
#include "ggc.h"
#include "langhooks.h"
#include "flags.h"
#include "function.h"
#include "tree-pretty-print.h"
#include "dumpfile.h"
#include "gimple.h"
#include "tree-flow.h"
#include "tree-inline.h"
#include "params.h"
#include "vec.h"
#include "bitmap.h"
#include "pointer-set.h"
#include "alloc-pool.h"
#include "tree-ssa-alias.h"

/* Broad overview of how alias analysis on gimple works:

   Statements clobbering or using memory are linked through the
   virtual operand factored use-def chain.  The virtual operand
   is unique per function, its symbol is accessible via gimple_vop (cfun).
   Virtual operands are used for efficiently walking memory statements
   in the gimple IL and are useful for things like value-numbering as
   a generation count for memory references.

   SSA_NAME pointers may have associated points-to information
   accessible via the SSA_NAME_PTR_INFO macro.  Flow-insensitive
   points-to information is (re-)computed by the TODO_rebuild_alias
   pass manager todo.  Points-to information is also used for more
   precise tracking of call-clobbered and call-used variables and
   related disambiguations.

   This file contains functions for disambiguating memory references,
   the so called alias-oracle and tools for walking of the gimple IL.

   The main alias-oracle entry-points are

   bool stmt_may_clobber_ref_p (gimple, tree)

     This function queries if a statement may invalidate (parts of)
     the memory designated by the reference tree argument.

   bool ref_maybe_used_by_stmt_p (gimple, tree)

     This function queries if a statement may need (parts of) the
     memory designated by the reference tree argument.

   There are variants of these functions that only handle the call
   part of a statement, call_may_clobber_ref_p and ref_maybe_used_by_call_p.
   Note that these do not disambiguate against a possible call lhs.

   bool refs_may_alias_p (tree, tree)

     This function tries to disambiguate two reference trees.

   bool ptr_deref_may_alias_global_p (tree)

     This function queries if dereferencing a pointer variable may
     alias global memory.

   More low-level disambiguators are available and documented in
   this file.  Low-level disambiguators dealing with points-to
   information are in tree-ssa-structalias.c.  */


/* Query statistics for the different low-level disambiguators.
   A high-level query may trigger multiple of them.  */

static struct {
  unsigned HOST_WIDE_INT refs_may_alias_p_may_alias;
  unsigned HOST_WIDE_INT refs_may_alias_p_no_alias;
  unsigned HOST_WIDE_INT ref_maybe_used_by_call_p_may_alias;
  unsigned HOST_WIDE_INT ref_maybe_used_by_call_p_no_alias;
  unsigned HOST_WIDE_INT call_may_clobber_ref_p_may_alias;
  unsigned HOST_WIDE_INT call_may_clobber_ref_p_no_alias;
} alias_stats;

void
dump_alias_stats (FILE *s)
{
  fprintf (s, "\nAlias oracle query stats:\n");
  fprintf (s, "  refs_may_alias_p: "
	   HOST_WIDE_INT_PRINT_DEC" disambiguations, "
	   HOST_WIDE_INT_PRINT_DEC" queries\n",
	   alias_stats.refs_may_alias_p_no_alias,
	   alias_stats.refs_may_alias_p_no_alias
	   + alias_stats.refs_may_alias_p_may_alias);
  fprintf (s, "  ref_maybe_used_by_call_p: "
	   HOST_WIDE_INT_PRINT_DEC" disambiguations, "
	   HOST_WIDE_INT_PRINT_DEC" queries\n",
	   alias_stats.ref_maybe_used_by_call_p_no_alias,
	   alias_stats.refs_may_alias_p_no_alias
	   + alias_stats.ref_maybe_used_by_call_p_may_alias);
  fprintf (s, "  call_may_clobber_ref_p: "
	   HOST_WIDE_INT_PRINT_DEC" disambiguations, "
	   HOST_WIDE_INT_PRINT_DEC" queries\n",
	   alias_stats.call_may_clobber_ref_p_no_alias,
	   alias_stats.call_may_clobber_ref_p_no_alias
	   + alias_stats.call_may_clobber_ref_p_may_alias);
}


/* Return true, if dereferencing PTR may alias with a global variable.  */

bool
ptr_deref_may_alias_global_p (tree ptr)
{
  struct ptr_info_def *pi;

  /* If we end up with a pointer constant here that may point
     to global memory.  */
  if (TREE_CODE (ptr) != SSA_NAME)
    return true;

  pi = SSA_NAME_PTR_INFO (ptr);

  /* If we do not have points-to information for this variable,
     we have to punt.  */
  if (!pi)
    return true;

  /* ???  This does not use TBAA to prune globals ptr may not access.  */
  return pt_solution_includes_global (&pi->pt);
}

/* Return true if dereferencing PTR may alias DECL.
   The caller is responsible for applying TBAA to see if PTR
   may access DECL at all.  */

static bool
ptr_deref_may_alias_decl_p (tree ptr, tree decl)
{
  struct ptr_info_def *pi;

  /* Conversions are irrelevant for points-to information and
     data-dependence analysis can feed us those.  */
  STRIP_NOPS (ptr);

  /* Anything we do not explicilty handle aliases.  */
  if ((TREE_CODE (ptr) != SSA_NAME
       && TREE_CODE (ptr) != ADDR_EXPR
       && TREE_CODE (ptr) != POINTER_PLUS_EXPR)
      || !POINTER_TYPE_P (TREE_TYPE (ptr))
      || (TREE_CODE (decl) != VAR_DECL
	  && TREE_CODE (decl) != PARM_DECL
	  && TREE_CODE (decl) != RESULT_DECL))
    return true;

  /* Disregard pointer offsetting.  */
  if (TREE_CODE (ptr) == POINTER_PLUS_EXPR)
    {
      do
	{
	  ptr = TREE_OPERAND (ptr, 0);
	}
      while (TREE_CODE (ptr) == POINTER_PLUS_EXPR);
      return ptr_deref_may_alias_decl_p (ptr, decl);
    }

  /* ADDR_EXPR pointers either just offset another pointer or directly
     specify the pointed-to set.  */
  if (TREE_CODE (ptr) == ADDR_EXPR)
    {
      tree base = get_base_address (TREE_OPERAND (ptr, 0));
      if (base
	  && (TREE_CODE (base) == MEM_REF
	      || TREE_CODE (base) == TARGET_MEM_REF))
	ptr = TREE_OPERAND (base, 0);
      else if (base
	       && DECL_P (base))
	return base == decl;
      else if (base
	       && CONSTANT_CLASS_P (base))
	return false;
      else
	return true;
    }

  /* Non-aliased variables can not be pointed to.  */
  if (!may_be_aliased (decl))
    return false;

  /* If we do not have useful points-to information for this pointer
     we cannot disambiguate anything else.  */
  pi = SSA_NAME_PTR_INFO (ptr);
  if (!pi)
    return true;

  return pt_solution_includes (&pi->pt, decl);
}

/* Return true if dereferenced PTR1 and PTR2 may alias.
   The caller is responsible for applying TBAA to see if accesses
   through PTR1 and PTR2 may conflict at all.  */

bool
ptr_derefs_may_alias_p (tree ptr1, tree ptr2)
{
  struct ptr_info_def *pi1, *pi2;

  /* Conversions are irrelevant for points-to information and
     data-dependence analysis can feed us those.  */
  STRIP_NOPS (ptr1);
  STRIP_NOPS (ptr2);

  /* Disregard pointer offsetting.  */
  if (TREE_CODE (ptr1) == POINTER_PLUS_EXPR)
    {
      do
	{
	  ptr1 = TREE_OPERAND (ptr1, 0);
	}
      while (TREE_CODE (ptr1) == POINTER_PLUS_EXPR);
      return ptr_derefs_may_alias_p (ptr1, ptr2);
    }
  if (TREE_CODE (ptr2) == POINTER_PLUS_EXPR)
    {
      do
	{
	  ptr2 = TREE_OPERAND (ptr2, 0);
	}
      while (TREE_CODE (ptr2) == POINTER_PLUS_EXPR);
      return ptr_derefs_may_alias_p (ptr1, ptr2);
    }

  /* ADDR_EXPR pointers either just offset another pointer or directly
     specify the pointed-to set.  */
  if (TREE_CODE (ptr1) == ADDR_EXPR)
    {
      tree base = get_base_address (TREE_OPERAND (ptr1, 0));
      if (base
	  && (TREE_CODE (base) == MEM_REF
	      || TREE_CODE (base) == TARGET_MEM_REF))
	return ptr_derefs_may_alias_p (TREE_OPERAND (base, 0), ptr2);
      else if (base
	       && DECL_P (base))
	return ptr_deref_may_alias_decl_p (ptr2, base);
      else
	return true;
    }
  if (TREE_CODE (ptr2) == ADDR_EXPR)
    {
      tree base = get_base_address (TREE_OPERAND (ptr2, 0));
      if (base
	  && (TREE_CODE (base) == MEM_REF
	      || TREE_CODE (base) == TARGET_MEM_REF))
	return ptr_derefs_may_alias_p (ptr1, TREE_OPERAND (base, 0));
      else if (base
	       && DECL_P (base))
	return ptr_deref_may_alias_decl_p (ptr1, base);
      else
	return true;
    }

  /* From here we require SSA name pointers.  Anything else aliases.  */
  if (TREE_CODE (ptr1) != SSA_NAME
      || TREE_CODE (ptr2) != SSA_NAME
      || !POINTER_TYPE_P (TREE_TYPE (ptr1))
      || !POINTER_TYPE_P (TREE_TYPE (ptr2)))
    return true;

  /* We may end up with two empty points-to solutions for two same pointers.
     In this case we still want to say both pointers alias, so shortcut
     that here.  */
  if (ptr1 == ptr2)
    return true;

  /* If we do not have useful points-to information for either pointer
     we cannot disambiguate anything else.  */
  pi1 = SSA_NAME_PTR_INFO (ptr1);
  pi2 = SSA_NAME_PTR_INFO (ptr2);
  if (!pi1 || !pi2)
    return true;

  /* ???  This does not use TBAA to prune decls from the intersection
     that not both pointers may access.  */
  return pt_solutions_intersect (&pi1->pt, &pi2->pt);
}

/* Return true if dereferencing PTR may alias *REF.
   The caller is responsible for applying TBAA to see if PTR
   may access *REF at all.  */

static bool
ptr_deref_may_alias_ref_p_1 (tree ptr, ao_ref *ref)
{
  tree base = ao_ref_base (ref);

  if (TREE_CODE (base) == MEM_REF
      || TREE_CODE (base) == TARGET_MEM_REF)
    return ptr_derefs_may_alias_p (ptr, TREE_OPERAND (base, 0));
  else if (DECL_P (base))
    return ptr_deref_may_alias_decl_p (ptr, base);

  return true;
}

/* Return true whether REF may refer to global memory.  */

bool
ref_may_alias_global_p (tree ref)
{
  tree base = get_base_address (ref);
  if (DECL_P (base))
    return is_global_var (base);
  else if (TREE_CODE (base) == MEM_REF
	   || TREE_CODE (base) == TARGET_MEM_REF)
    return ptr_deref_may_alias_global_p (TREE_OPERAND (base, 0));
  return true;
}

/* Return true whether STMT may clobber global memory.  */

bool
stmt_may_clobber_global_p (gimple stmt)
{
  tree lhs;

  if (!gimple_vdef (stmt))
    return false;

  /* ???  We can ask the oracle whether an artificial pointer
     dereference with a pointer with points-to information covering
     all global memory (what about non-address taken memory?) maybe
     clobbered by this call.  As there is at the moment no convenient
     way of doing that without generating garbage do some manual
     checking instead.
     ???  We could make a NULL ao_ref argument to the various
     predicates special, meaning any global memory.  */

  switch (gimple_code (stmt))
    {
    case GIMPLE_ASSIGN:
      lhs = gimple_assign_lhs (stmt);
      return (TREE_CODE (lhs) != SSA_NAME
	      && ref_may_alias_global_p (lhs));
    case GIMPLE_CALL:
      return true;
    default:
      return true;
    }
}


/* Dump alias information on FILE.  */

void
dump_alias_info (FILE *file)
{
  unsigned i;
  const char *funcname
    = lang_hooks.decl_printable_name (current_function_decl, 2);
  tree var;

  fprintf (file, "\n\nAlias information for %s\n\n", funcname);

  fprintf (file, "Aliased symbols\n\n");

  FOR_EACH_LOCAL_DECL (cfun, i, var)
    {
      if (may_be_aliased (var))
	dump_variable (file, var);
    }

  fprintf (file, "\nCall clobber information\n");

  fprintf (file, "\nESCAPED");
  dump_points_to_solution (file, &cfun->gimple_df->escaped);

  fprintf (file, "\n\nFlow-insensitive points-to information\n\n");

  for (i = 1; i < num_ssa_names; i++)
    {
      tree ptr = ssa_name (i);
      struct ptr_info_def *pi;

      if (ptr == NULL_TREE
	  || SSA_NAME_IN_FREE_LIST (ptr))
	continue;

      pi = SSA_NAME_PTR_INFO (ptr);
      if (pi)
	dump_points_to_info_for (file, ptr);
    }

  fprintf (file, "\n");
}


/* Dump alias information on stderr.  */

DEBUG_FUNCTION void
debug_alias_info (void)
{
  dump_alias_info (stderr);
}


/* Dump the points-to set *PT into FILE.  */

void
dump_points_to_solution (FILE *file, struct pt_solution *pt)
{
  if (pt->anything)
    fprintf (file, ", points-to anything");

  if (pt->nonlocal)
    fprintf (file, ", points-to non-local");

  if (pt->escaped)
    fprintf (file, ", points-to escaped");

  if (pt->ipa_escaped)
    fprintf (file, ", points-to unit escaped");

  if (pt->null)
    fprintf (file, ", points-to NULL");

  if (pt->vars)
    {
      fprintf (file, ", points-to vars: ");
      dump_decl_set (file, pt->vars);
      if (pt->vars_contains_global)
	fprintf (file, " (includes global vars)");
    }
}

/* Dump points-to information for SSA_NAME PTR into FILE.  */

void
dump_points_to_info_for (FILE *file, tree ptr)
{
  struct ptr_info_def *pi = SSA_NAME_PTR_INFO (ptr);

  print_generic_expr (file, ptr, dump_flags);

  if (pi)
    dump_points_to_solution (file, &pi->pt);
  else
    fprintf (file, ", points-to anything");

  fprintf (file, "\n");
}


/* Dump points-to information for VAR into stderr.  */

DEBUG_FUNCTION void
debug_points_to_info_for (tree var)
{
  dump_points_to_info_for (stderr, var);
}


/* Initializes the alias-oracle reference representation *R from REF.  */

void
ao_ref_init (ao_ref *r, tree ref)
{
  r->ref = ref;
  r->base = NULL_TREE;
  r->offset = 0;
  r->size = -1;
  r->max_size = -1;
  r->ref_alias_set = -1;
  r->base_alias_set = -1;
  r->volatile_p = ref ? TREE_THIS_VOLATILE (ref) : false;
}

/* Returns the base object of the memory reference *REF.  */

tree
ao_ref_base (ao_ref *ref)
{
  if (ref->base)
    return ref->base;
  ref->base = get_ref_base_and_extent (ref->ref, &ref->offset, &ref->size,
				       &ref->max_size);
  return ref->base;
}

/* Returns the base object alias set of the memory reference *REF.  */

static alias_set_type
ao_ref_base_alias_set (ao_ref *ref)
{
  tree base_ref;
  if (ref->base_alias_set != -1)
    return ref->base_alias_set;
  if (!ref->ref)
    return 0;
  base_ref = ref->ref;
  while (handled_component_p (base_ref))
    base_ref = TREE_OPERAND (base_ref, 0);
  ref->base_alias_set = get_alias_set (base_ref);
  return ref->base_alias_set;
}

/* Returns the reference alias set of the memory reference *REF.  */

alias_set_type
ao_ref_alias_set (ao_ref *ref)
{
  if (ref->ref_alias_set != -1)
    return ref->ref_alias_set;
  ref->ref_alias_set = get_alias_set (ref->ref);
  return ref->ref_alias_set;
}

/* Init an alias-oracle reference representation from a gimple pointer
   PTR and a gimple size SIZE in bytes.  If SIZE is NULL_TREE the the
   size is assumed to be unknown.  The access is assumed to be only
   to or after of the pointer target, not before it.  */

void
ao_ref_init_from_ptr_and_size (ao_ref *ref, tree ptr, tree size)
{
  HOST_WIDE_INT t1, t2;
  ref->ref = NULL_TREE;
  if (TREE_CODE (ptr) == ADDR_EXPR)
    ref->base = get_ref_base_and_extent (TREE_OPERAND (ptr, 0),
					 &ref->offset, &t1, &t2);
  else
    {
      ref->base = build2 (MEM_REF, char_type_node,
			  ptr, null_pointer_node);
      ref->offset = 0;
    }
  if (size
      && host_integerp (size, 0)
      && TREE_INT_CST_LOW (size) * 8 / 8 == TREE_INT_CST_LOW (size))
    ref->max_size = ref->size = TREE_INT_CST_LOW (size) * 8;
  else
    ref->max_size = ref->size = -1;
  ref->ref_alias_set = 0;
  ref->base_alias_set = 0;
  ref->volatile_p = false;
}

/* Return 1 if TYPE1 and TYPE2 are to be considered equivalent for the
   purpose of TBAA.  Return 0 if they are distinct and -1 if we cannot
   decide.  */

static inline int
same_type_for_tbaa (tree type1, tree type2)
{
  type1 = TYPE_MAIN_VARIANT (type1);
  type2 = TYPE_MAIN_VARIANT (type2);

  /* If we would have to do structural comparison bail out.  */
  if (TYPE_STRUCTURAL_EQUALITY_P (type1)
      || TYPE_STRUCTURAL_EQUALITY_P (type2))
    return -1;

  /* Compare the canonical types.  */
  if (TYPE_CANONICAL (type1) == TYPE_CANONICAL (type2))
    return 1;

  /* ??? Array types are not properly unified in all cases as we have
     spurious changes in the index types for example.  Removing this
     causes all sorts of problems with the Fortran frontend.  */
  if (TREE_CODE (type1) == ARRAY_TYPE
      && TREE_CODE (type2) == ARRAY_TYPE)
    return -1;

  /* ??? In Ada, an lvalue of an unconstrained type can be used to access an
     object of one of its constrained subtypes, e.g. when a function with an
     unconstrained parameter passed by reference is called on an object and
     inlined.  But, even in the case of a fixed size, type and subtypes are
     not equivalent enough as to share the same TYPE_CANONICAL, since this
     would mean that conversions between them are useless, whereas they are
     not (e.g. type and subtypes can have different modes).  So, in the end,
     they are only guaranteed to have the same alias set.  */
  if (get_alias_set (type1) == get_alias_set (type2))
    return -1;

  /* The types are known to be not equal.  */
  return 0;
}

/* Determine if the two component references REF1 and REF2 which are
   based on access types TYPE1 and TYPE2 and of which at least one is based
   on an indirect reference may alias.  REF2 is the only one that can
   be a decl in which case REF2_IS_DECL is true.
   REF1_ALIAS_SET, BASE1_ALIAS_SET, REF2_ALIAS_SET and BASE2_ALIAS_SET
   are the respective alias sets.  */

static bool
aliasing_component_refs_p (tree ref1,
			   alias_set_type ref1_alias_set,
			   alias_set_type base1_alias_set,
			   HOST_WIDE_INT offset1, HOST_WIDE_INT max_size1,
			   tree ref2,
			   alias_set_type ref2_alias_set,
			   alias_set_type base2_alias_set,
			   HOST_WIDE_INT offset2, HOST_WIDE_INT max_size2,
			   bool ref2_is_decl)
{
  /* If one reference is a component references through pointers try to find a
     common base and apply offset based disambiguation.  This handles
     for example
       struct A { int i; int j; } *q;
       struct B { struct A a; int k; } *p;
     disambiguating q->i and p->a.j.  */
  tree base1, base2;
  tree type1, type2;
  tree *refp;
  int same_p;

  /* Choose bases and base types to search for.  */
  base1 = ref1;
  while (handled_component_p (base1))
    base1 = TREE_OPERAND (base1, 0);
  type1 = TREE_TYPE (base1);
  base2 = ref2;
  while (handled_component_p (base2))
    base2 = TREE_OPERAND (base2, 0);
  type2 = TREE_TYPE (base2);

  /* Now search for the type1 in the access path of ref2.  This
     would be a common base for doing offset based disambiguation on.  */
  refp = &ref2;
  while (handled_component_p (*refp)
	 && same_type_for_tbaa (TREE_TYPE (*refp), type1) == 0)
    refp = &TREE_OPERAND (*refp, 0);
  same_p = same_type_for_tbaa (TREE_TYPE (*refp), type1);
  /* If we couldn't compare types we have to bail out.  */
  if (same_p == -1)
    return true;
  else if (same_p == 1)
    {
      HOST_WIDE_INT offadj, sztmp, msztmp;
      get_ref_base_and_extent (*refp, &offadj, &sztmp, &msztmp);
      offset2 -= offadj;
      get_ref_base_and_extent (base1, &offadj, &sztmp, &msztmp);
      offset1 -= offadj;
      return ranges_overlap_p (offset1, max_size1, offset2, max_size2);
    }
  /* If we didn't find a common base, try the other way around.  */
  refp = &ref1;
  while (handled_component_p (*refp)
	 && same_type_for_tbaa (TREE_TYPE (*refp), type2) == 0)
    refp = &TREE_OPERAND (*refp, 0);
  same_p = same_type_for_tbaa (TREE_TYPE (*refp), type2);
  /* If we couldn't compare types we have to bail out.  */
  if (same_p == -1)
    return true;
  else if (same_p == 1)
    {
      HOST_WIDE_INT offadj, sztmp, msztmp;
      get_ref_base_and_extent (*refp, &offadj, &sztmp, &msztmp);
      offset1 -= offadj;
      get_ref_base_and_extent (base2, &offadj, &sztmp, &msztmp);
      offset2 -= offadj;
      return ranges_overlap_p (offset1, max_size1, offset2, max_size2);
    }

  /* If we have two type access paths B1.path1 and B2.path2 they may
     only alias if either B1 is in B2.path2 or B2 is in B1.path1.
     But we can still have a path that goes B1.path1...B2.path2 with
     a part that we do not see.  So we can only disambiguate now
     if there is no B2 in the tail of path1 and no B1 on the
     tail of path2.  */
  if (base1_alias_set == ref2_alias_set
      || alias_set_subset_of (base1_alias_set, ref2_alias_set))
    return true;
  /* If this is ptr vs. decl then we know there is no ptr ... decl path.  */
  if (!ref2_is_decl)
    return (base2_alias_set == ref1_alias_set
	    || alias_set_subset_of (base2_alias_set, ref1_alias_set));
  return false;
}

/* Return true if two memory references based on the variables BASE1
   and BASE2 constrained to [OFFSET1, OFFSET1 + MAX_SIZE1) and
   [OFFSET2, OFFSET2 + MAX_SIZE2) may alias.  */

static bool
decl_refs_may_alias_p (tree base1,
		       HOST_WIDE_INT offset1, HOST_WIDE_INT max_size1,
		       tree base2,
		       HOST_WIDE_INT offset2, HOST_WIDE_INT max_size2)
{
  gcc_checking_assert (DECL_P (base1) && DECL_P (base2));

  /* If both references are based on different variables, they cannot alias.  */
  if (base1 != base2)
    return false;

  /* If both references are based on the same variable, they cannot alias if
     the accesses do not overlap.  */
  return ranges_overlap_p (offset1, max_size1, offset2, max_size2);
}

/* Return true if an indirect reference based on *PTR1 constrained
   to [OFFSET1, OFFSET1 + MAX_SIZE1) may alias a variable based on BASE2
   constrained to [OFFSET2, OFFSET2 + MAX_SIZE2).  *PTR1 and BASE2 have
   the alias sets BASE1_ALIAS_SET and BASE2_ALIAS_SET which can be -1
   in which case they are computed on-demand.  REF1 and REF2
   if non-NULL are the complete memory reference trees.  */

static bool
indirect_ref_may_alias_decl_p (tree ref1 ATTRIBUTE_UNUSED, tree base1,
			       HOST_WIDE_INT offset1,
			       HOST_WIDE_INT max_size1 ATTRIBUTE_UNUSED,
			       alias_set_type ref1_alias_set,
			       alias_set_type base1_alias_set,
			       tree ref2 ATTRIBUTE_UNUSED, tree base2,
			       HOST_WIDE_INT offset2, HOST_WIDE_INT max_size2,
			       alias_set_type ref2_alias_set,
			       alias_set_type base2_alias_set, bool tbaa_p)
{
  tree ptr1;
  tree ptrtype1, dbase2;
  HOST_WIDE_INT offset1p = offset1, offset2p = offset2;
  HOST_WIDE_INT doffset1, doffset2;
  double_int moff;

  gcc_checking_assert ((TREE_CODE (base1) == MEM_REF
			|| TREE_CODE (base1) == TARGET_MEM_REF)
		       && DECL_P (base2));

  ptr1 = TREE_OPERAND (base1, 0);

  /* The offset embedded in MEM_REFs can be negative.  Bias them
     so that the resulting offset adjustment is positive.  */
  moff = mem_ref_offset (base1);
  moff = moff.alshift (BITS_PER_UNIT == 8
		       ? 3 : exact_log2 (BITS_PER_UNIT),
		       HOST_BITS_PER_DOUBLE_INT);
  if (moff.is_negative ())
    offset2p += (-moff).low;
  else
    offset1p += moff.low;

  /* If only one reference is based on a variable, they cannot alias if
     the pointer access is beyond the extent of the variable access.
     (the pointer base cannot validly point to an offset less than zero
     of the variable).
     ???  IVOPTs creates bases that do not honor this restriction,
     so do not apply this optimization for TARGET_MEM_REFs.  */
  if (TREE_CODE (base1) != TARGET_MEM_REF
      && !ranges_overlap_p (MAX (0, offset1p), -1, offset2p, max_size2))
    return false;
  /* They also cannot alias if the pointer may not point to the decl.  */
  if (!ptr_deref_may_alias_decl_p (ptr1, base2))
    return false;

  /* Disambiguations that rely on strict aliasing rules follow.  */
  if (!flag_strict_aliasing || !tbaa_p)
    return true;

  ptrtype1 = TREE_TYPE (TREE_OPERAND (base1, 1));

  /* If the alias set for a pointer access is zero all bets are off.  */
  if (base1_alias_set == -1)
    base1_alias_set = get_deref_alias_set (ptrtype1);
  if (base1_alias_set == 0)
    return true;
  if (base2_alias_set == -1)
    base2_alias_set = get_alias_set (base2);

  /* When we are trying to disambiguate an access with a pointer dereference
     as base versus one with a decl as base we can use both the size
     of the decl and its dynamic type for extra disambiguation.
     ???  We do not know anything about the dynamic type of the decl
     other than that its alias-set contains base2_alias_set as a subset
     which does not help us here.  */
  /* As we know nothing useful about the dynamic type of the decl just
     use the usual conflict check rather than a subset test.
     ???  We could introduce -fvery-strict-aliasing when the language
     does not allow decls to have a dynamic type that differs from their
     static type.  Then we can check 
     !alias_set_subset_of (base1_alias_set, base2_alias_set) instead.  */
  if (base1_alias_set != base2_alias_set
      && !alias_sets_conflict_p (base1_alias_set, base2_alias_set))
    return false;
  /* If the size of the access relevant for TBAA through the pointer
     is bigger than the size of the decl we can't possibly access the
     decl via that pointer.  */
  if (DECL_SIZE (base2) && COMPLETE_TYPE_P (TREE_TYPE (ptrtype1))
      && TREE_CODE (DECL_SIZE (base2)) == INTEGER_CST
      && TREE_CODE (TYPE_SIZE (TREE_TYPE (ptrtype1))) == INTEGER_CST
      /* ???  This in turn may run afoul when a decl of type T which is
	 a member of union type U is accessed through a pointer to
	 type U and sizeof T is smaller than sizeof U.  */
      && TREE_CODE (TREE_TYPE (ptrtype1)) != UNION_TYPE
      && TREE_CODE (TREE_TYPE (ptrtype1)) != QUAL_UNION_TYPE
      && tree_int_cst_lt (DECL_SIZE (base2), TYPE_SIZE (TREE_TYPE (ptrtype1))))
    return false;

  if (!ref2)
    return true;

  /* If the decl is accessed via a MEM_REF, reconstruct the base
     we can use for TBAA and an appropriately adjusted offset.  */
  dbase2 = ref2;
  while (handled_component_p (dbase2))
    dbase2 = TREE_OPERAND (dbase2, 0);
  doffset1 = offset1;
  doffset2 = offset2;
  if (TREE_CODE (dbase2) == MEM_REF
      || TREE_CODE (dbase2) == TARGET_MEM_REF)
    {
      double_int moff = mem_ref_offset (dbase2);
      moff = moff.alshift (BITS_PER_UNIT == 8
			   ? 3 : exact_log2 (BITS_PER_UNIT),
			   HOST_BITS_PER_DOUBLE_INT);
      if (moff.is_negative ())
	doffset1 -= (-moff).low;
      else
	doffset2 -= moff.low;
    }

  /* If either reference is view-converted, give up now.  */
  if (same_type_for_tbaa (TREE_TYPE (base1), TREE_TYPE (ptrtype1)) != 1
      || same_type_for_tbaa (TREE_TYPE (dbase2), TREE_TYPE (base2)) != 1)
    return true;

  /* If both references are through the same type, they do not alias
     if the accesses do not overlap.  This does extra disambiguation
     for mixed/pointer accesses but requires strict aliasing.
     For MEM_REFs we require that the component-ref offset we computed
     is relative to the start of the type which we ensure by
     comparing rvalue and access type and disregarding the constant
     pointer offset.  */
  if ((TREE_CODE (base1) != TARGET_MEM_REF
       || (!TMR_INDEX (base1) && !TMR_INDEX2 (base1)))
      && same_type_for_tbaa (TREE_TYPE (base1), TREE_TYPE (dbase2)) == 1)
    return ranges_overlap_p (doffset1, max_size1, doffset2, max_size2);

  /* Do access-path based disambiguation.  */
  if (ref1 && ref2
      && (handled_component_p (ref1) || handled_component_p (ref2)))
    return aliasing_component_refs_p (ref1,
				      ref1_alias_set, base1_alias_set,
				      offset1, max_size1,
				      ref2,
				      ref2_alias_set, base2_alias_set,
				      offset2, max_size2, true);

  return true;
}

/* Return true if two indirect references based on *PTR1
   and *PTR2 constrained to [OFFSET1, OFFSET1 + MAX_SIZE1) and
   [OFFSET2, OFFSET2 + MAX_SIZE2) may alias.  *PTR1 and *PTR2 have
   the alias sets BASE1_ALIAS_SET and BASE2_ALIAS_SET which can be -1
   in which case they are computed on-demand.  REF1 and REF2
   if non-NULL are the complete memory reference trees. */

static bool
indirect_refs_may_alias_p (tree ref1 ATTRIBUTE_UNUSED, tree base1,
			   HOST_WIDE_INT offset1, HOST_WIDE_INT max_size1,
			   alias_set_type ref1_alias_set,
			   alias_set_type base1_alias_set,
			   tree ref2 ATTRIBUTE_UNUSED, tree base2,
			   HOST_WIDE_INT offset2, HOST_WIDE_INT max_size2,
			   alias_set_type ref2_alias_set,
			   alias_set_type base2_alias_set, bool tbaa_p)
{
  tree ptr1;
  tree ptr2;
  tree ptrtype1, ptrtype2;

  gcc_checking_assert ((TREE_CODE (base1) == MEM_REF
			|| TREE_CODE (base1) == TARGET_MEM_REF)
		       && (TREE_CODE (base2) == MEM_REF
			   || TREE_CODE (base2) == TARGET_MEM_REF));

  ptr1 = TREE_OPERAND (base1, 0);
  ptr2 = TREE_OPERAND (base2, 0);

  /* If both bases are based on pointers they cannot alias if they may not
     point to the same memory object or if they point to the same object
     and the accesses do not overlap.  */
  if ((!cfun || gimple_in_ssa_p (cfun))
      && operand_equal_p (ptr1, ptr2, 0)
      && (((TREE_CODE (base1) != TARGET_MEM_REF
	    || (!TMR_INDEX (base1) && !TMR_INDEX2 (base1)))
	   && (TREE_CODE (base2) != TARGET_MEM_REF
	       || (!TMR_INDEX (base2) && !TMR_INDEX2 (base2))))
	  || (TREE_CODE (base1) == TARGET_MEM_REF
	      && TREE_CODE (base2) == TARGET_MEM_REF
	      && (TMR_STEP (base1) == TMR_STEP (base2)
		  || (TMR_STEP (base1) && TMR_STEP (base2)
		      && operand_equal_p (TMR_STEP (base1),
					  TMR_STEP (base2), 0)))
	      && (TMR_INDEX (base1) == TMR_INDEX (base2)
		  || (TMR_INDEX (base1) && TMR_INDEX (base2)
		      && operand_equal_p (TMR_INDEX (base1),
					  TMR_INDEX (base2), 0)))
	      && (TMR_INDEX2 (base1) == TMR_INDEX2 (base2)
		  || (TMR_INDEX2 (base1) && TMR_INDEX2 (base2)
		      && operand_equal_p (TMR_INDEX2 (base1),
					  TMR_INDEX2 (base2), 0))))))
    {
      double_int moff;
      /* The offset embedded in MEM_REFs can be negative.  Bias them
	 so that the resulting offset adjustment is positive.  */
      moff = mem_ref_offset (base1);
      moff = moff.alshift (BITS_PER_UNIT == 8
			   ? 3 : exact_log2 (BITS_PER_UNIT),
			   HOST_BITS_PER_DOUBLE_INT);
      if (moff.is_negative ())
	offset2 += (-moff).low;
      else
	offset1 += moff.low;
      moff = mem_ref_offset (base2);
      moff = moff.alshift (BITS_PER_UNIT == 8
			   ? 3 : exact_log2 (BITS_PER_UNIT),
			   HOST_BITS_PER_DOUBLE_INT);
      if (moff.is_negative ())
	offset1 += (-moff).low;
      else
	offset2 += moff.low;
      return ranges_overlap_p (offset1, max_size1, offset2, max_size2);
    }
  if (!ptr_derefs_may_alias_p (ptr1, ptr2))
    return false;

  /* Disambiguations that rely on strict aliasing rules follow.  */
  if (!flag_strict_aliasing || !tbaa_p)
    return true;

  ptrtype1 = TREE_TYPE (TREE_OPERAND (base1, 1));
  ptrtype2 = TREE_TYPE (TREE_OPERAND (base2, 1));

  /* If the alias set for a pointer access is zero all bets are off.  */
  if (base1_alias_set == -1)
    base1_alias_set = get_deref_alias_set (ptrtype1);
  if (base1_alias_set == 0)
    return true;
  if (base2_alias_set == -1)
    base2_alias_set = get_deref_alias_set (ptrtype2);
  if (base2_alias_set == 0)
    return true;

  /* If both references are through the same type, they do not alias
     if the accesses do not overlap.  This does extra disambiguation
     for mixed/pointer accesses but requires strict aliasing.  */
  if ((TREE_CODE (base1) != TARGET_MEM_REF
       || (!TMR_INDEX (base1) && !TMR_INDEX2 (base1)))
      && (TREE_CODE (base2) != TARGET_MEM_REF
	  || (!TMR_INDEX (base2) && !TMR_INDEX2 (base2)))
      && same_type_for_tbaa (TREE_TYPE (base1), TREE_TYPE (ptrtype1)) == 1
      && same_type_for_tbaa (TREE_TYPE (base2), TREE_TYPE (ptrtype2)) == 1
      && same_type_for_tbaa (TREE_TYPE (ptrtype1),
			     TREE_TYPE (ptrtype2)) == 1)
    return ranges_overlap_p (offset1, max_size1, offset2, max_size2);

  /* Do type-based disambiguation.  */
  if (base1_alias_set != base2_alias_set
      && !alias_sets_conflict_p (base1_alias_set, base2_alias_set))
    return false;

  /* Do access-path based disambiguation.  */
  if (ref1 && ref2
      && (handled_component_p (ref1) || handled_component_p (ref2))
      && same_type_for_tbaa (TREE_TYPE (base1), TREE_TYPE (ptrtype1)) == 1
      && same_type_for_tbaa (TREE_TYPE (base2), TREE_TYPE (ptrtype2)) == 1)
    return aliasing_component_refs_p (ref1,
				      ref1_alias_set, base1_alias_set,
				      offset1, max_size1,
				      ref2,
				      ref2_alias_set, base2_alias_set,
				      offset2, max_size2, false);

  return true;
}

/* Return true, if the two memory references REF1 and REF2 may alias.  */

bool
refs_may_alias_p_1 (ao_ref *ref1, ao_ref *ref2, bool tbaa_p)
{
  tree base1, base2;
  HOST_WIDE_INT offset1 = 0, offset2 = 0;
  HOST_WIDE_INT max_size1 = -1, max_size2 = -1;
  bool var1_p, var2_p, ind1_p, ind2_p;

  gcc_checking_assert ((!ref1->ref
			|| TREE_CODE (ref1->ref) == SSA_NAME
			|| DECL_P (ref1->ref)
			|| TREE_CODE (ref1->ref) == STRING_CST
			|| handled_component_p (ref1->ref)
			|| TREE_CODE (ref1->ref) == MEM_REF
			|| TREE_CODE (ref1->ref) == TARGET_MEM_REF)
		       && (!ref2->ref
			   || TREE_CODE (ref2->ref) == SSA_NAME
			   || DECL_P (ref2->ref)
			   || TREE_CODE (ref2->ref) == STRING_CST
			   || handled_component_p (ref2->ref)
			   || TREE_CODE (ref2->ref) == MEM_REF
			   || TREE_CODE (ref2->ref) == TARGET_MEM_REF));

  /* Decompose the references into their base objects and the access.  */
  base1 = ao_ref_base (ref1);
  offset1 = ref1->offset;
  max_size1 = ref1->max_size;
  base2 = ao_ref_base (ref2);
  offset2 = ref2->offset;
  max_size2 = ref2->max_size;

  /* We can end up with registers or constants as bases for example from
     *D.1663_44 = VIEW_CONVERT_EXPR<struct DB_LSN>(__tmp$B0F64_59);
     which is seen as a struct copy.  */
  if (TREE_CODE (base1) == SSA_NAME
      || TREE_CODE (base1) == CONST_DECL
      || TREE_CODE (base1) == CONSTRUCTOR
      || TREE_CODE (base1) == ADDR_EXPR
      || CONSTANT_CLASS_P (base1)
      || TREE_CODE (base2) == SSA_NAME
      || TREE_CODE (base2) == CONST_DECL
      || TREE_CODE (base2) == CONSTRUCTOR
      || TREE_CODE (base2) == ADDR_EXPR
      || CONSTANT_CLASS_P (base2))
    return false;

  /* We can end up referring to code via function and label decls.
     As we likely do not properly track code aliases conservatively
     bail out.  */
  if (TREE_CODE (base1) == FUNCTION_DECL
      || TREE_CODE (base1) == LABEL_DECL
      || TREE_CODE (base2) == FUNCTION_DECL
      || TREE_CODE (base2) == LABEL_DECL)
    return true;

  /* Two volatile accesses always conflict.  */
  if (ref1->volatile_p
      && ref2->volatile_p)
    return true;

  /* Defer to simple offset based disambiguation if we have
     references based on two decls.  Do this before defering to
     TBAA to handle must-alias cases in conformance with the
     GCC extension of allowing type-punning through unions.  */
  var1_p = DECL_P (base1);
  var2_p = DECL_P (base2);
  if (var1_p && var2_p)
    return decl_refs_may_alias_p (base1, offset1, max_size1,
				  base2, offset2, max_size2);

  ind1_p = (TREE_CODE (base1) == MEM_REF
	    || TREE_CODE (base1) == TARGET_MEM_REF);
  ind2_p = (TREE_CODE (base2) == MEM_REF
	    || TREE_CODE (base2) == TARGET_MEM_REF);

  /* Canonicalize the pointer-vs-decl case.  */
  if (ind1_p && var2_p)
    {
      HOST_WIDE_INT tmp1;
      tree tmp2;
      ao_ref *tmp3;
      tmp1 = offset1; offset1 = offset2; offset2 = tmp1;
      tmp1 = max_size1; max_size1 = max_size2; max_size2 = tmp1;
      tmp2 = base1; base1 = base2; base2 = tmp2;
      tmp3 = ref1; ref1 = ref2; ref2 = tmp3;
      var1_p = true;
      ind1_p = false;
      var2_p = false;
      ind2_p = true;
    }

  /* First defer to TBAA if possible.  */
  if (tbaa_p
      && flag_strict_aliasing
      && !alias_sets_conflict_p (ao_ref_alias_set (ref1),
				 ao_ref_alias_set (ref2)))
    return false;

  /* Dispatch to the pointer-vs-decl or pointer-vs-pointer disambiguators.  */
  if (var1_p && ind2_p)
    return indirect_ref_may_alias_decl_p (ref2->ref, base2,
					  offset2, max_size2,
					  ao_ref_alias_set (ref2), -1,
					  ref1->ref, base1,
					  offset1, max_size1,
					  ao_ref_alias_set (ref1),
					  ao_ref_base_alias_set (ref1),
					  tbaa_p);
  else if (ind1_p && ind2_p)
    return indirect_refs_may_alias_p (ref1->ref, base1,
				      offset1, max_size1,
				      ao_ref_alias_set (ref1), -1,
				      ref2->ref, base2,
				      offset2, max_size2,
				      ao_ref_alias_set (ref2), -1,
				      tbaa_p);

  /* We really do not want to end up here, but returning true is safe.  */
#ifdef ENABLE_CHECKING
  gcc_unreachable ();
#else
  return true;
#endif
}

bool
refs_may_alias_p (tree ref1, tree ref2)
{
  ao_ref r1, r2;
  bool res;
  ao_ref_init (&r1, ref1);
  ao_ref_init (&r2, ref2);
  res = refs_may_alias_p_1 (&r1, &r2, true);
  if (res)
    ++alias_stats.refs_may_alias_p_may_alias;
  else
    ++alias_stats.refs_may_alias_p_no_alias;
  return res;
}

/* Returns true if there is a anti-dependence for the STORE that
   executes after the LOAD.  */

bool
refs_anti_dependent_p (tree load, tree store)
{
  ao_ref r1, r2;
  ao_ref_init (&r1, load);
  ao_ref_init (&r2, store);
  return refs_may_alias_p_1 (&r1, &r2, false);
}

/* Returns true if there is a output dependence for the stores
   STORE1 and STORE2.  */

bool
refs_output_dependent_p (tree store1, tree store2)
{
  ao_ref r1, r2;
  ao_ref_init (&r1, store1);
  ao_ref_init (&r2, store2);
  return refs_may_alias_p_1 (&r1, &r2, false);
}

/* If the call CALL may use the memory reference REF return true,
   otherwise return false.  */

static bool
ref_maybe_used_by_call_p_1 (gimple call, ao_ref *ref)
{
  tree base, callee;
  unsigned i;
  int flags = gimple_call_flags (call);

  /* Const functions without a static chain do not implicitly use memory.  */
  if (!gimple_call_chain (call)
      && (flags & (ECF_CONST|ECF_NOVOPS)))
    goto process_args;

  base = ao_ref_base (ref);
  if (!base)
    return true;

  /* A call that is not without side-effects might involve volatile
     accesses and thus conflicts with all other volatile accesses.  */
  if (ref->volatile_p)
    return true;

  /* If the reference is based on a decl that is not aliased the call
     cannot possibly use it.  */
  if (DECL_P (base)
      && !may_be_aliased (base)
      /* But local statics can be used through recursion.  */
      && !is_global_var (base))
    goto process_args;

  callee = gimple_call_fndecl (call);

  /* Handle those builtin functions explicitly that do not act as
     escape points.  See tree-ssa-structalias.c:find_func_aliases
     for the list of builtins we might need to handle here.  */
  if (callee != NULL_TREE
      && DECL_BUILT_IN_CLASS (callee) == BUILT_IN_NORMAL)
    switch (DECL_FUNCTION_CODE (callee))
      {
	/* All the following functions read memory pointed to by
	   their second argument.  strcat/strncat additionally
	   reads memory pointed to by the first argument.  */
	case BUILT_IN_STRCAT:
	case BUILT_IN_STRNCAT:
	  {
	    ao_ref dref;
	    ao_ref_init_from_ptr_and_size (&dref,
					   gimple_call_arg (call, 0),
					   NULL_TREE);
	    if (refs_may_alias_p_1 (&dref, ref, false))
	      return true;
	  }
	  /* FALLTHRU */
	case BUILT_IN_STRCPY:
	case BUILT_IN_STRNCPY:
	case BUILT_IN_MEMCPY:
	case BUILT_IN_MEMMOVE:
	case BUILT_IN_MEMPCPY:
	case BUILT_IN_STPCPY:
	case BUILT_IN_STPNCPY:
	case BUILT_IN_TM_MEMCPY:
	case BUILT_IN_TM_MEMMOVE:
	  {
	    ao_ref dref;
	    tree size = NULL_TREE;
	    if (gimple_call_num_args (call) == 3)
	      size = gimple_call_arg (call, 2);
	    ao_ref_init_from_ptr_and_size (&dref,
					   gimple_call_arg (call, 1),
					   size);
	    return refs_may_alias_p_1 (&dref, ref, false);
	  }
	case BUILT_IN_STRCAT_CHK:
	case BUILT_IN_STRNCAT_CHK:
	  {
	    ao_ref dref;
	    ao_ref_init_from_ptr_and_size (&dref,
					   gimple_call_arg (call, 0),
					   NULL_TREE);
	    if (refs_may_alias_p_1 (&dref, ref, false))
	      return true;
	  }
	  /* FALLTHRU */
	case BUILT_IN_STRCPY_CHK:
	case BUILT_IN_STRNCPY_CHK:
	case BUILT_IN_MEMCPY_CHK:
	case BUILT_IN_MEMMOVE_CHK:
	case BUILT_IN_MEMPCPY_CHK:
	case BUILT_IN_STPCPY_CHK:
	case BUILT_IN_STPNCPY_CHK:
	  {
	    ao_ref dref;
	    tree size = NULL_TREE;
	    if (gimple_call_num_args (call) == 4)
	      size = gimple_call_arg (call, 2);
	    ao_ref_init_from_ptr_and_size (&dref,
					   gimple_call_arg (call, 1),
					   size);
	    return refs_may_alias_p_1 (&dref, ref, false);
	  }
	case BUILT_IN_BCOPY:
	  {
	    ao_ref dref;
	    tree size = gimple_call_arg (call, 2);
	    ao_ref_init_from_ptr_and_size (&dref,
					   gimple_call_arg (call, 0),
					   size);
	    return refs_may_alias_p_1 (&dref, ref, false);
	  }

	/* The following functions read memory pointed to by their
	   first argument.  */
	CASE_BUILT_IN_TM_LOAD (1):
	CASE_BUILT_IN_TM_LOAD (2):
	CASE_BUILT_IN_TM_LOAD (4):
	CASE_BUILT_IN_TM_LOAD (8):
	CASE_BUILT_IN_TM_LOAD (FLOAT):
	CASE_BUILT_IN_TM_LOAD (DOUBLE):
	CASE_BUILT_IN_TM_LOAD (LDOUBLE):
	CASE_BUILT_IN_TM_LOAD (M64):
	CASE_BUILT_IN_TM_LOAD (M128):
	CASE_BUILT_IN_TM_LOAD (M256):
	case BUILT_IN_TM_LOG:
	case BUILT_IN_TM_LOG_1:
	case BUILT_IN_TM_LOG_2:
	case BUILT_IN_TM_LOG_4:
	case BUILT_IN_TM_LOG_8:
	case BUILT_IN_TM_LOG_FLOAT:
	case BUILT_IN_TM_LOG_DOUBLE:
	case BUILT_IN_TM_LOG_LDOUBLE:
	case BUILT_IN_TM_LOG_M64:
	case BUILT_IN_TM_LOG_M128:
	case BUILT_IN_TM_LOG_M256:
	  return ptr_deref_may_alias_ref_p_1 (gimple_call_arg (call, 0), ref);

	/* These read memory pointed to by the first argument.  */
	case BUILT_IN_STRDUP:
	case BUILT_IN_STRNDUP:
	  {
	    ao_ref dref;
	    tree size = NULL_TREE;
	    if (gimple_call_num_args (call) == 2)
	      size = gimple_call_arg (call, 1);
	    ao_ref_init_from_ptr_and_size (&dref,
					   gimple_call_arg (call, 0),
					   size);
	    return refs_may_alias_p_1 (&dref, ref, false);
	  }
	/* The following builtins do not read from memory.  */
	case BUILT_IN_FREE:
	case BUILT_IN_MALLOC:
	case BUILT_IN_CALLOC:
	case BUILT_IN_ALLOCA:
	case BUILT_IN_ALLOCA_WITH_ALIGN:
	case BUILT_IN_STACK_SAVE:
	case BUILT_IN_STACK_RESTORE:
	case BUILT_IN_MEMSET:
	case BUILT_IN_TM_MEMSET:
	case BUILT_IN_MEMSET_CHK:
	case BUILT_IN_FREXP:
	case BUILT_IN_FREXPF:
	case BUILT_IN_FREXPL:
	case BUILT_IN_GAMMA_R:
	case BUILT_IN_GAMMAF_R:
	case BUILT_IN_GAMMAL_R:
	case BUILT_IN_LGAMMA_R:
	case BUILT_IN_LGAMMAF_R:
	case BUILT_IN_LGAMMAL_R:
	case BUILT_IN_MODF:
	case BUILT_IN_MODFF:
	case BUILT_IN_MODFL:
	case BUILT_IN_REMQUO:
	case BUILT_IN_REMQUOF:
	case BUILT_IN_REMQUOL:
	case BUILT_IN_SINCOS:
	case BUILT_IN_SINCOSF:
	case BUILT_IN_SINCOSL:
	case BUILT_IN_ASSUME_ALIGNED:
	case BUILT_IN_VA_END:
	  return false;
	/* __sync_* builtins and some OpenMP builtins act as threading
	   barriers.  */
#undef DEF_SYNC_BUILTIN
#define DEF_SYNC_BUILTIN(ENUM, NAME, TYPE, ATTRS) case ENUM:
#include "sync-builtins.def"
#undef DEF_SYNC_BUILTIN
	case BUILT_IN_GOMP_ATOMIC_START:
	case BUILT_IN_GOMP_ATOMIC_END:
	case BUILT_IN_GOMP_BARRIER:
	case BUILT_IN_GOMP_TASKWAIT:
	case BUILT_IN_GOMP_CRITICAL_START:
	case BUILT_IN_GOMP_CRITICAL_END:
	case BUILT_IN_GOMP_CRITICAL_NAME_START:
	case BUILT_IN_GOMP_CRITICAL_NAME_END:
	case BUILT_IN_GOMP_LOOP_END:
	case BUILT_IN_GOMP_ORDERED_START:
	case BUILT_IN_GOMP_ORDERED_END:
	case BUILT_IN_GOMP_PARALLEL_END:
	case BUILT_IN_GOMP_SECTIONS_END:
	case BUILT_IN_GOMP_SINGLE_COPY_START:
	case BUILT_IN_GOMP_SINGLE_COPY_END:
	  return true;

	default:
	  /* Fallthru to general call handling.  */;
      }

  /* Check if base is a global static variable that is not read
     by the function.  */
  if (callee != NULL_TREE
      && TREE_CODE (base) == VAR_DECL
      && TREE_STATIC (base))
    {
      struct cgraph_node *node = cgraph_get_node (callee);
      bitmap not_read;

      /* FIXME: Callee can be an OMP builtin that does not have a call graph
	 node yet.  We should enforce that there are nodes for all decls in the
	 IL and remove this check instead.  */
      if (node
	  && (not_read = ipa_reference_get_not_read_global (node))
	  && bitmap_bit_p (not_read, DECL_UID (base)))
	goto process_args;
    }

  /* Check if the base variable is call-used.  */
  if (DECL_P (base))
    {
      if (pt_solution_includes (gimple_call_use_set (call), base))
	return true;
    }
  else if ((TREE_CODE (base) == MEM_REF
	    || TREE_CODE (base) == TARGET_MEM_REF)
	   && TREE_CODE (TREE_OPERAND (base, 0)) == SSA_NAME)
    {
      struct ptr_info_def *pi = SSA_NAME_PTR_INFO (TREE_OPERAND (base, 0));
      if (!pi)
	return true;

      if (pt_solutions_intersect (gimple_call_use_set (call), &pi->pt))
	return true;
    }
  else
    return true;

  /* Inspect call arguments for passed-by-value aliases.  */
process_args:
  for (i = 0; i < gimple_call_num_args (call); ++i)
    {
      tree op = gimple_call_arg (call, i);
      int flags = gimple_call_arg_flags (call, i);

      if (flags & EAF_UNUSED)
	continue;

      if (TREE_CODE (op) == WITH_SIZE_EXPR)
	op = TREE_OPERAND (op, 0);

      if (TREE_CODE (op) != SSA_NAME
	  && !is_gimple_min_invariant (op))
	{
	  ao_ref r;
	  ao_ref_init (&r, op);
	  if (refs_may_alias_p_1 (&r, ref, true))
	    return true;
	}
    }

  return false;
}

static bool
ref_maybe_used_by_call_p (gimple call, tree ref)
{
  ao_ref r;
  bool res;
  ao_ref_init (&r, ref);
  res = ref_maybe_used_by_call_p_1 (call, &r);
  if (res)
    ++alias_stats.ref_maybe_used_by_call_p_may_alias;
  else
    ++alias_stats.ref_maybe_used_by_call_p_no_alias;
  return res;
}


/* If the statement STMT may use the memory reference REF return
   true, otherwise return false.  */

bool
ref_maybe_used_by_stmt_p (gimple stmt, tree ref)
{
  if (is_gimple_assign (stmt))
    {
      tree rhs;

      /* All memory assign statements are single.  */
      if (!gimple_assign_single_p (stmt))
	return false;

      rhs = gimple_assign_rhs1 (stmt);
      if (is_gimple_reg (rhs)
	  || is_gimple_min_invariant (rhs)
	  || gimple_assign_rhs_code (stmt) == CONSTRUCTOR)
	return false;

      return refs_may_alias_p (rhs, ref);
    }
  else if (is_gimple_call (stmt))
    return ref_maybe_used_by_call_p (stmt, ref);
  else if (gimple_code (stmt) == GIMPLE_RETURN)
    {
      tree retval = gimple_return_retval (stmt);
      tree base;
      if (retval
	  && TREE_CODE (retval) != SSA_NAME
	  && !is_gimple_min_invariant (retval)
	  && refs_may_alias_p (retval, ref))
	return true;
      /* If ref escapes the function then the return acts as a use.  */
      base = get_base_address (ref);
      if (!base)
	;
      else if (DECL_P (base))
	return is_global_var (base);
      else if (TREE_CODE (base) == MEM_REF
	       || TREE_CODE (base) == TARGET_MEM_REF)
	return ptr_deref_may_alias_global_p (TREE_OPERAND (base, 0));
      return false;
    }

  return true;
}

/* If the call in statement CALL may clobber the memory reference REF
   return true, otherwise return false.  */

static bool
call_may_clobber_ref_p_1 (gimple call, ao_ref *ref)
{
  tree base;
  tree callee;

  /* If the call is pure or const it cannot clobber anything.  */
  if (gimple_call_flags (call)
      & (ECF_PURE|ECF_CONST|ECF_LOOPING_CONST_OR_PURE|ECF_NOVOPS))
    return false;

  base = ao_ref_base (ref);
  if (!base)
    return true;

  if (TREE_CODE (base) == SSA_NAME
      || CONSTANT_CLASS_P (base))
    return false;

  /* A call that is not without side-effects might involve volatile
     accesses and thus conflicts with all other volatile accesses.  */
  if (ref->volatile_p)
    return true;

  /* If the reference is based on a decl that is not aliased the call
     cannot possibly clobber it.  */
  if (DECL_P (base)
      && !may_be_aliased (base)
      /* But local non-readonly statics can be modified through recursion
         or the call may implement a threading barrier which we must
	 treat as may-def.  */
      && (TREE_READONLY (base)
	  || !is_global_var (base)))
    return false;

  callee = gimple_call_fndecl (call);

  /* Handle those builtin functions explicitly that do not act as
     escape points.  See tree-ssa-structalias.c:find_func_aliases
     for the list of builtins we might need to handle here.  */
  if (callee != NULL_TREE
      && DECL_BUILT_IN_CLASS (callee) == BUILT_IN_NORMAL)
    switch (DECL_FUNCTION_CODE (callee))
      {
	/* All the following functions clobber memory pointed to by
	   their first argument.  */
	case BUILT_IN_STRCPY:
	case BUILT_IN_STRNCPY:
	case BUILT_IN_MEMCPY:
	case BUILT_IN_MEMMOVE:
	case BUILT_IN_MEMPCPY:
	case BUILT_IN_STPCPY:
	case BUILT_IN_STPNCPY:
	case BUILT_IN_STRCAT:
	case BUILT_IN_STRNCAT:
	case BUILT_IN_MEMSET:
	case BUILT_IN_TM_MEMSET:
	CASE_BUILT_IN_TM_STORE (1):
	CASE_BUILT_IN_TM_STORE (2):
	CASE_BUILT_IN_TM_STORE (4):
	CASE_BUILT_IN_TM_STORE (8):
	CASE_BUILT_IN_TM_STORE (FLOAT):
	CASE_BUILT_IN_TM_STORE (DOUBLE):
	CASE_BUILT_IN_TM_STORE (LDOUBLE):
	CASE_BUILT_IN_TM_STORE (M64):
	CASE_BUILT_IN_TM_STORE (M128):
	CASE_BUILT_IN_TM_STORE (M256):
	case BUILT_IN_TM_MEMCPY:
	case BUILT_IN_TM_MEMMOVE:
	  {
	    ao_ref dref;
	    tree size = NULL_TREE;
	    /* Don't pass in size for strncat, as the maximum size
	       is strlen (dest) + n + 1 instead of n, resp.
	       n + 1 at dest + strlen (dest), but strlen (dest) isn't
	       known.  */
	    if (gimple_call_num_args (call) == 3
		&& DECL_FUNCTION_CODE (callee) != BUILT_IN_STRNCAT)
	      size = gimple_call_arg (call, 2);
	    ao_ref_init_from_ptr_and_size (&dref,
					   gimple_call_arg (call, 0),
					   size);
	    return refs_may_alias_p_1 (&dref, ref, false);
	  }
	case BUILT_IN_STRCPY_CHK:
	case BUILT_IN_STRNCPY_CHK:
	case BUILT_IN_MEMCPY_CHK:
	case BUILT_IN_MEMMOVE_CHK:
	case BUILT_IN_MEMPCPY_CHK:
	case BUILT_IN_STPCPY_CHK:
	case BUILT_IN_STPNCPY_CHK:
	case BUILT_IN_STRCAT_CHK:
	case BUILT_IN_STRNCAT_CHK:
	case BUILT_IN_MEMSET_CHK:
	  {
	    ao_ref dref;
	    tree size = NULL_TREE;
	    /* Don't pass in size for __strncat_chk, as the maximum size
	       is strlen (dest) + n + 1 instead of n, resp.
	       n + 1 at dest + strlen (dest), but strlen (dest) isn't
	       known.  */
	    if (gimple_call_num_args (call) == 4
		&& DECL_FUNCTION_CODE (callee) != BUILT_IN_STRNCAT_CHK)
	      size = gimple_call_arg (call, 2);
	    ao_ref_init_from_ptr_and_size (&dref,
					   gimple_call_arg (call, 0),
					   size);
	    return refs_may_alias_p_1 (&dref, ref, false);
	  }
	case BUILT_IN_BCOPY:
	  {
	    ao_ref dref;
	    tree size = gimple_call_arg (call, 2);
	    ao_ref_init_from_ptr_and_size (&dref,
					   gimple_call_arg (call, 1),
					   size);
	    return refs_may_alias_p_1 (&dref, ref, false);
	  }
	/* Allocating memory does not have any side-effects apart from
	   being the definition point for the pointer.  */
	case BUILT_IN_MALLOC:
	case BUILT_IN_CALLOC:
	case BUILT_IN_STRDUP:
	case BUILT_IN_STRNDUP:
	  /* Unix98 specifies that errno is set on allocation failure.  */
	  if (flag_errno_math
	      && targetm.ref_may_alias_errno (ref))
	    return true;
	  return false;
	case BUILT_IN_STACK_SAVE:
	case BUILT_IN_ALLOCA:
	case BUILT_IN_ALLOCA_WITH_ALIGN:
	case BUILT_IN_ASSUME_ALIGNED:
	  return false;
	/* Freeing memory kills the pointed-to memory.  More importantly
	   the call has to serve as a barrier for moving loads and stores
	   across it.  */
	case BUILT_IN_FREE:
	case BUILT_IN_VA_END:
	  {
	    tree ptr = gimple_call_arg (call, 0);
	    return ptr_deref_may_alias_ref_p_1 (ptr, ref);
	  }
	case BUILT_IN_GAMMA_R:
	case BUILT_IN_GAMMAF_R:
	case BUILT_IN_GAMMAL_R:
	case BUILT_IN_LGAMMA_R:
	case BUILT_IN_LGAMMAF_R:
	case BUILT_IN_LGAMMAL_R:
	  {
	    tree out = gimple_call_arg (call, 1);
	    if (ptr_deref_may_alias_ref_p_1 (out, ref))
	      return true;
	    if (flag_errno_math)
	      break;
	    return false;
	  }
	case BUILT_IN_FREXP:
	case BUILT_IN_FREXPF:
	case BUILT_IN_FREXPL:
	case BUILT_IN_MODF:
	case BUILT_IN_MODFF:
	case BUILT_IN_MODFL:
	  {
	    tree out = gimple_call_arg (call, 1);
	    return ptr_deref_may_alias_ref_p_1 (out, ref);
	  }
	case BUILT_IN_REMQUO:
	case BUILT_IN_REMQUOF:
	case BUILT_IN_REMQUOL:
	  {
	    tree out = gimple_call_arg (call, 2);
	    if (ptr_deref_may_alias_ref_p_1 (out, ref))
	      return true;
	    if (flag_errno_math)
	      break;
	    return false;
	  }
	case BUILT_IN_SINCOS:
	case BUILT_IN_SINCOSF:
	case BUILT_IN_SINCOSL:
	  {
	    tree sin = gimple_call_arg (call, 1);
	    tree cos = gimple_call_arg (call, 2);
	    return (ptr_deref_may_alias_ref_p_1 (sin, ref)
		    || ptr_deref_may_alias_ref_p_1 (cos, ref));
	  }
	/* __sync_* builtins and some OpenMP builtins act as threading
	   barriers.  */
#undef DEF_SYNC_BUILTIN
#define DEF_SYNC_BUILTIN(ENUM, NAME, TYPE, ATTRS) case ENUM:
#include "sync-builtins.def"
#undef DEF_SYNC_BUILTIN
	case BUILT_IN_GOMP_ATOMIC_START:
	case BUILT_IN_GOMP_ATOMIC_END:
	case BUILT_IN_GOMP_BARRIER:
	case BUILT_IN_GOMP_TASKWAIT:
	case BUILT_IN_GOMP_CRITICAL_START:
	case BUILT_IN_GOMP_CRITICAL_END:
	case BUILT_IN_GOMP_CRITICAL_NAME_START:
	case BUILT_IN_GOMP_CRITICAL_NAME_END:
	case BUILT_IN_GOMP_LOOP_END:
	case BUILT_IN_GOMP_ORDERED_START:
	case BUILT_IN_GOMP_ORDERED_END:
	case BUILT_IN_GOMP_PARALLEL_END:
	case BUILT_IN_GOMP_SECTIONS_END:
	case BUILT_IN_GOMP_SINGLE_COPY_START:
	case BUILT_IN_GOMP_SINGLE_COPY_END:
	  return true;
	default:
	  /* Fallthru to general call handling.  */;
      }

  /* Check if base is a global static variable that is not written
     by the function.  */
  if (callee != NULL_TREE
      && TREE_CODE (base) == VAR_DECL
      && TREE_STATIC (base))
    {
      struct cgraph_node *node = cgraph_get_node (callee);
      bitmap not_written;

      if (node
	  && (not_written = ipa_reference_get_not_written_global (node))
	  && bitmap_bit_p (not_written, DECL_UID (base)))
	return false;
    }

  /* Check if the base variable is call-clobbered.  */
  if (DECL_P (base))
    return pt_solution_includes (gimple_call_clobber_set (call), base);
  else if ((TREE_CODE (base) == MEM_REF
	    || TREE_CODE (base) == TARGET_MEM_REF)
	   && TREE_CODE (TREE_OPERAND (base, 0)) == SSA_NAME)
    {
      struct ptr_info_def *pi = SSA_NAME_PTR_INFO (TREE_OPERAND (base, 0));
      if (!pi)
	return true;

      return pt_solutions_intersect (gimple_call_clobber_set (call), &pi->pt);
    }

  return true;
}

/* If the call in statement CALL may clobber the memory reference REF
   return true, otherwise return false.  */

bool
call_may_clobber_ref_p (gimple call, tree ref)
{
  bool res;
  ao_ref r;
  ao_ref_init (&r, ref);
  res = call_may_clobber_ref_p_1 (call, &r);
  if (res)
    ++alias_stats.call_may_clobber_ref_p_may_alias;
  else
    ++alias_stats.call_may_clobber_ref_p_no_alias;
  return res;
}


/* If the statement STMT may clobber the memory reference REF return true,
   otherwise return false.  */

bool
stmt_may_clobber_ref_p_1 (gimple stmt, ao_ref *ref)
{
  if (is_gimple_call (stmt))
    {
      tree lhs = gimple_call_lhs (stmt);
      if (lhs
	  && TREE_CODE (lhs) != SSA_NAME)
	{
	  ao_ref r;
	  ao_ref_init (&r, lhs);
	  if (refs_may_alias_p_1 (ref, &r, true))
	    return true;
	}

      return call_may_clobber_ref_p_1 (stmt, ref);
    }
  else if (gimple_assign_single_p (stmt))
    {
      tree lhs = gimple_assign_lhs (stmt);
      if (TREE_CODE (lhs) != SSA_NAME)
	{
	  ao_ref r;
	  ao_ref_init (&r, lhs);
	  return refs_may_alias_p_1 (ref, &r, true);
	}
    }
  else if (gimple_code (stmt) == GIMPLE_ASM)
    return true;

  return false;
}

bool
stmt_may_clobber_ref_p (gimple stmt, tree ref)
{
  ao_ref r;
  ao_ref_init (&r, ref);
  return stmt_may_clobber_ref_p_1 (stmt, &r);
}

/* If STMT kills the memory reference REF return true, otherwise
   return false.  */

static bool
stmt_kills_ref_p_1 (gimple stmt, ao_ref *ref)
{
  /* For a must-alias check we need to be able to constrain
     the access properly.  */
  ao_ref_base (ref);
  if (ref->max_size == -1)
    return false;

  if (gimple_has_lhs (stmt)
      && TREE_CODE (gimple_get_lhs (stmt)) != SSA_NAME
      /* The assignment is not necessarily carried out if it can throw
	 and we can catch it in the current function where we could inspect
	 the previous value.
	 ???  We only need to care about the RHS throwing.  For aggregate
	 assignments or similar calls and non-call exceptions the LHS
	 might throw as well.  */
      && !stmt_can_throw_internal (stmt))
    {
      tree base, lhs = gimple_get_lhs (stmt);
      HOST_WIDE_INT size, offset, max_size;
      base = get_ref_base_and_extent (lhs, &offset, &size, &max_size);
      /* We can get MEM[symbol: sZ, index: D.8862_1] here,
	 so base == ref->base does not always hold.  */
      if (base == ref->base)
	{
	  /* For a must-alias check we need to be able to constrain
	     the access properly.  */
	  if (size != -1 && size == max_size)
	    {
	      if (offset <= ref->offset
		  && offset + size >= ref->offset + ref->max_size)
		return true;
	    }
	}
    }

  if (is_gimple_call (stmt))
    {
      tree callee = gimple_call_fndecl (stmt);
      if (callee != NULL_TREE
	  && DECL_BUILT_IN_CLASS (callee) == BUILT_IN_NORMAL)
	switch (DECL_FUNCTION_CODE (callee))
	  {
	  case BUILT_IN_MEMCPY:
	  case BUILT_IN_MEMPCPY:
	  case BUILT_IN_MEMMOVE:
	  case BUILT_IN_MEMSET:
	  case BUILT_IN_MEMCPY_CHK:
	  case BUILT_IN_MEMPCPY_CHK:
	  case BUILT_IN_MEMMOVE_CHK:
	  case BUILT_IN_MEMSET_CHK:
	    {
	      tree dest = gimple_call_arg (stmt, 0);
	      tree len = gimple_call_arg (stmt, 2);
	      tree base = NULL_TREE;
	      HOST_WIDE_INT offset = 0;
	      if (!host_integerp (len, 0))
		return false;
	      if (TREE_CODE (dest) == ADDR_EXPR)
		base = get_addr_base_and_unit_offset (TREE_OPERAND (dest, 0),
						      &offset);
	      else if (TREE_CODE (dest) == SSA_NAME)
		base = dest;
	      if (base
		  && base == ao_ref_base (ref))
		{
		  HOST_WIDE_INT size = TREE_INT_CST_LOW (len);
		  if (offset <= ref->offset / BITS_PER_UNIT
		      && (offset + size
		          >= ((ref->offset + ref->max_size + BITS_PER_UNIT - 1)
			      / BITS_PER_UNIT)))
		    return true;
		}
	      break;
	    }

	  case BUILT_IN_VA_END:
	    {
	      tree ptr = gimple_call_arg (stmt, 0);
	      if (TREE_CODE (ptr) == ADDR_EXPR)
		{
		  tree base = ao_ref_base (ref);
		  if (TREE_OPERAND (ptr, 0) == base)
		    return true;
		}
	      break;
	    }

	  default:;
	  }
    }
  return false;
}

bool
stmt_kills_ref_p (gimple stmt, tree ref)
{
  ao_ref r;
  ao_ref_init (&r, ref);
  return stmt_kills_ref_p_1 (stmt, &r);
}


/* Walk the virtual use-def chain of VUSE until hitting the virtual operand
   TARGET or a statement clobbering the memory reference REF in which
   case false is returned.  The walk starts with VUSE, one argument of PHI.  */

static bool
maybe_skip_until (gimple phi, tree target, ao_ref *ref,
		  tree vuse, unsigned int *cnt, bitmap *visited,
		  bool abort_on_visited)
{
  basic_block bb = gimple_bb (phi);

  if (!*visited)
    *visited = BITMAP_ALLOC (NULL);

  bitmap_set_bit (*visited, SSA_NAME_VERSION (PHI_RESULT (phi)));

  /* Walk until we hit the target.  */
  while (vuse != target)
    {
      gimple def_stmt = SSA_NAME_DEF_STMT (vuse);
      /* Recurse for PHI nodes.  */
      if (gimple_code (def_stmt) == GIMPLE_PHI)
	{
	  /* An already visited PHI node ends the walk successfully.  */
	  if (bitmap_bit_p (*visited, SSA_NAME_VERSION (PHI_RESULT (def_stmt))))
	    return !abort_on_visited;
	  vuse = get_continuation_for_phi (def_stmt, ref, cnt,
					   visited, abort_on_visited);
	  if (!vuse)
	    return false;
	  continue;
	}
      else if (gimple_nop_p (def_stmt))
	return false;
      else
	{
	  /* A clobbering statement or the end of the IL ends it failing.  */
	  ++*cnt;
	  if (stmt_may_clobber_ref_p_1 (def_stmt, ref))
	    return false;
	}
      /* If we reach a new basic-block see if we already skipped it
         in a previous walk that ended successfully.  */
      if (gimple_bb (def_stmt) != bb)
	{
	  if (!bitmap_set_bit (*visited, SSA_NAME_VERSION (vuse)))
	    return !abort_on_visited;
	  bb = gimple_bb (def_stmt);
	}
      vuse = gimple_vuse (def_stmt);
    }
  return true;
}

/* For two PHI arguments ARG0 and ARG1 try to skip non-aliasing code
   until we hit the phi argument definition that dominates the other one.
   Return that, or NULL_TREE if there is no such definition.  */

static tree
get_continuation_for_phi_1 (gimple phi, tree arg0, tree arg1,
			    ao_ref *ref, unsigned int *cnt,
			    bitmap *visited, bool abort_on_visited)
{
  gimple def0 = SSA_NAME_DEF_STMT (arg0);
  gimple def1 = SSA_NAME_DEF_STMT (arg1);
  tree common_vuse;

  if (arg0 == arg1)
    return arg0;
  else if (gimple_nop_p (def0)
	   || (!gimple_nop_p (def1)
	       && dominated_by_p (CDI_DOMINATORS,
				  gimple_bb (def1), gimple_bb (def0))))
    {
      if (maybe_skip_until (phi, arg0, ref, arg1, cnt,
			    visited, abort_on_visited))
	return arg0;
    }
  else if (gimple_nop_p (def1)
	   || dominated_by_p (CDI_DOMINATORS,
			      gimple_bb (def0), gimple_bb (def1)))
    {
      if (maybe_skip_until (phi, arg1, ref, arg0, cnt,
			    visited, abort_on_visited))
	return arg1;
    }
  /* Special case of a diamond:
       MEM_1 = ...
       goto (cond) ? L1 : L2
       L1: store1 = ...    #MEM_2 = vuse(MEM_1)
	   goto L3
       L2: store2 = ...    #MEM_3 = vuse(MEM_1)
       L3: MEM_4 = PHI<MEM_2, MEM_3>
     We were called with the PHI at L3, MEM_2 and MEM_3 don't
     dominate each other, but still we can easily skip this PHI node
     if we recognize that the vuse MEM operand is the same for both,
     and that we can skip both statements (they don't clobber us).
     This is still linear.  Don't use maybe_skip_until, that might
     potentially be slow.  */
  else if ((common_vuse = gimple_vuse (def0))
	   && common_vuse == gimple_vuse (def1))
    {
      *cnt += 2;
      if (!stmt_may_clobber_ref_p_1 (def0, ref)
	  && !stmt_may_clobber_ref_p_1 (def1, ref))
	return common_vuse;
    }

  return NULL_TREE;
}


/* Starting from a PHI node for the virtual operand of the memory reference
   REF find a continuation virtual operand that allows to continue walking
   statements dominating PHI skipping only statements that cannot possibly
   clobber REF.  Increments *CNT for each alias disambiguation done.
   Returns NULL_TREE if no suitable virtual operand can be found.  */

tree
get_continuation_for_phi (gimple phi, ao_ref *ref,
			  unsigned int *cnt, bitmap *visited,
			  bool abort_on_visited)
{
  unsigned nargs = gimple_phi_num_args (phi);

  /* Through a single-argument PHI we can simply look through.  */
  if (nargs == 1)
    return PHI_ARG_DEF (phi, 0);

  /* For two or more arguments try to pairwise skip non-aliasing code
     until we hit the phi argument definition that dominates the other one.  */
  else if (nargs >= 2)
    {
      tree arg0, arg1;
      unsigned i;

      /* Find a candidate for the virtual operand which definition
	 dominates those of all others.  */
      arg0 = PHI_ARG_DEF (phi, 0);
      if (!SSA_NAME_IS_DEFAULT_DEF (arg0))
	for (i = 1; i < nargs; ++i)
	  {
	    arg1 = PHI_ARG_DEF (phi, i);
	    if (SSA_NAME_IS_DEFAULT_DEF (arg1))
	      {
		arg0 = arg1;
		break;
	      }
	    if (dominated_by_p (CDI_DOMINATORS,
				gimple_bb (SSA_NAME_DEF_STMT (arg0)),
				gimple_bb (SSA_NAME_DEF_STMT (arg1))))
	      arg0 = arg1;
	  }

      /* Then pairwise reduce against the found candidate.  */
      for (i = 0; i < nargs; ++i)
	{
	  arg1 = PHI_ARG_DEF (phi, i);
	  arg0 = get_continuation_for_phi_1 (phi, arg0, arg1, ref,
					     cnt, visited, abort_on_visited);
	  if (!arg0)
	    return NULL_TREE;
	}

      return arg0;
    }

  return NULL_TREE;
}

/* Based on the memory reference REF and its virtual use VUSE call
   WALKER for each virtual use that is equivalent to VUSE, including VUSE
   itself.  That is, for each virtual use for which its defining statement
   does not clobber REF.

   WALKER is called with REF, the current virtual use and DATA.  If
   WALKER returns non-NULL the walk stops and its result is returned.
   At the end of a non-successful walk NULL is returned.

   TRANSLATE if non-NULL is called with a pointer to REF, the virtual
   use which definition is a statement that may clobber REF and DATA.
   If TRANSLATE returns (void *)-1 the walk stops and NULL is returned.
   If TRANSLATE returns non-NULL the walk stops and its result is returned.
   If TRANSLATE returns NULL the walk continues and TRANSLATE is supposed
   to adjust REF and *DATA to make that valid.

   TODO: Cache the vector of equivalent vuses per ref, vuse pair.  */

void *
walk_non_aliased_vuses (ao_ref *ref, tree vuse,
			void *(*walker)(ao_ref *, tree, unsigned int, void *),
			void *(*translate)(ao_ref *, tree, void *), void *data)
{
  bitmap visited = NULL;
  void *res;
  unsigned int cnt = 0;
  bool translated = false;

  timevar_push (TV_ALIAS_STMT_WALK);

  do
    {
      gimple def_stmt;

      /* ???  Do we want to account this to TV_ALIAS_STMT_WALK?  */
      res = (*walker) (ref, vuse, cnt, data);
      /* Abort walk.  */
      if (res == (void *)-1)
	{
	  res = NULL;
	  break;
	}
      /* Lookup succeeded.  */
      else if (res != NULL)
	break;

      def_stmt = SSA_NAME_DEF_STMT (vuse);
      if (gimple_nop_p (def_stmt))
	break;
      else if (gimple_code (def_stmt) == GIMPLE_PHI)
	vuse = get_continuation_for_phi (def_stmt, ref, &cnt,
					 &visited, translated);
      else
	{
	  cnt++;
	  if (stmt_may_clobber_ref_p_1 (def_stmt, ref))
	    {
	      if (!translate)
		break;
	      res = (*translate) (ref, vuse, data);
	      /* Failed lookup and translation.  */
	      if (res == (void *)-1)
		{
		  res = NULL;
		  break;
		}
	      /* Lookup succeeded.  */
	      else if (res != NULL)
		break;
	      /* Translation succeeded, continue walking.  */
	      translated = true;
	    }
	  vuse = gimple_vuse (def_stmt);
	}
    }
  while (vuse);

  if (visited)
    BITMAP_FREE (visited);

  timevar_pop (TV_ALIAS_STMT_WALK);

  return res;
}


/* Based on the memory reference REF call WALKER for each vdef which
   defining statement may clobber REF, starting with VDEF.  If REF
   is NULL_TREE, each defining statement is visited.

   WALKER is called with REF, the current vdef and DATA.  If WALKER
   returns true the walk is stopped, otherwise it continues.

   At PHI nodes walk_aliased_vdefs forks into one walk for reach
   PHI argument (but only one walk continues on merge points), the
   return value is true if any of the walks was successful.

   The function returns the number of statements walked.  */

static unsigned int
walk_aliased_vdefs_1 (ao_ref *ref, tree vdef,
		      bool (*walker)(ao_ref *, tree, void *), void *data,
		      bitmap *visited, unsigned int cnt)
{
  do
    {
      gimple def_stmt = SSA_NAME_DEF_STMT (vdef);

      if (*visited
	  && !bitmap_set_bit (*visited, SSA_NAME_VERSION (vdef)))
	return cnt;

      if (gimple_nop_p (def_stmt))
	return cnt;
      else if (gimple_code (def_stmt) == GIMPLE_PHI)
	{
	  unsigned i;
	  if (!*visited)
	    *visited = BITMAP_ALLOC (NULL);
	  for (i = 0; i < gimple_phi_num_args (def_stmt); ++i)
	    cnt += walk_aliased_vdefs_1 (ref, gimple_phi_arg_def (def_stmt, i),
					 walker, data, visited, 0);
	  return cnt;
	}

      /* ???  Do we want to account this to TV_ALIAS_STMT_WALK?  */
      cnt++;
      if ((!ref
	   || stmt_may_clobber_ref_p_1 (def_stmt, ref))
	  && (*walker) (ref, vdef, data))
	return cnt;

      vdef = gimple_vuse (def_stmt);
    }
  while (1);
}

unsigned int
walk_aliased_vdefs (ao_ref *ref, tree vdef,
		    bool (*walker)(ao_ref *, tree, void *), void *data,
		    bitmap *visited)
{
  bitmap local_visited = NULL;
  unsigned int ret;

  timevar_push (TV_ALIAS_STMT_WALK);

  ret = walk_aliased_vdefs_1 (ref, vdef, walker, data,
			      visited ? visited : &local_visited, 0);
  if (local_visited)
    BITMAP_FREE (local_visited);

  timevar_pop (TV_ALIAS_STMT_WALK);

  return ret;
}

