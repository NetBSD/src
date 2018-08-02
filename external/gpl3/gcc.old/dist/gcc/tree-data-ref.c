/* Data references and dependences detectors.
   Copyright (C) 2003-2016 Free Software Foundation, Inc.
   Contributed by Sebastian Pop <pop@cri.ensmp.fr>

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

/* This pass walks a given loop structure searching for array
   references.  The information about the array accesses is recorded
   in DATA_REFERENCE structures.

   The basic test for determining the dependences is:
   given two access functions chrec1 and chrec2 to a same array, and
   x and y two vectors from the iteration domain, the same element of
   the array is accessed twice at iterations x and y if and only if:
   |             chrec1 (x) == chrec2 (y).

   The goals of this analysis are:

   - to determine the independence: the relation between two
     independent accesses is qualified with the chrec_known (this
     information allows a loop parallelization),

   - when two data references access the same data, to qualify the
     dependence relation with classic dependence representations:

       - distance vectors
       - direction vectors
       - loop carried level dependence
       - polyhedron dependence
     or with the chains of recurrences based representation,

   - to define a knowledge base for storing the data dependence
     information,

   - to define an interface to access this data.


   Definitions:

   - subscript: given two array accesses a subscript is the tuple
   composed of the access functions for a given dimension.  Example:
   Given A[f1][f2][f3] and B[g1][g2][g3], there are three subscripts:
   (f1, g1), (f2, g2), (f3, g3).

   - Diophantine equation: an equation whose coefficients and
   solutions are integer constants, for example the equation
   |   3*x + 2*y = 1
   has an integer solution x = 1 and y = -1.

   References:

   - "Advanced Compilation for High Performance Computing" by Randy
   Allen and Ken Kennedy.
   http://citeseer.ist.psu.edu/goff91practical.html

   - "Loop Transformations for Restructuring Compilers - The Foundations"
   by Utpal Banerjee.


*/

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "rtl.h"
#include "tree.h"
#include "gimple.h"
#include "gimple-pretty-print.h"
#include "alias.h"
#include "fold-const.h"
#include "expr.h"
#include "gimple-iterator.h"
#include "tree-ssa-loop-niter.h"
#include "tree-ssa-loop.h"
#include "tree-ssa.h"
#include "cfgloop.h"
#include "tree-data-ref.h"
#include "tree-scalar-evolution.h"
#include "dumpfile.h"
#include "tree-affine.h"
#include "params.h"

static struct datadep_stats
{
  int num_dependence_tests;
  int num_dependence_dependent;
  int num_dependence_independent;
  int num_dependence_undetermined;

  int num_subscript_tests;
  int num_subscript_undetermined;
  int num_same_subscript_function;

  int num_ziv;
  int num_ziv_independent;
  int num_ziv_dependent;
  int num_ziv_unimplemented;

  int num_siv;
  int num_siv_independent;
  int num_siv_dependent;
  int num_siv_unimplemented;

  int num_miv;
  int num_miv_independent;
  int num_miv_dependent;
  int num_miv_unimplemented;
} dependence_stats;

static bool subscript_dependence_tester_1 (struct data_dependence_relation *,
					   struct data_reference *,
					   struct data_reference *,
					   struct loop *);
/* Returns true iff A divides B.  */

static inline bool
tree_fold_divides_p (const_tree a, const_tree b)
{
  gcc_assert (TREE_CODE (a) == INTEGER_CST);
  gcc_assert (TREE_CODE (b) == INTEGER_CST);
  return integer_zerop (int_const_binop (TRUNC_MOD_EXPR, b, a));
}

/* Returns true iff A divides B.  */

static inline bool
int_divides_p (int a, int b)
{
  return ((b % a) == 0);
}



/* Dump into FILE all the data references from DATAREFS.  */

static void
dump_data_references (FILE *file, vec<data_reference_p> datarefs)
{
  unsigned int i;
  struct data_reference *dr;

  FOR_EACH_VEC_ELT (datarefs, i, dr)
    dump_data_reference (file, dr);
}

/* Unified dump into FILE all the data references from DATAREFS.  */

DEBUG_FUNCTION void
debug (vec<data_reference_p> &ref)
{
  dump_data_references (stderr, ref);
}

DEBUG_FUNCTION void
debug (vec<data_reference_p> *ptr)
{
  if (ptr)
    debug (*ptr);
  else
    fprintf (stderr, "<nil>\n");
}


/* Dump into STDERR all the data references from DATAREFS.  */

DEBUG_FUNCTION void
debug_data_references (vec<data_reference_p> datarefs)
{
  dump_data_references (stderr, datarefs);
}

/* Print to STDERR the data_reference DR.  */

DEBUG_FUNCTION void
debug_data_reference (struct data_reference *dr)
{
  dump_data_reference (stderr, dr);
}

/* Dump function for a DATA_REFERENCE structure.  */

void
dump_data_reference (FILE *outf,
		     struct data_reference *dr)
{
  unsigned int i;

  fprintf (outf, "#(Data Ref: \n");
  fprintf (outf, "#  bb: %d \n", gimple_bb (DR_STMT (dr))->index);
  fprintf (outf, "#  stmt: ");
  print_gimple_stmt (outf, DR_STMT (dr), 0, 0);
  fprintf (outf, "#  ref: ");
  print_generic_stmt (outf, DR_REF (dr), 0);
  fprintf (outf, "#  base_object: ");
  print_generic_stmt (outf, DR_BASE_OBJECT (dr), 0);

  for (i = 0; i < DR_NUM_DIMENSIONS (dr); i++)
    {
      fprintf (outf, "#  Access function %d: ", i);
      print_generic_stmt (outf, DR_ACCESS_FN (dr, i), 0);
    }
  fprintf (outf, "#)\n");
}

/* Unified dump function for a DATA_REFERENCE structure.  */

DEBUG_FUNCTION void
debug (data_reference &ref)
{
  dump_data_reference (stderr, &ref);
}

DEBUG_FUNCTION void
debug (data_reference *ptr)
{
  if (ptr)
    debug (*ptr);
  else
    fprintf (stderr, "<nil>\n");
}


/* Dumps the affine function described by FN to the file OUTF.  */

DEBUG_FUNCTION void
dump_affine_function (FILE *outf, affine_fn fn)
{
  unsigned i;
  tree coef;

  print_generic_expr (outf, fn[0], TDF_SLIM);
  for (i = 1; fn.iterate (i, &coef); i++)
    {
      fprintf (outf, " + ");
      print_generic_expr (outf, coef, TDF_SLIM);
      fprintf (outf, " * x_%u", i);
    }
}

/* Dumps the conflict function CF to the file OUTF.  */

DEBUG_FUNCTION void
dump_conflict_function (FILE *outf, conflict_function *cf)
{
  unsigned i;

  if (cf->n == NO_DEPENDENCE)
    fprintf (outf, "no dependence");
  else if (cf->n == NOT_KNOWN)
    fprintf (outf, "not known");
  else
    {
      for (i = 0; i < cf->n; i++)
	{
	  if (i != 0)
	    fprintf (outf, " ");
	  fprintf (outf, "[");
	  dump_affine_function (outf, cf->fns[i]);
	  fprintf (outf, "]");
	}
    }
}

/* Dump function for a SUBSCRIPT structure.  */

DEBUG_FUNCTION void
dump_subscript (FILE *outf, struct subscript *subscript)
{
  conflict_function *cf = SUB_CONFLICTS_IN_A (subscript);

  fprintf (outf, "\n (subscript \n");
  fprintf (outf, "  iterations_that_access_an_element_twice_in_A: ");
  dump_conflict_function (outf, cf);
  if (CF_NONTRIVIAL_P (cf))
    {
      tree last_iteration = SUB_LAST_CONFLICT (subscript);
      fprintf (outf, "\n  last_conflict: ");
      print_generic_expr (outf, last_iteration, 0);
    }

  cf = SUB_CONFLICTS_IN_B (subscript);
  fprintf (outf, "\n  iterations_that_access_an_element_twice_in_B: ");
  dump_conflict_function (outf, cf);
  if (CF_NONTRIVIAL_P (cf))
    {
      tree last_iteration = SUB_LAST_CONFLICT (subscript);
      fprintf (outf, "\n  last_conflict: ");
      print_generic_expr (outf, last_iteration, 0);
    }

  fprintf (outf, "\n  (Subscript distance: ");
  print_generic_expr (outf, SUB_DISTANCE (subscript), 0);
  fprintf (outf, " ))\n");
}

/* Print the classic direction vector DIRV to OUTF.  */

DEBUG_FUNCTION void
print_direction_vector (FILE *outf,
			lambda_vector dirv,
			int length)
{
  int eq;

  for (eq = 0; eq < length; eq++)
    {
      enum data_dependence_direction dir = ((enum data_dependence_direction)
					    dirv[eq]);

      switch (dir)
	{
	case dir_positive:
	  fprintf (outf, "    +");
	  break;
	case dir_negative:
	  fprintf (outf, "    -");
	  break;
	case dir_equal:
	  fprintf (outf, "    =");
	  break;
	case dir_positive_or_equal:
	  fprintf (outf, "   +=");
	  break;
	case dir_positive_or_negative:
	  fprintf (outf, "   +-");
	  break;
	case dir_negative_or_equal:
	  fprintf (outf, "   -=");
	  break;
	case dir_star:
	  fprintf (outf, "    *");
	  break;
	default:
	  fprintf (outf, "indep");
	  break;
	}
    }
  fprintf (outf, "\n");
}

/* Print a vector of direction vectors.  */

DEBUG_FUNCTION void
print_dir_vectors (FILE *outf, vec<lambda_vector> dir_vects,
		   int length)
{
  unsigned j;
  lambda_vector v;

  FOR_EACH_VEC_ELT (dir_vects, j, v)
    print_direction_vector (outf, v, length);
}

/* Print out a vector VEC of length N to OUTFILE.  */

DEBUG_FUNCTION void
print_lambda_vector (FILE * outfile, lambda_vector vector, int n)
{
  int i;

  for (i = 0; i < n; i++)
    fprintf (outfile, "%3d ", vector[i]);
  fprintf (outfile, "\n");
}

/* Print a vector of distance vectors.  */

DEBUG_FUNCTION void
print_dist_vectors (FILE *outf, vec<lambda_vector> dist_vects,
		    int length)
{
  unsigned j;
  lambda_vector v;

  FOR_EACH_VEC_ELT (dist_vects, j, v)
    print_lambda_vector (outf, v, length);
}

/* Dump function for a DATA_DEPENDENCE_RELATION structure.  */

DEBUG_FUNCTION void
dump_data_dependence_relation (FILE *outf,
			       struct data_dependence_relation *ddr)
{
  struct data_reference *dra, *drb;

  fprintf (outf, "(Data Dep: \n");

  if (!ddr || DDR_ARE_DEPENDENT (ddr) == chrec_dont_know)
    {
      if (ddr)
	{
	  dra = DDR_A (ddr);
	  drb = DDR_B (ddr);
	  if (dra)
	    dump_data_reference (outf, dra);
	  else
	    fprintf (outf, "    (nil)\n");
	  if (drb)
	    dump_data_reference (outf, drb);
	  else
	    fprintf (outf, "    (nil)\n");
	}
      fprintf (outf, "    (don't know)\n)\n");
      return;
    }

  dra = DDR_A (ddr);
  drb = DDR_B (ddr);
  dump_data_reference (outf, dra);
  dump_data_reference (outf, drb);

  if (DDR_ARE_DEPENDENT (ddr) == chrec_known)
    fprintf (outf, "    (no dependence)\n");

  else if (DDR_ARE_DEPENDENT (ddr) == NULL_TREE)
    {
      unsigned int i;
      struct loop *loopi;

      for (i = 0; i < DDR_NUM_SUBSCRIPTS (ddr); i++)
	{
	  fprintf (outf, "  access_fn_A: ");
	  print_generic_stmt (outf, DR_ACCESS_FN (dra, i), 0);
	  fprintf (outf, "  access_fn_B: ");
	  print_generic_stmt (outf, DR_ACCESS_FN (drb, i), 0);
	  dump_subscript (outf, DDR_SUBSCRIPT (ddr, i));
	}

      fprintf (outf, "  inner loop index: %d\n", DDR_INNER_LOOP (ddr));
      fprintf (outf, "  loop nest: (");
      FOR_EACH_VEC_ELT (DDR_LOOP_NEST (ddr), i, loopi)
	fprintf (outf, "%d ", loopi->num);
      fprintf (outf, ")\n");

      for (i = 0; i < DDR_NUM_DIST_VECTS (ddr); i++)
	{
	  fprintf (outf, "  distance_vector: ");
	  print_lambda_vector (outf, DDR_DIST_VECT (ddr, i),
			       DDR_NB_LOOPS (ddr));
	}

      for (i = 0; i < DDR_NUM_DIR_VECTS (ddr); i++)
	{
	  fprintf (outf, "  direction_vector: ");
	  print_direction_vector (outf, DDR_DIR_VECT (ddr, i),
				  DDR_NB_LOOPS (ddr));
	}
    }

  fprintf (outf, ")\n");
}

/* Debug version.  */

DEBUG_FUNCTION void
debug_data_dependence_relation (struct data_dependence_relation *ddr)
{
  dump_data_dependence_relation (stderr, ddr);
}

/* Dump into FILE all the dependence relations from DDRS.  */

DEBUG_FUNCTION void
dump_data_dependence_relations (FILE *file,
				vec<ddr_p> ddrs)
{
  unsigned int i;
  struct data_dependence_relation *ddr;

  FOR_EACH_VEC_ELT (ddrs, i, ddr)
    dump_data_dependence_relation (file, ddr);
}

DEBUG_FUNCTION void
debug (vec<ddr_p> &ref)
{
  dump_data_dependence_relations (stderr, ref);
}

DEBUG_FUNCTION void
debug (vec<ddr_p> *ptr)
{
  if (ptr)
    debug (*ptr);
  else
    fprintf (stderr, "<nil>\n");
}


/* Dump to STDERR all the dependence relations from DDRS.  */

DEBUG_FUNCTION void
debug_data_dependence_relations (vec<ddr_p> ddrs)
{
  dump_data_dependence_relations (stderr, ddrs);
}

/* Dumps the distance and direction vectors in FILE.  DDRS contains
   the dependence relations, and VECT_SIZE is the size of the
   dependence vectors, or in other words the number of loops in the
   considered nest.  */

DEBUG_FUNCTION void
dump_dist_dir_vectors (FILE *file, vec<ddr_p> ddrs)
{
  unsigned int i, j;
  struct data_dependence_relation *ddr;
  lambda_vector v;

  FOR_EACH_VEC_ELT (ddrs, i, ddr)
    if (DDR_ARE_DEPENDENT (ddr) == NULL_TREE && DDR_AFFINE_P (ddr))
      {
	FOR_EACH_VEC_ELT (DDR_DIST_VECTS (ddr), j, v)
	  {
	    fprintf (file, "DISTANCE_V (");
	    print_lambda_vector (file, v, DDR_NB_LOOPS (ddr));
	    fprintf (file, ")\n");
	  }

	FOR_EACH_VEC_ELT (DDR_DIR_VECTS (ddr), j, v)
	  {
	    fprintf (file, "DIRECTION_V (");
	    print_direction_vector (file, v, DDR_NB_LOOPS (ddr));
	    fprintf (file, ")\n");
	  }
      }

  fprintf (file, "\n\n");
}

/* Dumps the data dependence relations DDRS in FILE.  */

DEBUG_FUNCTION void
dump_ddrs (FILE *file, vec<ddr_p> ddrs)
{
  unsigned int i;
  struct data_dependence_relation *ddr;

  FOR_EACH_VEC_ELT (ddrs, i, ddr)
    dump_data_dependence_relation (file, ddr);

  fprintf (file, "\n\n");
}

DEBUG_FUNCTION void
debug_ddrs (vec<ddr_p> ddrs)
{
  dump_ddrs (stderr, ddrs);
}

/* Helper function for split_constant_offset.  Expresses OP0 CODE OP1
   (the type of the result is TYPE) as VAR + OFF, where OFF is a nonzero
   constant of type ssizetype, and returns true.  If we cannot do this
   with OFF nonzero, OFF and VAR are set to NULL_TREE instead and false
   is returned.  */

