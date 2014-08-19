/* A pass for lowering trees to RTL.
   Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010
   Free Software Foundation, Inc.

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
#include "rtl.h"
#include "tm_p.h"
#include "basic-block.h"
#include "function.h"
#include "expr.h"
#include "langhooks.h"
#include "tree-flow.h"
#include "timevar.h"
#include "tree-dump.h"
#include "tree-pass.h"
#include "except.h"
#include "flags.h"
#include "diagnostic.h"
#include "toplev.h"
#include "debug.h"
#include "params.h"
#include "tree-inline.h"
#include "value-prof.h"
#include "target.h"
#include "ssaexpand.h"


/* This variable holds information helping the rewriting of SSA trees
   into RTL.  */
struct ssaexpand SA;

/* This variable holds the currently expanded gimple statement for purposes
   of comminucating the profile info to the builtin expanders.  */
gimple currently_expanding_gimple_stmt;

/* Return an expression tree corresponding to the RHS of GIMPLE
   statement STMT.  */

tree
gimple_assign_rhs_to_tree (gimple stmt)
{
  tree t;
  enum gimple_rhs_class grhs_class;

  grhs_class = get_gimple_rhs_class (gimple_expr_code (stmt));

  if (grhs_class == GIMPLE_BINARY_RHS)
    t = build2 (gimple_assign_rhs_code (stmt),
		TREE_TYPE (gimple_assign_lhs (stmt)),
		gimple_assign_rhs1 (stmt),
		gimple_assign_rhs2 (stmt));
  else if (grhs_class == GIMPLE_UNARY_RHS)
    t = build1 (gimple_assign_rhs_code (stmt),
		TREE_TYPE (gimple_assign_lhs (stmt)),
		gimple_assign_rhs1 (stmt));
  else if (grhs_class == GIMPLE_SINGLE_RHS)
    {
      t = gimple_assign_rhs1 (stmt);
      /* Avoid modifying this tree in place below.  */
      if ((gimple_has_location (stmt) && CAN_HAVE_LOCATION_P (t)
	   && gimple_location (stmt) != EXPR_LOCATION (t))
	  || (gimple_block (stmt)
	      && currently_expanding_to_rtl
	      && EXPR_P (t)
	      && gimple_block (stmt) != TREE_BLOCK (t)))
	t = copy_node (t);
    }
  else
    gcc_unreachable ();

  if (gimple_has_location (stmt) && CAN_HAVE_LOCATION_P (t))
    SET_EXPR_LOCATION (t, gimple_location (stmt));
  if (gimple_block (stmt) && currently_expanding_to_rtl && EXPR_P (t))
    TREE_BLOCK (t) = gimple_block (stmt);

  return t;
}


#ifndef STACK_ALIGNMENT_NEEDED
#define STACK_ALIGNMENT_NEEDED 1
#endif

#define SSAVAR(x) (TREE_CODE (x) == SSA_NAME ? SSA_NAME_VAR (x) : x)

/* Associate declaration T with storage space X.  If T is no
   SSA name this is exactly SET_DECL_RTL, otherwise make the
   partition of T associated with X.  */
static inline void
set_rtl (tree t, rtx x)
{
  if (TREE_CODE (t) == SSA_NAME)
    {
      SA.partition_to_pseudo[var_to_partition (SA.map, t)] = x;
      if (x && !MEM_P (x))
	set_reg_attrs_for_decl_rtl (SSA_NAME_VAR (t), x);
      /* For the benefit of debug information at -O0 (where vartracking
         doesn't run) record the place also in the base DECL if it's
	 a normal variable (not a parameter).  */
      if (x && x != pc_rtx && TREE_CODE (SSA_NAME_VAR (t)) == VAR_DECL)
	{
	  tree var = SSA_NAME_VAR (t);
	  /* If we don't yet have something recorded, just record it now.  */
	  if (!DECL_RTL_SET_P (var))
	    SET_DECL_RTL (var, x);
	  /* If we have it set alrady to "multiple places" don't
	     change this.  */
	  else if (DECL_RTL (var) == pc_rtx)
	    ;
	  /* If we have something recorded and it's not the same place
	     as we want to record now, we have multiple partitions for the
	     same base variable, with different places.  We can't just
	     randomly chose one, hence we have to say that we don't know.
	     This only happens with optimization, and there var-tracking
	     will figure out the right thing.  */
	  else if (DECL_RTL (var) != x)
	    SET_DECL_RTL (var, pc_rtx);
	}
    }
  else
    SET_DECL_RTL (t, x);
}

/* This structure holds data relevant to one variable that will be
   placed in a stack slot.  */
struct stack_var
{
  /* The Variable.  */
  tree decl;

  /* The offset of the variable.  During partitioning, this is the
     offset relative to the partition.  After partitioning, this
     is relative to the stack frame.  */
  HOST_WIDE_INT offset;

  /* Initially, the size of the variable.  Later, the size of the partition,
     if this variable becomes it's partition's representative.  */
  HOST_WIDE_INT size;

  /* The *byte* alignment required for this variable.  Or as, with the
     size, the alignment for this partition.  */
  unsigned int alignb;

  /* The partition representative.  */
  size_t representative;

  /* The next stack variable in the partition, or EOC.  */
  size_t next;

  /* The numbers of conflicting stack variables.  */
  bitmap conflicts;
};

#define EOC  ((size_t)-1)

/* We have an array of such objects while deciding allocation.  */
static struct stack_var *stack_vars;
static size_t stack_vars_alloc;
static size_t stack_vars_num;

/* An array of indices such that stack_vars[stack_vars_sorted[i]].size
   is non-decreasing.  */
static size_t *stack_vars_sorted;

/* The phase of the stack frame.  This is the known misalignment of
   virtual_stack_vars_rtx from PREFERRED_STACK_BOUNDARY.  That is,
   (frame_offset+frame_phase) % PREFERRED_STACK_BOUNDARY == 0.  */
static int frame_phase;

/* Used during expand_used_vars to remember if we saw any decls for
   which we'd like to enable stack smashing protection.  */
static bool has_protected_decls;

/* Used during expand_used_vars.  Remember if we say a character buffer
   smaller than our cutoff threshold.  Used for -Wstack-protector.  */
static bool has_short_buffer;

/* Discover the byte alignment to use for DECL.  Ignore alignment
   we can't do with expected alignment of the stack boundary.  */

static unsigned int
get_decl_align_unit (tree decl)
{
  unsigned int align;

  align = LOCAL_DECL_ALIGNMENT (decl);

  if (align > MAX_SUPPORTED_STACK_ALIGNMENT)
    align = MAX_SUPPORTED_STACK_ALIGNMENT;

  if (SUPPORTS_STACK_ALIGNMENT)
    {
      if (crtl->stack_alignment_estimated < align)
	{
	  gcc_assert(!crtl->stack_realign_processed);
          crtl->stack_alignment_estimated = align;
	}
    }

  /* stack_alignment_needed > PREFERRED_STACK_BOUNDARY is permitted.
     So here we only make sure stack_alignment_needed >= align.  */
  if (crtl->stack_alignment_needed < align)
    crtl->stack_alignment_needed = align;
  if (crtl->max_used_stack_slot_alignment < align)
    crtl->max_used_stack_slot_alignment = align;

  return align / BITS_PER_UNIT;
}

/* Allocate SIZE bytes at byte alignment ALIGN from the stack frame.
   Return the frame offset.  */

static HOST_WIDE_INT
alloc_stack_frame_space (HOST_WIDE_INT size, HOST_WIDE_INT align)
{
  HOST_WIDE_INT offset, new_frame_offset;

  new_frame_offset = frame_offset;
  if (FRAME_GROWS_DOWNWARD)
    {
      new_frame_offset -= size + frame_phase;
      new_frame_offset &= -align;
      new_frame_offset += frame_phase;
      offset = new_frame_offset;
    }
  else
    {
      new_frame_offset -= frame_phase;
      new_frame_offset += align - 1;
      new_frame_offset &= -align;
      new_frame_offset += frame_phase;
      offset = new_frame_offset;
      new_frame_offset += size;
    }
  frame_offset = new_frame_offset;

  if (frame_offset_overflow (frame_offset, cfun->decl))
    frame_offset = offset = 0;

  return offset;
}

/* Accumulate DECL into STACK_VARS.  */

static void
add_stack_var (tree decl)
{
  if (stack_vars_num >= stack_vars_alloc)
    {
      if (stack_vars_alloc)
	stack_vars_alloc = stack_vars_alloc * 3 / 2;
      else
	stack_vars_alloc = 32;
      stack_vars
	= XRESIZEVEC (struct stack_var, stack_vars, stack_vars_alloc);
    }
  stack_vars[stack_vars_num].decl = decl;
  stack_vars[stack_vars_num].offset = 0;
  stack_vars[stack_vars_num].size = tree_low_cst (DECL_SIZE_UNIT (SSAVAR (decl)), 1);
  stack_vars[stack_vars_num].alignb = get_decl_align_unit (SSAVAR (decl));

  /* All variables are initially in their own partition.  */
  stack_vars[stack_vars_num].representative = stack_vars_num;
  stack_vars[stack_vars_num].next = EOC;

  /* All variables initially conflict with no other.  */
  stack_vars[stack_vars_num].conflicts = NULL;

  /* Ensure that this decl doesn't get put onto the list twice.  */
  set_rtl (decl, pc_rtx);

  stack_vars_num++;
}

/* Make the decls associated with luid's X and Y conflict.  */

static void
add_stack_var_conflict (size_t x, size_t y)
{
  struct stack_var *a = &stack_vars[x];
  struct stack_var *b = &stack_vars[y];
  if (!a->conflicts)
    a->conflicts = BITMAP_ALLOC (NULL);
  if (!b->conflicts)
    b->conflicts = BITMAP_ALLOC (NULL);
  bitmap_set_bit (a->conflicts, y);
  bitmap_set_bit (b->conflicts, x);
}

/* Check whether the decls associated with luid's X and Y conflict.  */

static bool
stack_var_conflict_p (size_t x, size_t y)
{
  struct stack_var *a = &stack_vars[x];
  struct stack_var *b = &stack_vars[y];
  if (!a->conflicts || !b->conflicts)
    return false;
  return bitmap_bit_p (a->conflicts, y);
}

/* Returns true if TYPE is or contains a union type.  */

static bool
aggregate_contains_union_type (tree type)
{
  tree field;

  if (TREE_CODE (type) == UNION_TYPE
      || TREE_CODE (type) == QUAL_UNION_TYPE)
    return true;
  if (TREE_CODE (type) == ARRAY_TYPE)
    return aggregate_contains_union_type (TREE_TYPE (type));
  if (TREE_CODE (type) != RECORD_TYPE)
    return false;

  for (field = TYPE_FIELDS (type); field; field = TREE_CHAIN (field))
    if (TREE_CODE (field) == FIELD_DECL)
      if (aggregate_contains_union_type (TREE_TYPE (field)))
	return true;

  return false;
}

/* A subroutine of expand_used_vars.  If two variables X and Y have alias
   sets that do not conflict, then do add a conflict for these variables
   in the interference graph.  We also need to make sure to add conflicts
   for union containing structures.  Else RTL alias analysis comes along
   and due to type based aliasing rules decides that for two overlapping
   union temporaries { short s; int i; } accesses to the same mem through
   different types may not alias and happily reorders stores across
   life-time boundaries of the temporaries (See PR25654).
   We also have to mind MEM_IN_STRUCT_P and MEM_SCALAR_P.  */

static void
add_alias_set_conflicts (void)
{
  size_t i, j, n = stack_vars_num;

  for (i = 0; i < n; ++i)
    {
      tree type_i = TREE_TYPE (stack_vars[i].decl);
      bool aggr_i = AGGREGATE_TYPE_P (type_i);
      bool contains_union;

      contains_union = aggregate_contains_union_type (type_i);
      for (j = 0; j < i; ++j)
	{
	  tree type_j = TREE_TYPE (stack_vars[j].decl);
	  bool aggr_j = AGGREGATE_TYPE_P (type_j);
	  if (aggr_i != aggr_j
	      /* Either the objects conflict by means of type based
		 aliasing rules, or we need to add a conflict.  */
	      || !objects_must_conflict_p (type_i, type_j)
	      /* In case the types do not conflict ensure that access
		 to elements will conflict.  In case of unions we have
		 to be careful as type based aliasing rules may say
		 access to the same memory does not conflict.  So play
		 safe and add a conflict in this case.  */
	      || contains_union)
	    add_stack_var_conflict (i, j);
	}
    }
}

/* A subroutine of partition_stack_vars.  A comparison function for qsort,
   sorting an array of indices by the size and type of the object.  */

static int
stack_var_size_cmp (const void *a, const void *b)
{
  HOST_WIDE_INT sa = stack_vars[*(const size_t *)a].size;
  HOST_WIDE_INT sb = stack_vars[*(const size_t *)b].size;
  tree decla, declb;
  unsigned int uida, uidb;

  if (sa < sb)
    return -1;
  if (sa > sb)
    return 1;
  decla = stack_vars[*(const size_t *)a].decl;
  declb = stack_vars[*(const size_t *)b].decl;
  /* For stack variables of the same size use and id of the decls
     to make the sort stable.  Two SSA names are compared by their
     version, SSA names come before non-SSA names, and two normal
     decls are compared by their DECL_UID.  */
  if (TREE_CODE (decla) == SSA_NAME)
    {
      if (TREE_CODE (declb) == SSA_NAME)
	uida = SSA_NAME_VERSION (decla), uidb = SSA_NAME_VERSION (declb);
      else
	return -1;
    }
  else if (TREE_CODE (declb) == SSA_NAME)
    return 1;
  else
    uida = DECL_UID (decla), uidb = DECL_UID (declb);
  if (uida < uidb)
    return -1;
  if (uida > uidb)
    return 1;
  return 0;
}


/* If the points-to solution *PI points to variables that are in a partition
   together with other variables add all partition members to the pointed-to
   variables bitmap.  */

static void
add_partitioned_vars_to_ptset (struct pt_solution *pt,
			       struct pointer_map_t *decls_to_partitions,
			       struct pointer_set_t *visited, bitmap temp)
{
  bitmap_iterator bi;
  unsigned i;
  bitmap *part;

  if (pt->anything
      || pt->vars == NULL
      /* The pointed-to vars bitmap is shared, it is enough to
	 visit it once.  */
      || pointer_set_insert(visited, pt->vars))
    return;

  bitmap_clear (temp);

  /* By using a temporary bitmap to store all members of the partitions
     we have to add we make sure to visit each of the partitions only
     once.  */
  EXECUTE_IF_SET_IN_BITMAP (pt->vars, 0, i, bi)
    if ((!temp
	 || !bitmap_bit_p (temp, i))
	&& (part = (bitmap *) pointer_map_contains (decls_to_partitions,
						    (void *)(size_t) i)))
      bitmap_ior_into (temp, *part);
  if (!bitmap_empty_p (temp))
    bitmap_ior_into (pt->vars, temp);
}

/* Update points-to sets based on partition info, so we can use them on RTL.
   The bitmaps representing stack partitions will be saved until expand,
   where partitioned decls used as bases in memory expressions will be
   rewritten.  */

static void
update_alias_info_with_stack_vars (void)
{
  struct pointer_map_t *decls_to_partitions = NULL;
  size_t i, j;
  tree var = NULL_TREE;

  for (i = 0; i < stack_vars_num; i++)
    {
      bitmap part = NULL;
      tree name;
      struct ptr_info_def *pi;

      /* Not interested in partitions with single variable.  */
      if (stack_vars[i].representative != i
          || stack_vars[i].next == EOC)
        continue;

      if (!decls_to_partitions)
	{
	  decls_to_partitions = pointer_map_create ();
	  cfun->gimple_df->decls_to_pointers = pointer_map_create ();
	}

      /* Create an SSA_NAME that points to the partition for use
         as base during alias-oracle queries on RTL for bases that
	 have been partitioned.  */
      if (var == NULL_TREE)
	var = create_tmp_var (ptr_type_node, NULL);
      name = make_ssa_name (var, NULL);

      /* Create bitmaps representing partitions.  They will be used for
         points-to sets later, so use GGC alloc.  */
      part = BITMAP_GGC_ALLOC ();
      for (j = i; j != EOC; j = stack_vars[j].next)
	{
	  tree decl = stack_vars[j].decl;
	  unsigned int uid = DECL_UID (decl);
	  /* We should never end up partitioning SSA names (though they
	     may end up on the stack).  Neither should we allocate stack
	     space to something that is unused and thus unreferenced.  */
	  gcc_assert (DECL_P (decl)
		      && referenced_var_lookup (uid));
	  bitmap_set_bit (part, uid);
	  *((bitmap *) pointer_map_insert (decls_to_partitions,
					   (void *)(size_t) uid)) = part;
	  *((tree *) pointer_map_insert (cfun->gimple_df->decls_to_pointers,
					 decl)) = name;
	}

      /* Make the SSA name point to all partition members.  */
      pi = get_ptr_info (name);
      pt_solution_set (&pi->pt, part);
    }

  /* Make all points-to sets that contain one member of a partition
     contain all members of the partition.  */
  if (decls_to_partitions)
    {
      unsigned i;
      struct pointer_set_t *visited = pointer_set_create ();
      bitmap temp = BITMAP_ALLOC (NULL);

      for (i = 1; i < num_ssa_names; i++)
	{
	  tree name = ssa_name (i);
	  struct ptr_info_def *pi;

	  if (name
	      && POINTER_TYPE_P (TREE_TYPE (name))
	      && ((pi = SSA_NAME_PTR_INFO (name)) != NULL))
	    add_partitioned_vars_to_ptset (&pi->pt, decls_to_partitions,
					   visited, temp);
	}

      add_partitioned_vars_to_ptset (&cfun->gimple_df->escaped,
				     decls_to_partitions, visited, temp);
      add_partitioned_vars_to_ptset (&cfun->gimple_df->callused,
				     decls_to_partitions, visited, temp);

      pointer_set_destroy (visited);
      pointer_map_destroy (decls_to_partitions);
      BITMAP_FREE (temp);
    }
}

