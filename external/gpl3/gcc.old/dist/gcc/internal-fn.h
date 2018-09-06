/* Internal functions.
   Copyright (C) 2011-2016 Free Software Foundation, Inc.

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

#ifndef GCC_INTERNAL_FN_H
#define GCC_INTERNAL_FN_H

/* INTEGER_CST values for IFN_UNIQUE function arg-0.  */
enum ifn_unique_kind {
  IFN_UNIQUE_UNSPEC,  /* Undifferentiated UNIQUE.  */

  /* FORK and JOIN mark the points at which OpenACC partitioned
     execution is entered or exited.
     return: data dependency value
     arg-1: data dependency var
     arg-2: INTEGER_CST argument, indicating the axis.  */
  IFN_UNIQUE_OACC_FORK,
  IFN_UNIQUE_OACC_JOIN,

  /* HEAD_MARK and TAIL_MARK are used to demark the sequence entering
     or leaving partitioned execution.
     return: data dependency value
     arg-1: data dependency var
     arg-2: INTEGER_CST argument, remaining markers in this sequence
     arg-3...: varargs on primary header  */
  IFN_UNIQUE_OACC_HEAD_MARK,
  IFN_UNIQUE_OACC_TAIL_MARK
};

/* INTEGER_CST values for IFN_GOACC_LOOP arg-0.  Allows the precise
   stepping of the compute geometry over the loop iterations to be
   deferred until it is known which compiler is generating the code.
   The action is encoded in a constant first argument.

     CHUNK_MAX = LOOP (CODE_CHUNKS, DIR, RANGE, STEP, CHUNK_SIZE, MASK)
     STEP = LOOP (CODE_STEP, DIR, RANGE, STEP, CHUNK_SIZE, MASK)
     OFFSET = LOOP (CODE_OFFSET, DIR, RANGE, STEP, CHUNK_SIZE, MASK, CHUNK_NO)
     BOUND = LOOP (CODE_BOUND, DIR, RANGE, STEP, CHUNK_SIZE, MASK, OFFSET)

     DIR - +1 for up loop, -1 for down loop
     RANGE - Range of loop (END - BASE)
     STEP - iteration step size
     CHUNKING - size of chunking, (constant zero for no chunking)
     CHUNK_NO - chunk number
     MASK - partitioning mask.  */

enum ifn_goacc_loop_kind {
  IFN_GOACC_LOOP_CHUNKS,  /* Number of chunks.  */
  IFN_GOACC_LOOP_STEP,    /* Size of each thread's step.  */
  IFN_GOACC_LOOP_OFFSET,  /* Initial iteration value.  */
  IFN_GOACC_LOOP_BOUND    /* Limit of iteration value.  */
};

/* The GOACC_REDUCTION function defines a generic interface to support
   gang, worker and vector reductions.  All calls are of the following
   form:

     V = REDUCTION (CODE, REF_TO_RES, LOCAL_VAR, LEVEL, OP, OFFSET)

   REF_TO_RES - is a reference to the original reduction varl, may be NULL
   LOCAL_VAR is the intermediate reduction variable
   LEVEL corresponds to the GOMP_DIM of the reduction
   OP is the tree code of the reduction operation
   OFFSET may be used as an offset into a reduction array for the
          reductions occuring at this level.
   In general the return value is LOCAL_VAR, which creates a data
   dependency between calls operating on the same reduction.  */

enum ifn_goacc_reduction_kind {
  IFN_GOACC_REDUCTION_SETUP,
  IFN_GOACC_REDUCTION_INIT,
  IFN_GOACC_REDUCTION_FINI,
  IFN_GOACC_REDUCTION_TEARDOWN
};

/* Initialize internal function tables.  */

extern void init_internal_fns ();

/* Return the name of internal function FN.  The name is only meaningful
   for dumps; it has no linkage.  */

extern const char *const internal_fn_name_array[];

static inline const char *
internal_fn_name (enum internal_fn fn)
{
  return internal_fn_name_array[(int) fn];
}

/* Return the ECF_* flags for function FN.  */

extern const int internal_fn_flags_array[];

static inline int
internal_fn_flags (enum internal_fn fn)
{
  return internal_fn_flags_array[(int) fn];
}

/* Return fnspec for function FN.  */

extern GTY(()) const_tree internal_fn_fnspec_array[IFN_LAST + 1];

static inline const_tree
internal_fn_fnspec (enum internal_fn fn)
{
  return internal_fn_fnspec_array[(int) fn];
}

/* Describes an internal function that maps directly to an optab.  */
struct direct_internal_fn_info
{
  /* optabs can be parameterized by one or two modes.  These fields describe
     how to select those modes from the types of the return value and
     arguments.  A value of -1 says that the mode is determined by the
     return type while a value N >= 0 says that the mode is determined by
     the type of argument N.  A value of -2 says that this internal
     function isn't directly mapped to an optab.  */
  signed int type0 : 8;
  signed int type1 : 8;
  /* True if the function is pointwise, so that it can be vectorized by
     converting the return type and all argument types to vectors of the
     same number of elements.  E.g. we can vectorize an IFN_SQRT on
     floats as an IFN_SQRT on vectors of N floats.

     This only needs 1 bit, but occupies the full 16 to ensure a nice
     layout.  */
  unsigned int vectorizable : 16;
};

extern const direct_internal_fn_info direct_internal_fn_array[IFN_LAST + 1];

/* Return true if FN is mapped directly to an optab.  */

inline bool
direct_internal_fn_p (internal_fn fn)
{
  return direct_internal_fn_array[fn].type0 >= -1;
}

/* Return optab information about internal function FN.  Only meaningful
   if direct_internal_fn_p (FN).  */

inline const direct_internal_fn_info &
direct_internal_fn (internal_fn fn)
{
  gcc_checking_assert (direct_internal_fn_p (fn));
  return direct_internal_fn_array[fn];
}

extern tree_pair direct_internal_fn_types (internal_fn, tree, tree *);
extern tree_pair direct_internal_fn_types (internal_fn, gcall *);
extern bool direct_internal_fn_supported_p (internal_fn, tree_pair,
					    optimization_type);
extern bool direct_internal_fn_supported_p (internal_fn, tree,
					    optimization_type);
extern bool set_edom_supported_p (void);

extern void expand_internal_call (gcall *);
extern void expand_internal_call (internal_fn, gcall *);

#endif