static bool
split_constant_offset_1 (tree type, tree op0, enum tree_code code, tree op1,
			 tree *var, tree *off)
{
  tree var0, var1;
  tree off0, off1;
  enum tree_code ocode = code;

  *var = NULL_TREE;
  *off = NULL_TREE;

  switch (code)
    {
    case INTEGER_CST:
      *var = build_int_cst (type, 0);
      *off = fold_convert (ssizetype, op0);
      return true;

    case POINTER_PLUS_EXPR:
      ocode = PLUS_EXPR;
      /* FALLTHROUGH */
    case PLUS_EXPR:
    case MINUS_EXPR:
      split_constant_offset (op0, &var0, &off0);
      split_constant_offset (op1, &var1, &off1);
      *var = fold_build2 (code, type, var0, var1);
      *off = size_binop (ocode, off0, off1);
      return true;

    case MULT_EXPR:
      if (TREE_CODE (op1) != INTEGER_CST)
	return false;

      split_constant_offset (op0, &var0, &off0);
      *var = fold_build2 (MULT_EXPR, type, var0, op1);
      *off = size_binop (MULT_EXPR, off0, fold_convert (ssizetype, op1));
      return true;

    case ADDR_EXPR:
      {
	tree base, poffset;
	HOST_WIDE_INT pbitsize, pbitpos;
	machine_mode pmode;
	int punsignedp, preversep, pvolatilep;

	op0 = TREE_OPERAND (op0, 0);
	base
	  = get_inner_reference (op0, &pbitsize, &pbitpos, &poffset, &pmode,
				 &punsignedp, &preversep, &pvolatilep, false);

	if (pbitpos % BITS_PER_UNIT != 0)
	  return false;
	base = build_fold_addr_expr (base);
	off0 = ssize_int (pbitpos / BITS_PER_UNIT);

	if (poffset)
	  {
	    split_constant_offset (poffset, &poffset, &off1);
	    off0 = size_binop (PLUS_EXPR, off0, off1);
	    if (POINTER_TYPE_P (TREE_TYPE (base)))
	      base = fold_build_pointer_plus (base, poffset);
	    else
	      base = fold_build2 (PLUS_EXPR, TREE_TYPE (base), base,
				  fold_convert (TREE_TYPE (base), poffset));
	  }

	var0 = fold_convert (type, base);

	/* If variable length types are involved, punt, otherwise casts
	   might be converted into ARRAY_REFs in gimplify_conversion.
	   To compute that ARRAY_REF's element size TYPE_SIZE_UNIT, which
	   possibly no longer appears in current GIMPLE, might resurface.
	   This perhaps could run
	   if (CONVERT_EXPR_P (var0))
	     {
	       gimplify_conversion (&var0);
	       // Attempt to fill in any within var0 found ARRAY_REF's
	       // element size from corresponding op embedded ARRAY_REF,
	       // if unsuccessful, just punt.
	     }  */
	while (POINTER_TYPE_P (type))
	  type = TREE_TYPE (type);
	if (int_size_in_bytes (type) < 0)
	  return false;

	*var = var0;
	*off = off0;
	return true;
      }

    case SSA_NAME:
      {
	if (SSA_NAME_OCCURS_IN_ABNORMAL_PHI (op0))
	  return false;

	gimple *def_stmt = SSA_NAME_DEF_STMT (op0);
	enum tree_code subcode;

	if (gimple_code (def_stmt) != GIMPLE_ASSIGN)
	  return false;

	var0 = gimple_assign_rhs1 (def_stmt);
	subcode = gimple_assign_rhs_code (def_stmt);
	var1 = gimple_assign_rhs2 (def_stmt);

	return split_constant_offset_1 (type, var0, subcode, var1, var, off);
      }
    CASE_CONVERT:
      {
	/* We must not introduce undefined overflow, and we must not change the value.
	   Hence we're okay if the inner type doesn't overflow to start with
	   (pointer or signed), the outer type also is an integer or pointer
	   and the outer precision is at least as large as the inner.  */
	tree itype = TREE_TYPE (op0);
	if ((POINTER_TYPE_P (itype)
	     || (INTEGRAL_TYPE_P (itype) && TYPE_OVERFLOW_UNDEFINED (itype)))
	    && TYPE_PRECISION (type) >= TYPE_PRECISION (itype)
	    && (POINTER_TYPE_P (type) || INTEGRAL_TYPE_P (type)))
	  {
	    split_constant_offset (op0, &var0, off);
	    *var = fold_convert (type, var0);
	    return true;
	  }
	return false;
      }

    default:
      return false;
    }
}

/* Expresses EXP as VAR + OFF, where off is a constant.  The type of OFF
   will be ssizetype.  */

void
split_constant_offset (tree exp, tree *var, tree *off)
{
  tree type = TREE_TYPE (exp), otype, op0, op1, e, o;
  enum tree_code code;

  *var = exp;
  *off = ssize_int (0);
  STRIP_NOPS (exp);

  if (tree_is_chrec (exp)
      || get_gimple_rhs_class (TREE_CODE (exp)) == GIMPLE_TERNARY_RHS)
    return;

  otype = TREE_TYPE (exp);
  code = TREE_CODE (exp);
  extract_ops_from_tree (exp, &code, &op0, &op1);
  if (split_constant_offset_1 (otype, op0, code, op1, &e, &o))
    {
      *var = fold_convert (type, e);
      *off = o;
    }
}

/* Returns the address ADDR of an object in a canonical shape (without nop
   casts, and with type of pointer to the object).  */

static tree
canonicalize_base_object_address (tree addr)
{
  tree orig = addr;

  STRIP_NOPS (addr);

  /* The base address may be obtained by casting from integer, in that case
     keep the cast.  */
  if (!POINTER_TYPE_P (TREE_TYPE (addr)))
    return orig;

  if (TREE_CODE (addr) != ADDR_EXPR)
    return addr;

  return build_fold_addr_expr (TREE_OPERAND (addr, 0));
}

/* Analyzes the behavior of the memory reference DR in the innermost loop or
   basic block that contains it.  Returns true if analysis succeed or false
   otherwise.  */

bool
dr_analyze_innermost (struct data_reference *dr, struct loop *nest)
{
  gimple *stmt = DR_STMT (dr);
  struct loop *loop = loop_containing_stmt (stmt);
  tree ref = DR_REF (dr);
  HOST_WIDE_INT pbitsize, pbitpos;
  tree base, poffset;
  machine_mode pmode;
  int punsignedp, preversep, pvolatilep;
  affine_iv base_iv, offset_iv;
  tree init, dinit, step;
  bool in_loop = (loop && loop->num);

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "analyze_innermost: ");

  base = get_inner_reference (ref, &pbitsize, &pbitpos, &poffset, &pmode,
			      &punsignedp, &preversep, &pvolatilep, false);
  gcc_assert (base != NULL_TREE);

  if (pbitpos % BITS_PER_UNIT != 0)
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "failed: bit offset alignment.\n");
      return false;
    }

  if (preversep)
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "failed: reverse storage order.\n");
      return false;
    }

  if (TREE_CODE (base) == MEM_REF)
    {
      if (!integer_zerop (TREE_OPERAND (base, 1)))
	{
	  offset_int moff = mem_ref_offset (base);
	  tree mofft = wide_int_to_tree (sizetype, moff);
	  if (!poffset)
	    poffset = mofft;
	  else
	    poffset = size_binop (PLUS_EXPR, poffset, mofft);
	}
      base = TREE_OPERAND (base, 0);
    }
  else
    base = build_fold_addr_expr (base);

  if (in_loop)
    {
      if (!simple_iv (loop, loop_containing_stmt (stmt), base, &base_iv,
                      nest ? true : false))
        {
          if (nest)
            {
              if (dump_file && (dump_flags & TDF_DETAILS))
                fprintf (dump_file, "failed: evolution of base is not"
                                    " affine.\n");
              return false;
            }
          else
            {
              base_iv.base = base;
              base_iv.step = ssize_int (0);
              base_iv.no_overflow = true;
            }
        }
    }
  else
    {
      base_iv.base = base;
      base_iv.step = ssize_int (0);
      base_iv.no_overflow = true;
    }

  if (!poffset)
    {
      offset_iv.base = ssize_int (0);
      offset_iv.step = ssize_int (0);
    }
  else
    {
      if (!in_loop)
        {
          offset_iv.base = poffset;
          offset_iv.step = ssize_int (0);
        }
      else if (!simple_iv (loop, loop_containing_stmt (stmt),
                           poffset, &offset_iv,
			   nest ? true : false))
        {
          if (nest)
            {
              if (dump_file && (dump_flags & TDF_DETAILS))
                fprintf (dump_file, "failed: evolution of offset is not"
                                    " affine.\n");
              return false;
            }
          else
            {
              offset_iv.base = poffset;
              offset_iv.step = ssize_int (0);
            }
        }
    }

  init = ssize_int (pbitpos / BITS_PER_UNIT);
  split_constant_offset (base_iv.base, &base_iv.base, &dinit);
  init =  size_binop (PLUS_EXPR, init, dinit);
  split_constant_offset (offset_iv.base, &offset_iv.base, &dinit);
  init =  size_binop (PLUS_EXPR, init, dinit);

  step = size_binop (PLUS_EXPR,
		     fold_convert (ssizetype, base_iv.step),
		     fold_convert (ssizetype, offset_iv.step));

  DR_BASE_ADDRESS (dr) = canonicalize_base_object_address (base_iv.base);

  DR_OFFSET (dr) = fold_convert (ssizetype, offset_iv.base);
  DR_INIT (dr) = init;
  DR_STEP (dr) = step;

  DR_ALIGNED_TO (dr) = size_int (highest_pow2_factor (offset_iv.base));

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "success.\n");

  return true;
}

/* Determines the base object and the list of indices of memory reference
   DR, analyzed in LOOP and instantiated in loop nest NEST.  */

static void
dr_analyze_indices (struct data_reference *dr, loop_p nest, loop_p loop)
{
  vec<tree> access_fns = vNULL;
  tree ref, op;
  tree base, off, access_fn;
  basic_block before_loop;

  /* If analyzing a basic-block there are no indices to analyze
     and thus no access functions.  */
  if (!nest)
    {
      DR_BASE_OBJECT (dr) = DR_REF (dr);
      DR_ACCESS_FNS (dr).create (0);
      return;
    }

  ref = DR_REF (dr);
  before_loop = block_before_loop (nest);

  /* REALPART_EXPR and IMAGPART_EXPR can be handled like accesses
     into a two element array with a constant index.  The base is
     then just the immediate underlying object.  */
  if (TREE_CODE (ref) == REALPART_EXPR)
    {
      ref = TREE_OPERAND (ref, 0);
      access_fns.safe_push (integer_zero_node);
    }
  else if (TREE_CODE (ref) == IMAGPART_EXPR)
    {
      ref = TREE_OPERAND (ref, 0);
      access_fns.safe_push (integer_one_node);
    }

  /* Analyze access functions of dimensions we know to be independent.  */
  while (handled_component_p (ref))
    {
      if (TREE_CODE (ref) == ARRAY_REF)
	{
	  op = TREE_OPERAND (ref, 1);
	  access_fn = analyze_scalar_evolution (loop, op);
	  access_fn = instantiate_scev (before_loop, loop, access_fn);
	  access_fns.safe_push (access_fn);
	}
      else if (TREE_CODE (ref) == COMPONENT_REF
	       && TREE_CODE (TREE_TYPE (TREE_OPERAND (ref, 0))) == RECORD_TYPE)
	{
	  /* For COMPONENT_REFs of records (but not unions!) use the
	     FIELD_DECL offset as constant access function so we can
	     disambiguate a[i].f1 and a[i].f2.  */
	  tree off = component_ref_field_offset (ref);
	  off = size_binop (PLUS_EXPR,
			    size_binop (MULT_EXPR,
					fold_convert (bitsizetype, off),
					bitsize_int (BITS_PER_UNIT)),
			    DECL_FIELD_BIT_OFFSET (TREE_OPERAND (ref, 1)));
	  access_fns.safe_push (off);
	}
      else
	/* If we have an unhandled component we could not translate
	   to an access function stop analyzing.  We have determined
	   our base object in this case.  */
	break;

      ref = TREE_OPERAND (ref, 0);
    }

  /* If the address operand of a MEM_REF base has an evolution in the
     analyzed nest, add it as an additional independent access-function.  */
  if (TREE_CODE (ref) == MEM_REF)
    {
      op = TREE_OPERAND (ref, 0);
      access_fn = analyze_scalar_evolution (loop, op);
      access_fn = instantiate_scev (before_loop, loop, access_fn);
      if (TREE_CODE (access_fn) == POLYNOMIAL_CHREC)
	{
	  tree orig_type;
	  tree memoff = TREE_OPERAND (ref, 1);
	  base = initial_condition (access_fn);
	  orig_type = TREE_TYPE (base);
	  STRIP_USELESS_TYPE_CONVERSION (base);
	  split_constant_offset (base, &base, &off);
	  STRIP_USELESS_TYPE_CONVERSION (base);
	  /* Fold the MEM_REF offset into the evolutions initial
	     value to make more bases comparable.  */
	  if (!integer_zerop (memoff))
	    {
	      off = size_binop (PLUS_EXPR, off,
				fold_convert (ssizetype, memoff));
	      memoff = build_int_cst (TREE_TYPE (memoff), 0);
	    }
	  /* Adjust the offset so it is a multiple of the access type
	     size and thus we separate bases that can possibly be used
	     to produce partial overlaps (which the access_fn machinery
	     cannot handle).  */
	  wide_int rem;
	  if (TYPE_SIZE_UNIT (TREE_TYPE (ref))
	      && TREE_CODE (TYPE_SIZE_UNIT (TREE_TYPE (ref))) == INTEGER_CST
	      && !integer_zerop (TYPE_SIZE_UNIT (TREE_TYPE (ref))))
	    rem = wi::mod_trunc (off, TYPE_SIZE_UNIT (TREE_TYPE (ref)), SIGNED);
	  else
	    /* If we can't compute the remainder simply force the initial
	       condition to zero.  */
	    rem = off;
	  off = wide_int_to_tree (ssizetype, wi::sub (off, rem));
	  memoff = wide_int_to_tree (TREE_TYPE (memoff), rem);
	  /* And finally replace the initial condition.  */
	  access_fn = chrec_replace_initial_condition
	      (access_fn, fold_convert (orig_type, off));
	  /* ???  This is still not a suitable base object for
	     dr_may_alias_p - the base object needs to be an
	     access that covers the object as whole.  With
	     an evolution in the pointer this cannot be
	     guaranteed.
	     As a band-aid, mark the access so we can special-case
	     it in dr_may_alias_p.  */
	  tree old = ref;
	  ref = fold_build2_loc (EXPR_LOCATION (ref),
				 MEM_REF, TREE_TYPE (ref),
				 base, memoff);
	  MR_DEPENDENCE_CLIQUE (ref) = MR_DEPENDENCE_CLIQUE (old);
	  MR_DEPENDENCE_BASE (ref) = MR_DEPENDENCE_BASE (old);
	  DR_UNCONSTRAINED_BASE (dr) = true;
	  access_fns.safe_push (access_fn);
	}
    }
  else if (DECL_P (ref))
    {
      /* Canonicalize DR_BASE_OBJECT to MEM_REF form.  */
      ref = build2 (MEM_REF, TREE_TYPE (ref),
		    build_fold_addr_expr (ref),
		    build_int_cst (reference_alias_ptr_type (ref), 0));
    }

  DR_BASE_OBJECT (dr) = ref;
  DR_ACCESS_FNS (dr) = access_fns;
}

/* Extracts the alias analysis information from the memory reference DR.  */

static void
dr_analyze_alias (struct data_reference *dr)
{
  tree ref = DR_REF (dr);
  tree base = get_base_address (ref), addr;

  if (INDIRECT_REF_P (base)
      || TREE_CODE (base) == MEM_REF)
    {
      addr = TREE_OPERAND (base, 0);
      if (TREE_CODE (addr) == SSA_NAME)
	DR_PTR_INFO (dr) = SSA_NAME_PTR_INFO (addr);
    }
}

/* Frees data reference DR.  */

void
free_data_ref (data_reference_p dr)
{
  DR_ACCESS_FNS (dr).release ();
  free (dr);
}

/* Analyzes memory reference MEMREF accessed in STMT.  The reference
   is read if IS_READ is true, write otherwise.  Returns the
   data_reference description of MEMREF.  NEST is the outermost loop
   in which the reference should be instantiated, LOOP is the loop in
   which the data reference should be analyzed.  */

struct data_reference *
create_data_ref (loop_p nest, loop_p loop, tree memref, gimple *stmt,
		 bool is_read)
{
  struct data_reference *dr;

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Creating dr for ");
      print_generic_expr (dump_file, memref, TDF_SLIM);
      fprintf (dump_file, "\n");
    }

  dr = XCNEW (struct data_reference);
  DR_STMT (dr) = stmt;
  DR_REF (dr) = memref;
  DR_IS_READ (dr) = is_read;

  dr_analyze_innermost (dr, nest);
  dr_analyze_indices (dr, nest, loop);
  dr_analyze_alias (dr);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      unsigned i;
      fprintf (dump_file, "\tbase_address: ");
      print_generic_expr (dump_file, DR_BASE_ADDRESS (dr), TDF_SLIM);
      fprintf (dump_file, "\n\toffset from base address: ");
      print_generic_expr (dump_file, DR_OFFSET (dr), TDF_SLIM);
      fprintf (dump_file, "\n\tconstant offset from base address: ");
      print_generic_expr (dump_file, DR_INIT (dr), TDF_SLIM);
      fprintf (dump_file, "\n\tstep: ");
      print_generic_expr (dump_file, DR_STEP (dr), TDF_SLIM);
      fprintf (dump_file, "\n\taligned to: ");
      print_generic_expr (dump_file, DR_ALIGNED_TO (dr), TDF_SLIM);
      fprintf (dump_file, "\n\tbase_object: ");
      print_generic_expr (dump_file, DR_BASE_OBJECT (dr), TDF_SLIM);
      fprintf (dump_file, "\n");
      for (i = 0; i < DR_NUM_DIMENSIONS (dr); i++)
	{
	  fprintf (dump_file, "\tAccess function %d: ", i);
	  print_generic_stmt (dump_file, DR_ACCESS_FN (dr, i), TDF_SLIM);
	}
    }

  return dr;
}

/* Check if OFFSET1 and OFFSET2 (DR_OFFSETs of some data-refs) are identical
   expressions.  */