/* A subroutine of partition_stack_vars.  The UNION portion of a UNION/FIND
   partitioning algorithm.  Partitions A and B are known to be non-conflicting.
   Merge them into a single partition A.

   At the same time, add OFFSET to all variables in partition B.  At the end
   of the partitioning process we've have a nice block easy to lay out within
   the stack frame.  */

static void
union_stack_vars (size_t a, size_t b, HOST_WIDE_INT offset)
{
  size_t i, last;
  struct stack_var *vb = &stack_vars[b];
  bitmap_iterator bi;
  unsigned u;

  /* Update each element of partition B with the given offset,
     and merge them into partition A.  */
  for (last = i = b; i != EOC; last = i, i = stack_vars[i].next)
    {
      stack_vars[i].offset += offset;
      stack_vars[i].representative = a;
    }
  stack_vars[last].next = stack_vars[a].next;
  stack_vars[a].next = b;

  /* Update the required alignment of partition A to account for B.  */
  if (stack_vars[a].alignb < stack_vars[b].alignb)
    stack_vars[a].alignb = stack_vars[b].alignb;

  /* Update the interference graph and merge the conflicts.  */
  if (vb->conflicts)
    {
      EXECUTE_IF_SET_IN_BITMAP (vb->conflicts, 0, u, bi)
	add_stack_var_conflict (a, stack_vars[u].representative);
      BITMAP_FREE (vb->conflicts);
    }
}

/* A subroutine of expand_used_vars.  Binpack the variables into
   partitions constrained by the interference graph.  The overall
   algorithm used is as follows:

	Sort the objects by size.
	For each object A {
	  S = size(A)
	  O = 0
	  loop {
	    Look for the largest non-conflicting object B with size <= S.
	    UNION (A, B)
	    offset(B) = O
	    O += size(B)
	    S -= size(B)
	  }
	}
*/

static void
partition_stack_vars (void)
{
  size_t si, sj, n = stack_vars_num;

  stack_vars_sorted = XNEWVEC (size_t, stack_vars_num);
  for (si = 0; si < n; ++si)
    stack_vars_sorted[si] = si;

  if (n == 1)
    return;

  qsort (stack_vars_sorted, n, sizeof (size_t), stack_var_size_cmp);

  for (si = 0; si < n; ++si)
    {
      size_t i = stack_vars_sorted[si];
      HOST_WIDE_INT isize = stack_vars[i].size;
      HOST_WIDE_INT offset = 0;

      for (sj = si; sj-- > 0; )
	{
	  size_t j = stack_vars_sorted[sj];
	  HOST_WIDE_INT jsize = stack_vars[j].size;
	  unsigned int jalign = stack_vars[j].alignb;

	  /* Ignore objects that aren't partition representatives.  */
	  if (stack_vars[j].representative != j)
	    continue;

	  /* Ignore objects too large for the remaining space.  */
	  if (isize < jsize)
	    continue;

	  /* Ignore conflicting objects.  */
	  if (stack_var_conflict_p (i, j))
	    continue;

	  /* Refine the remaining space check to include alignment.  */
	  if (offset & (jalign - 1))
	    {
	      HOST_WIDE_INT toff = offset;
	      toff += jalign - 1;
	      toff &= -(HOST_WIDE_INT)jalign;
	      if (isize - (toff - offset) < jsize)
		continue;

	      isize -= toff - offset;
	      offset = toff;
	    }

	  /* UNION the objects, placing J at OFFSET.  */
	  union_stack_vars (i, j, offset);

	  isize -= jsize;
	  if (isize == 0)
	    break;
	}
    }

  if (optimize)
    update_alias_info_with_stack_vars ();
}

/* A debugging aid for expand_used_vars.  Dump the generated partitions.  */

static void
dump_stack_var_partition (void)
{
  size_t si, i, j, n = stack_vars_num;

  for (si = 0; si < n; ++si)
    {
      i = stack_vars_sorted[si];

      /* Skip variables that aren't partition representatives, for now.  */
      if (stack_vars[i].representative != i)
	continue;

      fprintf (dump_file, "Partition %lu: size " HOST_WIDE_INT_PRINT_DEC
	       " align %u\n", (unsigned long) i, stack_vars[i].size,
	       stack_vars[i].alignb);

      for (j = i; j != EOC; j = stack_vars[j].next)
	{
	  fputc ('\t', dump_file);
	  print_generic_expr (dump_file, stack_vars[j].decl, dump_flags);
	  fprintf (dump_file, ", offset " HOST_WIDE_INT_PRINT_DEC "\n",
		   stack_vars[j].offset);
	}
    }
}

/* Assign rtl to DECL at frame offset OFFSET.  */

static void
expand_one_stack_var_at (tree decl, HOST_WIDE_INT offset)
{
  /* Alignment is unsigned.   */
  unsigned HOST_WIDE_INT align, max_align;
  rtx x;

  /* If this fails, we've overflowed the stack frame.  Error nicely?  */
  gcc_assert (offset == trunc_int_for_mode (offset, Pmode));

  x = plus_constant (virtual_stack_vars_rtx, offset);
  x = gen_rtx_MEM (DECL_MODE (SSAVAR (decl)), x);

  if (TREE_CODE (decl) != SSA_NAME)
    {
      /* Set alignment we actually gave this decl if it isn't an SSA name.
         If it is we generate stack slots only accidentally so it isn't as
	 important, we'll simply use the alignment that is already set.  */
      offset -= frame_phase;
      align = offset & -offset;
      align *= BITS_PER_UNIT;
      max_align = crtl->max_used_stack_slot_alignment;
      if (align == 0 || align > max_align)
	align = max_align;

      DECL_ALIGN (decl) = align;
      DECL_USER_ALIGN (decl) = 0;
    }

  set_mem_attributes (x, SSAVAR (decl), true);
  set_rtl (decl, x);
}

/* A subroutine of expand_used_vars.  Give each partition representative
   a unique location within the stack frame.  Update each partition member
   with that location.  */

static void
expand_stack_vars (bool (*pred) (tree))
{
  size_t si, i, j, n = stack_vars_num;

  for (si = 0; si < n; ++si)
    {
      HOST_WIDE_INT offset;

      i = stack_vars_sorted[si];

      /* Skip variables that aren't partition representatives, for now.  */
      if (stack_vars[i].representative != i)
	continue;

      /* Skip variables that have already had rtl assigned.  See also
	 add_stack_var where we perpetrate this pc_rtx hack.  */
      if ((TREE_CODE (stack_vars[i].decl) == SSA_NAME
	   ? SA.partition_to_pseudo[var_to_partition (SA.map, stack_vars[i].decl)]
	   : DECL_RTL (stack_vars[i].decl)) != pc_rtx)
	continue;

      /* Check the predicate to see whether this variable should be
	 allocated in this pass.  */
      if (pred && !pred (stack_vars[i].decl))
	continue;

      offset = alloc_stack_frame_space (stack_vars[i].size,
					stack_vars[i].alignb);

      /* Create rtl for each variable based on their location within the
	 partition.  */
      for (j = i; j != EOC; j = stack_vars[j].next)
	{
	  gcc_assert (stack_vars[j].offset <= stack_vars[i].size);
	  expand_one_stack_var_at (stack_vars[j].decl,
				   stack_vars[j].offset + offset);
	}
    }
}

/* Take into account all sizes of partitions and reset DECL_RTLs.  */
static HOST_WIDE_INT
account_stack_vars (void)
{
  size_t si, j, i, n = stack_vars_num;
  HOST_WIDE_INT size = 0;

  for (si = 0; si < n; ++si)
    {
      i = stack_vars_sorted[si];

      /* Skip variables that aren't partition representatives, for now.  */
      if (stack_vars[i].representative != i)
	continue;

      size += stack_vars[i].size;
      for (j = i; j != EOC; j = stack_vars[j].next)
	set_rtl (stack_vars[j].decl, NULL);
    }
  return size;
}

/* A subroutine of expand_one_var.  Called to immediately assign rtl
   to a variable to be allocated in the stack frame.  */

static void
expand_one_stack_var (tree var)
{
  HOST_WIDE_INT size, offset, align;

  size = tree_low_cst (DECL_SIZE_UNIT (SSAVAR (var)), 1);
  align = get_decl_align_unit (SSAVAR (var));
  offset = alloc_stack_frame_space (size, align);

  expand_one_stack_var_at (var, offset);
}

/* A subroutine of expand_one_var.  Called to assign rtl to a VAR_DECL
   that will reside in a hard register.  */

static void
expand_one_hard_reg_var (tree var)
{
  rest_of_decl_compilation (var, 0, 0);
}

/* A subroutine of expand_one_var.  Called to assign rtl to a VAR_DECL
   that will reside in a pseudo register.  */

static void
expand_one_register_var (tree var)
{
  tree decl = SSAVAR (var);
  tree type = TREE_TYPE (decl);
  enum machine_mode reg_mode = promote_decl_mode (decl, NULL);
  rtx x = gen_reg_rtx (reg_mode);

  set_rtl (var, x);

  /* Note if the object is a user variable.  */
  if (!DECL_ARTIFICIAL (decl))
    mark_user_reg (x);

  if (POINTER_TYPE_P (type))
    mark_reg_pointer (x, TYPE_ALIGN (TREE_TYPE (type)));
}

/* A subroutine of expand_one_var.  Called to assign rtl to a VAR_DECL that
   has some associated error, e.g. its type is error-mark.  We just need
   to pick something that won't crash the rest of the compiler.  */

static void
expand_one_error_var (tree var)
{
  enum machine_mode mode = DECL_MODE (var);
  rtx x;

  if (mode == BLKmode)
    x = gen_rtx_MEM (BLKmode, const0_rtx);
  else if (mode == VOIDmode)
    x = const0_rtx;
  else
    x = gen_reg_rtx (mode);

  SET_DECL_RTL (var, x);
}

/* A subroutine of expand_one_var.  VAR is a variable that will be
   allocated to the local stack frame.  Return true if we wish to
   add VAR to STACK_VARS so that it will be coalesced with other
   variables.  Return false to allocate VAR immediately.

   This function is used to reduce the number of variables considered
   for coalescing, which reduces the size of the quadratic problem.  */

static bool
defer_stack_allocation (tree var, bool toplevel)
{
  /* If stack protection is enabled, *all* stack variables must be deferred,
     so that we can re-order the strings to the top of the frame.  */
  if (flag_stack_protect)
    return true;

  /* Variables in the outermost scope automatically conflict with
     every other variable.  The only reason to want to defer them
     at all is that, after sorting, we can more efficiently pack
     small variables in the stack frame.  Continue to defer at -O2.  */
  if (toplevel && optimize < 2)
    return false;

  /* Without optimization, *most* variables are allocated from the
     stack, which makes the quadratic problem large exactly when we
     want compilation to proceed as quickly as possible.  On the
     other hand, we don't want the function's stack frame size to
     get completely out of hand.  So we avoid adding scalars and
     "small" aggregates to the list at all.  */
  if (optimize == 0 && tree_low_cst (DECL_SIZE_UNIT (var), 1) < 32)
    return false;

  return true;
}

/* A subroutine of expand_used_vars.  Expand one variable according to
   its flavor.  Variables to be placed on the stack are not actually
   expanded yet, merely recorded.
   When REALLY_EXPAND is false, only add stack values to be allocated.
   Return stack usage this variable is supposed to take.
*/

static HOST_WIDE_INT
expand_one_var (tree var, bool toplevel, bool really_expand)
{
  tree origvar = var;
  var = SSAVAR (var);

  if (SUPPORTS_STACK_ALIGNMENT
      && TREE_TYPE (var) != error_mark_node
      && TREE_CODE (var) == VAR_DECL)
    {
      unsigned int align;

      /* Because we don't know if VAR will be in register or on stack,
	 we conservatively assume it will be on stack even if VAR is
	 eventually put into register after RA pass.  For non-automatic
	 variables, which won't be on stack, we collect alignment of
	 type and ignore user specified alignment.  */
      if (TREE_STATIC (var) || DECL_EXTERNAL (var))
	align = MINIMUM_ALIGNMENT (TREE_TYPE (var),
				   TYPE_MODE (TREE_TYPE (var)),
				   TYPE_ALIGN (TREE_TYPE (var)));
      else
	align = MINIMUM_ALIGNMENT (var, DECL_MODE (var), DECL_ALIGN (var));

      if (crtl->stack_alignment_estimated < align)
        {
          /* stack_alignment_estimated shouldn't change after stack
             realign decision made */
          gcc_assert(!crtl->stack_realign_processed);
	  crtl->stack_alignment_estimated = align;
	}
    }

  if (TREE_CODE (origvar) == SSA_NAME)
    {
      gcc_assert (TREE_CODE (var) != VAR_DECL
		  || (!DECL_EXTERNAL (var)
		      && !DECL_HAS_VALUE_EXPR_P (var)
		      && !TREE_STATIC (var)
		      && TREE_TYPE (var) != error_mark_node
		      && !DECL_HARD_REGISTER (var)
		      && really_expand));
    }
  if (TREE_CODE (var) != VAR_DECL && TREE_CODE (origvar) != SSA_NAME)
    ;
  else if (DECL_EXTERNAL (var))
    ;
  else if (DECL_HAS_VALUE_EXPR_P (var))
    ;
  else if (TREE_STATIC (var))
    ;
  else if (TREE_CODE (origvar) != SSA_NAME && DECL_RTL_SET_P (var))
    ;
  else if (TREE_TYPE (var) == error_mark_node)
    {
      if (really_expand)
        expand_one_error_var (var);
    }
  else if (TREE_CODE (var) == VAR_DECL && DECL_HARD_REGISTER (var))
    {
      if (really_expand)
        expand_one_hard_reg_var (var);
    }
  else if (use_register_for_decl (var))
    {
      if (really_expand)
        expand_one_register_var (origvar);
    }
  else if (!host_integerp (DECL_SIZE_UNIT (var), 1))
    {
      if (really_expand)
	{
	  error ("size of variable %q+D is too large", var);
	  expand_one_error_var (var);
	}
    }
  else if (defer_stack_allocation (var, toplevel))
    add_stack_var (origvar);
  else
    {
      if (really_expand)
        expand_one_stack_var (origvar);
      return tree_low_cst (DECL_SIZE_UNIT (var), 1);
    }
  return 0;
}

/* A subroutine of expand_used_vars.  Walk down through the BLOCK tree
   expanding variables.  Those variables that can be put into registers
   are allocated pseudos; those that can't are put on the stack.

   TOPLEVEL is true if this is the outermost BLOCK.  */

static void
expand_used_vars_for_block (tree block, bool toplevel)
{
  size_t i, j, old_sv_num, this_sv_num, new_sv_num;
  tree t;

  old_sv_num = toplevel ? 0 : stack_vars_num;

  /* Expand all variables at this level.  */
  for (t = BLOCK_VARS (block); t ; t = TREE_CHAIN (t))
    if (TREE_USED (t))
      expand_one_var (t, toplevel, true);

  this_sv_num = stack_vars_num;

  /* Expand all variables at containing levels.  */
  for (t = BLOCK_SUBBLOCKS (block); t ; t = BLOCK_CHAIN (t))
    expand_used_vars_for_block (t, false);

  /* Since we do not track exact variable lifetimes (which is not even
     possible for variables whose address escapes), we mirror the block
     tree in the interference graph.  Here we cause all variables at this
     level, and all sublevels, to conflict.  */
  if (old_sv_num < this_sv_num)
    {
      new_sv_num = stack_vars_num;

      for (i = old_sv_num; i < new_sv_num; ++i)
	for (j = i < this_sv_num ? i : this_sv_num; j-- > old_sv_num ;)
	  add_stack_var_conflict (i, j);
    }
}

/* A subroutine of expand_used_vars.  Walk down through the BLOCK tree
   and clear TREE_USED on all local variables.  */

static void
clear_tree_used (tree block)
{
  tree t;

  for (t = BLOCK_VARS (block); t ; t = TREE_CHAIN (t))
    /* if (!TREE_STATIC (t) && !DECL_EXTERNAL (t)) */
      TREE_USED (t) = 0;

  for (t = BLOCK_SUBBLOCKS (block); t ; t = BLOCK_CHAIN (t))
    clear_tree_used (t);
}

/* Examine TYPE and determine a bit mask of the following features.  */

#define SPCT_HAS_LARGE_CHAR_ARRAY	1
#define SPCT_HAS_SMALL_CHAR_ARRAY	2
#define SPCT_HAS_ARRAY			4
#define SPCT_HAS_AGGREGATE		8

static unsigned int
stack_protect_classify_type (tree type)
{
  unsigned int ret = 0;
  tree t;

  switch (TREE_CODE (type))
    {
    case ARRAY_TYPE:
      t = TYPE_MAIN_VARIANT (TREE_TYPE (type));
      if (t == char_type_node
	  || t == signed_char_type_node
	  || t == unsigned_char_type_node)
	{
	  unsigned HOST_WIDE_INT max = PARAM_VALUE (PARAM_SSP_BUFFER_SIZE);
	  unsigned HOST_WIDE_INT len;

	  if (!TYPE_SIZE_UNIT (type)
	      || !host_integerp (TYPE_SIZE_UNIT (type), 1))
	    len = max;
	  else
	    len = tree_low_cst (TYPE_SIZE_UNIT (type), 1);

	  if (len == 0)
	    ret = SPCT_HAS_ARRAY;
	  else if (len < max)
	    ret = SPCT_HAS_SMALL_CHAR_ARRAY | SPCT_HAS_ARRAY;
	  else
	    ret = SPCT_HAS_LARGE_CHAR_ARRAY | SPCT_HAS_ARRAY;
	}
      else
	ret = SPCT_HAS_ARRAY;
      break;

    case UNION_TYPE:
    case QUAL_UNION_TYPE:
    case RECORD_TYPE:
      ret = SPCT_HAS_AGGREGATE;
      for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
	if (TREE_CODE (t) == FIELD_DECL)
	  ret |= stack_protect_classify_type (TREE_TYPE (t));
      break;

    default:
      break;
    }

  return ret;
}

/* Return nonzero if DECL should be segregated into the "vulnerable" upper
   part of the local stack frame.  Remember if we ever return nonzero for
   any variable in this function.  The return value is the phase number in
   which the variable should be allocated.  */

static int
stack_protect_decl_phase (tree decl)
{
  unsigned int bits = stack_protect_classify_type (TREE_TYPE (decl));
  int ret = 0;

  if (bits & SPCT_HAS_SMALL_CHAR_ARRAY)
    has_short_buffer = true;

  if (flag_stack_protect == 2)
    {
      if ((bits & (SPCT_HAS_SMALL_CHAR_ARRAY | SPCT_HAS_LARGE_CHAR_ARRAY))
	  && !(bits & SPCT_HAS_AGGREGATE))
	ret = 1;
      else if (bits & SPCT_HAS_ARRAY)
	ret = 2;
    }
  else
    ret = (bits & SPCT_HAS_LARGE_CHAR_ARRAY) != 0;

  if (ret)
    has_protected_decls = true;

  return ret;
}

/* Two helper routines that check for phase 1 and phase 2.  These are used
   as callbacks for expand_stack_vars.  */

static bool
stack_protect_decl_phase_1 (tree decl)
{
  return stack_protect_decl_phase (decl) == 1;
}

static bool
stack_protect_decl_phase_2 (tree decl)
{
  return stack_protect_decl_phase (decl) == 2;
}

/* Ensure that variables in different stack protection phases conflict
   so that they are not merged and share the same stack slot.  */

static void
add_stack_protection_conflicts (void)
{
  size_t i, j, n = stack_vars_num;
  unsigned char *phase;

  phase = XNEWVEC (unsigned char, n);
  for (i = 0; i < n; ++i)
    phase[i] = stack_protect_decl_phase (stack_vars[i].decl);

  for (i = 0; i < n; ++i)
    {
      unsigned char ph_i = phase[i];
      for (j = 0; j < i; ++j)
	if (ph_i != phase[j])
	  add_stack_var_conflict (i, j);
    }

  XDELETEVEC (phase);
}

/* Create a decl for the guard at the top of the stack frame.  */

static void
create_stack_guard (void)
{
  tree guard = build_decl (DECL_SOURCE_LOCATION (current_function_decl),
			   VAR_DECL, NULL, ptr_type_node);
  TREE_THIS_VOLATILE (guard) = 1;
  TREE_USED (guard) = 1;
  expand_one_stack_var (guard);
  crtl->stack_protect_guard = guard;
}

/* A subroutine of expand_used_vars.  Walk down through the BLOCK tree
   expanding variables.  Those variables that can be put into registers
   are allocated pseudos; those that can't are put on the stack.

   TOPLEVEL is true if this is the outermost BLOCK.  */

static HOST_WIDE_INT
account_used_vars_for_block (tree block, bool toplevel)
{
  tree t;
  HOST_WIDE_INT size = 0;

  /* Expand all variables at this level.  */
  for (t = BLOCK_VARS (block); t ; t = TREE_CHAIN (t))
    if (TREE_USED (t))
      size += expand_one_var (t, toplevel, false);

  /* Expand all variables at containing levels.  */
  for (t = BLOCK_SUBBLOCKS (block); t ; t = BLOCK_CHAIN (t))
    size += account_used_vars_for_block (t, false);

  return size;
}

/* Prepare for expanding variables.  */
static void
init_vars_expansion (void)
{
  tree t;
  /* Set TREE_USED on all variables in the local_decls.  */
  for (t = cfun->local_decls; t; t = TREE_CHAIN (t))
    TREE_USED (TREE_VALUE (t)) = 1;

  /* Clear TREE_USED on all variables associated with a block scope.  */
  clear_tree_used (DECL_INITIAL (current_function_decl));

  /* Initialize local stack smashing state.  */
  has_protected_decls = false;
  has_short_buffer = false;
}

/* Free up stack variable graph data.  */
static void
fini_vars_expansion (void)
{
  size_t i, n = stack_vars_num;
  for (i = 0; i < n; i++)
    BITMAP_FREE (stack_vars[i].conflicts);
  XDELETEVEC (stack_vars);
  XDELETEVEC (stack_vars_sorted);
  stack_vars = NULL;
  stack_vars_alloc = stack_vars_num = 0;
}

/* Make a fair guess for the size of the stack frame of the current
   function.  This doesn't have to be exact, the result is only used
   in the inline heuristics.  So we don't want to run the full stack
   var packing algorithm (which is quadratic in the number of stack
   vars).  Instead, we calculate the total size of all stack vars.
   This turns out to be a pretty fair estimate -- packing of stack
   vars doesn't happen very often.  */

HOST_WIDE_INT
estimated_stack_frame_size (void)
{
  HOST_WIDE_INT size = 0;
  size_t i;
  tree t, outer_block = DECL_INITIAL (current_function_decl);

  init_vars_expansion ();

  for (t = cfun->local_decls; t; t = TREE_CHAIN (t))
    {
      tree var = TREE_VALUE (t);

      if (TREE_USED (var))
        size += expand_one_var (var, true, false);
      TREE_USED (var) = 1;
    }
  size += account_used_vars_for_block (outer_block, true);

  if (stack_vars_num > 0)
    {
      /* Fake sorting the stack vars for account_stack_vars ().  */
      stack_vars_sorted = XNEWVEC (size_t, stack_vars_num);
      for (i = 0; i < stack_vars_num; ++i)
	stack_vars_sorted[i] = i;
      size += account_stack_vars ();
      fini_vars_expansion ();
    }

  return size;
}

/* Expand all variables used in the function.  */

static void
expand_used_vars (void)
{
  tree t, next, outer_block = DECL_INITIAL (current_function_decl);
  tree maybe_local_decls = NULL_TREE;
  unsigned i;

  /* Compute the phase of the stack frame for this function.  */
  {
    int align = PREFERRED_STACK_BOUNDARY / BITS_PER_UNIT;
    int off = STARTING_FRAME_OFFSET % align;
    frame_phase = off ? align - off : 0;
  }

  init_vars_expansion ();

  for (i = 0; i < SA.map->num_partitions; i++)
    {
      tree var = partition_to_var (SA.map, i);

      gcc_assert (is_gimple_reg (var));
      if (TREE_CODE (SSA_NAME_VAR (var)) == VAR_DECL)
	expand_one_var (var, true, true);
      else
	{
	  /* This is a PARM_DECL or RESULT_DECL.  For those partitions that
	     contain the default def (representing the parm or result itself)
	     we don't do anything here.  But those which don't contain the
	     default def (representing a temporary based on the parm/result)
	     we need to allocate space just like for normal VAR_DECLs.  */
	  if (!bitmap_bit_p (SA.partition_has_default_def, i))
	    {
	      expand_one_var (var, true, true);
	      gcc_assert (SA.partition_to_pseudo[i]);
	    }
	}
    }

  /* At this point all variables on the local_decls with TREE_USED
     set are not associated with any block scope.  Lay them out.  */
  t = cfun->local_decls;
  cfun->local_decls = NULL_TREE;
  for (; t; t = next)
    {
      tree var = TREE_VALUE (t);
      bool expand_now = false;

      next = TREE_CHAIN (t);

      /* Expanded above already.  */
      if (is_gimple_reg (var))
	{
	  TREE_USED (var) = 0;
	  goto next;
	}
      /* We didn't set a block for static or extern because it's hard
	 to tell the difference between a global variable (re)declared
	 in a local scope, and one that's really declared there to
	 begin with.  And it doesn't really matter much, since we're
	 not giving them stack space.  Expand them now.  */
      else if (TREE_STATIC (var) || DECL_EXTERNAL (var))
	expand_now = true;

      /* If the variable is not associated with any block, then it
	 was created by the optimizers, and could be live anywhere
	 in the function.  */
      else if (TREE_USED (var))
	expand_now = true;

      /* Finally, mark all variables on the list as used.  We'll use
	 this in a moment when we expand those associated with scopes.  */
      TREE_USED (var) = 1;

      if (expand_now)
	expand_one_var (var, true, true);

    next:
      if (DECL_ARTIFICIAL (var) && !DECL_IGNORED_P (var))
	{
	  rtx rtl = DECL_RTL_IF_SET (var);

	  /* Keep artificial non-ignored vars in cfun->local_decls
	     chain until instantiate_decls.  */
	  if (rtl && (MEM_P (rtl) || GET_CODE (rtl) == CONCAT))
	    {
	      TREE_CHAIN (t) = cfun->local_decls;
	      cfun->local_decls = t;
	      continue;
	    }
	  else if (rtl == NULL_RTX)
	    {
	      /* If rtl isn't set yet, which can happen e.g. with
		 -fstack-protector, retry before returning from this
		 function.  */
	      TREE_CHAIN (t) = maybe_local_decls;
	      maybe_local_decls = t;
	      continue;
	    }
	}

      ggc_free (t);
    }

  /* At this point, all variables within the block tree with TREE_USED
     set are actually used by the optimized function.  Lay them out.  */
  expand_used_vars_for_block (outer_block, true);

  if (stack_vars_num > 0)
    {
      /* Due to the way alias sets work, no variables with non-conflicting
	 alias sets may be assigned the same address.  Add conflicts to
	 reflect this.  */
      add_alias_set_conflicts ();

      /* If stack protection is enabled, we don't share space between
	 vulnerable data and non-vulnerable data.  */
      if (flag_stack_protect)
	add_stack_protection_conflicts ();

      /* Now that we have collected all stack variables, and have computed a
	 minimal interference graph, attempt to save some stack space.  */
      partition_stack_vars ();
      if (dump_file)
	dump_stack_var_partition ();
    }

  /* There are several conditions under which we should create a
     stack guard: protect-all, alloca used, protected decls present.  */
  if (flag_stack_protect == 2
      || (flag_stack_protect
	  && (cfun->calls_alloca || has_protected_decls)))
    create_stack_guard ();

  /* Assign rtl to each variable based on these partitions.  */
  if (stack_vars_num > 0)
    {
      /* Reorder decls to be protected by iterating over the variables
	 array multiple times, and allocating out of each phase in turn.  */
      /* ??? We could probably integrate this into the qsort we did
	 earlier, such that we naturally see these variables first,
	 and thus naturally allocate things in the right order.  */
      if (has_protected_decls)
	{
	  /* Phase 1 contains only character arrays.  */
	  expand_stack_vars (stack_protect_decl_phase_1);

	  /* Phase 2 contains other kinds of arrays.  */
	  if (flag_stack_protect == 2)
	    expand_stack_vars (stack_protect_decl_phase_2);
	}

      expand_stack_vars (NULL);

      fini_vars_expansion ();
    }

  /* If there were any artificial non-ignored vars without rtl
     found earlier, see if deferred stack allocation hasn't assigned
     rtl to them.  */
  for (t = maybe_local_decls; t; t = next)
    {
      tree var = TREE_VALUE (t);
      rtx rtl = DECL_RTL_IF_SET (var);

      next = TREE_CHAIN (t);

      /* Keep artificial non-ignored vars in cfun->local_decls
	 chain until instantiate_decls.  */
      if (rtl && (MEM_P (rtl) || GET_CODE (rtl) == CONCAT))
	{
	  TREE_CHAIN (t) = cfun->local_decls;
	  cfun->local_decls = t;
	  continue;
	}

      ggc_free (t);
    }

  /* If the target requires that FRAME_OFFSET be aligned, do it.  */
  if (STACK_ALIGNMENT_NEEDED)
    {
      HOST_WIDE_INT align = PREFERRED_STACK_BOUNDARY / BITS_PER_UNIT;
      if (!FRAME_GROWS_DOWNWARD)
	frame_offset += align - 1;
      frame_offset &= -align;
    }
}


/* If we need to produce a detailed dump, print the tree representation
   for STMT to the dump file.  SINCE is the last RTX after which the RTL
   generated for STMT should have been appended.  */

static void
maybe_dump_rtl_for_gimple_stmt (gimple stmt, rtx since)
{
  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "\n;; ");
      print_gimple_stmt (dump_file, stmt, 0,
			 TDF_SLIM | (dump_flags & TDF_LINENO));
      fprintf (dump_file, "\n");

      print_rtl (dump_file, since ? NEXT_INSN (since) : since);
    }
}

/* Maps the blocks that do not contain tree labels to rtx labels.  */

static struct pointer_map_t *lab_rtx_for_bb;

/* Returns the label_rtx expression for a label starting basic block BB.  */

static rtx
label_rtx_for_bb (basic_block bb ATTRIBUTE_UNUSED)
{
  gimple_stmt_iterator gsi;
  tree lab;
  gimple lab_stmt;
  void **elt;

  if (bb->flags & BB_RTL)
    return block_label (bb);

  elt = pointer_map_contains (lab_rtx_for_bb, bb);
  if (elt)
    return (rtx) *elt;

  /* Find the tree label if it is present.  */

  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      lab_stmt = gsi_stmt (gsi);
      if (gimple_code (lab_stmt) != GIMPLE_LABEL)
	break;

      lab = gimple_label_label (lab_stmt);
      if (DECL_NONLOCAL (lab))
	break;

      return label_rtx (lab);
    }

  elt = pointer_map_insert (lab_rtx_for_bb, bb);
  *elt = gen_label_rtx ();
  return (rtx) *elt;
}


/* A subroutine of expand_gimple_cond.  Given E, a fallthrough edge
   of a basic block where we just expanded the conditional at the end,
   possibly clean up the CFG and instruction sequence.  LAST is the
   last instruction before the just emitted jump sequence.  */

static void
maybe_cleanup_end_of_block (edge e, rtx last)
{
  /* Special case: when jumpif decides that the condition is
     trivial it emits an unconditional jump (and the necessary
     barrier).  But we still have two edges, the fallthru one is
     wrong.  purge_dead_edges would clean this up later.  Unfortunately
     we have to insert insns (and split edges) before
     find_many_sub_basic_blocks and hence before purge_dead_edges.
     But splitting edges might create new blocks which depend on the
     fact that if there are two edges there's no barrier.  So the
     barrier would get lost and verify_flow_info would ICE.  Instead
     of auditing all edge splitters to care for the barrier (which
     normally isn't there in a cleaned CFG), fix it here.  */
  if (BARRIER_P (get_last_insn ()))
    {
      rtx insn;
      remove_edge (e);
      /* Now, we have a single successor block, if we have insns to
	 insert on the remaining edge we potentially will insert
	 it at the end of this block (if the dest block isn't feasible)
	 in order to avoid splitting the edge.  This insertion will take
	 place in front of the last jump.  But we might have emitted
	 multiple jumps (conditional and one unconditional) to the
	 same destination.  Inserting in front of the last one then
	 is a problem.  See PR 40021.  We fix this by deleting all
	 jumps except the last unconditional one.  */
      insn = PREV_INSN (get_last_insn ());
      /* Make sure we have an unconditional jump.  Otherwise we're
	 confused.  */
      gcc_assert (JUMP_P (insn) && !any_condjump_p (insn));
      for (insn = PREV_INSN (insn); insn != last;)
	{
	  insn = PREV_INSN (insn);
	  if (JUMP_P (NEXT_INSN (insn)))
	    {
	      if (!any_condjump_p (NEXT_INSN (insn)))
		{
		  gcc_assert (BARRIER_P (NEXT_INSN (NEXT_INSN (insn))));
		  delete_insn (NEXT_INSN (NEXT_INSN (insn)));
		}
	      delete_insn (NEXT_INSN (insn));
	    }
	}
    }
}