static bool
dr_equal_offsets_p1 (tree offset1, tree offset2)
{
  bool res;

  STRIP_NOPS (offset1);
  STRIP_NOPS (offset2);

  if (offset1 == offset2)
    return true;

  if (TREE_CODE (offset1) != TREE_CODE (offset2)
      || (!BINARY_CLASS_P (offset1) && !UNARY_CLASS_P (offset1)))
    return false;

  res = dr_equal_offsets_p1 (TREE_OPERAND (offset1, 0),
                             TREE_OPERAND (offset2, 0));

  if (!res || !BINARY_CLASS_P (offset1))
    return res;

  res = dr_equal_offsets_p1 (TREE_OPERAND (offset1, 1),
                             TREE_OPERAND (offset2, 1));

  return res;
}

/* Check if DRA and DRB have equal offsets.  */
bool
dr_equal_offsets_p (struct data_reference *dra,
                    struct data_reference *drb)
{
  tree offset1, offset2;

  offset1 = DR_OFFSET (dra);
  offset2 = DR_OFFSET (drb);

  return dr_equal_offsets_p1 (offset1, offset2);
}

/* Returns true if FNA == FNB.  */

static bool
affine_function_equal_p (affine_fn fna, affine_fn fnb)
{
  unsigned i, n = fna.length ();

  if (n != fnb.length ())
    return false;

  for (i = 0; i < n; i++)
    if (!operand_equal_p (fna[i], fnb[i], 0))
      return false;

  return true;
}

/* If all the functions in CF are the same, returns one of them,
   otherwise returns NULL.  */

static affine_fn
common_affine_function (conflict_function *cf)
{
  unsigned i;
  affine_fn comm;

  if (!CF_NONTRIVIAL_P (cf))
    return affine_fn ();

  comm = cf->fns[0];

  for (i = 1; i < cf->n; i++)
    if (!affine_function_equal_p (comm, cf->fns[i]))
      return affine_fn ();

  return comm;
}

/* Returns the base of the affine function FN.  */

static tree
affine_function_base (affine_fn fn)
{
  return fn[0];
}

/* Returns true if FN is a constant.  */

static bool
affine_function_constant_p (affine_fn fn)
{
  unsigned i;
  tree coef;

  for (i = 1; fn.iterate (i, &coef); i++)
    if (!integer_zerop (coef))
      return false;

  return true;
}

/* Returns true if FN is the zero constant function.  */

static bool
affine_function_zero_p (affine_fn fn)
{
  return (integer_zerop (affine_function_base (fn))
	  && affine_function_constant_p (fn));
}

/* Returns a signed integer type with the largest precision from TA
   and TB.  */

static tree
signed_type_for_types (tree ta, tree tb)
{
  if (TYPE_PRECISION (ta) > TYPE_PRECISION (tb))
    return signed_type_for (ta);
  else
    return signed_type_for (tb);
}

/* Applies operation OP on affine functions FNA and FNB, and returns the
   result.  */

static affine_fn
affine_fn_op (enum tree_code op, affine_fn fna, affine_fn fnb)
{
  unsigned i, n, m;
  affine_fn ret;
  tree coef;

  if (fnb.length () > fna.length ())
    {
      n = fna.length ();
      m = fnb.length ();
    }
  else
    {
      n = fnb.length ();
      m = fna.length ();
    }

  ret.create (m);
  for (i = 0; i < n; i++)
    {
      tree type = signed_type_for_types (TREE_TYPE (fna[i]),
					 TREE_TYPE (fnb[i]));
      ret.quick_push (fold_build2 (op, type, fna[i], fnb[i]));
    }

  for (; fna.iterate (i, &coef); i++)
    ret.quick_push (fold_build2 (op, signed_type_for (TREE_TYPE (coef)),
				 coef, integer_zero_node));
  for (; fnb.iterate (i, &coef); i++)
    ret.quick_push (fold_build2 (op, signed_type_for (TREE_TYPE (coef)),
				 integer_zero_node, coef));

  return ret;
}

/* Returns the sum of affine functions FNA and FNB.  */

static affine_fn
affine_fn_plus (affine_fn fna, affine_fn fnb)
{
  return affine_fn_op (PLUS_EXPR, fna, fnb);
}

/* Returns the difference of affine functions FNA and FNB.  */

static affine_fn
affine_fn_minus (affine_fn fna, affine_fn fnb)
{
  return affine_fn_op (MINUS_EXPR, fna, fnb);
}

/* Frees affine function FN.  */

static void
affine_fn_free (affine_fn fn)
{
  fn.release ();
}

/* Determine for each subscript in the data dependence relation DDR
   the distance.  */

static void
compute_subscript_distance (struct data_dependence_relation *ddr)
{
  conflict_function *cf_a, *cf_b;
  affine_fn fn_a, fn_b, diff;

  if (DDR_ARE_DEPENDENT (ddr) == NULL_TREE)
    {
      unsigned int i;

      for (i = 0; i < DDR_NUM_SUBSCRIPTS (ddr); i++)
 	{
 	  struct subscript *subscript;

 	  subscript = DDR_SUBSCRIPT (ddr, i);
 	  cf_a = SUB_CONFLICTS_IN_A (subscript);
 	  cf_b = SUB_CONFLICTS_IN_B (subscript);

	  fn_a = common_affine_function (cf_a);
	  fn_b = common_affine_function (cf_b);
	  if (!fn_a.exists () || !fn_b.exists ())
	    {
	      SUB_DISTANCE (subscript) = chrec_dont_know;
	      return;
	    }
	  diff = affine_fn_minus (fn_a, fn_b);

 	  if (affine_function_constant_p (diff))
 	    SUB_DISTANCE (subscript) = affine_function_base (diff);
 	  else
 	    SUB_DISTANCE (subscript) = chrec_dont_know;

	  affine_fn_free (diff);
 	}
    }
}

/* Returns the conflict function for "unknown".  */

static conflict_function *
conflict_fn_not_known (void)
{
  conflict_function *fn = XCNEW (conflict_function);
  fn->n = NOT_KNOWN;

  return fn;
}

/* Returns the conflict function for "independent".  */

static conflict_function *
conflict_fn_no_dependence (void)
{
  conflict_function *fn = XCNEW (conflict_function);
  fn->n = NO_DEPENDENCE;

  return fn;
}

/* Returns true if the address of OBJ is invariant in LOOP.  */

static bool
object_address_invariant_in_loop_p (const struct loop *loop, const_tree obj)
{
  while (handled_component_p (obj))
    {
      if (TREE_CODE (obj) == ARRAY_REF)
	{
	  /* Index of the ARRAY_REF was zeroed in analyze_indices, thus we only
	     need to check the stride and the lower bound of the reference.  */
	  if (chrec_contains_symbols_defined_in_loop (TREE_OPERAND (obj, 2),
						      loop->num)
	      || chrec_contains_symbols_defined_in_loop (TREE_OPERAND (obj, 3),
							 loop->num))
	    return false;
	}
      else if (TREE_CODE (obj) == COMPONENT_REF)
	{
	  if (chrec_contains_symbols_defined_in_loop (TREE_OPERAND (obj, 2),
						      loop->num))
	    return false;
	}
      obj = TREE_OPERAND (obj, 0);
    }

  if (!INDIRECT_REF_P (obj)
      && TREE_CODE (obj) != MEM_REF)
    return true;

  return !chrec_contains_symbols_defined_in_loop (TREE_OPERAND (obj, 0),
						  loop->num);
}

/* Returns false if we can prove that data references A and B do not alias,
   true otherwise.  If LOOP_NEST is false no cross-iteration aliases are
   considered.  */

bool
dr_may_alias_p (const struct data_reference *a, const struct data_reference *b,
		bool loop_nest)
{
  tree addr_a = DR_BASE_OBJECT (a);
  tree addr_b = DR_BASE_OBJECT (b);

  /* If we are not processing a loop nest but scalar code we
     do not need to care about possible cross-iteration dependences
     and thus can process the full original reference.  Do so,
     similar to how loop invariant motion applies extra offset-based
     disambiguation.  */
  if (!loop_nest)
    {
      aff_tree off1, off2;
      widest_int size1, size2;
      get_inner_reference_aff (DR_REF (a), &off1, &size1);
      get_inner_reference_aff (DR_REF (b), &off2, &size2);
      aff_combination_scale (&off1, -1);
      aff_combination_add (&off2, &off1);
      if (aff_comb_cannot_overlap_p (&off2, size1, size2))
	return false;
    }

  if ((TREE_CODE (addr_a) == MEM_REF || TREE_CODE (addr_a) == TARGET_MEM_REF)
      && (TREE_CODE (addr_b) == MEM_REF || TREE_CODE (addr_b) == TARGET_MEM_REF)
      && MR_DEPENDENCE_CLIQUE (addr_a) == MR_DEPENDENCE_CLIQUE (addr_b)
      && MR_DEPENDENCE_BASE (addr_a) != MR_DEPENDENCE_BASE (addr_b))
    return false;

  /* If we had an evolution in a pointer-based MEM_REF BASE_OBJECT we
     do not know the size of the base-object.  So we cannot do any
     offset/overlap based analysis but have to rely on points-to
     information only.  */
  if (TREE_CODE (addr_a) == MEM_REF
      && (DR_UNCONSTRAINED_BASE (a)
	  || TREE_CODE (TREE_OPERAND (addr_a, 0)) == SSA_NAME))
    {
      /* For true dependences we can apply TBAA.  */
      if (flag_strict_aliasing
	  && DR_IS_WRITE (a) && DR_IS_READ (b)
	  && !alias_sets_conflict_p (get_alias_set (DR_REF (a)),
				     get_alias_set (DR_REF (b))))
	return false;
      if (TREE_CODE (addr_b) == MEM_REF)
	return ptr_derefs_may_alias_p (TREE_OPERAND (addr_a, 0),
				       TREE_OPERAND (addr_b, 0));
      else
	return ptr_derefs_may_alias_p (TREE_OPERAND (addr_a, 0),
				       build_fold_addr_expr (addr_b));
    }
  else if (TREE_CODE (addr_b) == MEM_REF
	   && (DR_UNCONSTRAINED_BASE (b)
	       || TREE_CODE (TREE_OPERAND (addr_b, 0)) == SSA_NAME))
    {
      /* For true dependences we can apply TBAA.  */
      if (flag_strict_aliasing
	  && DR_IS_WRITE (a) && DR_IS_READ (b)
	  && !alias_sets_conflict_p (get_alias_set (DR_REF (a)),
				     get_alias_set (DR_REF (b))))
	return false;
      if (TREE_CODE (addr_a) == MEM_REF)
	return ptr_derefs_may_alias_p (TREE_OPERAND (addr_a, 0),
				       TREE_OPERAND (addr_b, 0));
      else
	return ptr_derefs_may_alias_p (build_fold_addr_expr (addr_a),
				       TREE_OPERAND (addr_b, 0));
    }

  /* Otherwise DR_BASE_OBJECT is an access that covers the whole object
     that is being subsetted in the loop nest.  */
  if (DR_IS_WRITE (a) && DR_IS_WRITE (b))
    return refs_output_dependent_p (addr_a, addr_b);
  else if (DR_IS_READ (a) && DR_IS_WRITE (b))
    return refs_anti_dependent_p (addr_a, addr_b);
  return refs_may_alias_p (addr_a, addr_b);
}

/* Initialize a data dependence relation between data accesses A and
   B.  NB_LOOPS is the number of loops surrounding the references: the
   size of the classic distance/direction vectors.  */

struct data_dependence_relation *
initialize_data_dependence_relation (struct data_reference *a,
				     struct data_reference *b,
 				     vec<loop_p> loop_nest)
{
  struct data_dependence_relation *res;
  unsigned int i;

  res = XNEW (struct data_dependence_relation);
  DDR_A (res) = a;
  DDR_B (res) = b;
  DDR_LOOP_NEST (res).create (0);
  DDR_REVERSED_P (res) = false;
  DDR_SUBSCRIPTS (res).create (0);
  DDR_DIR_VECTS (res).create (0);
  DDR_DIST_VECTS (res).create (0);

  if (a == NULL || b == NULL)
    {
      DDR_ARE_DEPENDENT (res) = chrec_dont_know;
      return res;
    }

  /* If the data references do not alias, then they are independent.  */
  if (!dr_may_alias_p (a, b, loop_nest.exists ()))
    {
      DDR_ARE_DEPENDENT (res) = chrec_known;
      return res;
    }

  /* The case where the references are exactly the same.  */
  if (operand_equal_p (DR_REF (a), DR_REF (b), 0))
    {
      if ((loop_nest.exists ()
	   && !object_address_invariant_in_loop_p (loop_nest[0],
						   DR_BASE_OBJECT (a)))
	  || DR_NUM_DIMENSIONS (a) == 0)
	{
	  DDR_ARE_DEPENDENT (res) = chrec_dont_know;
	  return res;
	}
      DDR_AFFINE_P (res) = true;
      DDR_ARE_DEPENDENT (res) = NULL_TREE;
      DDR_SUBSCRIPTS (res).create (DR_NUM_DIMENSIONS (a));
      DDR_LOOP_NEST (res) = loop_nest;
      DDR_INNER_LOOP (res) = 0;
      DDR_SELF_REFERENCE (res) = true;
      for (i = 0; i < DR_NUM_DIMENSIONS (a); i++)
       {
         struct subscript *subscript;

         subscript = XNEW (struct subscript);
         SUB_CONFLICTS_IN_A (subscript) = conflict_fn_not_known ();
         SUB_CONFLICTS_IN_B (subscript) = conflict_fn_not_known ();
         SUB_LAST_CONFLICT (subscript) = chrec_dont_know;
         SUB_DISTANCE (subscript) = chrec_dont_know;
         DDR_SUBSCRIPTS (res).safe_push (subscript);
       }
      return res;
    }

  /* If the references do not access the same object, we do not know
     whether they alias or not.  */
  if (!operand_equal_p (DR_BASE_OBJECT (a), DR_BASE_OBJECT (b), 0))
    {
      DDR_ARE_DEPENDENT (res) = chrec_dont_know;
      return res;
    }

  /* If the base of the object is not invariant in the loop nest, we cannot
     analyze it.  TODO -- in fact, it would suffice to record that there may
     be arbitrary dependences in the loops where the base object varies.  */
  if ((loop_nest.exists ()
       && !object_address_invariant_in_loop_p (loop_nest[0], DR_BASE_OBJECT (a)))
      || DR_NUM_DIMENSIONS (a) == 0)
    {
      DDR_ARE_DEPENDENT (res) = chrec_dont_know;
      return res;
    }

  /* If the number of dimensions of the access to not agree we can have
     a pointer access to a component of the array element type and an
     array access while the base-objects are still the same.  Punt.  */
  if (DR_NUM_DIMENSIONS (a) != DR_NUM_DIMENSIONS (b))
    {
      DDR_ARE_DEPENDENT (res) = chrec_dont_know;
      return res;
    }

  DDR_AFFINE_P (res) = true;
  DDR_ARE_DEPENDENT (res) = NULL_TREE;
  DDR_SUBSCRIPTS (res).create (DR_NUM_DIMENSIONS (a));
  DDR_LOOP_NEST (res) = loop_nest;
  DDR_INNER_LOOP (res) = 0;
  DDR_SELF_REFERENCE (res) = false;

  for (i = 0; i < DR_NUM_DIMENSIONS (a); i++)
    {
      struct subscript *subscript;

      subscript = XNEW (struct subscript);
      SUB_CONFLICTS_IN_A (subscript) = conflict_fn_not_known ();
      SUB_CONFLICTS_IN_B (subscript) = conflict_fn_not_known ();
      SUB_LAST_CONFLICT (subscript) = chrec_dont_know;
      SUB_DISTANCE (subscript) = chrec_dont_know;
      DDR_SUBSCRIPTS (res).safe_push (subscript);
    }

  return res;
}

/* Frees memory used by the conflict function F.  */

static void
free_conflict_function (conflict_function *f)
{
  unsigned i;

  if (CF_NONTRIVIAL_P (f))
    {
      for (i = 0; i < f->n; i++)
	affine_fn_free (f->fns[i]);
    }
  free (f);
}

/* Frees memory used by SUBSCRIPTS.  */

static void
free_subscripts (vec<subscript_p> subscripts)
{
  unsigned i;
  subscript_p s;

  FOR_EACH_VEC_ELT (subscripts, i, s)
    {
      free_conflict_function (s->conflicting_iterations_in_a);
      free_conflict_function (s->conflicting_iterations_in_b);
      free (s);
    }
  subscripts.release ();
}

/* Set DDR_ARE_DEPENDENT to CHREC and finalize the subscript overlap
   description.  */

static inline void
finalize_ddr_dependent (struct data_dependence_relation *ddr,
			tree chrec)
{
  DDR_ARE_DEPENDENT (ddr) = chrec;
  free_subscripts (DDR_SUBSCRIPTS (ddr));
  DDR_SUBSCRIPTS (ddr).create (0);
}

/* The dependence relation DDR cannot be represented by a distance
   vector.  */

static inline void
non_affine_dependence_relation (struct data_dependence_relation *ddr)
{
  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "(Dependence relation cannot be represented by distance vector.) \n");

  DDR_AFFINE_P (ddr) = false;
}



/* This section contains the classic Banerjee tests.  */

/* Returns true iff CHREC_A and CHREC_B are not dependent on any index
   variables, i.e., if the ZIV (Zero Index Variable) test is true.  */

static inline bool
ziv_subscript_p (const_tree chrec_a, const_tree chrec_b)
{
  return (evolution_function_is_constant_p (chrec_a)
	  && evolution_function_is_constant_p (chrec_b));
}

/* Returns true iff CHREC_A and CHREC_B are dependent on an index
   variable, i.e., if the SIV (Single Index Variable) test is true.  */