/* A subroutine of expand_gimple_basic_block.  Expand one GIMPLE_COND.
   Returns a new basic block if we've terminated the current basic
   block and created a new one.  */

static basic_block
expand_gimple_cond (basic_block bb, gimple stmt)
{
  basic_block new_bb, dest;
  edge new_edge;
  edge true_edge;
  edge false_edge;
  rtx last2, last;
  enum tree_code code;
  tree op0, op1;

  code = gimple_cond_code (stmt);
  op0 = gimple_cond_lhs (stmt);
  op1 = gimple_cond_rhs (stmt);
  /* We're sometimes presented with such code:
       D.123_1 = x < y;
       if (D.123_1 != 0)
         ...
     This would expand to two comparisons which then later might
     be cleaned up by combine.  But some pattern matchers like if-conversion
     work better when there's only one compare, so make up for this
     here as special exception if TER would have made the same change.  */
  if (gimple_cond_single_var_p (stmt)
      && SA.values
      && TREE_CODE (op0) == SSA_NAME
      && bitmap_bit_p (SA.values, SSA_NAME_VERSION (op0)))
    {
      gimple second = SSA_NAME_DEF_STMT (op0);
      if (gimple_code (second) == GIMPLE_ASSIGN)
	{
	  enum tree_code code2 = gimple_assign_rhs_code (second);
	  if (TREE_CODE_CLASS (code2) == tcc_comparison)
	    {
	      code = code2;
	      op0 = gimple_assign_rhs1 (second);
	      op1 = gimple_assign_rhs2 (second);
	    }
	  /* If jumps are cheap turn some more codes into
	     jumpy sequences.  */
	  else if (BRANCH_COST (optimize_insn_for_speed_p (), false) < 4)
	    {
	      if ((code2 == BIT_AND_EXPR
		   && TYPE_PRECISION (TREE_TYPE (op0)) == 1
		   && TREE_CODE (gimple_assign_rhs2 (second)) != INTEGER_CST)
		  || code2 == TRUTH_AND_EXPR)
		{
		  code = TRUTH_ANDIF_EXPR;
		  op0 = gimple_assign_rhs1 (second);
		  op1 = gimple_assign_rhs2 (second);
		}
	      else if (code2 == BIT_IOR_EXPR || code2 == TRUTH_OR_EXPR)
		{
		  code = TRUTH_ORIF_EXPR;
		  op0 = gimple_assign_rhs1 (second);
		  op1 = gimple_assign_rhs2 (second);
		}
	    }
	}
    }

  last2 = last = get_last_insn ();

  extract_true_false_edges_from_block (bb, &true_edge, &false_edge);
  if (gimple_has_location (stmt))
    {
      set_curr_insn_source_location (gimple_location (stmt));
      set_curr_insn_block (gimple_block (stmt));
    }

  /* These flags have no purpose in RTL land.  */
  true_edge->flags &= ~EDGE_TRUE_VALUE;
  false_edge->flags &= ~EDGE_FALSE_VALUE;

  /* We can either have a pure conditional jump with one fallthru edge or
     two-way jump that needs to be decomposed into two basic blocks.  */
  if (false_edge->dest == bb->next_bb)
    {
      jumpif_1 (code, op0, op1, label_rtx_for_bb (true_edge->dest),
		true_edge->probability);
      maybe_dump_rtl_for_gimple_stmt (stmt, last);
      if (true_edge->goto_locus)
	{
	  set_curr_insn_source_location (true_edge->goto_locus);
	  set_curr_insn_block (true_edge->goto_block);
	  true_edge->goto_locus = curr_insn_locator ();
	}
      true_edge->goto_block = NULL;
      false_edge->flags |= EDGE_FALLTHRU;
      maybe_cleanup_end_of_block (false_edge, last);
      return NULL;
    }
  if (true_edge->dest == bb->next_bb)
    {
      jumpifnot_1 (code, op0, op1, label_rtx_for_bb (false_edge->dest),
		   false_edge->probability);
      maybe_dump_rtl_for_gimple_stmt (stmt, last);
      if (false_edge->goto_locus)
	{
	  set_curr_insn_source_location (false_edge->goto_locus);
	  set_curr_insn_block (false_edge->goto_block);
	  false_edge->goto_locus = curr_insn_locator ();
	}
      false_edge->goto_block = NULL;
      true_edge->flags |= EDGE_FALLTHRU;
      maybe_cleanup_end_of_block (true_edge, last);
      return NULL;
    }

  jumpif_1 (code, op0, op1, label_rtx_for_bb (true_edge->dest),
	    true_edge->probability);
  last = get_last_insn ();
  if (false_edge->goto_locus)
    {
      set_curr_insn_source_location (false_edge->goto_locus);
      set_curr_insn_block (false_edge->goto_block);
      false_edge->goto_locus = curr_insn_locator ();
    }
  false_edge->goto_block = NULL;
  emit_jump (label_rtx_for_bb (false_edge->dest));

  BB_END (bb) = last;
  if (BARRIER_P (BB_END (bb)))
    BB_END (bb) = PREV_INSN (BB_END (bb));
  update_bb_for_insn (bb);

  new_bb = create_basic_block (NEXT_INSN (last), get_last_insn (), bb);
  dest = false_edge->dest;
  redirect_edge_succ (false_edge, new_bb);
  false_edge->flags |= EDGE_FALLTHRU;
  new_bb->count = false_edge->count;
  new_bb->frequency = EDGE_FREQUENCY (false_edge);
  new_edge = make_edge (new_bb, dest, 0);
  new_edge->probability = REG_BR_PROB_BASE;
  new_edge->count = new_bb->count;
  if (BARRIER_P (BB_END (new_bb)))
    BB_END (new_bb) = PREV_INSN (BB_END (new_bb));
  update_bb_for_insn (new_bb);

  maybe_dump_rtl_for_gimple_stmt (stmt, last2);

  if (true_edge->goto_locus)
    {
      set_curr_insn_source_location (true_edge->goto_locus);
      set_curr_insn_block (true_edge->goto_block);
      true_edge->goto_locus = curr_insn_locator ();
    }
  true_edge->goto_block = NULL;

  return new_bb;
}

/* A subroutine of expand_gimple_stmt_1, expanding one GIMPLE_CALL
   statement STMT.  */

static void
expand_call_stmt (gimple stmt)
{
  tree exp;
  tree lhs = gimple_call_lhs (stmt);
  size_t i;
  bool builtin_p;
  tree decl;

  exp = build_vl_exp (CALL_EXPR, gimple_call_num_args (stmt) + 3);

  CALL_EXPR_FN (exp) = gimple_call_fn (stmt);
  decl = gimple_call_fndecl (stmt);
  builtin_p = decl && DECL_BUILT_IN (decl);

  TREE_TYPE (exp) = gimple_call_return_type (stmt);
  CALL_EXPR_STATIC_CHAIN (exp) = gimple_call_chain (stmt);

  for (i = 0; i < gimple_call_num_args (stmt); i++)
    {
      tree arg = gimple_call_arg (stmt, i);
      gimple def;
      /* TER addresses into arguments of builtin functions so we have a
	 chance to infer more correct alignment information.  See PR39954.  */
      if (builtin_p
	  && TREE_CODE (arg) == SSA_NAME
	  && (def = get_gimple_for_ssa_name (arg))
	  && gimple_assign_rhs_code (def) == ADDR_EXPR)
	arg = gimple_assign_rhs1 (def);
      CALL_EXPR_ARG (exp, i) = arg;
    }

  if (gimple_has_side_effects (stmt))
    TREE_SIDE_EFFECTS (exp) = 1;

  if (gimple_call_nothrow_p (stmt))
    TREE_NOTHROW (exp) = 1;

  CALL_EXPR_TAILCALL (exp) = gimple_call_tail_p (stmt);
  CALL_EXPR_RETURN_SLOT_OPT (exp) = gimple_call_return_slot_opt_p (stmt);
  CALL_FROM_THUNK_P (exp) = gimple_call_from_thunk_p (stmt);
  CALL_CANNOT_INLINE_P (exp) = gimple_call_cannot_inline_p (stmt);
  CALL_EXPR_VA_ARG_PACK (exp) = gimple_call_va_arg_pack_p (stmt);
  SET_EXPR_LOCATION (exp, gimple_location (stmt));
  TREE_BLOCK (exp) = gimple_block (stmt);

  if (lhs)
    expand_assignment (lhs, exp, false);
  else
    expand_expr_real_1 (exp, const0_rtx, VOIDmode, EXPAND_NORMAL, NULL);
}

/* A subroutine of expand_gimple_stmt, expanding one gimple statement
   STMT that doesn't require special handling for outgoing edges.  That
   is no tailcalls and no GIMPLE_COND.  */

static void
expand_gimple_stmt_1 (gimple stmt)
{
  tree op0;
  switch (gimple_code (stmt))
    {
    case GIMPLE_GOTO:
      op0 = gimple_goto_dest (stmt);
      if (TREE_CODE (op0) == LABEL_DECL)
	expand_goto (op0);
      else
	expand_computed_goto (op0);
      break;
    case GIMPLE_LABEL:
      expand_label (gimple_label_label (stmt));
      break;
    case GIMPLE_NOP:
    case GIMPLE_PREDICT:
      break;
    case GIMPLE_SWITCH:
      expand_case (stmt);
      break;
    case GIMPLE_ASM:
      expand_asm_stmt (stmt);
      break;
    case GIMPLE_CALL:
      expand_call_stmt (stmt);
      break;

    case GIMPLE_RETURN:
      op0 = gimple_return_retval (stmt);

      if (op0 && op0 != error_mark_node)
	{
	  tree result = DECL_RESULT (current_function_decl);

	  /* If we are not returning the current function's RESULT_DECL,
	     build an assignment to it.  */
	  if (op0 != result)
	    {
	      /* I believe that a function's RESULT_DECL is unique.  */
	      gcc_assert (TREE_CODE (op0) != RESULT_DECL);

	      /* ??? We'd like to use simply expand_assignment here,
	         but this fails if the value is of BLKmode but the return
		 decl is a register.  expand_return has special handling
		 for this combination, which eventually should move
		 to common code.  See comments there.  Until then, let's
		 build a modify expression :-/  */
	      op0 = build2 (MODIFY_EXPR, TREE_TYPE (result),
			    result, op0);
	    }
	}
      if (!op0)
	expand_null_return ();
      else
	expand_return (op0);
      break;

    case GIMPLE_ASSIGN:
      {
	tree lhs = gimple_assign_lhs (stmt);

	/* Tree expand used to fiddle with |= and &= of two bitfield
	   COMPONENT_REFs here.  This can't happen with gimple, the LHS
	   of binary assigns must be a gimple reg.  */

	if (TREE_CODE (lhs) != SSA_NAME
	    || get_gimple_rhs_class (gimple_expr_code (stmt))
	       == GIMPLE_SINGLE_RHS)
	  {
	    tree rhs = gimple_assign_rhs1 (stmt);
	    gcc_assert (get_gimple_rhs_class (gimple_expr_code (stmt))
			== GIMPLE_SINGLE_RHS);
	    if (gimple_has_location (stmt) && CAN_HAVE_LOCATION_P (rhs))
	      SET_EXPR_LOCATION (rhs, gimple_location (stmt));
	    expand_assignment (lhs, rhs,
			       gimple_assign_nontemporal_move_p (stmt));
	  }
	else
	  {
	    rtx target, temp;
	    bool nontemporal = gimple_assign_nontemporal_move_p (stmt);
	    struct separate_ops ops;
	    bool promoted = false;

	    target = expand_expr (lhs, NULL_RTX, VOIDmode, EXPAND_WRITE);
	    if (GET_CODE (target) == SUBREG && SUBREG_PROMOTED_VAR_P (target))
	      promoted = true;

	    ops.code = gimple_assign_rhs_code (stmt);
	    ops.type = TREE_TYPE (lhs);
	    switch (get_gimple_rhs_class (gimple_expr_code (stmt)))
	      {
		case GIMPLE_BINARY_RHS:
		  ops.op1 = gimple_assign_rhs2 (stmt);
		  /* Fallthru */
		case GIMPLE_UNARY_RHS:
		  ops.op0 = gimple_assign_rhs1 (stmt);
		  break;
		default:
		  gcc_unreachable ();
	      }
	    ops.location = gimple_location (stmt);

	    /* If we want to use a nontemporal store, force the value to
	       register first.  If we store into a promoted register,
	       don't directly expand to target.  */
	    temp = nontemporal || promoted ? NULL_RTX : target;
	    temp = expand_expr_real_2 (&ops, temp, GET_MODE (target),
				       EXPAND_NORMAL);

	    if (temp == target)
	      ;
	    else if (promoted)
	      {
		int unsignedp = SUBREG_PROMOTED_UNSIGNED_P (target);
		/* If TEMP is a VOIDmode constant, use convert_modes to make
		   sure that we properly convert it.  */
		if (CONSTANT_P (temp) && GET_MODE (temp) == VOIDmode)
		  {
		    temp = convert_modes (GET_MODE (target),
					  TYPE_MODE (ops.type),
					  temp, unsignedp);
		    temp = convert_modes (GET_MODE (SUBREG_REG (target)),
					  GET_MODE (target), temp, unsignedp);
		  }

		convert_move (SUBREG_REG (target), temp, unsignedp);
	      }
	    else if (nontemporal && emit_storent_insn (target, temp))
	      ;
	    else
	      {
		temp = force_operand (temp, target);
		if (temp != target)
		  emit_move_insn (target, temp);
	      }
	  }
      }
      break;

    default:
      gcc_unreachable ();
    }
}

/* Expand one gimple statement STMT and return the last RTL instruction
   before any of the newly generated ones.

   In addition to generating the necessary RTL instructions this also
   sets REG_EH_REGION notes if necessary and sets the current source
   location for diagnostics.  */

static rtx
expand_gimple_stmt (gimple stmt)
{
  int lp_nr = 0;
  rtx last = NULL;
  location_t saved_location = input_location;

  last = get_last_insn ();

  /* If this is an expression of some kind and it has an associated line
     number, then emit the line number before expanding the expression.

     We need to save and restore the file and line information so that
     errors discovered during expansion are emitted with the right
     information.  It would be better of the diagnostic routines
     used the file/line information embedded in the tree nodes rather
     than globals.  */
  gcc_assert (cfun);

  if (gimple_has_location (stmt))
    {
      input_location = gimple_location (stmt);
      set_curr_insn_source_location (input_location);

      /* Record where the insns produced belong.  */
      set_curr_insn_block (gimple_block (stmt));
    }

  expand_gimple_stmt_1 (stmt);
  /* Free any temporaries used to evaluate this statement.  */
  free_temp_slots ();

  input_location = saved_location;

  /* Mark all insns that may trap.  */
  lp_nr = lookup_stmt_eh_lp (stmt);
  if (lp_nr)
    {
      rtx insn;
      for (insn = next_real_insn (last); insn;
	   insn = next_real_insn (insn))
	{
	  if (! find_reg_note (insn, REG_EH_REGION, NULL_RTX)
	      /* If we want exceptions for non-call insns, any
		 may_trap_p instruction may throw.  */
	      && GET_CODE (PATTERN (insn)) != CLOBBER
	      && GET_CODE (PATTERN (insn)) != USE
	      && insn_could_throw_p (insn))
	    make_reg_eh_region_note (insn, 0, lp_nr);
	}
    }

  return last;
}

/* A subroutine of expand_gimple_basic_block.  Expand one GIMPLE_CALL
   that has CALL_EXPR_TAILCALL set.  Returns non-null if we actually
   generated a tail call (something that might be denied by the ABI
   rules governing the call; see calls.c).

   Sets CAN_FALLTHRU if we generated a *conditional* tail call, and
   can still reach the rest of BB.  The case here is __builtin_sqrt,
   where the NaN result goes through the external function (with a
   tailcall) and the normal result happens via a sqrt instruction.  */

static basic_block
expand_gimple_tailcall (basic_block bb, gimple stmt, bool *can_fallthru)
{
  rtx last2, last;
  edge e;
  edge_iterator ei;
  int probability;
  gcov_type count;

  last2 = last = expand_gimple_stmt (stmt);

  for (last = NEXT_INSN (last); last; last = NEXT_INSN (last))
    if (CALL_P (last) && SIBLING_CALL_P (last))
      goto found;

  maybe_dump_rtl_for_gimple_stmt (stmt, last2);

  *can_fallthru = true;
  return NULL;

 found:
  /* ??? Wouldn't it be better to just reset any pending stack adjust?
     Any instructions emitted here are about to be deleted.  */
  do_pending_stack_adjust ();

  /* Remove any non-eh, non-abnormal edges that don't go to exit.  */
  /* ??? I.e. the fallthrough edge.  HOWEVER!  If there were to be
     EH or abnormal edges, we shouldn't have created a tail call in
     the first place.  So it seems to me we should just be removing
     all edges here, or redirecting the existing fallthru edge to
     the exit block.  */

  probability = 0;
  count = 0;

  for (ei = ei_start (bb->succs); (e = ei_safe_edge (ei)); )
    {
      if (!(e->flags & (EDGE_ABNORMAL | EDGE_EH)))
	{
	  if (e->dest != EXIT_BLOCK_PTR)
	    {
	      e->dest->count -= e->count;
	      e->dest->frequency -= EDGE_FREQUENCY (e);
	      if (e->dest->count < 0)
		e->dest->count = 0;
	      if (e->dest->frequency < 0)
		e->dest->frequency = 0;
	    }
	  count += e->count;
	  probability += e->probability;
	  remove_edge (e);
	}
      else
	ei_next (&ei);
    }

  /* This is somewhat ugly: the call_expr expander often emits instructions
     after the sibcall (to perform the function return).  These confuse the
     find_many_sub_basic_blocks code, so we need to get rid of these.  */
  last = NEXT_INSN (last);
  gcc_assert (BARRIER_P (last));

  *can_fallthru = false;
  while (NEXT_INSN (last))
    {
      /* For instance an sqrt builtin expander expands if with
	 sibcall in the then and label for `else`.  */
      if (LABEL_P (NEXT_INSN (last)))
	{
	  *can_fallthru = true;
	  break;
	}
      delete_insn (NEXT_INSN (last));
    }

  e = make_edge (bb, EXIT_BLOCK_PTR, EDGE_ABNORMAL | EDGE_SIBCALL);
  e->probability += probability;
  e->count += count;
  BB_END (bb) = last;
  update_bb_for_insn (bb);

  if (NEXT_INSN (last))
    {
      bb = create_basic_block (NEXT_INSN (last), get_last_insn (), bb);

      last = BB_END (bb);
      if (BARRIER_P (last))
	BB_END (bb) = PREV_INSN (last);
    }

  maybe_dump_rtl_for_gimple_stmt (stmt, last2);

  return bb;
}

/* Return the difference between the floor and the truncated result of
   a signed division by OP1 with remainder MOD.  */
static rtx
floor_sdiv_adjust (enum machine_mode mode, rtx mod, rtx op1)
{
  /* (mod != 0 ? (op1 / mod < 0 ? -1 : 0) : 0) */
  return gen_rtx_IF_THEN_ELSE
    (mode, gen_rtx_NE (BImode, mod, const0_rtx),
     gen_rtx_IF_THEN_ELSE
     (mode, gen_rtx_LT (BImode,
			gen_rtx_DIV (mode, op1, mod),
			const0_rtx),
      constm1_rtx, const0_rtx),
     const0_rtx);
}

/* Return the difference between the ceil and the truncated result of
   a signed division by OP1 with remainder MOD.  */
static rtx
ceil_sdiv_adjust (enum machine_mode mode, rtx mod, rtx op1)
{
  /* (mod != 0 ? (op1 / mod > 0 ? 1 : 0) : 0) */
  return gen_rtx_IF_THEN_ELSE
    (mode, gen_rtx_NE (BImode, mod, const0_rtx),
     gen_rtx_IF_THEN_ELSE
     (mode, gen_rtx_GT (BImode,
			gen_rtx_DIV (mode, op1, mod),
			const0_rtx),
      const1_rtx, const0_rtx),
     const0_rtx);
}

/* Return the difference between the ceil and the truncated result of
   an unsigned division by OP1 with remainder MOD.  */
static rtx
ceil_udiv_adjust (enum machine_mode mode, rtx mod, rtx op1 ATTRIBUTE_UNUSED)
{
  /* (mod != 0 ? 1 : 0) */
  return gen_rtx_IF_THEN_ELSE
    (mode, gen_rtx_NE (BImode, mod, const0_rtx),
     const1_rtx, const0_rtx);
}

/* Return the difference between the rounded and the truncated result
   of a signed division by OP1 with remainder MOD.  Halfway cases are
   rounded away from zero, rather than to the nearest even number.  */
static rtx
round_sdiv_adjust (enum machine_mode mode, rtx mod, rtx op1)
{
  /* (abs (mod) >= abs (op1) - abs (mod)
      ? (op1 / mod > 0 ? 1 : -1)
      : 0) */
  return gen_rtx_IF_THEN_ELSE
    (mode, gen_rtx_GE (BImode, gen_rtx_ABS (mode, mod),
		       gen_rtx_MINUS (mode,
				      gen_rtx_ABS (mode, op1),
				      gen_rtx_ABS (mode, mod))),
     gen_rtx_IF_THEN_ELSE
     (mode, gen_rtx_GT (BImode,
			gen_rtx_DIV (mode, op1, mod),
			const0_rtx),
      const1_rtx, constm1_rtx),
     const0_rtx);
}

/* Return the difference between the rounded and the truncated result
   of a unsigned division by OP1 with remainder MOD.  Halfway cases
   are rounded away from zero, rather than to the nearest even
   number.  */
static rtx
round_udiv_adjust (enum machine_mode mode, rtx mod, rtx op1)
{
  /* (mod >= op1 - mod ? 1 : 0) */
  return gen_rtx_IF_THEN_ELSE
    (mode, gen_rtx_GE (BImode, mod,
		       gen_rtx_MINUS (mode, op1, mod)),
     const1_rtx, const0_rtx);
}

/* Convert X to MODE, that must be Pmode or ptr_mode, without emitting
   any rtl.  */

static rtx
convert_debug_memory_address (enum machine_mode mode, rtx x)
{
  enum machine_mode xmode = GET_MODE (x);

#ifndef POINTERS_EXTEND_UNSIGNED
  gcc_assert (mode == Pmode);
  gcc_assert (xmode == mode || xmode == VOIDmode);
#else
  gcc_assert (mode == Pmode || mode == ptr_mode);

  if (GET_MODE (x) == mode || GET_MODE (x) == VOIDmode)
    return x;

  if (GET_MODE_BITSIZE (mode) < GET_MODE_BITSIZE (xmode))
    x = simplify_gen_subreg (mode, x, xmode,
			     subreg_lowpart_offset
			     (mode, xmode));
  else if (POINTERS_EXTEND_UNSIGNED > 0)
    x = gen_rtx_ZERO_EXTEND (mode, x);
  else if (!POINTERS_EXTEND_UNSIGNED)
    x = gen_rtx_SIGN_EXTEND (mode, x);
  else
    gcc_unreachable ();
#endif /* POINTERS_EXTEND_UNSIGNED */

  return x;
}

/* Return an RTX equivalent to the value of the tree expression
   EXP.  */