static bool
siv_subscript_p (const_tree chrec_a, const_tree chrec_b)
{
  if ((evolution_function_is_constant_p (chrec_a)
       && evolution_function_is_univariate_p (chrec_b))
      || (evolution_function_is_constant_p (chrec_b)
	  && evolution_function_is_univariate_p (chrec_a)))
    return true;

  if (evolution_function_is_univariate_p (chrec_a)
      && evolution_function_is_univariate_p (chrec_b))
    {
      switch (TREE_CODE (chrec_a))
	{
	case POLYNOMIAL_CHREC:
	  switch (TREE_CODE (chrec_b))
	    {
	    case POLYNOMIAL_CHREC:
	      if (CHREC_VARIABLE (chrec_a) != CHREC_VARIABLE (chrec_b))
		return false;

	    default:
	      return true;
	    }

	default:
	  return true;
	}
    }

  return false;
}

/* Creates a conflict function with N dimensions.  The affine functions
   in each dimension follow.  */

static conflict_function *
conflict_fn (unsigned n, ...)
{
  unsigned i;
  conflict_function *ret = XCNEW (conflict_function);
  va_list ap;

  gcc_assert (0 < n && n <= MAX_DIM);
  va_start (ap, n);

  ret->n = n;
  for (i = 0; i < n; i++)
    ret->fns[i] = va_arg (ap, affine_fn);
  va_end (ap);

  return ret;
}

/* Returns constant affine function with value CST.  */

static affine_fn
affine_fn_cst (tree cst)
{
  affine_fn fn;
  fn.create (1);
  fn.quick_push (cst);
  return fn;
}

/* Returns affine function with single variable, CST + COEF * x_DIM.  */

static affine_fn
affine_fn_univar (tree cst, unsigned dim, tree coef)
{
  affine_fn fn;
  fn.create (dim + 1);
  unsigned i;

  gcc_assert (dim > 0);
  fn.quick_push (cst);
  for (i = 1; i < dim; i++)
    fn.quick_push (integer_zero_node);
  fn.quick_push (coef);
  return fn;
}

/* Analyze a ZIV (Zero Index Variable) subscript.  *OVERLAPS_A and
   *OVERLAPS_B are initialized to the functions that describe the
   relation between the elements accessed twice by CHREC_A and
   CHREC_B.  For k >= 0, the following property is verified:

   CHREC_A (*OVERLAPS_A (k)) = CHREC_B (*OVERLAPS_B (k)).  */

static void
analyze_ziv_subscript (tree chrec_a,
		       tree chrec_b,
		       conflict_function **overlaps_a,
		       conflict_function **overlaps_b,
		       tree *last_conflicts)
{
  tree type, difference;
  dependence_stats.num_ziv++;

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "(analyze_ziv_subscript \n");

  type = signed_type_for_types (TREE_TYPE (chrec_a), TREE_TYPE (chrec_b));
  chrec_a = chrec_convert (type, chrec_a, NULL);
  chrec_b = chrec_convert (type, chrec_b, NULL);
  difference = chrec_fold_minus (type, chrec_a, chrec_b);

  switch (TREE_CODE (difference))
    {
    case INTEGER_CST:
      if (integer_zerop (difference))
	{
	  /* The difference is equal to zero: the accessed index
	     overlaps for each iteration in the loop.  */
	  *overlaps_a = conflict_fn (1, affine_fn_cst (integer_zero_node));
	  *overlaps_b = conflict_fn (1, affine_fn_cst (integer_zero_node));
	  *last_conflicts = chrec_dont_know;
	  dependence_stats.num_ziv_dependent++;
	}
      else
	{
	  /* The accesses do not overlap.  */
	  *overlaps_a = conflict_fn_no_dependence ();
	  *overlaps_b = conflict_fn_no_dependence ();
	  *last_conflicts = integer_zero_node;
	  dependence_stats.num_ziv_independent++;
	}
      break;

    default:
      /* We're not sure whether the indexes overlap.  For the moment,
	 conservatively answer "don't know".  */
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "ziv test failed: difference is non-integer.\n");

      *overlaps_a = conflict_fn_not_known ();
      *overlaps_b = conflict_fn_not_known ();
      *last_conflicts = chrec_dont_know;
      dependence_stats.num_ziv_unimplemented++;
      break;
    }

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, ")\n");
}

/* Similar to max_stmt_executions_int, but returns the bound as a tree,
   and only if it fits to the int type.  If this is not the case, or the
   bound  on the number of iterations of LOOP could not be derived, returns
   chrec_dont_know.  */

static tree
max_stmt_executions_tree (struct loop *loop)
{
  widest_int nit;

  if (!max_stmt_executions (loop, &nit))
    return chrec_dont_know;

  if (!wi::fits_to_tree_p (nit, unsigned_type_node))
    return chrec_dont_know;

  return wide_int_to_tree (unsigned_type_node, nit);
}

/* Determine whether the CHREC is always positive/negative.  If the expression
   cannot be statically analyzed, return false, otherwise set the answer into
   VALUE.  */

static bool
chrec_is_positive (tree chrec, bool *value)
{
  bool value0, value1, value2;
  tree end_value, nb_iter;

  switch (TREE_CODE (chrec))
    {
    case POLYNOMIAL_CHREC:
      if (!chrec_is_positive (CHREC_LEFT (chrec), &value0)
	  || !chrec_is_positive (CHREC_RIGHT (chrec), &value1))
	return false;

      /* FIXME -- overflows.  */
      if (value0 == value1)
	{
	  *value = value0;
	  return true;
	}

      /* Otherwise the chrec is under the form: "{-197, +, 2}_1",
	 and the proof consists in showing that the sign never
	 changes during the execution of the loop, from 0 to
	 loop->nb_iterations.  */
      if (!evolution_function_is_affine_p (chrec))
	return false;

      nb_iter = number_of_latch_executions (get_chrec_loop (chrec));
      if (chrec_contains_undetermined (nb_iter))
	return false;

#if 0
      /* TODO -- If the test is after the exit, we may decrease the number of
	 iterations by one.  */
      if (after_exit)
	nb_iter = chrec_fold_minus (type, nb_iter, build_int_cst (type, 1));
#endif

      end_value = chrec_apply (CHREC_VARIABLE (chrec), chrec, nb_iter);

      if (!chrec_is_positive (end_value, &value2))
	return false;

      *value = value0;
      return value0 == value1;

    case INTEGER_CST:
      switch (tree_int_cst_sgn (chrec))
	{
	case -1:
	  *value = false;
	  break;
	case 1:
	  *value = true;
	  break;
	default:
	  return false;
	}
      return true;

    default:
      return false;
    }
}


/* Analyze a SIV (Single Index Variable) subscript where CHREC_A is a
   constant, and CHREC_B is an affine function.  *OVERLAPS_A and
   *OVERLAPS_B are initialized to the functions that describe the
   relation between the elements accessed twice by CHREC_A and
   CHREC_B.  For k >= 0, the following property is verified:

   CHREC_A (*OVERLAPS_A (k)) = CHREC_B (*OVERLAPS_B (k)).  */

static void
analyze_siv_subscript_cst_affine (tree chrec_a,
				  tree chrec_b,
				  conflict_function **overlaps_a,
				  conflict_function **overlaps_b,
				  tree *last_conflicts)
{
  bool value0, value1, value2;
  tree type, difference, tmp;

  type = signed_type_for_types (TREE_TYPE (chrec_a), TREE_TYPE (chrec_b));
  chrec_a = chrec_convert (type, chrec_a, NULL);
  chrec_b = chrec_convert (type, chrec_b, NULL);
  difference = chrec_fold_minus (type, initial_condition (chrec_b), chrec_a);

  /* Special case overlap in the first iteration.  */
  if (integer_zerop (difference))
    {
      *overlaps_a = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *overlaps_b = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *last_conflicts = integer_one_node;
      return;
    }

  if (!chrec_is_positive (initial_condition (difference), &value0))
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "siv test failed: chrec is not positive.\n");

      dependence_stats.num_siv_unimplemented++;
      *overlaps_a = conflict_fn_not_known ();
      *overlaps_b = conflict_fn_not_known ();
      *last_conflicts = chrec_dont_know;
      return;
    }
  else
    {
      if (value0 == false)
	{
	  if (!chrec_is_positive (CHREC_RIGHT (chrec_b), &value1))
	    {
	      if (dump_file && (dump_flags & TDF_DETAILS))
		fprintf (dump_file, "siv test failed: chrec not positive.\n");

	      *overlaps_a = conflict_fn_not_known ();
	      *overlaps_b = conflict_fn_not_known ();
	      *last_conflicts = chrec_dont_know;
	      dependence_stats.num_siv_unimplemented++;
	      return;
	    }
	  else
	    {
	      if (value1 == true)
		{
		  /* Example:
		     chrec_a = 12
		     chrec_b = {10, +, 1}
		  */

		  if (tree_fold_divides_p (CHREC_RIGHT (chrec_b), difference))
		    {
		      HOST_WIDE_INT numiter;
		      struct loop *loop = get_chrec_loop (chrec_b);

		      *overlaps_a = conflict_fn (1, affine_fn_cst (integer_zero_node));
		      tmp = fold_build2 (EXACT_DIV_EXPR, type,
					 fold_build1 (ABS_EXPR, type, difference),
					 CHREC_RIGHT (chrec_b));
		      *overlaps_b = conflict_fn (1, affine_fn_cst (tmp));
		      *last_conflicts = integer_one_node;


		      /* Perform weak-zero siv test to see if overlap is
			 outside the loop bounds.  */
		      numiter = max_stmt_executions_int (loop);

		      if (numiter >= 0
			  && compare_tree_int (tmp, numiter) > 0)
			{
			  free_conflict_function (*overlaps_a);
			  free_conflict_function (*overlaps_b);
			  *overlaps_a = conflict_fn_no_dependence ();
			  *overlaps_b = conflict_fn_no_dependence ();
			  *last_conflicts = integer_zero_node;
			  dependence_stats.num_siv_independent++;
			  return;
			}
		      dependence_stats.num_siv_dependent++;
		      return;
		    }

		  /* When the step does not divide the difference, there are
		     no overlaps.  */
		  else
		    {
		      *overlaps_a = conflict_fn_no_dependence ();
		      *overlaps_b = conflict_fn_no_dependence ();
		      *last_conflicts = integer_zero_node;
		      dependence_stats.num_siv_independent++;
		      return;
		    }
		}

	      else
		{
		  /* Example:
		     chrec_a = 12
		     chrec_b = {10, +, -1}

		     In this case, chrec_a will not overlap with chrec_b.  */
		  *overlaps_a = conflict_fn_no_dependence ();
		  *overlaps_b = conflict_fn_no_dependence ();
		  *last_conflicts = integer_zero_node;
		  dependence_stats.num_siv_independent++;
		  return;
		}
	    }
	}
      else
	{
	  if (!chrec_is_positive (CHREC_RIGHT (chrec_b), &value2))
	    {
	      if (dump_file && (dump_flags & TDF_DETAILS))
		fprintf (dump_file, "siv test failed: chrec not positive.\n");

	      *overlaps_a = conflict_fn_not_known ();
	      *overlaps_b = conflict_fn_not_known ();
	      *last_conflicts = chrec_dont_know;
	      dependence_stats.num_siv_unimplemented++;
	      return;
	    }
	  else
	    {
	      if (value2 == false)
		{
		  /* Example:
		     chrec_a = 3
		     chrec_b = {10, +, -1}
		  */
		  if (tree_fold_divides_p (CHREC_RIGHT (chrec_b), difference))
		    {
		      HOST_WIDE_INT numiter;
		      struct loop *loop = get_chrec_loop (chrec_b);

		      *overlaps_a = conflict_fn (1, affine_fn_cst (integer_zero_node));
		      tmp = fold_build2 (EXACT_DIV_EXPR, type, difference,
					 CHREC_RIGHT (chrec_b));
		      *overlaps_b = conflict_fn (1, affine_fn_cst (tmp));
		      *last_conflicts = integer_one_node;

		      /* Perform weak-zero siv test to see if overlap is
			 outside the loop bounds.  */
		      numiter = max_stmt_executions_int (loop);

		      if (numiter >= 0
			  && compare_tree_int (tmp, numiter) > 0)
			{
			  free_conflict_function (*overlaps_a);
			  free_conflict_function (*overlaps_b);
			  *overlaps_a = conflict_fn_no_dependence ();
			  *overlaps_b = conflict_fn_no_dependence ();
			  *last_conflicts = integer_zero_node;
			  dependence_stats.num_siv_independent++;
			  return;
			}
		      dependence_stats.num_siv_dependent++;
		      return;
		    }

		  /* When the step does not divide the difference, there
		     are no overlaps.  */
		  else
		    {
		      *overlaps_a = conflict_fn_no_dependence ();
		      *overlaps_b = conflict_fn_no_dependence ();
		      *last_conflicts = integer_zero_node;
		      dependence_stats.num_siv_independent++;
		      return;
		    }
		}
	      else
		{
		  /* Example:
		     chrec_a = 3
		     chrec_b = {4, +, 1}

		     In this case, chrec_a will not overlap with chrec_b.  */
		  *overlaps_a = conflict_fn_no_dependence ();
		  *overlaps_b = conflict_fn_no_dependence ();
		  *last_conflicts = integer_zero_node;
		  dependence_stats.num_siv_independent++;
		  return;
		}
	    }
	}
    }
}

/* Helper recursive function for initializing the matrix A.  Returns
   the initial value of CHREC.  */

static tree
initialize_matrix_A (lambda_matrix A, tree chrec, unsigned index, int mult)
{
  gcc_assert (chrec);

  switch (TREE_CODE (chrec))
    {
    case POLYNOMIAL_CHREC:
      A[index][0] = mult * int_cst_value (CHREC_RIGHT (chrec));
      return initialize_matrix_A (A, CHREC_LEFT (chrec), index + 1, mult);

    case PLUS_EXPR:
    case MULT_EXPR:
    case MINUS_EXPR:
      {
	tree op0 = initialize_matrix_A (A, TREE_OPERAND (chrec, 0), index, mult);
	tree op1 = initialize_matrix_A (A, TREE_OPERAND (chrec, 1), index, mult);

	return chrec_fold_op (TREE_CODE (chrec), chrec_type (chrec), op0, op1);
      }

    CASE_CONVERT:
      {
	tree op = initialize_matrix_A (A, TREE_OPERAND (chrec, 0), index, mult);
	return chrec_convert (chrec_type (chrec), op, NULL);
      }

    case BIT_NOT_EXPR:
      {
	/* Handle ~X as -1 - X.  */
	tree op = initialize_matrix_A (A, TREE_OPERAND (chrec, 0), index, mult);
	return chrec_fold_op (MINUS_EXPR, chrec_type (chrec),
			      build_int_cst (TREE_TYPE (chrec), -1), op);
      }

    case INTEGER_CST:
      return chrec;

    default:
      gcc_unreachable ();
      return NULL_TREE;
    }
}

#define FLOOR_DIV(x,y) ((x) / (y))

/* Solves the special case of the Diophantine equation:
   | {0, +, STEP_A}_x (OVERLAPS_A) = {0, +, STEP_B}_y (OVERLAPS_B)

   Computes the descriptions OVERLAPS_A and OVERLAPS_B.  NITER is the
   number of iterations that loops X and Y run.  The overlaps will be
   constructed as evolutions in dimension DIM.  */

static void
compute_overlap_steps_for_affine_univar (int niter, int step_a, int step_b,
					 affine_fn *overlaps_a,
					 affine_fn *overlaps_b,
					 tree *last_conflicts, int dim)
{
  if (((step_a > 0 && step_b > 0)
       || (step_a < 0 && step_b < 0)))
    {
      int step_overlaps_a, step_overlaps_b;
      int gcd_steps_a_b, last_conflict, tau2;

      gcd_steps_a_b = gcd (step_a, step_b);
      step_overlaps_a = step_b / gcd_steps_a_b;
      step_overlaps_b = step_a / gcd_steps_a_b;

      if (niter > 0)
	{
	  tau2 = FLOOR_DIV (niter, step_overlaps_a);
	  tau2 = MIN (tau2, FLOOR_DIV (niter, step_overlaps_b));
	  last_conflict = tau2;
	  *last_conflicts = build_int_cst (NULL_TREE, last_conflict);
	}
      else
	*last_conflicts = chrec_dont_know;

      *overlaps_a = affine_fn_univar (integer_zero_node, dim,
				      build_int_cst (NULL_TREE,
						     step_overlaps_a));
      *overlaps_b = affine_fn_univar (integer_zero_node, dim,
				      build_int_cst (NULL_TREE,
						     step_overlaps_b));
    }

  else
    {
      *overlaps_a = affine_fn_cst (integer_zero_node);
      *overlaps_b = affine_fn_cst (integer_zero_node);
      *last_conflicts = integer_zero_node;
    }
}

/* Solves the special case of a Diophantine equation where CHREC_A is
   an affine bivariate function, and CHREC_B is an affine univariate
   function.  For example,

   | {{0, +, 1}_x, +, 1335}_y = {0, +, 1336}_z

   has the following overlapping functions:

   | x (t, u, v) = {{0, +, 1336}_t, +, 1}_v
   | y (t, u, v) = {{0, +, 1336}_u, +, 1}_v
   | z (t, u, v) = {{{0, +, 1}_t, +, 1335}_u, +, 1}_v

   FORNOW: This is a specialized implementation for a case occurring in
   a common benchmark.  Implement the general algorithm.  */