static rtx
expand_debug_expr (tree exp)
{
  rtx op0 = NULL_RTX, op1 = NULL_RTX, op2 = NULL_RTX;
  enum machine_mode mode = TYPE_MODE (TREE_TYPE (exp));
  int unsignedp = TYPE_UNSIGNED (TREE_TYPE (exp));
  addr_space_t as;
  enum machine_mode address_mode;

  switch (TREE_CODE_CLASS (TREE_CODE (exp)))
    {
    case tcc_expression:
      switch (TREE_CODE (exp))
	{
	case COND_EXPR:
	case DOT_PROD_EXPR:
	  goto ternary;

	case TRUTH_ANDIF_EXPR:
	case TRUTH_ORIF_EXPR:
	case TRUTH_AND_EXPR:
	case TRUTH_OR_EXPR:
	case TRUTH_XOR_EXPR:
	  goto binary;

	case TRUTH_NOT_EXPR:
	  goto unary;

	default:
	  break;
	}
      break;

    ternary:
      op2 = expand_debug_expr (TREE_OPERAND (exp, 2));
      if (!op2)
	return NULL_RTX;
      /* Fall through.  */

    binary:
    case tcc_binary:
    case tcc_comparison:
      op1 = expand_debug_expr (TREE_OPERAND (exp, 1));
      if (!op1)
	return NULL_RTX;
      /* Fall through.  */

    unary:
    case tcc_unary:
      op0 = expand_debug_expr (TREE_OPERAND (exp, 0));
      if (!op0)
	return NULL_RTX;
      break;

    case tcc_type:
    case tcc_statement:
      gcc_unreachable ();

    case tcc_constant:
    case tcc_exceptional:
    case tcc_declaration:
    case tcc_reference:
    case tcc_vl_exp:
      break;
    }

  switch (TREE_CODE (exp))
    {
    case STRING_CST:
      if (!lookup_constant_def (exp))
	{
	  if (strlen (TREE_STRING_POINTER (exp)) + 1
	      != (size_t) TREE_STRING_LENGTH (exp))
	    return NULL_RTX;
	  op0 = gen_rtx_CONST_STRING (Pmode, TREE_STRING_POINTER (exp));
	  op0 = gen_rtx_MEM (BLKmode, op0);
	  set_mem_attributes (op0, exp, 0);
	  return op0;
	}
      /* Fall through...  */

    case INTEGER_CST:
    case REAL_CST:
    case FIXED_CST:
      op0 = expand_expr (exp, NULL_RTX, mode, EXPAND_INITIALIZER);
      return op0;

    case COMPLEX_CST:
      gcc_assert (COMPLEX_MODE_P (mode));
      op0 = expand_debug_expr (TREE_REALPART (exp));
      op1 = expand_debug_expr (TREE_IMAGPART (exp));
      return gen_rtx_CONCAT (mode, op0, op1);

    case DEBUG_EXPR_DECL:
      op0 = DECL_RTL_IF_SET (exp);

      if (op0)
	return op0;

      op0 = gen_rtx_DEBUG_EXPR (mode);
      DEBUG_EXPR_TREE_DECL (op0) = exp;
      SET_DECL_RTL (exp, op0);

      return op0;

    case VAR_DECL:
    case PARM_DECL:
    case FUNCTION_DECL:
    case LABEL_DECL:
    case CONST_DECL:
    case RESULT_DECL:
      op0 = DECL_RTL_IF_SET (exp);

      /* This decl was probably optimized away.  */
      if (!op0)
	{
	  if (TREE_CODE (exp) != VAR_DECL
	      || DECL_EXTERNAL (exp)
	      || !TREE_STATIC (exp)
	      || !DECL_NAME (exp)
	      || DECL_HARD_REGISTER (exp)
	      || mode == VOIDmode)
	    return NULL;

	  op0 = make_decl_rtl_for_debug (exp);
	  if (!MEM_P (op0)
	      || GET_CODE (XEXP (op0, 0)) != SYMBOL_REF
	      || SYMBOL_REF_DECL (XEXP (op0, 0)) != exp)
	    return NULL;
	}
      else
	op0 = copy_rtx (op0);

      if (GET_MODE (op0) == BLKmode
	  /* If op0 is not BLKmode, but BLKmode is, adjust_mode
	     below would ICE.  While it is likely a FE bug,
	     try to be robust here.  See PR43166.  */
	  || mode == BLKmode
	  || (mode == VOIDmode && GET_MODE (op0) != VOIDmode))
	{
	  gcc_assert (MEM_P (op0));
	  op0 = adjust_address_nv (op0, mode, 0);
	  return op0;
	}

      /* Fall through.  */

    adjust_mode:
    case PAREN_EXPR:
    case NOP_EXPR:
    case CONVERT_EXPR:
      {
	enum machine_mode inner_mode = GET_MODE (op0);

	if (mode == inner_mode)
	  return op0;

	if (inner_mode == VOIDmode)
	  {
	    if (TREE_CODE (exp) == SSA_NAME)
	      inner_mode = TYPE_MODE (TREE_TYPE (exp));
	    else
	      inner_mode = TYPE_MODE (TREE_TYPE (TREE_OPERAND (exp, 0)));
	    if (mode == inner_mode)
	      return op0;
	  }

	if (FLOAT_MODE_P (mode) && FLOAT_MODE_P (inner_mode))
	  {
	    if (GET_MODE_BITSIZE (mode) == GET_MODE_BITSIZE (inner_mode))
	      op0 = simplify_gen_subreg (mode, op0, inner_mode, 0);
	    else if (GET_MODE_BITSIZE (mode) < GET_MODE_BITSIZE (inner_mode))
	      op0 = simplify_gen_unary (FLOAT_TRUNCATE, mode, op0, inner_mode);
	    else
	      op0 = simplify_gen_unary (FLOAT_EXTEND, mode, op0, inner_mode);
	  }
	else if (FLOAT_MODE_P (mode))
	  {
	    gcc_assert (TREE_CODE (exp) != SSA_NAME);
	    if (TYPE_UNSIGNED (TREE_TYPE (TREE_OPERAND (exp, 0))))
	      op0 = simplify_gen_unary (UNSIGNED_FLOAT, mode, op0, inner_mode);
	    else
	      op0 = simplify_gen_unary (FLOAT, mode, op0, inner_mode);
	  }
	else if (FLOAT_MODE_P (inner_mode))
	  {
	    if (unsignedp)
	      op0 = simplify_gen_unary (UNSIGNED_FIX, mode, op0, inner_mode);
	    else
	      op0 = simplify_gen_unary (FIX, mode, op0, inner_mode);
	  }
	else if (CONSTANT_P (op0)
		 || GET_MODE_BITSIZE (mode) <= GET_MODE_BITSIZE (inner_mode))
	  op0 = simplify_gen_subreg (mode, op0, inner_mode,
				     subreg_lowpart_offset (mode,
							    inner_mode));
	else if (unsignedp)
	  op0 = gen_rtx_ZERO_EXTEND (mode, op0);
	else
	  op0 = gen_rtx_SIGN_EXTEND (mode, op0);

	return op0;
      }

    case INDIRECT_REF:
    case ALIGN_INDIRECT_REF:
    case MISALIGNED_INDIRECT_REF:
      op0 = expand_debug_expr (TREE_OPERAND (exp, 0));
      if (!op0)
	return NULL;

      if (POINTER_TYPE_P (TREE_TYPE (exp)))
	{
	  as = TYPE_ADDR_SPACE (TREE_TYPE (TREE_TYPE (exp)));
	  address_mode = targetm.addr_space.address_mode (as);
	}
      else
	{
	  as = ADDR_SPACE_GENERIC;
	  address_mode = Pmode;
	}

      if (TREE_CODE (exp) == ALIGN_INDIRECT_REF)
	{
	  int align = TYPE_ALIGN_UNIT (TREE_TYPE (exp));
	  op0 = gen_rtx_AND (address_mode, op0, GEN_INT (-align));
	}

      op0 = gen_rtx_MEM (mode, op0);

      set_mem_attributes (op0, exp, 0);
      set_mem_addr_space (op0, as);

      return op0;

    case TARGET_MEM_REF:
      if (TMR_SYMBOL (exp) && !DECL_RTL_SET_P (TMR_SYMBOL (exp)))
	return NULL;

      op0 = expand_debug_expr
	    (tree_mem_ref_addr (build_pointer_type (TREE_TYPE (exp)), exp));
      if (!op0)
	return NULL;

      as = TYPE_ADDR_SPACE (TREE_TYPE (exp));

      op0 = gen_rtx_MEM (mode, op0);

      set_mem_attributes (op0, exp, 0);
      set_mem_addr_space (op0, as);

      return op0;

    case ARRAY_REF:
    case ARRAY_RANGE_REF:
    case COMPONENT_REF:
    case BIT_FIELD_REF:
    case REALPART_EXPR:
    case IMAGPART_EXPR:
    case VIEW_CONVERT_EXPR:
      {
	enum machine_mode mode1;
	HOST_WIDE_INT bitsize, bitpos;
	tree offset;
	int volatilep = 0;
	tree tem = get_inner_reference (exp, &bitsize, &bitpos, &offset,
					&mode1, &unsignedp, &volatilep, false);
	rtx orig_op0;

	if (bitsize == 0)
	  return NULL;

	orig_op0 = op0 = expand_debug_expr (tem);

	if (!op0)
	  return NULL;

	if (offset)
	  {
	    enum machine_mode addrmode, offmode;

	    if (!MEM_P (op0))
	      return NULL;

	    op0 = XEXP (op0, 0);
	    addrmode = GET_MODE (op0);
	    if (addrmode == VOIDmode)
	      addrmode = Pmode;

	    op1 = expand_debug_expr (offset);
	    if (!op1)
	      return NULL;

	    offmode = GET_MODE (op1);
	    if (offmode == VOIDmode)
	      offmode = TYPE_MODE (TREE_TYPE (offset));

	    if (addrmode != offmode)
	      op1 = simplify_gen_subreg (addrmode, op1, offmode,
					 subreg_lowpart_offset (addrmode,
								offmode));

	    /* Don't use offset_address here, we don't need a
	       recognizable address, and we don't want to generate
	       code.  */
	    op0 = gen_rtx_MEM (mode, gen_rtx_PLUS (addrmode, op0, op1));
	  }

	if (MEM_P (op0))
	  {
	    if (mode1 == VOIDmode)
	      /* Bitfield.  */
	      mode1 = smallest_mode_for_size (bitsize, MODE_INT);
	    if (bitpos >= BITS_PER_UNIT)
	      {
		op0 = adjust_address_nv (op0, mode1, bitpos / BITS_PER_UNIT);
		bitpos %= BITS_PER_UNIT;
	      }
	    else if (bitpos < 0)
	      {
		HOST_WIDE_INT units
		  = (-bitpos + BITS_PER_UNIT - 1) / BITS_PER_UNIT;
		op0 = adjust_address_nv (op0, mode1, units);
		bitpos += units * BITS_PER_UNIT;
	      }
	    else if (bitpos == 0 && bitsize == GET_MODE_BITSIZE (mode))
	      op0 = adjust_address_nv (op0, mode, 0);
	    else if (GET_MODE (op0) != mode1)
	      op0 = adjust_address_nv (op0, mode1, 0);
	    else
	      op0 = copy_rtx (op0);
	    if (op0 == orig_op0)
	      op0 = shallow_copy_rtx (op0);
	    set_mem_attributes (op0, exp, 0);
	  }

	if (bitpos == 0 && mode == GET_MODE (op0))
	  return op0;

        if (bitpos < 0)
          return NULL;

	if (GET_MODE (op0) == BLKmode)
	  return NULL;

	if ((bitpos % BITS_PER_UNIT) == 0
	    && bitsize == GET_MODE_BITSIZE (mode1))
	  {
	    enum machine_mode opmode = GET_MODE (op0);

	    if (opmode == VOIDmode)
	      opmode = TYPE_MODE (TREE_TYPE (tem));

	    /* This condition may hold if we're expanding the address
	       right past the end of an array that turned out not to
	       be addressable (i.e., the address was only computed in
	       debug stmts).  The gen_subreg below would rightfully
	       crash, and the address doesn't really exist, so just
	       drop it.  */
	    if (bitpos >= GET_MODE_BITSIZE (opmode))
	      return NULL;

	    if ((bitpos % GET_MODE_BITSIZE (mode)) == 0)
	      return simplify_gen_subreg (mode, op0, opmode,
					  bitpos / BITS_PER_UNIT);
	  }

	return simplify_gen_ternary (SCALAR_INT_MODE_P (GET_MODE (op0))
				     && TYPE_UNSIGNED (TREE_TYPE (exp))
				     ? SIGN_EXTRACT
				     : ZERO_EXTRACT, mode,
				     GET_MODE (op0) != VOIDmode
				     ? GET_MODE (op0)
				     : TYPE_MODE (TREE_TYPE (tem)),
				     op0, GEN_INT (bitsize), GEN_INT (bitpos));
      }

    case ABS_EXPR:
      return gen_rtx_ABS (mode, op0);

    case NEGATE_EXPR:
      return gen_rtx_NEG (mode, op0);

    case BIT_NOT_EXPR:
      return gen_rtx_NOT (mode, op0);

    case FLOAT_EXPR:
      if (unsignedp)
	return gen_rtx_UNSIGNED_FLOAT (mode, op0);
      else
	return gen_rtx_FLOAT (mode, op0);

    case FIX_TRUNC_EXPR:
      if (unsignedp)
	return gen_rtx_UNSIGNED_FIX (mode, op0);
      else
	return gen_rtx_FIX (mode, op0);

    case POINTER_PLUS_EXPR:
      /* For the rare target where pointers are not the same size as
	 size_t, we need to check for mis-matched modes and correct
	 the addend.  */
      if (op0 && op1
	  && GET_MODE (op0) != VOIDmode && GET_MODE (op1) != VOIDmode
	  && GET_MODE (op0) != GET_MODE (op1))
	{
	  if (GET_MODE_BITSIZE (GET_MODE (op0)) < GET_MODE_BITSIZE (GET_MODE (op1)))
	    op1 = gen_rtx_TRUNCATE (GET_MODE (op0), op1);
	  else
	    /* We always sign-extend, regardless of the signedness of
	       the operand, because the operand is always unsigned
	       here even if the original C expression is signed.  */
	    op1 = gen_rtx_SIGN_EXTEND (GET_MODE (op0), op1);
	}
      /* Fall through.  */
    case PLUS_EXPR:
      return gen_rtx_PLUS (mode, op0, op1);

    case MINUS_EXPR:
      return gen_rtx_MINUS (mode, op0, op1);

    case MULT_EXPR:
      return gen_rtx_MULT (mode, op0, op1);

    case RDIV_EXPR:
    case TRUNC_DIV_EXPR:
    case EXACT_DIV_EXPR:
      if (unsignedp)
	return gen_rtx_UDIV (mode, op0, op1);
      else
	return gen_rtx_DIV (mode, op0, op1);

    case TRUNC_MOD_EXPR:
      if (unsignedp)
	return gen_rtx_UMOD (mode, op0, op1);
      else
	return gen_rtx_MOD (mode, op0, op1);

    case FLOOR_DIV_EXPR:
      if (unsignedp)
	return gen_rtx_UDIV (mode, op0, op1);
      else
	{
	  rtx div = gen_rtx_DIV (mode, op0, op1);
	  rtx mod = gen_rtx_MOD (mode, op0, op1);
	  rtx adj = floor_sdiv_adjust (mode, mod, op1);
	  return gen_rtx_PLUS (mode, div, adj);
	}

    case FLOOR_MOD_EXPR:
      if (unsignedp)
	return gen_rtx_UMOD (mode, op0, op1);
      else
	{
	  rtx mod = gen_rtx_MOD (mode, op0, op1);
	  rtx adj = floor_sdiv_adjust (mode, mod, op1);
	  adj = gen_rtx_NEG (mode, gen_rtx_MULT (mode, adj, op1));
	  return gen_rtx_PLUS (mode, mod, adj);
	}

    case CEIL_DIV_EXPR:
      if (unsignedp)
	{
	  rtx div = gen_rtx_UDIV (mode, op0, op1);
	  rtx mod = gen_rtx_UMOD (mode, op0, op1);
	  rtx adj = ceil_udiv_adjust (mode, mod, op1);
	  return gen_rtx_PLUS (mode, div, adj);
	}
      else
	{
	  rtx div = gen_rtx_DIV (mode, op0, op1);
	  rtx mod = gen_rtx_MOD (mode, op0, op1);
	  rtx adj = ceil_sdiv_adjust (mode, mod, op1);
	  return gen_rtx_PLUS (mode, div, adj);
	}

    case CEIL_MOD_EXPR:
      if (unsignedp)
	{
	  rtx mod = gen_rtx_UMOD (mode, op0, op1);
	  rtx adj = ceil_udiv_adjust (mode, mod, op1);
	  adj = gen_rtx_NEG (mode, gen_rtx_MULT (mode, adj, op1));
	  return gen_rtx_PLUS (mode, mod, adj);
	}
      else
	{
	  rtx mod = gen_rtx_MOD (mode, op0, op1);
	  rtx adj = ceil_sdiv_adjust (mode, mod, op1);
	  adj = gen_rtx_NEG (mode, gen_rtx_MULT (mode, adj, op1));
	  return gen_rtx_PLUS (mode, mod, adj);
	}

    case ROUND_DIV_EXPR:
      if (unsignedp)
	{
	  rtx div = gen_rtx_UDIV (mode, op0, op1);
	  rtx mod = gen_rtx_UMOD (mode, op0, op1);
	  rtx adj = round_udiv_adjust (mode, mod, op1);
	  return gen_rtx_PLUS (mode, div, adj);
	}
      else
	{
	  rtx div = gen_rtx_DIV (mode, op0, op1);
	  rtx mod = gen_rtx_MOD (mode, op0, op1);
	  rtx adj = round_sdiv_adjust (mode, mod, op1);
	  return gen_rtx_PLUS (mode, div, adj);
	}

    case ROUND_MOD_EXPR:
      if (unsignedp)
	{
	  rtx mod = gen_rtx_UMOD (mode, op0, op1);
	  rtx adj = round_udiv_adjust (mode, mod, op1);
	  adj = gen_rtx_NEG (mode, gen_rtx_MULT (mode, adj, op1));
	  return gen_rtx_PLUS (mode, mod, adj);
	}
      else
	{
	  rtx mod = gen_rtx_MOD (mode, op0, op1);
	  rtx adj = round_sdiv_adjust (mode, mod, op1);
	  adj = gen_rtx_NEG (mode, gen_rtx_MULT (mode, adj, op1));
	  return gen_rtx_PLUS (mode, mod, adj);
	}

    case LSHIFT_EXPR:
      return gen_rtx_ASHIFT (mode, op0, op1);

    case RSHIFT_EXPR:
      if (unsignedp)
	return gen_rtx_LSHIFTRT (mode, op0, op1);
      else
	return gen_rtx_ASHIFTRT (mode, op0, op1);

    case LROTATE_EXPR:
      return gen_rtx_ROTATE (mode, op0, op1);

    case RROTATE_EXPR:
      return gen_rtx_ROTATERT (mode, op0, op1);

    case MIN_EXPR:
      if (unsignedp)
	return gen_rtx_UMIN (mode, op0, op1);
      else
	return gen_rtx_SMIN (mode, op0, op1);

    case MAX_EXPR:
      if (unsignedp)
	return gen_rtx_UMAX (mode, op0, op1);
      else
	return gen_rtx_SMAX (mode, op0, op1);

    case BIT_AND_EXPR:
    case TRUTH_AND_EXPR:
      return gen_rtx_AND (mode, op0, op1);

    case BIT_IOR_EXPR:
    case TRUTH_OR_EXPR:
      return gen_rtx_IOR (mode, op0, op1);

    case BIT_XOR_EXPR:
    case TRUTH_XOR_EXPR:
      return gen_rtx_XOR (mode, op0, op1);

    case TRUTH_ANDIF_EXPR:
      return gen_rtx_IF_THEN_ELSE (mode, op0, op1, const0_rtx);

    case TRUTH_ORIF_EXPR:
      return gen_rtx_IF_THEN_ELSE (mode, op0, const_true_rtx, op1);

    case TRUTH_NOT_EXPR:
      return gen_rtx_EQ (mode, op0, const0_rtx);

    case LT_EXPR:
      if (unsignedp)
	return gen_rtx_LTU (mode, op0, op1);
      else
	return gen_rtx_LT (mode, op0, op1);

    case LE_EXPR:
      if (unsignedp)
	return gen_rtx_LEU (mode, op0, op1);
      else
	return gen_rtx_LE (mode, op0, op1);

    case GT_EXPR:
      if (unsignedp)
	return gen_rtx_GTU (mode, op0, op1);
      else
	return gen_rtx_GT (mode, op0, op1);

    case GE_EXPR:
      if (unsignedp)
	return gen_rtx_GEU (mode, op0, op1);
      else
	return gen_rtx_GE (mode, op0, op1);

    case EQ_EXPR:
      return gen_rtx_EQ (mode, op0, op1);

    case NE_EXPR:
      return gen_rtx_NE (mode, op0, op1);

    case UNORDERED_EXPR:
      return gen_rtx_UNORDERED (mode, op0, op1);

    case ORDERED_EXPR:
      return gen_rtx_ORDERED (mode, op0, op1);

    case UNLT_EXPR:
      return gen_rtx_UNLT (mode, op0, op1);

    case UNLE_EXPR:
      return gen_rtx_UNLE (mode, op0, op1);

    case UNGT_EXPR:
      return gen_rtx_UNGT (mode, op0, op1);

    case UNGE_EXPR:
      return gen_rtx_UNGE (mode, op0, op1);

    case UNEQ_EXPR:
      return gen_rtx_UNEQ (mode, op0, op1);

    case LTGT_EXPR:
      return gen_rtx_LTGT (mode, op0, op1);

    case COND_EXPR:
      return gen_rtx_IF_THEN_ELSE (mode, op0, op1, op2);

    case COMPLEX_EXPR:
      gcc_assert (COMPLEX_MODE_P (mode));
      if (GET_MODE (op0) == VOIDmode)
	op0 = gen_rtx_CONST (GET_MODE_INNER (mode), op0);
      if (GET_MODE (op1) == VOIDmode)
	op1 = gen_rtx_CONST (GET_MODE_INNER (mode), op1);
      return gen_rtx_CONCAT (mode, op0, op1);

    case CONJ_EXPR:
      if (GET_CODE (op0) == CONCAT)
	return gen_rtx_CONCAT (mode, XEXP (op0, 0),
			       gen_rtx_NEG (GET_MODE_INNER (mode),
					    XEXP (op0, 1)));
      else
	{
	  enum machine_mode imode = GET_MODE_INNER (mode);
	  rtx re, im;

	  if (MEM_P (op0))
	    {
	      re = adjust_address_nv (op0, imode, 0);
	      im = adjust_address_nv (op0, imode, GET_MODE_SIZE (imode));
	    }
	  else
	    {
	      enum machine_mode ifmode = int_mode_for_mode (mode);
	      enum machine_mode ihmode = int_mode_for_mode (imode);
	      rtx halfsize;
	      if (ifmode == BLKmode || ihmode == BLKmode)
		return NULL;
	      halfsize = GEN_INT (GET_MODE_BITSIZE (ihmode));
	      re = op0;
	      if (mode != ifmode)
		re = gen_rtx_SUBREG (ifmode, re, 0);
	      re = gen_rtx_ZERO_EXTRACT (ihmode, re, halfsize, const0_rtx);
	      if (imode != ihmode)
		re = gen_rtx_SUBREG (imode, re, 0);
	      im = copy_rtx (op0);
	      if (mode != ifmode)
		im = gen_rtx_SUBREG (ifmode, im, 0);
	      im = gen_rtx_ZERO_EXTRACT (ihmode, im, halfsize, halfsize);
	      if (imode != ihmode)
		im = gen_rtx_SUBREG (imode, im, 0);
	    }
	  im = gen_rtx_NEG (imode, im);
	  return gen_rtx_CONCAT (mode, re, im);
	}

    case ADDR_EXPR:
      op0 = expand_debug_expr (TREE_OPERAND (exp, 0));
      if (!op0 || !MEM_P (op0))
	return NULL;

      op0 = convert_debug_memory_address (mode, XEXP (op0, 0));

      return op0;

    case VECTOR_CST:
      exp = build_constructor_from_list (TREE_TYPE (exp),
					 TREE_VECTOR_CST_ELTS (exp));
      /* Fall through.  */

    case CONSTRUCTOR:
      if (TREE_CODE (TREE_TYPE (exp)) == VECTOR_TYPE)
	{
	  unsigned i;
	  tree val;

	  op0 = gen_rtx_CONCATN
	    (mode, rtvec_alloc (TYPE_VECTOR_SUBPARTS (TREE_TYPE (exp))));

	  FOR_EACH_CONSTRUCTOR_VALUE (CONSTRUCTOR_ELTS (exp), i, val)
	    {
	      op1 = expand_debug_expr (val);
	      if (!op1)
		return NULL;
	      XVECEXP (op0, 0, i) = op1;
	    }

	  if (i < TYPE_VECTOR_SUBPARTS (TREE_TYPE (exp)))
	    {
	      op1 = expand_debug_expr
		(fold_convert (TREE_TYPE (TREE_TYPE (exp)), integer_zero_node));

	      if (!op1)
		return NULL;

	      for (; i < TYPE_VECTOR_SUBPARTS (TREE_TYPE (exp)); i++)
		XVECEXP (op0, 0, i) = op1;
	    }

	  return op0;
	}
      else
	goto flag_unsupported;

    case CALL_EXPR:
      /* ??? Maybe handle some builtins?  */
      return NULL;

    case SSA_NAME:
      {
	gimple g = get_gimple_for_ssa_name (exp);
	if (g)
	  {
	    op0 = expand_debug_expr (gimple_assign_rhs_to_tree (g));
	    if (!op0)
	      return NULL;
	  }
	else
	  {
	    int part = var_to_partition (SA.map, exp);

	    if (part == NO_PARTITION)
	      return NULL;

	    gcc_assert (part >= 0 && (unsigned)part < SA.map->num_partitions);

	    op0 = copy_rtx (SA.partition_to_pseudo[part]);
	  }
	goto adjust_mode;
      }

    case ERROR_MARK:
      return NULL;

    /* Vector stuff.  For most of the codes we don't have rtl codes.  */
    case REALIGN_LOAD_EXPR:
    case REDUC_MAX_EXPR:
    case REDUC_MIN_EXPR:
    case REDUC_PLUS_EXPR:
    case VEC_COND_EXPR:
    case VEC_EXTRACT_EVEN_EXPR:
    case VEC_EXTRACT_ODD_EXPR:
    case VEC_INTERLEAVE_HIGH_EXPR:
    case VEC_INTERLEAVE_LOW_EXPR:
    case VEC_LSHIFT_EXPR:
    case VEC_PACK_FIX_TRUNC_EXPR:
    case VEC_PACK_SAT_EXPR:
    case VEC_PACK_TRUNC_EXPR:
    case VEC_RSHIFT_EXPR:
    case VEC_UNPACK_FLOAT_HI_EXPR:
    case VEC_UNPACK_FLOAT_LO_EXPR:
    case VEC_UNPACK_HI_EXPR:
    case VEC_UNPACK_LO_EXPR:
    case VEC_WIDEN_MULT_HI_EXPR:
    case VEC_WIDEN_MULT_LO_EXPR:
      return NULL;

   /* Misc codes.  */
    case ADDR_SPACE_CONVERT_EXPR:
    case FIXED_CONVERT_EXPR:
    case OBJ_TYPE_REF:
    case WITH_SIZE_EXPR:
      return NULL;

    case DOT_PROD_EXPR:
      if (SCALAR_INT_MODE_P (GET_MODE (op0))
	  && SCALAR_INT_MODE_P (mode))
	{
	  if (TYPE_UNSIGNED (TREE_TYPE (TREE_OPERAND (exp, 0))))
	    op0 = gen_rtx_ZERO_EXTEND (mode, op0);
	  else
	    op0 = gen_rtx_SIGN_EXTEND (mode, op0);
	  if (TYPE_UNSIGNED (TREE_TYPE (TREE_OPERAND (exp, 1))))
	    op1 = gen_rtx_ZERO_EXTEND (mode, op1);
	  else
	    op1 = gen_rtx_SIGN_EXTEND (mode, op1);
	  op0 = gen_rtx_MULT (mode, op0, op1);
	  return gen_rtx_PLUS (mode, op0, op2);
	}
      return NULL;

    case WIDEN_MULT_EXPR:
      if (SCALAR_INT_MODE_P (GET_MODE (op0))
	  && SCALAR_INT_MODE_P (mode))
	{
	  if (TYPE_UNSIGNED (TREE_TYPE (TREE_OPERAND (exp, 0))))
	    op0 = gen_rtx_ZERO_EXTEND (mode, op0);
	  else
	    op0 = gen_rtx_SIGN_EXTEND (mode, op0);
	  if (TYPE_UNSIGNED (TREE_TYPE (TREE_OPERAND (exp, 1))))
	    op1 = gen_rtx_ZERO_EXTEND (mode, op1);
	  else
	    op1 = gen_rtx_SIGN_EXTEND (mode, op1);
	  return gen_rtx_MULT (mode, op0, op1);
	}
      return NULL;

    case WIDEN_SUM_EXPR:
      if (SCALAR_INT_MODE_P (GET_MODE (op0))
	  && SCALAR_INT_MODE_P (mode))
	{
	  if (TYPE_UNSIGNED (TREE_TYPE (TREE_OPERAND (exp, 0))))
	    op0 = gen_rtx_ZERO_EXTEND (mode, op0);
	  else
	    op0 = gen_rtx_SIGN_EXTEND (mode, op0);
	  return gen_rtx_PLUS (mode, op0, op1);
	}
      return NULL;

    default:
    flag_unsupported:
#ifdef ENABLE_CHECKING
      debug_tree (exp);
      gcc_unreachable ();
#else
      return NULL;
#endif
    }
}

/* Expand the _LOCs in debug insns.  We run this after expanding all
   regular insns, so that any variables referenced in the function
   will have their DECL_RTLs set.  */

static void
expand_debug_locations (void)
{
  rtx insn;
  rtx last = get_last_insn ();
  int save_strict_alias = flag_strict_aliasing;

  /* New alias sets while setting up memory attributes cause
     -fcompare-debug failures, even though it doesn't bring about any
     codegen changes.  */
  flag_strict_aliasing = 0;

  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    if (DEBUG_INSN_P (insn))
      {
	tree value = (tree)INSN_VAR_LOCATION_LOC (insn);
	rtx val;
	enum machine_mode mode;

	if (value == NULL_TREE)
	  val = NULL_RTX;
	else
	  {
	    val = expand_debug_expr (value);
	    gcc_assert (last == get_last_insn ());
	  }

	if (!val)
	  val = gen_rtx_UNKNOWN_VAR_LOC ();
	else
	  {
	    mode = GET_MODE (INSN_VAR_LOCATION (insn));

	    gcc_assert (mode == GET_MODE (val)
			|| (GET_MODE (val) == VOIDmode
			    && (CONST_INT_P (val)
				|| GET_CODE (val) == CONST_FIXED
				|| GET_CODE (val) == CONST_DOUBLE
				|| GET_CODE (val) == LABEL_REF)));
	  }

	INSN_VAR_LOCATION_LOC (insn) = val;
      }

  flag_strict_aliasing = save_strict_alias;
}

/* Expand basic block BB from GIMPLE trees to RTL.  */

static basic_block
expand_gimple_basic_block (basic_block bb)
{
  gimple_stmt_iterator gsi;
  gimple_seq stmts;
  gimple stmt = NULL;
  rtx note, last;
  edge e;
  edge_iterator ei;
  void **elt;

  if (dump_file)
    fprintf (dump_file, "\n;; Generating RTL for gimple basic block %d\n",
	     bb->index);

  /* Note that since we are now transitioning from GIMPLE to RTL, we
     cannot use the gsi_*_bb() routines because they expect the basic
     block to be in GIMPLE, instead of RTL.  Therefore, we need to
     access the BB sequence directly.  */
  stmts = bb_seq (bb);
  bb->il.gimple = NULL;
  rtl_profile_for_bb (bb);
  init_rtl_bb_info (bb);
  bb->flags |= BB_RTL;

  /* Remove the RETURN_EXPR if we may fall though to the exit
     instead.  */
  gsi = gsi_last (stmts);
  if (!gsi_end_p (gsi)
      && gimple_code (gsi_stmt (gsi)) == GIMPLE_RETURN)
    {
      gimple ret_stmt = gsi_stmt (gsi);

      gcc_assert (single_succ_p (bb));
      gcc_assert (single_succ (bb) == EXIT_BLOCK_PTR);

      if (bb->next_bb == EXIT_BLOCK_PTR
	  && !gimple_return_retval (ret_stmt))
	{
	  gsi_remove (&gsi, false);
	  single_succ_edge (bb)->flags |= EDGE_FALLTHRU;
	}
    }

  gsi = gsi_start (stmts);
  if (!gsi_end_p (gsi))
    {
      stmt = gsi_stmt (gsi);
      if (gimple_code (stmt) != GIMPLE_LABEL)
	stmt = NULL;
    }

  elt = pointer_map_contains (lab_rtx_for_bb, bb);

  if (stmt || elt)
    {
      last = get_last_insn ();

      if (stmt)
	{
	  expand_gimple_stmt (stmt);
	  gsi_next (&gsi);
	}

      if (elt)
	emit_label ((rtx) *elt);

      /* Java emits line number notes in the top of labels.
	 ??? Make this go away once line number notes are obsoleted.  */
      BB_HEAD (bb) = NEXT_INSN (last);
      if (NOTE_P (BB_HEAD (bb)))
	BB_HEAD (bb) = NEXT_INSN (BB_HEAD (bb));
      note = emit_note_after (NOTE_INSN_BASIC_BLOCK, BB_HEAD (bb));

      maybe_dump_rtl_for_gimple_stmt (stmt, last);
    }
  else
    note = BB_HEAD (bb) = emit_note (NOTE_INSN_BASIC_BLOCK);

  NOTE_BASIC_BLOCK (note) = bb;

  for (; !gsi_end_p (gsi); gsi_next (&gsi))
    {
      basic_block new_bb;

      stmt = gsi_stmt (gsi);

      /* If this statement is a non-debug one, and we generate debug
	 insns, then this one might be the last real use of a TERed
	 SSA_NAME, but where there are still some debug uses further
	 down.  Expanding the current SSA name in such further debug
	 uses by their RHS might lead to wrong debug info, as coalescing
	 might make the operands of such RHS be placed into the same
	 pseudo as something else.  Like so:
	   a_1 = a_0 + 1;   // Assume a_1 is TERed and a_0 is dead
	   use(a_1);
	   a_2 = ...
           #DEBUG ... => a_1
	 As a_0 and a_2 don't overlap in lifetime, assume they are coalesced.
	 If we now would expand a_1 by it's RHS (a_0 + 1) in the debug use,
	 the write to a_2 would actually have clobbered the place which
	 formerly held a_0.

	 So, instead of that, we recognize the situation, and generate
	 debug temporaries at the last real use of TERed SSA names:
	   a_1 = a_0 + 1;
           #DEBUG #D1 => a_1
	   use(a_1);
	   a_2 = ...
           #DEBUG ... => #D1
	 */
      if (MAY_HAVE_DEBUG_INSNS
	  && SA.values
	  && !is_gimple_debug (stmt))
	{
	  ssa_op_iter iter;
	  tree op;
	  gimple def;

	  location_t sloc = get_curr_insn_source_location ();
	  tree sblock = get_curr_insn_block ();

	  /* Look for SSA names that have their last use here (TERed
	     names always have only one real use).  */
	  FOR_EACH_SSA_TREE_OPERAND (op, stmt, iter, SSA_OP_USE)
	    if ((def = get_gimple_for_ssa_name (op)))
	      {
		imm_use_iterator imm_iter;
		use_operand_p use_p;
		bool have_debug_uses = false;

		FOR_EACH_IMM_USE_FAST (use_p, imm_iter, op)
		  {
		    if (gimple_debug_bind_p (USE_STMT (use_p)))
		      {
			have_debug_uses = true;
			break;
		      }
		  }

		if (have_debug_uses)
		  {
		    /* OP is a TERed SSA name, with DEF it's defining
		       statement, and where OP is used in further debug
		       instructions.  Generate a debug temporary, and
		       replace all uses of OP in debug insns with that
		       temporary.  */
		    gimple debugstmt;
		    tree value = gimple_assign_rhs_to_tree (def);
		    tree vexpr = make_node (DEBUG_EXPR_DECL);
		    rtx val;
		    enum machine_mode mode;

		    set_curr_insn_source_location (gimple_location (def));
		    set_curr_insn_block (gimple_block (def));

		    DECL_ARTIFICIAL (vexpr) = 1;
		    TREE_TYPE (vexpr) = TREE_TYPE (value);
		    if (DECL_P (value))
		      mode = DECL_MODE (value);
		    else
		      mode = TYPE_MODE (TREE_TYPE (value));
		    DECL_MODE (vexpr) = mode;

		    val = gen_rtx_VAR_LOCATION
			(mode, vexpr, (rtx)value, VAR_INIT_STATUS_INITIALIZED);

		    val = emit_debug_insn (val);

		    FOR_EACH_IMM_USE_STMT (debugstmt, imm_iter, op)
		      {
			if (!gimple_debug_bind_p (debugstmt))
			  continue;

			FOR_EACH_IMM_USE_ON_STMT (use_p, imm_iter)
			  SET_USE (use_p, vexpr);

			update_stmt (debugstmt);
		      }
		  }
	      }
	  set_curr_insn_source_location (sloc);
	  set_curr_insn_block (sblock);
	}

      currently_expanding_gimple_stmt = stmt;

      /* Expand this statement, then evaluate the resulting RTL and
	 fixup the CFG accordingly.  */
      if (gimple_code (stmt) == GIMPLE_COND)
	{
	  new_bb = expand_gimple_cond (bb, stmt);
	  if (new_bb)
	    return new_bb;
	}
      else if (gimple_debug_bind_p (stmt))
	{
	  location_t sloc = get_curr_insn_source_location ();
	  tree sblock = get_curr_insn_block ();
	  gimple_stmt_iterator nsi = gsi;

	  for (;;)
	    {
	      tree var = gimple_debug_bind_get_var (stmt);
	      tree value;
	      rtx val;
	      enum machine_mode mode;

	      if (gimple_debug_bind_has_value_p (stmt))
		value = gimple_debug_bind_get_value (stmt);
	      else
		value = NULL_TREE;

	      last = get_last_insn ();

	      set_curr_insn_source_location (gimple_location (stmt));
	      set_curr_insn_block (gimple_block (stmt));

	      if (DECL_P (var))
		mode = DECL_MODE (var);
	      else
		mode = TYPE_MODE (TREE_TYPE (var));

	      val = gen_rtx_VAR_LOCATION
		(mode, var, (rtx)value, VAR_INIT_STATUS_INITIALIZED);

	      val = emit_debug_insn (val);

	      if (dump_file && (dump_flags & TDF_DETAILS))
		{
		  /* We can't dump the insn with a TREE where an RTX
		     is expected.  */
		  INSN_VAR_LOCATION_LOC (val) = const0_rtx;
		  maybe_dump_rtl_for_gimple_stmt (stmt, last);
		  INSN_VAR_LOCATION_LOC (val) = (rtx)value;
		}

	      /* In order not to generate too many debug temporaries,
	         we delink all uses of debug statements we already expanded.
		 Therefore debug statements between definition and real
		 use of TERed SSA names will continue to use the SSA name,
		 and not be replaced with debug temps.  */
	      delink_stmt_imm_use (stmt);

	      gsi = nsi;
	      gsi_next (&nsi);
	      if (gsi_end_p (nsi))
		break;
	      stmt = gsi_stmt (nsi);
	      if (!gimple_debug_bind_p (stmt))
		break;
	    }

	  set_curr_insn_source_location (sloc);
	  set_curr_insn_block (sblock);
	}
      else
	{
	  if (is_gimple_call (stmt) && gimple_call_tail_p (stmt))
	    {
	      bool can_fallthru;
	      new_bb = expand_gimple_tailcall (bb, stmt, &can_fallthru);
	      if (new_bb)
		{
		  if (can_fallthru)
		    bb = new_bb;
		  else
		    return new_bb;
		}
	    }
	  else
	    {
	      def_operand_p def_p;
	      def_p = SINGLE_SSA_DEF_OPERAND (stmt, SSA_OP_DEF);

	      if (def_p != NULL)
		{
		  /* Ignore this stmt if it is in the list of
		     replaceable expressions.  */
		  if (SA.values
		      && bitmap_bit_p (SA.values,
				       SSA_NAME_VERSION (DEF_FROM_PTR (def_p))))
		    continue;
		}
	      last = expand_gimple_stmt (stmt);
	      maybe_dump_rtl_for_gimple_stmt (stmt, last);
	    }
	}
    }

  currently_expanding_gimple_stmt = NULL;

  /* Expand implicit goto and convert goto_locus.  */
  FOR_EACH_EDGE (e, ei, bb->succs)
    {
      if (e->goto_locus && e->goto_block)
	{
	  set_curr_insn_source_location (e->goto_locus);
	  set_curr_insn_block (e->goto_block);
	  e->goto_locus = curr_insn_locator ();
	}
      e->goto_block = NULL;
      if ((e->flags & EDGE_FALLTHRU) && e->dest != bb->next_bb)
	{
	  emit_jump (label_rtx_for_bb (e->dest));
	  e->flags &= ~EDGE_FALLTHRU;
	}
    }

  /* Expanded RTL can create a jump in the last instruction of block.
     This later might be assumed to be a jump to successor and break edge insertion.
     We need to insert dummy move to prevent this. PR41440. */
  if (single_succ_p (bb)
      && (single_succ_edge (bb)->flags & EDGE_FALLTHRU)
      && (last = get_last_insn ())
      && JUMP_P (last))
    {
      rtx dummy = gen_reg_rtx (SImode);
      emit_insn_after_noloc (gen_move_insn (dummy, dummy), last, NULL);
    }

  do_pending_stack_adjust ();

  /* Find the block tail.  The last insn in the block is the insn
     before a barrier and/or table jump insn.  */
  last = get_last_insn ();
  if (BARRIER_P (last))
    last = PREV_INSN (last);
  if (JUMP_TABLE_DATA_P (last))
    last = PREV_INSN (PREV_INSN (last));
  BB_END (bb) = last;

  update_bb_for_insn (bb);

  return bb;
}


/* Create a basic block for initialization code.  */

static basic_block
construct_init_block (void)
{
  basic_block init_block, first_block;
  edge e = NULL;
  int flags;

  /* Multiple entry points not supported yet.  */
  gcc_assert (EDGE_COUNT (ENTRY_BLOCK_PTR->succs) == 1);
  init_rtl_bb_info (ENTRY_BLOCK_PTR);
  init_rtl_bb_info (EXIT_BLOCK_PTR);
  ENTRY_BLOCK_PTR->flags |= BB_RTL;
  EXIT_BLOCK_PTR->flags |= BB_RTL;

  e = EDGE_SUCC (ENTRY_BLOCK_PTR, 0);

  /* When entry edge points to first basic block, we don't need jump,
     otherwise we have to jump into proper target.  */
  if (e && e->dest != ENTRY_BLOCK_PTR->next_bb)
    {
      tree label = gimple_block_label (e->dest);

      emit_jump (label_rtx (label));
      flags = 0;
    }
  else
    flags = EDGE_FALLTHRU;

  init_block = create_basic_block (NEXT_INSN (get_insns ()),
				   get_last_insn (),
				   ENTRY_BLOCK_PTR);
  init_block->frequency = ENTRY_BLOCK_PTR->frequency;
  init_block->count = ENTRY_BLOCK_PTR->count;
  if (e)
    {
      first_block = e->dest;
      redirect_edge_succ (e, init_block);
      e = make_edge (init_block, first_block, flags);
    }
  else
    e = make_edge (init_block, EXIT_BLOCK_PTR, EDGE_FALLTHRU);
  e->probability = REG_BR_PROB_BASE;
  e->count = ENTRY_BLOCK_PTR->count;

  update_bb_for_insn (init_block);
  return init_block;
}

/* For each lexical block, set BLOCK_NUMBER to the depth at which it is
   found in the block tree.  */

static void
set_block_levels (tree block, int level)
{
  while (block)
    {
      BLOCK_NUMBER (block) = level;
      set_block_levels (BLOCK_SUBBLOCKS (block), level + 1);
      block = BLOCK_CHAIN (block);
    }
}

/* Create a block containing landing pads and similar stuff.  */