static void
compute_overlap_steps_for_affine_1_2 (tree chrec_a, tree chrec_b,
				      conflict_function **overlaps_a,
				      conflict_function **overlaps_b,
				      tree *last_conflicts)
{
  bool xz_p, yz_p, xyz_p;
  int step_x, step_y, step_z;
  HOST_WIDE_INT niter_x, niter_y, niter_z, niter;
  affine_fn overlaps_a_xz, overlaps_b_xz;
  affine_fn overlaps_a_yz, overlaps_b_yz;
  affine_fn overlaps_a_xyz, overlaps_b_xyz;
  affine_fn ova1, ova2, ovb;
  tree last_conflicts_xz, last_conflicts_yz, last_conflicts_xyz;

  step_x = int_cst_value (CHREC_RIGHT (CHREC_LEFT (chrec_a)));
  step_y = int_cst_value (CHREC_RIGHT (chrec_a));
  step_z = int_cst_value (CHREC_RIGHT (chrec_b));

  niter_x = max_stmt_executions_int (get_chrec_loop (CHREC_LEFT (chrec_a)));
  niter_y = max_stmt_executions_int (get_chrec_loop (chrec_a));
  niter_z = max_stmt_executions_int (get_chrec_loop (chrec_b));

  if (niter_x < 0 || niter_y < 0 || niter_z < 0)
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "overlap steps test failed: no iteration counts.\n");

      *overlaps_a = conflict_fn_not_known ();
      *overlaps_b = conflict_fn_not_known ();
      *last_conflicts = chrec_dont_know;
      return;
    }

  niter = MIN (niter_x, niter_z);
  compute_overlap_steps_for_affine_univar (niter, step_x, step_z,
					   &overlaps_a_xz,
					   &overlaps_b_xz,
					   &last_conflicts_xz, 1);
  niter = MIN (niter_y, niter_z);
  compute_overlap_steps_for_affine_univar (niter, step_y, step_z,
					   &overlaps_a_yz,
					   &overlaps_b_yz,
					   &last_conflicts_yz, 2);
  niter = MIN (niter_x, niter_z);
  niter = MIN (niter_y, niter);
  compute_overlap_steps_for_affine_univar (niter, step_x + step_y, step_z,
					   &overlaps_a_xyz,
					   &overlaps_b_xyz,
					   &last_conflicts_xyz, 3);

  xz_p = !integer_zerop (last_conflicts_xz);
  yz_p = !integer_zerop (last_conflicts_yz);
  xyz_p = !integer_zerop (last_conflicts_xyz);

  if (xz_p || yz_p || xyz_p)
    {
      ova1 = affine_fn_cst (integer_zero_node);
      ova2 = affine_fn_cst (integer_zero_node);
      ovb = affine_fn_cst (integer_zero_node);
      if (xz_p)
	{
	  affine_fn t0 = ova1;
	  affine_fn t2 = ovb;

	  ova1 = affine_fn_plus (ova1, overlaps_a_xz);
	  ovb = affine_fn_plus (ovb, overlaps_b_xz);
	  affine_fn_free (t0);
	  affine_fn_free (t2);
	  *last_conflicts = last_conflicts_xz;
	}
      if (yz_p)
	{
	  affine_fn t0 = ova2;
	  affine_fn t2 = ovb;

	  ova2 = affine_fn_plus (ova2, overlaps_a_yz);
	  ovb = affine_fn_plus (ovb, overlaps_b_yz);
	  affine_fn_free (t0);
	  affine_fn_free (t2);
	  *last_conflicts = last_conflicts_yz;
	}
      if (xyz_p)
	{
	  affine_fn t0 = ova1;
	  affine_fn t2 = ova2;
	  affine_fn t4 = ovb;

	  ova1 = affine_fn_plus (ova1, overlaps_a_xyz);
	  ova2 = affine_fn_plus (ova2, overlaps_a_xyz);
	  ovb = affine_fn_plus (ovb, overlaps_b_xyz);
	  affine_fn_free (t0);
	  affine_fn_free (t2);
	  affine_fn_free (t4);
	  *last_conflicts = last_conflicts_xyz;
	}
      *overlaps_a = conflict_fn (2, ova1, ova2);
      *overlaps_b = conflict_fn (1, ovb);
    }
  else
    {
      *overlaps_a = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *overlaps_b = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *last_conflicts = integer_zero_node;
    }

  affine_fn_free (overlaps_a_xz);
  affine_fn_free (overlaps_b_xz);
  affine_fn_free (overlaps_a_yz);
  affine_fn_free (overlaps_b_yz);
  affine_fn_free (overlaps_a_xyz);
  affine_fn_free (overlaps_b_xyz);
}

/* Copy the elements of vector VEC1 with length SIZE to VEC2.  */

static void
lambda_vector_copy (lambda_vector vec1, lambda_vector vec2,
		    int size)
{
  memcpy (vec2, vec1, size * sizeof (*vec1));
}

/* Copy the elements of M x N matrix MAT1 to MAT2.  */

static void
lambda_matrix_copy (lambda_matrix mat1, lambda_matrix mat2,
		    int m, int n)
{
  int i;

  for (i = 0; i < m; i++)
    lambda_vector_copy (mat1[i], mat2[i], n);
}

/* Store the N x N identity matrix in MAT.  */

static void
lambda_matrix_id (lambda_matrix mat, int size)
{
  int i, j;

  for (i = 0; i < size; i++)
    for (j = 0; j < size; j++)
      mat[i][j] = (i == j) ? 1 : 0;
}

/* Return the first nonzero element of vector VEC1 between START and N.
   We must have START <= N.   Returns N if VEC1 is the zero vector.  */

static int
lambda_vector_first_nz (lambda_vector vec1, int n, int start)
{
  int j = start;
  while (j < n && vec1[j] == 0)
    j++;
  return j;
}

/* Add a multiple of row R1 of matrix MAT with N columns to row R2:
   R2 = R2 + CONST1 * R1.  */

static void
lambda_matrix_row_add (lambda_matrix mat, int n, int r1, int r2, int const1)
{
  int i;

  if (const1 == 0)
    return;

  for (i = 0; i < n; i++)
    mat[r2][i] += const1 * mat[r1][i];
}

/* Multiply vector VEC1 of length SIZE by a constant CONST1,
   and store the result in VEC2.  */

static void
lambda_vector_mult_const (lambda_vector vec1, lambda_vector vec2,
			  int size, int const1)
{
  int i;

  if (const1 == 0)
    lambda_vector_clear (vec2, size);
  else
    for (i = 0; i < size; i++)
      vec2[i] = const1 * vec1[i];
}

/* Negate vector VEC1 with length SIZE and store it in VEC2.  */

static void
lambda_vector_negate (lambda_vector vec1, lambda_vector vec2,
		      int size)
{
  lambda_vector_mult_const (vec1, vec2, size, -1);
}

/* Negate row R1 of matrix MAT which has N columns.  */

static void
lambda_matrix_row_negate (lambda_matrix mat, int n, int r1)
{
  lambda_vector_negate (mat[r1], mat[r1], n);
}

/* Return true if two vectors are equal.  */

static bool
lambda_vector_equal (lambda_vector vec1, lambda_vector vec2, int size)
{
  int i;
  for (i = 0; i < size; i++)
    if (vec1[i] != vec2[i])
      return false;
  return true;
}

/* Given an M x N integer matrix A, this function determines an M x
   M unimodular matrix U, and an M x N echelon matrix S such that
   "U.A = S".  This decomposition is also known as "right Hermite".

   Ref: Algorithm 2.1 page 33 in "Loop Transformations for
   Restructuring Compilers" Utpal Banerjee.  */

static void
lambda_matrix_right_hermite (lambda_matrix A, int m, int n,
			     lambda_matrix S, lambda_matrix U)
{
  int i, j, i0 = 0;

  lambda_matrix_copy (A, S, m, n);
  lambda_matrix_id (U, m);

  for (j = 0; j < n; j++)
    {
      if (lambda_vector_first_nz (S[j], m, i0) < m)
	{
	  ++i0;
	  for (i = m - 1; i >= i0; i--)
	    {
	      while (S[i][j] != 0)
		{
		  int sigma, factor, a, b;

		  a = S[i-1][j];
		  b = S[i][j];
		  sigma = (a * b < 0) ? -1: 1;
		  a = abs (a);
		  b = abs (b);
		  factor = sigma * (a / b);

		  lambda_matrix_row_add (S, n, i, i-1, -factor);
		  std::swap (S[i], S[i-1]);

		  lambda_matrix_row_add (U, m, i, i-1, -factor);
		  std::swap (U[i], U[i-1]);
		}
	    }
	}
    }
}

/* Determines the overlapping elements due to accesses CHREC_A and
   CHREC_B, that are affine functions.  This function cannot handle
   symbolic evolution functions, ie. when initial conditions are
   parameters, because it uses lambda matrices of integers.  */

static void
analyze_subscript_affine_affine (tree chrec_a,
				 tree chrec_b,
				 conflict_function **overlaps_a,
				 conflict_function **overlaps_b,
				 tree *last_conflicts)
{
  unsigned nb_vars_a, nb_vars_b, dim;
  HOST_WIDE_INT init_a, init_b, gamma, gcd_alpha_beta;
  lambda_matrix A, U, S;
  struct obstack scratch_obstack;

  if (eq_evolutions_p (chrec_a, chrec_b))
    {
      /* The accessed index overlaps for each iteration in the
	 loop.  */
      *overlaps_a = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *overlaps_b = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *last_conflicts = chrec_dont_know;
      return;
    }
  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "(analyze_subscript_affine_affine \n");

  /* For determining the initial intersection, we have to solve a
     Diophantine equation.  This is the most time consuming part.

     For answering to the question: "Is there a dependence?" we have
     to prove that there exists a solution to the Diophantine
     equation, and that the solution is in the iteration domain,
     i.e. the solution is positive or zero, and that the solution
     happens before the upper bound loop.nb_iterations.  Otherwise
     there is no dependence.  This function outputs a description of
     the iterations that hold the intersections.  */

  nb_vars_a = nb_vars_in_chrec (chrec_a);
  nb_vars_b = nb_vars_in_chrec (chrec_b);

  gcc_obstack_init (&scratch_obstack);

  dim = nb_vars_a + nb_vars_b;
  U = lambda_matrix_new (dim, dim, &scratch_obstack);
  A = lambda_matrix_new (dim, 1, &scratch_obstack);
  S = lambda_matrix_new (dim, 1, &scratch_obstack);

  init_a = int_cst_value (initialize_matrix_A (A, chrec_a, 0, 1));
  init_b = int_cst_value (initialize_matrix_A (A, chrec_b, nb_vars_a, -1));
  gamma = init_b - init_a;

  /* Don't do all the hard work of solving the Diophantine equation
     when we already know the solution: for example,
     | {3, +, 1}_1
     | {3, +, 4}_2
     | gamma = 3 - 3 = 0.
     Then the first overlap occurs during the first iterations:
     | {3, +, 1}_1 ({0, +, 4}_x) = {3, +, 4}_2 ({0, +, 1}_x)
  */
  if (gamma == 0)
    {
      if (nb_vars_a == 1 && nb_vars_b == 1)
	{
	  HOST_WIDE_INT step_a, step_b;
	  HOST_WIDE_INT niter, niter_a, niter_b;
	  affine_fn ova, ovb;

	  niter_a = max_stmt_executions_int (get_chrec_loop (chrec_a));
	  niter_b = max_stmt_executions_int (get_chrec_loop (chrec_b));
	  niter = MIN (niter_a, niter_b);
	  step_a = int_cst_value (CHREC_RIGHT (chrec_a));
	  step_b = int_cst_value (CHREC_RIGHT (chrec_b));

	  compute_overlap_steps_for_affine_univar (niter, step_a, step_b,
						   &ova, &ovb,
						   last_conflicts, 1);
	  *overlaps_a = conflict_fn (1, ova);
	  *overlaps_b = conflict_fn (1, ovb);
	}

      else if (nb_vars_a == 2 && nb_vars_b == 1)
	compute_overlap_steps_for_affine_1_2
	  (chrec_a, chrec_b, overlaps_a, overlaps_b, last_conflicts);

      else if (nb_vars_a == 1 && nb_vars_b == 2)
	compute_overlap_steps_for_affine_1_2
	  (chrec_b, chrec_a, overlaps_b, overlaps_a, last_conflicts);

      else
	{
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file, "affine-affine test failed: too many variables.\n");
	  *overlaps_a = conflict_fn_not_known ();
	  *overlaps_b = conflict_fn_not_known ();
	  *last_conflicts = chrec_dont_know;
	}
      goto end_analyze_subs_aa;
    }

  /* U.A = S */
  lambda_matrix_right_hermite (A, dim, 1, S, U);

  if (S[0][0] < 0)
    {
      S[0][0] *= -1;
      lambda_matrix_row_negate (U, dim, 0);
    }
  gcd_alpha_beta = S[0][0];

  /* Something went wrong: for example in {1, +, 0}_5 vs. {0, +, 0}_5,
     but that is a quite strange case.  Instead of ICEing, answer
     don't know.  */
  if (gcd_alpha_beta == 0)
    {
      *overlaps_a = conflict_fn_not_known ();
      *overlaps_b = conflict_fn_not_known ();
      *last_conflicts = chrec_dont_know;
      goto end_analyze_subs_aa;
    }

  /* The classic "gcd-test".  */
  if (!int_divides_p (gcd_alpha_beta, gamma))
    {
      /* The "gcd-test" has determined that there is no integer
	 solution, i.e. there is no dependence.  */
      *overlaps_a = conflict_fn_no_dependence ();
      *overlaps_b = conflict_fn_no_dependence ();
      *last_conflicts = integer_zero_node;
    }

  /* Both access functions are univariate.  This includes SIV and MIV cases.  */
  else if (nb_vars_a == 1 && nb_vars_b == 1)
    {
      /* Both functions should have the same evolution sign.  */
      if (((A[0][0] > 0 && -A[1][0] > 0)
	   || (A[0][0] < 0 && -A[1][0] < 0)))
	{
	  /* The solutions are given by:
	     |
	     | [GAMMA/GCD_ALPHA_BETA  t].[u11 u12]  = [x0]
	     |                           [u21 u22]    [y0]

	     For a given integer t.  Using the following variables,

	     | i0 = u11 * gamma / gcd_alpha_beta
	     | j0 = u12 * gamma / gcd_alpha_beta
	     | i1 = u21
	     | j1 = u22

	     the solutions are:

	     | x0 = i0 + i1 * t,
	     | y0 = j0 + j1 * t.  */
      	  HOST_WIDE_INT i0, j0, i1, j1;

	  i0 = U[0][0] * gamma / gcd_alpha_beta;
	  j0 = U[0][1] * gamma / gcd_alpha_beta;
	  i1 = U[1][0];
	  j1 = U[1][1];

	  if ((i1 == 0 && i0 < 0)
	      || (j1 == 0 && j0 < 0))
	    {
	      /* There is no solution.
		 FIXME: The case "i0 > nb_iterations, j0 > nb_iterations"
		 falls in here, but for the moment we don't look at the
		 upper bound of the iteration domain.  */
	      *overlaps_a = conflict_fn_no_dependence ();
	      *overlaps_b = conflict_fn_no_dependence ();
	      *last_conflicts = integer_zero_node;
	      goto end_analyze_subs_aa;
	    }

	  if (i1 > 0 && j1 > 0)
	    {
	      HOST_WIDE_INT niter_a
		= max_stmt_executions_int (get_chrec_loop (chrec_a));
	      HOST_WIDE_INT niter_b
		= max_stmt_executions_int (get_chrec_loop (chrec_b));
	      HOST_WIDE_INT niter = MIN (niter_a, niter_b);

	      /* (X0, Y0) is a solution of the Diophantine equation:
		 "chrec_a (X0) = chrec_b (Y0)".  */
	      HOST_WIDE_INT tau1 = MAX (CEIL (-i0, i1),
					CEIL (-j0, j1));
	      HOST_WIDE_INT x0 = i1 * tau1 + i0;
	      HOST_WIDE_INT y0 = j1 * tau1 + j0;

	      /* (X1, Y1) is the smallest positive solution of the eq
		 "chrec_a (X1) = chrec_b (Y1)", i.e. this is where the
		 first conflict occurs.  */
	      HOST_WIDE_INT min_multiple = MIN (x0 / i1, y0 / j1);
	      HOST_WIDE_INT x1 = x0 - i1 * min_multiple;
	      HOST_WIDE_INT y1 = y0 - j1 * min_multiple;

	      if (niter > 0)
		{
		  HOST_WIDE_INT tau2 = MIN (FLOOR_DIV (niter_a - i0, i1),
					    FLOOR_DIV (niter_b - j0, j1));
		  HOST_WIDE_INT last_conflict = tau2 - (x1 - i0)/i1;

		  /* If the overlap occurs outside of the bounds of the
		     loop, there is no dependence.  */
		  if (x1 >= niter_a || y1 >= niter_b)
		    {
		      *overlaps_a = conflict_fn_no_dependence ();
		      *overlaps_b = conflict_fn_no_dependence ();
		      *last_conflicts = integer_zero_node;
		      goto end_analyze_subs_aa;
		    }
		  else
		    *last_conflicts = build_int_cst (NULL_TREE, last_conflict);
		}
	      else
		*last_conflicts = chrec_dont_know;

	      *overlaps_a
		= conflict_fn (1,
			       affine_fn_univar (build_int_cst (NULL_TREE, x1),
						 1,
						 build_int_cst (NULL_TREE, i1)));
	      *overlaps_b
		= conflict_fn (1,
			       affine_fn_univar (build_int_cst (NULL_TREE, y1),
						 1,
						 build_int_cst (NULL_TREE, j1)));
	    }
	  else
	    {
	      /* FIXME: For the moment, the upper bound of the
		 iteration domain for i and j is not checked.  */
	      if (dump_file && (dump_flags & TDF_DETAILS))
		fprintf (dump_file, "affine-affine test failed: unimplemented.\n");
	      *overlaps_a = conflict_fn_not_known ();
	      *overlaps_b = conflict_fn_not_known ();
	      *last_conflicts = chrec_dont_know;
	    }
	}
      else
	{
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file, "affine-affine test failed: unimplemented.\n");
	  *overlaps_a = conflict_fn_not_known ();
	  *overlaps_b = conflict_fn_not_known ();
	  *last_conflicts = chrec_dont_know;
	}
    }
  else
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "affine-affine test failed: unimplemented.\n");
      *overlaps_a = conflict_fn_not_known ();
      *overlaps_b = conflict_fn_not_known ();
      *last_conflicts = chrec_dont_know;
    }