static void
construct_exit_block (void)
{
  rtx head = get_last_insn ();
  rtx end;
  basic_block exit_block;
  edge e, e2;
  unsigned ix;
  edge_iterator ei;
  rtx orig_end = BB_END (EXIT_BLOCK_PTR->prev_bb);

  rtl_profile_for_bb (EXIT_BLOCK_PTR);

  /* Make sure the locus is set to the end of the function, so that
     epilogue line numbers and warnings are set properly.  */
  if (cfun->function_end_locus != UNKNOWN_LOCATION)
    input_location = cfun->function_end_locus;

  /* The following insns belong to the top scope.  */
  set_curr_insn_block (DECL_INITIAL (current_function_decl));

  /* Generate rtl for function exit.  */
  expand_function_end ();

  end = get_last_insn ();
  if (head == end)
    return;
  /* While emitting the function end we could move end of the last basic block.
   */
  BB_END (EXIT_BLOCK_PTR->prev_bb) = orig_end;
  while (NEXT_INSN (head) && NOTE_P (NEXT_INSN (head)))
    head = NEXT_INSN (head);
  exit_block = create_basic_block (NEXT_INSN (head), end,
				   EXIT_BLOCK_PTR->prev_bb);
  exit_block->frequency = EXIT_BLOCK_PTR->frequency;
  exit_block->count = EXIT_BLOCK_PTR->count;

  ix = 0;
  while (ix < EDGE_COUNT (EXIT_BLOCK_PTR->preds))
    {
      e = EDGE_PRED (EXIT_BLOCK_PTR, ix);
      if (!(e->flags & EDGE_ABNORMAL))
	redirect_edge_succ (e, exit_block);
      else
	ix++;
    }

  e = make_edge (exit_block, EXIT_BLOCK_PTR, EDGE_FALLTHRU);
  e->probability = REG_BR_PROB_BASE;
  e->count = EXIT_BLOCK_PTR->count;
  FOR_EACH_EDGE (e2, ei, EXIT_BLOCK_PTR->preds)
    if (e2 != e)
      {
	e->count -= e2->count;
	exit_block->count -= e2->count;
	exit_block->frequency -= EDGE_FREQUENCY (e2);
      }
  if (e->count < 0)
    e->count = 0;
  if (exit_block->count < 0)
    exit_block->count = 0;
  if (exit_block->frequency < 0)
    exit_block->frequency = 0;
  update_bb_for_insn (exit_block);
}

/* Helper function for discover_nonconstant_array_refs.
   Look for ARRAY_REF nodes with non-constant indexes and mark them
   addressable.  */

static tree
discover_nonconstant_array_refs_r (tree * tp, int *walk_subtrees,
				   void *data ATTRIBUTE_UNUSED)
{
  tree t = *tp;

  if (IS_TYPE_OR_DECL_P (t))
    *walk_subtrees = 0;
  else if (TREE_CODE (t) == ARRAY_REF || TREE_CODE (t) == ARRAY_RANGE_REF)
    {
      while (((TREE_CODE (t) == ARRAY_REF || TREE_CODE (t) == ARRAY_RANGE_REF)
	      && is_gimple_min_invariant (TREE_OPERAND (t, 1))
	      && (!TREE_OPERAND (t, 2)
		  || is_gimple_min_invariant (TREE_OPERAND (t, 2))))
	     || (TREE_CODE (t) == COMPONENT_REF
		 && (!TREE_OPERAND (t,2)
		     || is_gimple_min_invariant (TREE_OPERAND (t, 2))))
	     || TREE_CODE (t) == BIT_FIELD_REF
	     || TREE_CODE (t) == REALPART_EXPR
	     || TREE_CODE (t) == IMAGPART_EXPR
	     || TREE_CODE (t) == VIEW_CONVERT_EXPR
	     || CONVERT_EXPR_P (t))
	t = TREE_OPERAND (t, 0);

      if (TREE_CODE (t) == ARRAY_REF || TREE_CODE (t) == ARRAY_RANGE_REF)
	{
	  t = get_base_address (t);
	  if (t && DECL_P (t)
              && DECL_MODE (t) != BLKmode)
	    TREE_ADDRESSABLE (t) = 1;
	}

      *walk_subtrees = 0;
    }

  return NULL_TREE;
}

/* RTL expansion is not able to compile array references with variable
   offsets for arrays stored in single register.  Discover such
   expressions and mark variables as addressable to avoid this
   scenario.  */

static void
discover_nonconstant_array_refs (void)
{
  basic_block bb;
  gimple_stmt_iterator gsi;

  FOR_EACH_BB (bb)
    for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
      {
	gimple stmt = gsi_stmt (gsi);
	if (!is_gimple_debug (stmt))
	  walk_gimple_op (stmt, discover_nonconstant_array_refs_r, NULL);
      }
}

/* This function sets crtl->args.internal_arg_pointer to a virtual
   register if DRAP is needed.  Local register allocator will replace
   virtual_incoming_args_rtx with the virtual register.  */

static void
expand_stack_alignment (void)
{
  rtx drap_rtx;
  unsigned int preferred_stack_boundary;

  if (! SUPPORTS_STACK_ALIGNMENT)
    return;

  if (cfun->calls_alloca
      || cfun->has_nonlocal_label
      || crtl->has_nonlocal_goto)
    crtl->need_drap = true;

  /* Call update_stack_boundary here again to update incoming stack
     boundary.  It may set incoming stack alignment to a different
     value after RTL expansion.  TARGET_FUNCTION_OK_FOR_SIBCALL may
     use the minimum incoming stack alignment to check if it is OK
     to perform sibcall optimization since sibcall optimization will
     only align the outgoing stack to incoming stack boundary.  */
  if (targetm.calls.update_stack_boundary)
    targetm.calls.update_stack_boundary ();

  /* The incoming stack frame has to be aligned at least at
     parm_stack_boundary.  */
  gcc_assert (crtl->parm_stack_boundary <= INCOMING_STACK_BOUNDARY);

  /* Update crtl->stack_alignment_estimated and use it later to align
     stack.  We check PREFERRED_STACK_BOUNDARY if there may be non-call
     exceptions since callgraph doesn't collect incoming stack alignment
     in this case.  */
  if (flag_non_call_exceptions
      && PREFERRED_STACK_BOUNDARY > crtl->preferred_stack_boundary)
    preferred_stack_boundary = PREFERRED_STACK_BOUNDARY;
  else
    preferred_stack_boundary = crtl->preferred_stack_boundary;
  if (preferred_stack_boundary > crtl->stack_alignment_estimated)
    crtl->stack_alignment_estimated = preferred_stack_boundary;
  if (preferred_stack_boundary > crtl->stack_alignment_needed)
    crtl->stack_alignment_needed = preferred_stack_boundary;

  gcc_assert (crtl->stack_alignment_needed
	      <= crtl->stack_alignment_estimated);

  crtl->stack_realign_needed
    = INCOMING_STACK_BOUNDARY < crtl->stack_alignment_estimated;
  crtl->stack_realign_tried = crtl->stack_realign_needed;

  crtl->stack_realign_processed = true;

  /* Target has to redefine TARGET_GET_DRAP_RTX to support stack
     alignment.  */
  gcc_assert (targetm.calls.get_drap_rtx != NULL);
  drap_rtx = targetm.calls.get_drap_rtx ();

  /* stack_realign_drap and drap_rtx must match.  */
  gcc_assert ((stack_realign_drap != 0) == (drap_rtx != NULL));

  /* Do nothing if NULL is returned, which means DRAP is not needed.  */
  if (NULL != drap_rtx)
    {
      crtl->args.internal_arg_pointer = drap_rtx;

      /* Call fixup_tail_calls to clean up REG_EQUIV note if DRAP is
         needed. */
      fixup_tail_calls ();
    }
}

/* Translate the intermediate representation contained in the CFG
   from GIMPLE trees to RTL.

   We do conversion per basic block and preserve/update the tree CFG.
   This implies we have to do some magic as the CFG can simultaneously
   consist of basic blocks containing RTL and GIMPLE trees.  This can
   confuse the CFG hooks, so be careful to not manipulate CFG during
   the expansion.  */

static unsigned int
gimple_expand_cfg (void)
{
  basic_block bb, init_block;
  sbitmap blocks;
  edge_iterator ei;
  edge e;
  unsigned i;

  rewrite_out_of_ssa (&SA);
  SA.partition_to_pseudo = (rtx *)xcalloc (SA.map->num_partitions,
					   sizeof (rtx));

  /* Some backends want to know that we are expanding to RTL.  */
  currently_expanding_to_rtl = 1;

  rtl_profile_for_bb (ENTRY_BLOCK_PTR);

  insn_locators_alloc ();
  if (!DECL_IS_BUILTIN (current_function_decl))
    {
      /* Eventually, all FEs should explicitly set function_start_locus.  */
      if (cfun->function_start_locus == UNKNOWN_LOCATION)
       set_curr_insn_source_location
         (DECL_SOURCE_LOCATION (current_function_decl));
      else
       set_curr_insn_source_location (cfun->function_start_locus);
    }
  set_curr_insn_block (DECL_INITIAL (current_function_decl));
  prologue_locator = curr_insn_locator ();

  /* Make sure first insn is a note even if we don't want linenums.
     This makes sure the first insn will never be deleted.
     Also, final expects a note to appear there.  */
  emit_note (NOTE_INSN_DELETED);

  /* Mark arrays indexed with non-constant indices with TREE_ADDRESSABLE.  */
  discover_nonconstant_array_refs ();

  targetm.expand_to_rtl_hook ();
  crtl->stack_alignment_needed = STACK_BOUNDARY;
  crtl->max_used_stack_slot_alignment = STACK_BOUNDARY;
  crtl->stack_alignment_estimated = 0;
  crtl->preferred_stack_boundary = STACK_BOUNDARY;
  cfun->cfg->max_jumptable_ents = 0;


  /* Expand the variables recorded during gimple lowering.  */
  expand_used_vars ();

  /* Honor stack protection warnings.  */
  if (warn_stack_protect)
    {
      if (cfun->calls_alloca)
	warning (OPT_Wstack_protector,
		 "not protecting local variables: variable length buffer");
      if (has_short_buffer && !crtl->stack_protect_guard)
	warning (OPT_Wstack_protector,
		 "not protecting function: no buffer at least %d bytes long",
		 (int) PARAM_VALUE (PARAM_SSP_BUFFER_SIZE));
    }

  /* Set up parameters and prepare for return, for the function.  */
  expand_function_start (current_function_decl);

  /* Now that we also have the parameter RTXs, copy them over to our
     partitions.  */
  for (i = 0; i < SA.map->num_partitions; i++)
    {
      tree var = SSA_NAME_VAR (partition_to_var (SA.map, i));

      if (TREE_CODE (var) != VAR_DECL
	  && !SA.partition_to_pseudo[i])
	SA.partition_to_pseudo[i] = DECL_RTL_IF_SET (var);
      gcc_assert (SA.partition_to_pseudo[i]);

      /* If this decl was marked as living in multiple places, reset
         this now to NULL.  */
      if (DECL_RTL_IF_SET (var) == pc_rtx)
	SET_DECL_RTL (var, NULL);

      /* Some RTL parts really want to look at DECL_RTL(x) when x
         was a decl marked in REG_ATTR or MEM_ATTR.  We could use
	 SET_DECL_RTL here making this available, but that would mean
	 to select one of the potentially many RTLs for one DECL.  Instead
	 of doing that we simply reset the MEM_EXPR of the RTL in question,
	 then nobody can get at it and hence nobody can call DECL_RTL on it.  */
      if (!DECL_RTL_SET_P (var))
	{
	  if (MEM_P (SA.partition_to_pseudo[i]))
	    set_mem_expr (SA.partition_to_pseudo[i], NULL);
	}
    }

  /* If this function is `main', emit a call to `__main'
     to run global initializers, etc.  */
  if (DECL_NAME (current_function_decl)
      && MAIN_NAME_P (DECL_NAME (current_function_decl))
      && DECL_FILE_SCOPE_P (current_function_decl))
    expand_main_function ();

  /* Initialize the stack_protect_guard field.  This must happen after the
     call to __main (if any) so that the external decl is initialized.  */
  if (crtl->stack_protect_guard)
    stack_protect_prologue ();

  expand_phi_nodes (&SA);

  /* Register rtl specific functions for cfg.  */
  rtl_register_cfg_hooks ();

  init_block = construct_init_block ();

  /* Clear EDGE_EXECUTABLE on the entry edge(s).  It is cleaned from the
     remaining edges later.  */
  FOR_EACH_EDGE (e, ei, ENTRY_BLOCK_PTR->succs)
    e->flags &= ~EDGE_EXECUTABLE;

  lab_rtx_for_bb = pointer_map_create ();
  FOR_BB_BETWEEN (bb, init_block->next_bb, EXIT_BLOCK_PTR, next_bb)
    bb = expand_gimple_basic_block (bb);

  if (MAY_HAVE_DEBUG_INSNS)
    expand_debug_locations ();

  execute_free_datastructures ();
  finish_out_of_ssa (&SA);

  /* We are no longer in SSA form.  */
  cfun->gimple_df->in_ssa_p = false;

  /* Expansion is used by optimization passes too, set maybe_hot_insn_p
     conservatively to true until they are all profile aware.  */
  pointer_map_destroy (lab_rtx_for_bb);
  free_histograms ();

  construct_exit_block ();
  set_curr_insn_block (DECL_INITIAL (current_function_decl));
  insn_locators_finalize ();

  /* Zap the tree EH table.  */
  set_eh_throw_stmt_table (cfun, NULL);

  rebuild_jump_labels (get_insns ());

  FOR_BB_BETWEEN (bb, ENTRY_BLOCK_PTR, EXIT_BLOCK_PTR, next_bb)
    {
      edge e;
      edge_iterator ei;
      for (ei = ei_start (bb->succs); (e = ei_safe_edge (ei)); )
	{
	  if (e->insns.r)
	    commit_one_edge_insertion (e);
	  else
	    ei_next (&ei);
	}
    }

  /* We're done expanding trees to RTL.  */
  currently_expanding_to_rtl = 0;

  FOR_BB_BETWEEN (bb, ENTRY_BLOCK_PTR->next_bb, EXIT_BLOCK_PTR, next_bb)
    {
      edge e;
      edge_iterator ei;
      for (ei = ei_start (bb->succs); (e = ei_safe_edge (ei)); )
	{
	  /* Clear EDGE_EXECUTABLE.  This flag is never used in the backend.  */
	  e->flags &= ~EDGE_EXECUTABLE;

	  /* At the moment not all abnormal edges match the RTL
	     representation.  It is safe to remove them here as
	     find_many_sub_basic_blocks will rediscover them.
	     In the future we should get this fixed properly.  */
	  if ((e->flags & EDGE_ABNORMAL)
	      && !(e->flags & EDGE_SIBCALL))
	    remove_edge (e);
	  else
	    ei_next (&ei);
	}
    }

  blocks = sbitmap_alloc (last_basic_block);
  sbitmap_ones (blocks);
  find_many_sub_basic_blocks (blocks);
  sbitmap_free (blocks);
  purge_all_dead_edges ();

  compact_blocks ();

  expand_stack_alignment ();

#ifdef ENABLE_CHECKING
  verify_flow_info ();
#endif

  /* There's no need to defer outputting this function any more; we
     know we want to output it.  */
  DECL_DEFER_OUTPUT (current_function_decl) = 0;

  /* Now that we're done expanding trees to RTL, we shouldn't have any
     more CONCATs anywhere.  */
  generating_concat_p = 0;

  if (dump_file)
    {
      fprintf (dump_file,
	       "\n\n;;\n;; Full RTL generated for this function:\n;;\n");
      /* And the pass manager will dump RTL for us.  */
    }

  /* If we're emitting a nested function, make sure its parent gets
     emitted as well.  Doing otherwise confuses debug info.  */
  {
    tree parent;
    for (parent = DECL_CONTEXT (current_function_decl);
	 parent != NULL_TREE;
	 parent = get_containing_scope (parent))
      if (TREE_CODE (parent) == FUNCTION_DECL)
	TREE_SYMBOL_REFERENCED (DECL_ASSEMBLER_NAME (parent)) = 1;
  }

  /* We are now committed to emitting code for this function.  Do any
     preparation, such as emitting abstract debug info for the inline
     before it gets mangled by optimization.  */
  if (cgraph_function_possibly_inlined_p (current_function_decl))
    (*debug_hooks->outlining_inline_function) (current_function_decl);

  TREE_ASM_WRITTEN (current_function_decl) = 1;

  /* After expanding, the return labels are no longer needed. */
  return_label = NULL;
  naked_return_label = NULL;
  /* Tag the blocks with a depth number so that change_scope can find
     the common parent easily.  */
  set_block_levels (DECL_INITIAL (cfun->decl), 0);
  default_rtl_profile ();
  return 0;
}

struct rtl_opt_pass pass_expand =
{
 {
  RTL_PASS,
  "expand",				/* name */
  NULL,                                 /* gate */
  gimple_expand_cfg,			/* execute */
  NULL,                                 /* sub */
  NULL,                                 /* next */
  0,                                    /* static_pass_number */
  TV_EXPAND,				/* tv_id */
  PROP_ssa | PROP_gimple_leh | PROP_cfg
    | PROP_gimple_lcx,			/* properties_required */
  PROP_rtl,                             /* properties_provided */
  PROP_ssa | PROP_trees,		/* properties_destroyed */
  TODO_verify_ssa | TODO_verify_flow
    | TODO_verify_stmts,		/* todo_flags_start */
  TODO_dump_func
  | TODO_ggc_collect			/* todo_flags_finish */
 }
};