end_analyze_subs_aa:
  obstack_free (&scratch_obstack, NULL);
  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "  (overlaps_a = ");
      dump_conflict_function (dump_file, *overlaps_a);
      fprintf (dump_file, ")\n  (overlaps_b = ");
      dump_conflict_function (dump_file, *overlaps_b);
      fprintf (dump_file, "))\n");
    }
}

/* Returns true when analyze_subscript_affine_affine can be used for
   determining the dependence relation between chrec_a and chrec_b,
   that contain symbols.  This function modifies chrec_a and chrec_b
   such that the analysis result is the same, and such that they don't
   contain symbols, and then can safely be passed to the analyzer.

   Example: The analysis of the following tuples of evolutions produce
   the same results: {x+1, +, 1}_1 vs. {x+3, +, 1}_1, and {-2, +, 1}_1
   vs. {0, +, 1}_1

   {x+1, +, 1}_1 ({2, +, 1}_1) = {x+3, +, 1}_1 ({0, +, 1}_1)
   {-2, +, 1}_1 ({2, +, 1}_1) = {0, +, 1}_1 ({0, +, 1}_1)
*/

static bool
can_use_analyze_subscript_affine_affine (tree *chrec_a, tree *chrec_b)
{
  tree diff, type, left_a, left_b, right_b;

  if (chrec_contains_symbols (CHREC_RIGHT (*chrec_a))
      || chrec_contains_symbols (CHREC_RIGHT (*chrec_b)))
    /* FIXME: For the moment not handled.  Might be refined later.  */
    return false;

  type = chrec_type (*chrec_a);
  left_a = CHREC_LEFT (*chrec_a);
  left_b = chrec_convert (type, CHREC_LEFT (*chrec_b), NULL);
  diff = chrec_fold_minus (type, left_a, left_b);

  if (!evolution_function_is_constant_p (diff))
    return false;

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "can_use_subscript_aff_aff_for_symbolic \n");

  *chrec_a = build_polynomial_chrec (CHREC_VARIABLE (*chrec_a),
				     diff, CHREC_RIGHT (*chrec_a));
  right_b = chrec_convert (type, CHREC_RIGHT (*chrec_b), NULL);
  *chrec_b = build_polynomial_chrec (CHREC_VARIABLE (*chrec_b),
				     build_int_cst (type, 0),
				     right_b);
  return true;
}

/* Analyze a SIV (Single Index Variable) subscript.  *OVERLAPS_A and
   *OVERLAPS_B are initialized to the functions that describe the
   relation between the elements accessed twice by CHREC_A and
   CHREC_B.  For k >= 0, the following property is verified:

   CHREC_A (*OVERLAPS_A (k)) = CHREC_B (*OVERLAPS_B (k)).  */

static void
analyze_siv_subscript (tree chrec_a,
		       tree chrec_b,
		       conflict_function **overlaps_a,
		       conflict_function **overlaps_b,
		       tree *last_conflicts,
		       int loop_nest_num)
{
  dependence_stats.num_siv++;

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "(analyze_siv_subscript \n");

  if (evolution_function_is_constant_p (chrec_a)
      && evolution_function_is_affine_in_loop (chrec_b, loop_nest_num))
    analyze_siv_subscript_cst_affine (chrec_a, chrec_b,
				      overlaps_a, overlaps_b, last_conflicts);

  else if (evolution_function_is_affine_in_loop (chrec_a, loop_nest_num)
	   && evolution_function_is_constant_p (chrec_b))
    analyze_siv_subscript_cst_affine (chrec_b, chrec_a,
				      overlaps_b, overlaps_a, last_conflicts);

  else if (evolution_function_is_affine_in_loop (chrec_a, loop_nest_num)
	   && evolution_function_is_affine_in_loop (chrec_b, loop_nest_num))
    {
      if (!chrec_contains_symbols (chrec_a)
	  && !chrec_contains_symbols (chrec_b))
	{
	  analyze_subscript_affine_affine (chrec_a, chrec_b,
					   overlaps_a, overlaps_b,
					   last_conflicts);

	  if (CF_NOT_KNOWN_P (*overlaps_a)
	      || CF_NOT_KNOWN_P (*overlaps_b))
	    dependence_stats.num_siv_unimplemented++;
	  else if (CF_NO_DEPENDENCE_P (*overlaps_a)
		   || CF_NO_DEPENDENCE_P (*overlaps_b))
	    dependence_stats.num_siv_independent++;
	  else
	    dependence_stats.num_siv_dependent++;
	}
      else if (can_use_analyze_subscript_affine_affine (&chrec_a,
							&chrec_b))
	{
	  analyze_subscript_affine_affine (chrec_a, chrec_b,
					   overlaps_a, overlaps_b,
					   last_conflicts);

	  if (CF_NOT_KNOWN_P (*overlaps_a)
	      || CF_NOT_KNOWN_P (*overlaps_b))
	    dependence_stats.num_siv_unimplemented++;
	  else if (CF_NO_DEPENDENCE_P (*overlaps_a)
		   || CF_NO_DEPENDENCE_P (*overlaps_b))
	    dependence_stats.num_siv_independent++;
	  else
	    dependence_stats.num_siv_dependent++;
	}
      else
	goto siv_subscript_dontknow;
    }

  else
    {
    siv_subscript_dontknow:;
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "  siv test failed: unimplemented");
      *overlaps_a = conflict_fn_not_known ();
      *overlaps_b = conflict_fn_not_known ();
      *last_conflicts = chrec_dont_know;
      dependence_stats.num_siv_unimplemented++;
    }

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, ")\n");
}

/* Returns false if we can prove that the greatest common divisor of the steps
   of CHREC does not divide CST, false otherwise.  */

static bool
gcd_of_steps_may_divide_p (const_tree chrec, const_tree cst)
{
  HOST_WIDE_INT cd = 0, val;
  tree step;

  if (!tree_fits_shwi_p (cst))
    return true;
  val = tree_to_shwi (cst);

  while (TREE_CODE (chrec) == POLYNOMIAL_CHREC)
    {
      step = CHREC_RIGHT (chrec);
      if (!tree_fits_shwi_p (step))
	return true;
      cd = gcd (cd, tree_to_shwi (step));
      chrec = CHREC_LEFT (chrec);
    }

  return val % cd == 0;
}

/* Analyze a MIV (Multiple Index Variable) subscript with respect to
   LOOP_NEST.  *OVERLAPS_A and *OVERLAPS_B are initialized to the
   functions that describe the relation between the elements accessed
   twice by CHREC_A and CHREC_B.  For k >= 0, the following property
   is verified:

   CHREC_A (*OVERLAPS_A (k)) = CHREC_B (*OVERLAPS_B (k)).  */

static void
analyze_miv_subscript (tree chrec_a,
		       tree chrec_b,
		       conflict_function **overlaps_a,
		       conflict_function **overlaps_b,
		       tree *last_conflicts,
		       struct loop *loop_nest)
{
  tree type, difference;

  dependence_stats.num_miv++;
  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "(analyze_miv_subscript \n");

  type = signed_type_for_types (TREE_TYPE (chrec_a), TREE_TYPE (chrec_b));
  chrec_a = chrec_convert (type, chrec_a, NULL);
  chrec_b = chrec_convert (type, chrec_b, NULL);
  difference = chrec_fold_minus (type, chrec_a, chrec_b);

  if (eq_evolutions_p (chrec_a, chrec_b))
    {
      /* Access functions are the same: all the elements are accessed
	 in the same order.  */
      *overlaps_a = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *overlaps_b = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *last_conflicts = max_stmt_executions_tree (get_chrec_loop (chrec_a));
      dependence_stats.num_miv_dependent++;
    }

  else if (evolution_function_is_constant_p (difference)
	   /* For the moment, the following is verified:
	      evolution_function_is_affine_multivariate_p (chrec_a,
	      loop_nest->num) */
	   && !gcd_of_steps_may_divide_p (chrec_a, difference))
    {
      /* testsuite/.../ssa-chrec-33.c
	 {{21, +, 2}_1, +, -2}_2  vs.  {{20, +, 2}_1, +, -2}_2

	 The difference is 1, and all the evolution steps are multiples
	 of 2, consequently there are no overlapping elements.  */
      *overlaps_a = conflict_fn_no_dependence ();
      *overlaps_b = conflict_fn_no_dependence ();
      *last_conflicts = integer_zero_node;
      dependence_stats.num_miv_independent++;
    }

  else if (evolution_function_is_affine_multivariate_p (chrec_a, loop_nest->num)
	   && !chrec_contains_symbols (chrec_a)
	   && evolution_function_is_affine_multivariate_p (chrec_b, loop_nest->num)
	   && !chrec_contains_symbols (chrec_b))
    {
      /* testsuite/.../ssa-chrec-35.c
	 {0, +, 1}_2  vs.  {0, +, 1}_3
	 the overlapping elements are respectively located at iterations:
	 {0, +, 1}_x and {0, +, 1}_x,
	 in other words, we have the equality:
	 {0, +, 1}_2 ({0, +, 1}_x) = {0, +, 1}_3 ({0, +, 1}_x)

	 Other examples:
	 {{0, +, 1}_1, +, 2}_2 ({0, +, 1}_x, {0, +, 1}_y) =
	 {0, +, 1}_1 ({{0, +, 1}_x, +, 2}_y)

	 {{0, +, 2}_1, +, 3}_2 ({0, +, 1}_y, {0, +, 1}_x) =
	 {{0, +, 3}_1, +, 2}_2 ({0, +, 1}_x, {0, +, 1}_y)
      */
      analyze_subscript_affine_affine (chrec_a, chrec_b,
				       overlaps_a, overlaps_b, last_conflicts);

      if (CF_NOT_KNOWN_P (*overlaps_a)
 	  || CF_NOT_KNOWN_P (*overlaps_b))
	dependence_stats.num_miv_unimplemented++;
      else if (CF_NO_DEPENDENCE_P (*overlaps_a)
	       || CF_NO_DEPENDENCE_P (*overlaps_b))
	dependence_stats.num_miv_independent++;
      else
	dependence_stats.num_miv_dependent++;
    }

  else
    {
      /* When the analysis is too difficult, answer "don't know".  */
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "analyze_miv_subscript test failed: unimplemented.\n");

      *overlaps_a = conflict_fn_not_known ();
      *overlaps_b = conflict_fn_not_known ();
      *last_conflicts = chrec_dont_know;
      dependence_stats.num_miv_unimplemented++;
    }

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, ")\n");
}

/* Determines the iterations for which CHREC_A is equal to CHREC_B in
   with respect to LOOP_NEST.  OVERLAP_ITERATIONS_A and
   OVERLAP_ITERATIONS_B are initialized with two functions that
   describe the iterations that contain conflicting elements.

   Remark: For an integer k >= 0, the following equality is true:

   CHREC_A (OVERLAP_ITERATIONS_A (k)) == CHREC_B (OVERLAP_ITERATIONS_B (k)).
*/

static void
analyze_overlapping_iterations (tree chrec_a,
				tree chrec_b,
				conflict_function **overlap_iterations_a,
				conflict_function **overlap_iterations_b,
				tree *last_conflicts, struct loop *loop_nest)
{
  unsigned int lnn = loop_nest->num;

  dependence_stats.num_subscript_tests++;

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "(analyze_overlapping_iterations \n");
      fprintf (dump_file, "  (chrec_a = ");
      print_generic_expr (dump_file, chrec_a, 0);
      fprintf (dump_file, ")\n  (chrec_b = ");
      print_generic_expr (dump_file, chrec_b, 0);
      fprintf (dump_file, ")\n");
    }

  if (chrec_a == NULL_TREE
      || chrec_b == NULL_TREE
      || chrec_contains_undetermined (chrec_a)
      || chrec_contains_undetermined (chrec_b))
    {
      dependence_stats.num_subscript_undetermined++;

      *overlap_iterations_a = conflict_fn_not_known ();
      *overlap_iterations_b = conflict_fn_not_known ();
    }

  /* If they are the same chrec, and are affine, they overlap
     on every iteration.  */
  else if (eq_evolutions_p (chrec_a, chrec_b)
	   && (evolution_function_is_affine_multivariate_p (chrec_a, lnn)
	       || operand_equal_p (chrec_a, chrec_b, 0)))
    {
      dependence_stats.num_same_subscript_function++;
      *overlap_iterations_a = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *overlap_iterations_b = conflict_fn (1, affine_fn_cst (integer_zero_node));
      *last_conflicts = chrec_dont_know;
    }

  /* If they aren't the same, and aren't affine, we can't do anything
     yet.  */
  else if ((chrec_contains_symbols (chrec_a)
	    || chrec_contains_symbols (chrec_b))
	   && (!evolution_function_is_affine_multivariate_p (chrec_a, lnn)
	       || !evolution_function_is_affine_multivariate_p (chrec_b, lnn)))
    {
      dependence_stats.num_subscript_undetermined++;
      *overlap_iterations_a = conflict_fn_not_known ();
      *overlap_iterations_b = conflict_fn_not_known ();
    }

  else if (ziv_subscript_p (chrec_a, chrec_b))
    analyze_ziv_subscript (chrec_a, chrec_b,
			   overlap_iterations_a, overlap_iterations_b,
			   last_conflicts);

  else if (siv_subscript_p (chrec_a, chrec_b))
    analyze_siv_subscript (chrec_a, chrec_b,
			   overlap_iterations_a, overlap_iterations_b,
			   last_conflicts, lnn);

  else
    analyze_miv_subscript (chrec_a, chrec_b,
			   overlap_iterations_a, overlap_iterations_b,
			   last_conflicts, loop_nest);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "  (overlap_iterations_a = ");
      dump_conflict_function (dump_file, *overlap_iterations_a);
      fprintf (dump_file, ")\n  (overlap_iterations_b = ");
      dump_conflict_function (dump_file, *overlap_iterations_b);
      fprintf (dump_file, "))\n");
    }
}

/* Helper function for uniquely inserting distance vectors.  */

static void
save_dist_v (struct data_dependence_relation *ddr, lambda_vector dist_v)
{
  unsigned i;
  lambda_vector v;

  FOR_EACH_VEC_ELT (DDR_DIST_VECTS (ddr), i, v)
    if (lambda_vector_equal (v, dist_v, DDR_NB_LOOPS (ddr)))
      return;

  DDR_DIST_VECTS (ddr).safe_push (dist_v);
}

/* Helper function for uniquely inserting direction vectors.  */

static void
save_dir_v (struct data_dependence_relation *ddr, lambda_vector dir_v)
{
  unsigned i;
  lambda_vector v;

  FOR_EACH_VEC_ELT (DDR_DIR_VECTS (ddr), i, v)
    if (lambda_vector_equal (v, dir_v, DDR_NB_LOOPS (ddr)))
      return;

  DDR_DIR_VECTS (ddr).safe_push (dir_v);
}

/* Add a distance of 1 on all the loops outer than INDEX.  If we
   haven't yet determined a distance for this outer loop, push a new
   distance vector composed of the previous distance, and a distance
   of 1 for this outer loop.  Example:

   | loop_1
   |   loop_2
   |     A[10]
   |   endloop_2
   | endloop_1

   Saved vectors are of the form (dist_in_1, dist_in_2).  First, we
   save (0, 1), then we have to save (1, 0).  */

static void
add_outer_distances (struct data_dependence_relation *ddr,
		     lambda_vector dist_v, int index)
{
  /* For each outer loop where init_v is not set, the accesses are
     in dependence of distance 1 in the loop.  */
  while (--index >= 0)
    {
      lambda_vector save_v = lambda_vector_new (DDR_NB_LOOPS (ddr));
      lambda_vector_copy (dist_v, save_v, DDR_NB_LOOPS (ddr));
      save_v[index] = 1;
      save_dist_v (ddr, save_v);
    }
}

/* Return false when fail to represent the data dependence as a
   distance vector.  INIT_B is set to true when a component has been
   added to the distance vector DIST_V.  INDEX_CARRY is then set to
   the index in DIST_V that carries the dependence.  */

static bool
build_classic_dist_vector_1 (struct data_dependence_relation *ddr,
			     struct data_reference *ddr_a,
			     struct data_reference *ddr_b,
			     lambda_vector dist_v, bool *init_b,
			     int *index_carry)
{
  unsigned i;
  lambda_vector init_v = lambda_vector_new (DDR_NB_LOOPS (ddr));

  for (i = 0; i < DDR_NUM_SUBSCRIPTS (ddr); i++)
    {
      tree access_fn_a, access_fn_b;
      struct subscript *subscript = DDR_SUBSCRIPT (ddr, i);

      if (chrec_contains_undetermined (SUB_DISTANCE (subscript)))
	{
	  non_affine_dependence_relation (ddr);
	  return false;
	}

      access_fn_a = DR_ACCESS_FN (ddr_a, i);
      access_fn_b = DR_ACCESS_FN (ddr_b, i);

      if (TREE_CODE (access_fn_a) == POLYNOMIAL_CHREC
	  && TREE_CODE (access_fn_b) == POLYNOMIAL_CHREC)
	{
	  int dist, index;
	  int var_a = CHREC_VARIABLE (access_fn_a);
	  int var_b = CHREC_VARIABLE (access_fn_b);

	  if (var_a != var_b
	      || chrec_contains_undetermined (SUB_DISTANCE (subscript)))
	    {
	      non_affine_dependence_relation (ddr);
	      return false;
	    }

	  dist = int_cst_value (SUB_DISTANCE (subscript));
	  index = index_in_loop_nest (var_a, DDR_LOOP_NEST (ddr));
	  *index_carry = MIN (index, *index_carry);

	  /* This is the subscript coupling test.  If we have already
	     recorded a distance for this loop (a distance coming from
	     another subscript), it should be the same.  For example,
	     in the following code, there is no dependence:

	     | loop i = 0, N, 1
	     |   T[i+1][i] = ...
	     |   ... = T[i][i]
	     | endloop
	  */
	  if (init_v[index] != 0 && dist_v[index] != dist)
	    {
	      finalize_ddr_dependent (ddr, chrec_known);
	      return false;
	    }

	  dist_v[index] = dist;
	  init_v[index] = 1;
	  *init_b = true;
	}
      else if (!operand_equal_p (access_fn_a, access_fn_b, 0))
	{
	  /* This can be for example an affine vs. constant dependence
	     (T[i] vs. T[3]) that is not an affine dependence and is
	     not representable as a distance vector.  */
	  non_affine_dependence_relation (ddr);
	  return false;
	}
    }

  return true;
}

/* Return true when the DDR contains only constant access functions.  */

static bool
constant_access_functions (const struct data_dependence_relation *ddr)
{
  unsigned i;

  for (i = 0; i < DDR_NUM_SUBSCRIPTS (ddr); i++)
    if (!evolution_function_is_constant_p (DR_ACCESS_FN (DDR_A (ddr), i))
	|| !evolution_function_is_constant_p (DR_ACCESS_FN (DDR_B (ddr), i)))
      return false;

  return true;
}

/* Helper function for the case where DDR_A and DDR_B are the same
   multivariate access function with a constant step.  For an example
   see pr34635-1.c.  */

static void
add_multivariate_self_dist (struct data_dependence_relation *ddr, tree c_2)
{
  int x_1, x_2;
  tree c_1 = CHREC_LEFT (c_2);
  tree c_0 = CHREC_LEFT (c_1);
  lambda_vector dist_v;
  int v1, v2, cd;

  /* Polynomials with more than 2 variables are not handled yet.  When
     the evolution steps are parameters, it is not possible to
     represent the dependence using classical distance vectors.  */
  if (TREE_CODE (c_0) != INTEGER_CST
      || TREE_CODE (CHREC_RIGHT (c_1)) != INTEGER_CST
      || TREE_CODE (CHREC_RIGHT (c_2)) != INTEGER_CST)
    {
      DDR_AFFINE_P (ddr) = false;
      return;
    }

  x_2 = index_in_loop_nest (CHREC_VARIABLE (c_2), DDR_LOOP_NEST (ddr));
  x_1 = index_in_loop_nest (CHREC_VARIABLE (c_1), DDR_LOOP_NEST (ddr));

  /* For "{{0, +, 2}_1, +, 3}_2" the distance vector is (3, -2).  */
  dist_v = lambda_vector_new (DDR_NB_LOOPS (ddr));
  v1 = int_cst_value (CHREC_RIGHT (c_1));
  v2 = int_cst_value (CHREC_RIGHT (c_2));
  cd = gcd (v1, v2);
  v1 /= cd;
  v2 /= cd;

  if (v2 < 0)
    {
      v2 = -v2;
      v1 = -v1;
    }

  dist_v[x_1] = v2;
  dist_v[x_2] = -v1;
  save_dist_v (ddr, dist_v);

  add_outer_distances (ddr, dist_v, x_1);
}

/* Helper function for the case where DDR_A and DDR_B are the same
   access functions.  */

static void
add_other_self_distances (struct data_dependence_relation *ddr)
{
  lambda_vector dist_v;
  unsigned i;
  int index_carry = DDR_NB_LOOPS (ddr);

  for (i = 0; i < DDR_NUM_SUBSCRIPTS (ddr); i++)
    {
      tree access_fun = DR_ACCESS_FN (DDR_A (ddr), i);

      if (TREE_CODE (access_fun) == POLYNOMIAL_CHREC)
	{
	  if (!evolution_function_is_univariate_p (access_fun))
	    {
	      if (DDR_NUM_SUBSCRIPTS (ddr) != 1)
		{
		  DDR_ARE_DEPENDENT (ddr) = chrec_dont_know;
		  return;
		}

	      access_fun = DR_ACCESS_FN (DDR_A (ddr), 0);

	      if (TREE_CODE (CHREC_LEFT (access_fun)) == POLYNOMIAL_CHREC)
		add_multivariate_self_dist (ddr, access_fun);
	      else
		/* The evolution step is not constant: it varies in
		   the outer loop, so this cannot be represented by a
		   distance vector.  For example in pr34635.c the
		   evolution is {0, +, {0, +, 4}_1}_2.  */
		DDR_AFFINE_P (ddr) = false;

	      return;
	    }

	  index_carry = MIN (index_carry,
			     index_in_loop_nest (CHREC_VARIABLE (access_fun),
						 DDR_LOOP_NEST (ddr)));
	}
    }

  dist_v = lambda_vector_new (DDR_NB_LOOPS (ddr));
  add_outer_distances (ddr, dist_v, index_carry);
}

static void
insert_innermost_unit_dist_vector (struct data_dependence_relation *ddr)
{
  lambda_vector dist_v = lambda_vector_new (DDR_NB_LOOPS (ddr));

  dist_v[DDR_INNER_LOOP (ddr)] = 1;
  save_dist_v (ddr, dist_v);
}

/* Adds a unit distance vector to DDR when there is a 0 overlap.  This
   is the case for example when access functions are the same and
   equal to a constant, as in:

   | loop_1
   |   A[3] = ...
   |   ... = A[3]
   | endloop_1

   in which case the distance vectors are (0) and (1).  */

static void
add_distance_for_zero_overlaps (struct data_dependence_relation *ddr)
{
  unsigned i, j;

  for (i = 0; i < DDR_NUM_SUBSCRIPTS (ddr); i++)
    {
      subscript_p sub = DDR_SUBSCRIPT (ddr, i);
      conflict_function *ca = SUB_CONFLICTS_IN_A (sub);
      conflict_function *cb = SUB_CONFLICTS_IN_B (sub);

      for (j = 0; j < ca->n; j++)
	if (affine_function_zero_p (ca->fns[j]))
	  {
	    insert_innermost_unit_dist_vector (ddr);
	    return;
	  }

      for (j = 0; j < cb->n; j++)
	if (affine_function_zero_p (cb->fns[j]))
	  {
	    insert_innermost_unit_dist_vector (ddr);
	    return;
	  }
    }
}

/* Compute the classic per loop distance vector.  DDR is the data
   dependence relation to build a vector from.  Return false when fail
   to represent the data dependence as a distance vector.  */

static bool
build_classic_dist_vector (struct data_dependence_relation *ddr,
			   struct loop *loop_nest)
{
  bool init_b = false;
  int index_carry = DDR_NB_LOOPS (ddr);
  lambda_vector dist_v;

  if (DDR_ARE_DEPENDENT (ddr) != NULL_TREE)
    return false;

  if (same_access_functions (ddr))
    {
      /* Save the 0 vector.  */
      dist_v = lambda_vector_new (DDR_NB_LOOPS (ddr));
      save_dist_v (ddr, dist_v);

      if (constant_access_functions (ddr))
	add_distance_for_zero_overlaps (ddr);

      if (DDR_NB_LOOPS (ddr) > 1)
	add_other_self_distances (ddr);

      return true;
    }

  dist_v = lambda_vector_new (DDR_NB_LOOPS (ddr));
  if (!build_classic_dist_vector_1 (ddr, DDR_A (ddr), DDR_B (ddr),
				    dist_v, &init_b, &index_carry))
    return false;

  /* Save the distance vector if we initialized one.  */
  if (init_b)
    {
      /* Verify a basic constraint: classic distance vectors should
	 always be lexicographically positive.

	 Data references are collected in the order of execution of
	 the program, thus for the following loop

	 | for (i = 1; i < 100; i++)
	 |   for (j = 1; j < 100; j++)
	 |     {
	 |       t = T[j+1][i-1];  // A
	 |       T[j][i] = t + 2;  // B
	 |     }

	 references are collected following the direction of the wind:
	 A then B.  The data dependence tests are performed also
	 following this order, such that we're looking at the distance
	 separating the elements accessed by A from the elements later
	 accessed by B.  But in this example, the distance returned by
	 test_dep (A, B) is lexicographically negative (-1, 1), that
	 means that the access A occurs later than B with respect to
	 the outer loop, ie. we're actually looking upwind.  In this
	 case we solve test_dep (B, A) looking downwind to the
	 lexicographically positive solution, that returns the
	 distance vector (1, -1).  */
      if (!lambda_vector_lexico_pos (dist_v, DDR_NB_LOOPS (ddr)))
	{
	  lambda_vector save_v = lambda_vector_new (DDR_NB_LOOPS (ddr));
	  if (!subscript_dependence_tester_1 (ddr, DDR_B (ddr), DDR_A (ddr),
					      loop_nest))
	    return false;
	  compute_subscript_distance (ddr);
	  if (!build_classic_dist_vector_1 (ddr, DDR_B (ddr), DDR_A (ddr),
					    save_v, &init_b, &index_carry))
	    return false;
	  save_dist_v (ddr, save_v);
	  DDR_REVERSED_P (ddr) = true;

	  /* In this case there is a dependence forward for all the
	     outer loops:

	     | for (k = 1; k < 100; k++)
	     |  for (i = 1; i < 100; i++)
	     |   for (j = 1; j < 100; j++)
	     |     {
	     |       t = T[j+1][i-1];  // A
	     |       T[j][i] = t + 2;  // B
	     |     }

	     the vectors are:
	     (0,  1, -1)
	     (1,  1, -1)
	     (1, -1,  1)
	  */
	  if (DDR_NB_LOOPS (ddr) > 1)
	    {
 	      add_outer_distances (ddr, save_v, index_carry);
	      add_outer_distances (ddr, dist_v, index_carry);
	    }
	}
      else
	{
	  lambda_vector save_v = lambda_vector_new (DDR_NB_LOOPS (ddr));
	  lambda_vector_copy (dist_v, save_v, DDR_NB_LOOPS (ddr));

	  if (DDR_NB_LOOPS (ddr) > 1)
	    {
	      lambda_vector opposite_v = lambda_vector_new (DDR_NB_LOOPS (ddr));

	      if (!subscript_dependence_tester_1 (ddr, DDR_B (ddr),
						  DDR_A (ddr), loop_nest))
		return false;
	      compute_subscript_distance (ddr);
	      if (!build_classic_dist_vector_1 (ddr, DDR_B (ddr), DDR_A (ddr),
						opposite_v, &init_b,
						&index_carry))
		return false;

	      save_dist_v (ddr, save_v);
	      add_outer_distances (ddr, dist_v, index_carry);
	      add_outer_distances (ddr, opposite_v, index_carry);
	    }
	  else
	    save_dist_v (ddr, save_v);
	}
    }
  else
    {
      /* There is a distance of 1 on all the outer loops: Example:
	 there is a dependence of distance 1 on loop_1 for the array A.

	 | loop_1
	 |   A[5] = ...
	 | endloop
      */
      add_outer_distances (ddr, dist_v,
			   lambda_vector_first_nz (dist_v,
						   DDR_NB_LOOPS (ddr), 0));
    }

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      unsigned i;

      fprintf (dump_file, "(build_classic_dist_vector\n");
      for (i = 0; i < DDR_NUM_DIST_VECTS (ddr); i++)
	{
	  fprintf (dump_file, "  dist_vector = (");
	  print_lambda_vector (dump_file, DDR_DIST_VECT (ddr, i),
			       DDR_NB_LOOPS (ddr));
	  fprintf (dump_file, "  )\n");
	}
      fprintf (dump_file, ")\n");
    }

  return true;
}

/* Return the direction for a given distance.
   FIXME: Computing dir this way is suboptimal, since dir can catch
   cases that dist is unable to represent.  */

static inline enum data_dependence_direction
dir_from_dist (int dist)
{
  if (dist > 0)
    return dir_positive;
  else if (dist < 0)
    return dir_negative;
  else
    return dir_equal;
}

/* Compute the classic per loop direction vector.  DDR is the data
   dependence relation to build a vector from.  */

static void
build_classic_dir_vector (struct data_dependence_relation *ddr)
{
  unsigned i, j;
  lambda_vector dist_v;

  FOR_EACH_VEC_ELT (DDR_DIST_VECTS (ddr), i, dist_v)
    {
      lambda_vector dir_v = lambda_vector_new (DDR_NB_LOOPS (ddr));

      for (j = 0; j < DDR_NB_LOOPS (ddr); j++)
	dir_v[j] = dir_from_dist (dist_v[j]);

      save_dir_v (ddr, dir_v);
    }
}

/* Helper function.  Returns true when there is a dependence between
   data references DRA and DRB.  */

static bool
subscript_dependence_tester_1 (struct data_dependence_relation *ddr,
			       struct data_reference *dra,
			       struct data_reference *drb,
			       struct loop *loop_nest)
{
  unsigned int i;
  tree last_conflicts;
  struct subscript *subscript;
  tree res = NULL_TREE;

  for (i = 0; DDR_SUBSCRIPTS (ddr).iterate (i, &subscript); i++)
    {
      conflict_function *overlaps_a, *overlaps_b;

      analyze_overlapping_iterations (DR_ACCESS_FN (dra, i),
				      DR_ACCESS_FN (drb, i),
				      &overlaps_a, &overlaps_b,
				      &last_conflicts, loop_nest);

      if (SUB_CONFLICTS_IN_A (subscript))
	free_conflict_function (SUB_CONFLICTS_IN_A (subscript));
      if (SUB_CONFLICTS_IN_B (subscript))
	free_conflict_function (SUB_CONFLICTS_IN_B (subscript));

      SUB_CONFLICTS_IN_A (subscript) = overlaps_a;
      SUB_CONFLICTS_IN_B (subscript) = overlaps_b;
      SUB_LAST_CONFLICT (subscript) = last_conflicts;

      /* If there is any undetermined conflict function we have to
         give a conservative answer in case we cannot prove that
	 no dependence exists when analyzing another subscript.  */
      if (CF_NOT_KNOWN_P (overlaps_a)
 	  || CF_NOT_KNOWN_P (overlaps_b))
 	{
	  res = chrec_dont_know;
	  continue;
 	}

      /* When there is a subscript with no dependence we can stop.  */
      else if (CF_NO_DEPENDENCE_P (overlaps_a)
 	       || CF_NO_DEPENDENCE_P (overlaps_b))
 	{
	  res = chrec_known;
	  break;
 	}
    }

  if (res == NULL_TREE)
    return true;

  if (res == chrec_known)
    dependence_stats.num_dependence_independent++;
  else
    dependence_stats.num_dependence_undetermined++;
  finalize_ddr_dependent (ddr, res);
  return false;
}

/* Computes the conflicting iterations in LOOP_NEST, and initialize DDR.  */

static void
subscript_dependence_tester (struct data_dependence_relation *ddr,
			     struct loop *loop_nest)
{
  if (subscript_dependence_tester_1 (ddr, DDR_A (ddr), DDR_B (ddr), loop_nest))
    dependence_stats.num_dependence_dependent++;

  compute_subscript_distance (ddr);
  if (build_classic_dist_vector (ddr, loop_nest))
    build_classic_dir_vector (ddr);
}

/* Returns true when all the access functions of A are affine or
   constant with respect to LOOP_NEST.  */

static bool
access_functions_are_affine_or_constant_p (const struct data_reference *a,
					   const struct loop *loop_nest)
{
  unsigned int i;
  vec<tree> fns = DR_ACCESS_FNS (a);
  tree t;

  FOR_EACH_VEC_ELT (fns, i, t)
    if (!evolution_function_is_invariant_p (t, loop_nest->num)
	&& !evolution_function_is_affine_multivariate_p (t, loop_nest->num))
      return false;

  return true;
}

/* This computes the affine dependence relation between A and B with
   respect to LOOP_NEST.  CHREC_KNOWN is used for representing the
   independence between two accesses, while CHREC_DONT_KNOW is used
   for representing the unknown relation.

   Note that it is possible to stop the computation of the dependence
   relation the first time we detect a CHREC_KNOWN element for a given
   subscript.  */

void
compute_affine_dependence (struct data_dependence_relation *ddr,
			   struct loop *loop_nest)
{
  struct data_reference *dra = DDR_A (ddr);
  struct data_reference *drb = DDR_B (ddr);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "(compute_affine_dependence\n");
      fprintf (dump_file, "  stmt_a: ");
      print_gimple_stmt (dump_file, DR_STMT (dra), 0, TDF_SLIM);
      fprintf (dump_file, "  stmt_b: ");
      print_gimple_stmt (dump_file, DR_STMT (drb), 0, TDF_SLIM);
    }

  /* Analyze only when the dependence relation is not yet known.  */
  if (DDR_ARE_DEPENDENT (ddr) == NULL_TREE)
    {
      dependence_stats.num_dependence_tests++;

      if (access_functions_are_affine_or_constant_p (dra, loop_nest)
	  && access_functions_are_affine_or_constant_p (drb, loop_nest))
	subscript_dependence_tester (ddr, loop_nest);

      /* As a last case, if the dependence cannot be determined, or if
	 the dependence is considered too difficult to determine, answer
	 "don't know".  */
      else
	{
	  dependence_stats.num_dependence_undetermined++;

	  if (dump_file && (dump_flags & TDF_DETAILS))
	    {
	      fprintf (dump_file, "Data ref a:\n");
	      dump_data_reference (dump_file, dra);
	      fprintf (dump_file, "Data ref b:\n");
	      dump_data_reference (dump_file, drb);
	      fprintf (dump_file, "affine dependence test not usable: access function not affine or constant.\n");
	    }
	  finalize_ddr_dependent (ddr, chrec_dont_know);
	}
    }

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      if (DDR_ARE_DEPENDENT (ddr) == chrec_known)
	fprintf (dump_file, ") -> no dependence\n");
      else if (DDR_ARE_DEPENDENT (ddr) == chrec_dont_know)
	fprintf (dump_file, ") -> dependence analysis failed\n");
      else
	fprintf (dump_file, ")\n");
    }
}

/* Compute in DEPENDENCE_RELATIONS the data dependence graph for all
   the data references in DATAREFS, in the LOOP_NEST.  When
   COMPUTE_SELF_AND_RR is FALSE, don't compute read-read and self
   relations.  Return true when successful, i.e. data references number
   is small enough to be handled.  */

bool
compute_all_dependences (vec<data_reference_p> datarefs,
			 vec<ddr_p> *dependence_relations,
			 vec<loop_p> loop_nest,
			 bool compute_self_and_rr)
{
  struct data_dependence_relation *ddr;
  struct data_reference *a, *b;
  unsigned int i, j;

  if ((int) datarefs.length ()
      > PARAM_VALUE (PARAM_LOOP_MAX_DATAREFS_FOR_DATADEPS))
    {
      struct data_dependence_relation *ddr;

      /* Insert a single relation into dependence_relations:
	 chrec_dont_know.  */
      ddr = initialize_data_dependence_relation (NULL, NULL, loop_nest);
      dependence_relations->safe_push (ddr);
      return false;
    }

  FOR_EACH_VEC_ELT (datarefs, i, a)
    for (j = i + 1; datarefs.iterate (j, &b); j++)
      if (DR_IS_WRITE (a) || DR_IS_WRITE (b) || compute_self_and_rr)
	{
	  ddr = initialize_data_dependence_relation (a, b, loop_nest);
	  dependence_relations->safe_push (ddr);
          if (loop_nest.exists ())
   	    compute_affine_dependence (ddr, loop_nest[0]);
	}

  if (compute_self_and_rr)
    FOR_EACH_VEC_ELT (datarefs, i, a)
      {
	ddr = initialize_data_dependence_relation (a, a, loop_nest);
	dependence_relations->safe_push (ddr);
        if (loop_nest.exists ())
   	  compute_affine_dependence (ddr, loop_nest[0]);
      }

  return true;
}

/* Describes a location of a memory reference.  */

struct data_ref_loc
{
  /* The memory reference.  */
  tree ref;

  /* True if the memory reference is read.  */
  bool is_read;
};


/* Stores the locations of memory references in STMT to REFERENCES.  Returns
   true if STMT clobbers memory, false otherwise.  */

static bool
get_references_in_stmt (gimple *stmt, vec<data_ref_loc, va_heap> *references)
{
  bool clobbers_memory = false;
  data_ref_loc ref;
  tree op0, op1;
  enum gimple_code stmt_code = gimple_code (stmt);

  /* ASM_EXPR and CALL_EXPR may embed arbitrary side effects.
     As we cannot model data-references to not spelled out
     accesses give up if they may occur.  */
  if (stmt_code == GIMPLE_CALL
      && !(gimple_call_flags (stmt) & ECF_CONST))
    {
      /* Allow IFN_GOMP_SIMD_LANE in their own loops.  */
      if (gimple_call_internal_p (stmt))
	switch (gimple_call_internal_fn (stmt))
	  {
	  case IFN_GOMP_SIMD_LANE:
	    {
	      struct loop *loop = gimple_bb (stmt)->loop_father;
	      tree uid = gimple_call_arg (stmt, 0);
	      gcc_assert (TREE_CODE (uid) == SSA_NAME);
	      if (loop == NULL
		  || loop->simduid != SSA_NAME_VAR (uid))
		clobbers_memory = true;
	      break;
	    }
	  case IFN_MASK_LOAD:
	  case IFN_MASK_STORE:
	    break;
	  default:
	    clobbers_memory = true;
	    break;
	  }
      else
	clobbers_memory = true;
    }
  else if (stmt_code == GIMPLE_ASM
	   && (gimple_asm_volatile_p (as_a <gasm *> (stmt))
	       || gimple_vuse (stmt)))
    clobbers_memory = true;

  if (!gimple_vuse (stmt))
    return clobbers_memory;

  if (stmt_code == GIMPLE_ASSIGN)
    {
      tree base;
      op0 = gimple_assign_lhs (stmt);
      op1 = gimple_assign_rhs1 (stmt);

      if (DECL_P (op1)
	  || (REFERENCE_CLASS_P (op1)
	      && (base = get_base_address (op1))
	      && TREE_CODE (base) != SSA_NAME))
	{
	  ref.ref = op1;
	  ref.is_read = true;
	  references->safe_push (ref);
	}
    }
  else if (stmt_code == GIMPLE_CALL)
    {
      unsigned i, n;
      tree ptr, type;
      unsigned int align;

      ref.is_read = false;
      if (gimple_call_internal_p (stmt))
	switch (gimple_call_internal_fn (stmt))
	  {
	  case IFN_MASK_LOAD:
	    if (gimple_call_lhs (stmt) == NULL_TREE)
	      break;
	    ref.is_read = true;
	  case IFN_MASK_STORE:
	    ptr = build_int_cst (TREE_TYPE (gimple_call_arg (stmt, 1)), 0);
	    align = tree_to_shwi (gimple_call_arg (stmt, 1));
	    if (ref.is_read)
	      type = TREE_TYPE (gimple_call_lhs (stmt));
	    else
	      type = TREE_TYPE (gimple_call_arg (stmt, 3));
	    if (TYPE_ALIGN (type) != align)
	      type = build_aligned_type (type, align);
	    ref.ref = fold_build2 (MEM_REF, type, gimple_call_arg (stmt, 0),
				   ptr);
	    references->safe_push (ref);
	    return false;
	  default:
	    break;
	  }

      op0 = gimple_call_lhs (stmt);
      n = gimple_call_num_args (stmt);
      for (i = 0; i < n; i++)
	{
	  op1 = gimple_call_arg (stmt, i);

	  if (DECL_P (op1)
	      || (REFERENCE_CLASS_P (op1) && get_base_address (op1)))
	    {
	      ref.ref = op1;
	      ref.is_read = true;
	      references->safe_push (ref);
	    }
	}
    }
  else
    return clobbers_memory;

  if (op0
      && (DECL_P (op0)
	  || (REFERENCE_CLASS_P (op0) && get_base_address (op0))))
    {
      ref.ref = op0;
      ref.is_read = false;
      references->safe_push (ref);
    }
  return clobbers_memory;
}


/* Returns true if the loop-nest has any data reference.  */

bool
loop_nest_has_data_refs (loop_p loop)
{
  basic_block *bbs = get_loop_body (loop);
  vec<data_ref_loc> references;
  references.create (3);

  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      basic_block bb = bbs[i];
      gimple_stmt_iterator bsi;

      for (bsi = gsi_start_bb (bb); !gsi_end_p (bsi); gsi_next (&bsi))
	{
	  gimple *stmt = gsi_stmt (bsi);
	  get_references_in_stmt (stmt, &references);
	  if (references.length ())
	    {
	      free (bbs);
	      references.release ();
	      return true;
	    }
	}
    }
  free (bbs);
  references.release ();

  if (loop->inner)
    {
      loop = loop->inner;
      while (loop)
	{
	  if (loop_nest_has_data_refs (loop))
	    return true;
	  loop = loop->next;
	}
    }
  return false;
}

/* Stores the data references in STMT to DATAREFS.  If there is an unanalyzable
   reference, returns false, otherwise returns true.  NEST is the outermost
   loop of the loop nest in which the references should be analyzed.  */

bool
find_data_references_in_stmt (struct loop *nest, gimple *stmt,
			      vec<data_reference_p> *datarefs)
{
  unsigned i;
  auto_vec<data_ref_loc, 2> references;
  data_ref_loc *ref;
  bool ret = true;
  data_reference_p dr;

  if (get_references_in_stmt (stmt, &references))
    return false;

  FOR_EACH_VEC_ELT (references, i, ref)
    {
      dr = create_data_ref (nest, loop_containing_stmt (stmt),
			    ref->ref, stmt, ref->is_read);
      gcc_assert (dr != NULL);
      datarefs->safe_push (dr);
    }
  references.release ();
  return ret;
}

/* Stores the data references in STMT to DATAREFS.  If there is an
   unanalyzable reference, returns false, otherwise returns true.
   NEST is the outermost loop of the loop nest in which the references
   should be instantiated, LOOP is the loop in which the references
   should be analyzed.  */

bool
graphite_find_data_references_in_stmt (loop_p nest, loop_p loop, gimple *stmt,
				       vec<data_reference_p> *datarefs)
{
  unsigned i;
  auto_vec<data_ref_loc, 2> references;
  data_ref_loc *ref;
  bool ret = true;
  data_reference_p dr;

  if (get_references_in_stmt (stmt, &references))
    return false;

  FOR_EACH_VEC_ELT (references, i, ref)
    {
      dr = create_data_ref (nest, loop, ref->ref, stmt, ref->is_read);
      gcc_assert (dr != NULL);
      datarefs->safe_push (dr);
    }

  references.release ();
  return ret;
}

/* Search the data references in LOOP, and record the information into
   DATAREFS.  Returns chrec_dont_know when failing to analyze a
   difficult case, returns NULL_TREE otherwise.  */

tree
find_data_references_in_bb (struct loop *loop, basic_block bb,
                            vec<data_reference_p> *datarefs)
{
  gimple_stmt_iterator bsi;

  for (bsi = gsi_start_bb (bb); !gsi_end_p (bsi); gsi_next (&bsi))
    {
      gimple *stmt = gsi_stmt (bsi);

      if (!find_data_references_in_stmt (loop, stmt, datarefs))
        {
          struct data_reference *res;
          res = XCNEW (struct data_reference);
          datarefs->safe_push (res);

          return chrec_dont_know;
        }
    }

  return NULL_TREE;
}

/* Search the data references in LOOP, and record the information into
   DATAREFS.  Returns chrec_dont_know when failing to analyze a
   difficult case, returns NULL_TREE otherwise.

   TODO: This function should be made smarter so that it can handle address
   arithmetic as if they were array accesses, etc.  */

tree
find_data_references_in_loop (struct loop *loop,
			      vec<data_reference_p> *datarefs)
{
  basic_block bb, *bbs;
  unsigned int i;

  bbs = get_loop_body_in_dom_order (loop);

  for (i = 0; i < loop->num_nodes; i++)
    {
      bb = bbs[i];

      if (find_data_references_in_bb (loop, bb, datarefs) == chrec_dont_know)
        {
          free (bbs);
          return chrec_dont_know;
        }
    }
  free (bbs);

  return NULL_TREE;
}

/* Recursive helper function.  */

static bool
find_loop_nest_1 (struct loop *loop, vec<loop_p> *loop_nest)
{
  /* Inner loops of the nest should not contain siblings.  Example:
     when there are two consecutive loops,

     | loop_0
     |   loop_1
     |     A[{0, +, 1}_1]
     |   endloop_1
     |   loop_2
     |     A[{0, +, 1}_2]
     |   endloop_2
     | endloop_0

     the dependence relation cannot be captured by the distance
     abstraction.  */
  if (loop->next)
    return false;

  loop_nest->safe_push (loop);
  if (loop->inner)
    return find_loop_nest_1 (loop->inner, loop_nest);
  return true;
}

/* Return false when the LOOP is not well nested.  Otherwise return
   true and insert in LOOP_NEST the loops of the nest.  LOOP_NEST will
   contain the loops from the outermost to the innermost, as they will
   appear in the classic distance vector.  */

bool
find_loop_nest (struct loop *loop, vec<loop_p> *loop_nest)
{
  loop_nest->safe_push (loop);
  if (loop->inner)
    return find_loop_nest_1 (loop->inner, loop_nest);
  return true;
}

/* Returns true when the data dependences have been computed, false otherwise.
   Given a loop nest LOOP, the following vectors are returned:
   DATAREFS is initialized to all the array elements contained in this loop,
   DEPENDENCE_RELATIONS contains the relations between the data references.
   Compute read-read and self relations if
   COMPUTE_SELF_AND_READ_READ_DEPENDENCES is TRUE.  */

bool
compute_data_dependences_for_loop (struct loop *loop,
				   bool compute_self_and_read_read_dependences,
				   vec<loop_p> *loop_nest,
				   vec<data_reference_p> *datarefs,
				   vec<ddr_p> *dependence_relations)
{
  bool res = true;

  memset (&dependence_stats, 0, sizeof (dependence_stats));

  /* If the loop nest is not well formed, or one of the data references
     is not computable, give up without spending time to compute other
     dependences.  */
  if (!loop
      || !find_loop_nest (loop, loop_nest)
      || find_data_references_in_loop (loop, datarefs) == chrec_dont_know
      || !compute_all_dependences (*datarefs, dependence_relations, *loop_nest,
				   compute_self_and_read_read_dependences))
    res = false;

  if (dump_file && (dump_flags & TDF_STATS))
    {
      fprintf (dump_file, "Dependence tester statistics:\n");

      fprintf (dump_file, "Number of dependence tests: %d\n",
	       dependence_stats.num_dependence_tests);
      fprintf (dump_file, "Number of dependence tests classified dependent: %d\n",
	       dependence_stats.num_dependence_dependent);
      fprintf (dump_file, "Number of dependence tests classified independent: %d\n",
	       dependence_stats.num_dependence_independent);
      fprintf (dump_file, "Number of undetermined dependence tests: %d\n",
	       dependence_stats.num_dependence_undetermined);

      fprintf (dump_file, "Number of subscript tests: %d\n",
	       dependence_stats.num_subscript_tests);
      fprintf (dump_file, "Number of undetermined subscript tests: %d\n",
	       dependence_stats.num_subscript_undetermined);
      fprintf (dump_file, "Number of same subscript function: %d\n",
	       dependence_stats.num_same_subscript_function);

      fprintf (dump_file, "Number of ziv tests: %d\n",
	       dependence_stats.num_ziv);
      fprintf (dump_file, "Number of ziv tests returning dependent: %d\n",
	       dependence_stats.num_ziv_dependent);
      fprintf (dump_file, "Number of ziv tests returning independent: %d\n",
	       dependence_stats.num_ziv_independent);
      fprintf (dump_file, "Number of ziv tests unimplemented: %d\n",
	       dependence_stats.num_ziv_unimplemented);

      fprintf (dump_file, "Number of siv tests: %d\n",
	       dependence_stats.num_siv);
      fprintf (dump_file, "Number of siv tests returning dependent: %d\n",
	       dependence_stats.num_siv_dependent);
      fprintf (dump_file, "Number of siv tests returning independent: %d\n",
	       dependence_stats.num_siv_independent);
      fprintf (dump_file, "Number of siv tests unimplemented: %d\n",
	       dependence_stats.num_siv_unimplemented);

      fprintf (dump_file, "Number of miv tests: %d\n",
	       dependence_stats.num_miv);
      fprintf (dump_file, "Number of miv tests returning dependent: %d\n",
	       dependence_stats.num_miv_dependent);
      fprintf (dump_file, "Number of miv tests returning independent: %d\n",
	       dependence_stats.num_miv_independent);
      fprintf (dump_file, "Number of miv tests unimplemented: %d\n",
	       dependence_stats.num_miv_unimplemented);
    }

  return res;
}

/* Free the memory used by a data dependence relation DDR.  */

void
free_dependence_relation (struct data_dependence_relation *ddr)
{
  if (ddr == NULL)
    return;

  if (DDR_SUBSCRIPTS (ddr).exists ())
    free_subscripts (DDR_SUBSCRIPTS (ddr));
  DDR_DIST_VECTS (ddr).release ();
  DDR_DIR_VECTS (ddr).release ();

  free (ddr);
}

/* Free the memory used by the data dependence relations from
   DEPENDENCE_RELATIONS.  */

void
free_dependence_relations (vec<ddr_p> dependence_relations)
{
  unsigned int i;
  struct data_dependence_relation *ddr;

  FOR_EACH_VEC_ELT (dependence_relations, i, ddr)
    if (ddr)
      free_dependence_relation (ddr);

  dependence_relations.release ();
}

/* Free the memory used by the data references from DATAREFS.  */

void
free_data_refs (vec<data_reference_p> datarefs)
{
  unsigned int i;
  struct data_reference *dr;

  FOR_EACH_VEC_ELT (datarefs, i, dr)
    free_data_ref (dr);
  datarefs.release ();
}
