/* Gimple ranger SSA cache implementation.
   Copyright (C) 2017-2022 Free Software Foundation, Inc.
   Contributed by Andrew MacLeod <amacleod@redhat.com>.

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
#include "insn-codes.h"
#include "tree.h"
#include "gimple.h"
#include "ssa.h"
#include "gimple-pretty-print.h"
#include "gimple-range.h"
#include "tree-cfg.h"
#include "target.h"
#include "attribs.h"
#include "gimple-iterator.h"
#include "gimple-walk.h"
#include "cfganal.h"

#define DEBUG_RANGE_CACHE (dump_file					\
			   && (param_ranger_debug & RANGER_DEBUG_CACHE))

// During contructor, allocate the vector of ssa_names.

non_null_ref::non_null_ref ()
{
  m_nn.create (num_ssa_names);
  m_nn.quick_grow_cleared (num_ssa_names);
  bitmap_obstack_initialize (&m_bitmaps);
}

// Free any bitmaps which were allocated,a swell as the vector itself.

non_null_ref::~non_null_ref ()
{
  bitmap_obstack_release (&m_bitmaps);
  m_nn.release ();
}

// This routine will update NAME in BB to be nonnull if it is not already.
// return TRUE if the update happens.

bool
non_null_ref::set_nonnull (basic_block bb, tree name)
{
  gcc_checking_assert (gimple_range_ssa_p (name)
		       && POINTER_TYPE_P (TREE_TYPE (name)));
  // Only process when its not already set.
  if (non_null_deref_p  (name, bb, false))
    return false;
  bitmap_set_bit (m_nn[SSA_NAME_VERSION (name)], bb->index);
  return true;
}

// Return true if NAME has a non-null dereference in block bb.  If this is the
// first query for NAME, calculate the summary first.
// If SEARCH_DOM is true, the search the dominator tree as well.

bool
non_null_ref::non_null_deref_p (tree name, basic_block bb, bool search_dom)
{
  if (!POINTER_TYPE_P (TREE_TYPE (name)))
    return false;

  unsigned v = SSA_NAME_VERSION (name);
  if (v >= m_nn.length ())
    m_nn.safe_grow_cleared (num_ssa_names + 1);

  if (!m_nn[v])
    process_name (name);

  if (bitmap_bit_p (m_nn[v], bb->index))
    return true;

  // See if any dominator has set non-zero.
  if (search_dom && dom_info_available_p (CDI_DOMINATORS))
    {
      // Search back to the Def block, or the top, whichever is closer.
      basic_block def_bb = gimple_bb (SSA_NAME_DEF_STMT (name));
      basic_block def_dom = def_bb
			    ? get_immediate_dominator (CDI_DOMINATORS, def_bb)
			    : NULL;
      for ( ;
	    bb && bb != def_dom;
	    bb = get_immediate_dominator (CDI_DOMINATORS, bb))
	if (bitmap_bit_p (m_nn[v], bb->index))
	  return true;
    }
  return false;
}

// Allocate an populate the bitmap for NAME.  An ON bit for a block
// index indicates there is a non-null reference in that block.  In
// order to populate the bitmap, a quick run of all the immediate uses
// are made and the statement checked to see if a non-null dereference
// is made on that statement.

void
non_null_ref::process_name (tree name)
{
  unsigned v = SSA_NAME_VERSION (name);
  use_operand_p use_p;
  imm_use_iterator iter;
  bitmap b;

  // Only tracked for pointers.
  if (!POINTER_TYPE_P (TREE_TYPE (name)))
    return;

  // Already processed if a bitmap has been allocated.
  if (m_nn[v])
    return;

  b = BITMAP_ALLOC (&m_bitmaps);

  // Loop over each immediate use and see if it implies a non-null value.
  FOR_EACH_IMM_USE_FAST (use_p, iter, name)
    {
      gimple *s = USE_STMT (use_p);
      unsigned index = gimple_bb (s)->index;

      // If bit is already set for this block, dont bother looking again.
      if (bitmap_bit_p (b, index))
	continue;

      // If we can infer a nonnull range, then set the bit for this BB
      if (!SSA_NAME_OCCURS_IN_ABNORMAL_PHI (name)
	  && infer_nonnull_range (s, name))
	bitmap_set_bit (b, index);
    }

  m_nn[v] = b;
}

// -------------------------------------------------------------------------

// This class represents the API into a cache of ranges for an SSA_NAME.
// Routines must be implemented to set, get, and query if a value is set.

class ssa_block_ranges
{
public:
  virtual bool set_bb_range (const_basic_block bb, const irange &r) = 0;
  virtual bool get_bb_range (irange &r, const_basic_block bb) = 0;
  virtual bool bb_range_p (const_basic_block bb) = 0;

  void dump(FILE *f);
};

// Print the list of known ranges for file F in a nice format.

void
ssa_block_ranges::dump (FILE *f)
{
  basic_block bb;
  int_range_max r;

  FOR_EACH_BB_FN (bb, cfun)
    if (get_bb_range (r, bb))
      {
	fprintf (f, "BB%d  -> ", bb->index);
	r.dump (f);
	fprintf (f, "\n");
      }
}

// This class implements the range cache as a linear vector, indexed by BB.
// It caches a varying and undefined range which are used instead of
// allocating new ones each time.

class sbr_vector : public ssa_block_ranges
{
public:
  sbr_vector (tree t, irange_allocator *allocator);

  virtual bool set_bb_range (const_basic_block bb, const irange &r) OVERRIDE;
  virtual bool get_bb_range (irange &r, const_basic_block bb) OVERRIDE;
  virtual bool bb_range_p (const_basic_block bb) OVERRIDE;
protected:
  irange **m_tab;	// Non growing vector.
  int m_tab_size;
  int_range<2> m_varying;
  int_range<2> m_undefined;
  tree m_type;
  irange_allocator *m_irange_allocator;
  void grow ();
};


// Initialize a block cache for an ssa_name of type T.

sbr_vector::sbr_vector (tree t, irange_allocator *allocator)
{
  gcc_checking_assert (TYPE_P (t));
  m_type = t;
  m_irange_allocator = allocator;
  m_tab_size = last_basic_block_for_fn (cfun) + 1;
  m_tab = (irange **)allocator->get_memory (m_tab_size * sizeof (irange *));
  memset (m_tab, 0, m_tab_size * sizeof (irange *));

  // Create the cached type range.
  m_varying.set_varying (t);
  m_undefined.set_undefined ();
}

// Grow the vector when the CFG has increased in size.

void
sbr_vector::grow ()
{
  int curr_bb_size = last_basic_block_for_fn (cfun);
  gcc_checking_assert (curr_bb_size > m_tab_size);

  // Increase the max of a)128, b)needed increase * 2, c)10% of current_size.
  int inc = MAX ((curr_bb_size - m_tab_size) * 2, 128);
  inc = MAX (inc, curr_bb_size / 10);
  int new_size = inc + curr_bb_size;

  // Allocate new memory, copy the old vector and clear the new space.
  irange **t = (irange **)m_irange_allocator->get_memory (new_size
							  * sizeof (irange *));
  memcpy (t, m_tab, m_tab_size * sizeof (irange *));
  memset (t + m_tab_size, 0, (new_size - m_tab_size) * sizeof (irange *));

  m_tab = t;
  m_tab_size = new_size;
}

// Set the range for block BB to be R.

bool
sbr_vector::set_bb_range (const_basic_block bb, const irange &r)
{
  irange *m;
  if (bb->index >= m_tab_size)
    grow ();
  if (r.varying_p ())
    m = &m_varying;
  else if (r.undefined_p ())
    m = &m_undefined;
  else
    m = m_irange_allocator->allocate (r);
  m_tab[bb->index] = m;
  return true;
}

// Return the range associated with block BB in R.  Return false if
// there is no range.

bool
sbr_vector::get_bb_range (irange &r, const_basic_block bb)
{
  if (bb->index >= m_tab_size)
    return false;
  irange *m = m_tab[bb->index];
  if (m)
    {
      r = *m;
      return true;
    }
  return false;
}

// Return true if a range is present.

bool
sbr_vector::bb_range_p (const_basic_block bb)
{
  if (bb->index < m_tab_size)
    return m_tab[bb->index] != NULL;
  return false;
}

// This class implements the on entry cache via a sparse bitmap.
// It uses the quad bit routines to access 4 bits at a time.
// A value of 0 (the default) means there is no entry, and a value of
// 1 thru SBR_NUM represents an element in the m_range vector.
// Varying is given the first value (1) and pre-cached.
// SBR_NUM + 1 represents the value of UNDEFINED, and is never stored.
// SBR_NUM is the number of values that can be cached.
// Indexes are 1..SBR_NUM and are stored locally at m_range[0..SBR_NUM-1]

#define SBR_NUM		14
#define SBR_UNDEF	SBR_NUM + 1
#define SBR_VARYING	1

class sbr_sparse_bitmap : public ssa_block_ranges
{
public:
  sbr_sparse_bitmap (tree t, irange_allocator *allocator, bitmap_obstack *bm);
  virtual bool set_bb_range (const_basic_block bb, const irange &r) OVERRIDE;
  virtual bool get_bb_range (irange &r, const_basic_block bb) OVERRIDE;
  virtual bool bb_range_p (const_basic_block bb) OVERRIDE;
private:
  void bitmap_set_quad (bitmap head, int quad, int quad_value);
  int bitmap_get_quad (const_bitmap head, int quad);
  irange_allocator *m_irange_allocator;
  irange *m_range[SBR_NUM];
  bitmap_head bitvec;
  tree m_type;
};

// Initialize a block cache for an ssa_name of type T.

sbr_sparse_bitmap::sbr_sparse_bitmap (tree t, irange_allocator *allocator,
				bitmap_obstack *bm)
{
  gcc_checking_assert (TYPE_P (t));
  m_type = t;
  bitmap_initialize (&bitvec, bm);
  bitmap_tree_view (&bitvec);
  m_irange_allocator = allocator;
  // Pre-cache varying.
  m_range[0] = m_irange_allocator->allocate (2);
  m_range[0]->set_varying (t);
  // Pre-cache zero and non-zero values for pointers.
  if (POINTER_TYPE_P (t))
    {
      m_range[1] = m_irange_allocator->allocate (2);
      m_range[1]->set_nonzero (t);
      m_range[2] = m_irange_allocator->allocate (2);
      m_range[2]->set_zero (t);
    }
  else
    m_range[1] = m_range[2] = NULL;
  // Clear SBR_NUM entries.
  for (int x = 3; x < SBR_NUM; x++)
    m_range[x] = 0;
}

// Set 4 bit values in a sparse bitmap. This allows a bitmap to
// function as a sparse array of 4 bit values.
// QUAD is the index, QUAD_VALUE is the 4 bit value to set.

inline void
sbr_sparse_bitmap::bitmap_set_quad (bitmap head, int quad, int quad_value)
{
  bitmap_set_aligned_chunk (head, quad, 4, (BITMAP_WORD) quad_value);
}

// Get a 4 bit value from a sparse bitmap. This allows a bitmap to
// function as a sparse array of 4 bit values.
// QUAD is the index.
inline int
sbr_sparse_bitmap::bitmap_get_quad (const_bitmap head, int quad)
{
  return (int) bitmap_get_aligned_chunk (head, quad, 4);
}

// Set the range on entry to basic block BB to R.

bool
sbr_sparse_bitmap::set_bb_range (const_basic_block bb, const irange &r)
{
  if (r.undefined_p ())
    {
      bitmap_set_quad (&bitvec, bb->index, SBR_UNDEF);
      return true;
    }

  // Loop thru the values to see if R is already present.
  for (int x = 0; x < SBR_NUM; x++)
    if (!m_range[x] || r == *(m_range[x]))
      {
	if (!m_range[x])
	  m_range[x] = m_irange_allocator->allocate (r);
	bitmap_set_quad (&bitvec, bb->index, x + 1);
	return true;
      }
  // All values are taken, default to VARYING.
  bitmap_set_quad (&bitvec, bb->index, SBR_VARYING);
  return false;
}

// Return the range associated with block BB in R.  Return false if
// there is no range.

bool
sbr_sparse_bitmap::get_bb_range (irange &r, const_basic_block bb)
{
  int value = bitmap_get_quad (&bitvec, bb->index);

  if (!value)
    return false;

  gcc_checking_assert (value <= SBR_UNDEF);
  if (value == SBR_UNDEF)
    r.set_undefined ();
  else
    r = *(m_range[value - 1]);
  return true;
}

// Return true if a range is present.

bool
sbr_sparse_bitmap::bb_range_p (const_basic_block bb)
{
  return (bitmap_get_quad (&bitvec, bb->index) != 0);
}

// -------------------------------------------------------------------------

// Initialize the block cache.

block_range_cache::block_range_cache ()
{
  bitmap_obstack_initialize (&m_bitmaps);
  m_ssa_ranges.create (0);
  m_ssa_ranges.safe_grow_cleared (num_ssa_names);
  m_irange_allocator = new irange_allocator;
}

// Remove any m_block_caches which have been created.

block_range_cache::~block_range_cache ()
{
  delete m_irange_allocator;
  // Release the vector itself.
  m_ssa_ranges.release ();
  bitmap_obstack_release (&m_bitmaps);
}

// Set the range for NAME on entry to block BB to R.
// If it has not been accessed yet, allocate it first.

bool
block_range_cache::set_bb_range (tree name, const_basic_block bb,
				 const irange &r)
{
  unsigned v = SSA_NAME_VERSION (name);
  if (v >= m_ssa_ranges.length ())
    m_ssa_ranges.safe_grow_cleared (num_ssa_names + 1);

  if (!m_ssa_ranges[v])
    {
      // Use sparse representation if there are too many basic blocks.
      if (last_basic_block_for_fn (cfun) > param_evrp_sparse_threshold)
	{
	  void *r = m_irange_allocator->get_memory (sizeof (sbr_sparse_bitmap));
	  m_ssa_ranges[v] = new (r) sbr_sparse_bitmap (TREE_TYPE (name),
						       m_irange_allocator,
						       &m_bitmaps);
	}
      else
	{
	  // Otherwise use the default vector implemntation.
	  void *r = m_irange_allocator->get_memory (sizeof (sbr_vector));
	  m_ssa_ranges[v] = new (r) sbr_vector (TREE_TYPE (name),
						m_irange_allocator);
	}
    }
  return m_ssa_ranges[v]->set_bb_range (bb, r);
}


// Return a pointer to the ssa_block_cache for NAME.  If it has not been
// accessed yet, return NULL.

inline ssa_block_ranges *
block_range_cache::query_block_ranges (tree name)
{
  unsigned v = SSA_NAME_VERSION (name);
  if (v >= m_ssa_ranges.length () || !m_ssa_ranges[v])
    return NULL;
  return m_ssa_ranges[v];
}



// Return the range for NAME on entry to BB in R.  Return true if there
// is one.

bool
block_range_cache::get_bb_range (irange &r, tree name, const_basic_block bb)
{
  ssa_block_ranges *ptr = query_block_ranges (name);
  if (ptr)
    return ptr->get_bb_range (r, bb);
  return false;
}

// Return true if NAME has a range set in block BB.

bool
block_range_cache::bb_range_p (tree name, const_basic_block bb)
{
  ssa_block_ranges *ptr = query_block_ranges (name);
  if (ptr)
    return ptr->bb_range_p (bb);
  return false;
}

// Print all known block caches to file F.

void
block_range_cache::dump (FILE *f)
{
  unsigned x;
  for (x = 0; x < m_ssa_ranges.length (); ++x)
    {
      if (m_ssa_ranges[x])
	{
	  fprintf (f, " Ranges for ");
	  print_generic_expr (f, ssa_name (x), TDF_NONE);
	  fprintf (f, ":\n");
	  m_ssa_ranges[x]->dump (f);
	  fprintf (f, "\n");
	}
    }
}

// Print all known ranges on entry to blobk BB to file F.

void
block_range_cache::dump (FILE *f, basic_block bb, bool print_varying)
{
  unsigned x;
  int_range_max r;
  bool summarize_varying = false;
  for (x = 1; x < m_ssa_ranges.length (); ++x)
    {
      if (!gimple_range_ssa_p (ssa_name (x)))
	continue;
      if (m_ssa_ranges[x] && m_ssa_ranges[x]->get_bb_range (r, bb))
	{
	  if (!print_varying && r.varying_p ())
	    {
	      summarize_varying = true;
	      continue;
	    }
	  print_generic_expr (f, ssa_name (x), TDF_NONE);
	  fprintf (f, "\t");
	  r.dump(f);
	  fprintf (f, "\n");
	}
    }
  // If there were any varying entries, lump them all together.
  if (summarize_varying)
    {
      fprintf (f, "VARYING_P on entry : ");
      for (x = 1; x < num_ssa_names; ++x)
	{
	  if (!gimple_range_ssa_p (ssa_name (x)))
	    continue;
	  if (m_ssa_ranges[x] && m_ssa_ranges[x]->get_bb_range (r, bb))
	    {
	      if (r.varying_p ())
		{
		  print_generic_expr (f, ssa_name (x), TDF_NONE);
		  fprintf (f, "  ");
		}
	    }
	}
      fprintf (f, "\n");
    }
}

// -------------------------------------------------------------------------

// Initialize a global cache.

ssa_global_cache::ssa_global_cache ()
{
  m_tab.create (0);
  m_irange_allocator = new irange_allocator;
}

// Deconstruct a global cache.

ssa_global_cache::~ssa_global_cache ()
{
  m_tab.release ();
  delete m_irange_allocator;
}

// Retrieve the global range of NAME from cache memory if it exists. 
// Return the value in R.

bool
ssa_global_cache::get_global_range (irange &r, tree name) const
{
  unsigned v = SSA_NAME_VERSION (name);
  if (v >= m_tab.length ())
    return false;

  irange *stow = m_tab[v];
  if (!stow)
    return false;
  r = *stow;
  return true;
}

// Set the range for NAME to R in the global cache.
// Return TRUE if there was already a range set, otherwise false.

bool
ssa_global_cache::set_global_range (tree name, const irange &r)
{
  unsigned v = SSA_NAME_VERSION (name);
  if (v >= m_tab.length ())
    m_tab.safe_grow_cleared (num_ssa_names + 1);

  irange *m = m_tab[v];
  if (m && m->fits_p (r))
    *m = r;
  else
    m_tab[v] = m_irange_allocator->allocate (r);
  return m != NULL;
}

// Set the range for NAME to R in the glonbal cache.

void
ssa_global_cache::clear_global_range (tree name)
{
  unsigned v = SSA_NAME_VERSION (name);
  if (v >= m_tab.length ())
    m_tab.safe_grow_cleared (num_ssa_names + 1);
  m_tab[v] = NULL;
}

// Clear the global cache.

void
ssa_global_cache::clear ()
{
  if (m_tab.address ())
    memset (m_tab.address(), 0, m_tab.length () * sizeof (irange *));
}

// Dump the contents of the global cache to F.

void
ssa_global_cache::dump (FILE *f)
{
  /* Cleared after the table header has been printed.  */
  bool print_header = true;
  for (unsigned x = 1; x < num_ssa_names; x++)
    {
      int_range_max r;
      if (gimple_range_ssa_p (ssa_name (x)) &&
	  get_global_range (r, ssa_name (x))  && !r.varying_p ())
	{
	  if (print_header)
	    {
	      /* Print the header only when there's something else
		 to print below.  */
	      fprintf (f, "Non-varying global ranges:\n");
	      fprintf (f, "=========================:\n");
	      print_header = false;
	    }

	  print_generic_expr (f, ssa_name (x), TDF_NONE);
	  fprintf (f, "  : ");
	  r.dump (f);
	  fprintf (f, "\n");
	}
    }

  if (!print_header)
    fputc ('\n', f);
}

// --------------------------------------------------------------------------


// This class will manage the timestamps for each ssa_name.
// When a value is calculated, the timestamp is set to the current time.
// Current time is then incremented.  Any dependencies will already have
// been calculated, and will thus have older timestamps.
// If one of those values is ever calculated again, it will get a newer
// timestamp, and the "current_p" check will fail.

class temporal_cache
{
public:
  temporal_cache ();
  ~temporal_cache ();
  bool current_p (tree name, tree dep1, tree dep2) const;
  void set_timestamp (tree name);
  void set_always_current (tree name);
private:
  unsigned temporal_value (unsigned ssa) const;

  unsigned m_current_time;
  vec <unsigned> m_timestamp;
};

inline
temporal_cache::temporal_cache ()
{
  m_current_time = 1;
  m_timestamp.create (0);
  m_timestamp.safe_grow_cleared (num_ssa_names);
}

inline
temporal_cache::~temporal_cache ()
{
  m_timestamp.release ();
}

// Return the timestamp value for SSA, or 0 if there isnt one.

inline unsigned
temporal_cache::temporal_value (unsigned ssa) const
{
  if (ssa >= m_timestamp.length ())
    return 0;
  return m_timestamp[ssa];
}

// Return TRUE if the timestampe for NAME is newer than any of its dependents.
// Up to 2 dependencies can be checked.

bool
temporal_cache::current_p (tree name, tree dep1, tree dep2) const
{
  unsigned ts = temporal_value (SSA_NAME_VERSION (name));
  if (ts == 0)
    return true;

  // Any non-registered dependencies will have a value of 0 and thus be older.
  // Return true if time is newer than either dependent.

  if (dep1 && ts < temporal_value (SSA_NAME_VERSION (dep1)))
    return false;
  if (dep2 && ts < temporal_value (SSA_NAME_VERSION (dep2)))
    return false;

  return true;
}

// This increments the global timer and sets the timestamp for NAME.

inline void
temporal_cache::set_timestamp (tree name)
{
  unsigned v = SSA_NAME_VERSION (name);
  if (v >= m_timestamp.length ())
    m_timestamp.safe_grow_cleared (num_ssa_names + 20);
  m_timestamp[v] = ++m_current_time;
}

// Set the timestamp to 0, marking it as "always up to date".

inline void
temporal_cache::set_always_current (tree name)
{
  unsigned v = SSA_NAME_VERSION (name);
  if (v >= m_timestamp.length ())
    m_timestamp.safe_grow_cleared (num_ssa_names + 20);
  m_timestamp[v] = 0;
}

// --------------------------------------------------------------------------

// This class provides an abstraction of a list of blocks to be updated
// by the cache.  It is currently a stack but could be changed.  It also
// maintains a list of blocks which have failed propagation, and does not
// enter any of those blocks into the list.

// A vector over the BBs is maintained, and an entry of 0 means it is not in
// a list.  Otherwise, the entry is the next block in the list. -1 terminates
// the list.  m_head points to the top of the list, -1 if the list is empty.

class update_list
{
public:
  update_list ();
  ~update_list ();
  void add (basic_block bb);
  basic_block pop ();
  inline bool empty_p () { return m_update_head == -1; }
  inline void clear_failures () { bitmap_clear (m_propfail); }
  inline void propagation_failed (basic_block bb)
				  { bitmap_set_bit (m_propfail, bb->index); }
private:
  vec<int> m_update_list;
  int m_update_head;
  bitmap m_propfail;
};

// Create an update list.

update_list::update_list ()
{
  m_update_list.create (0);
  m_update_list.safe_grow_cleared (last_basic_block_for_fn (cfun) + 64);
  m_update_head = -1;
  m_propfail = BITMAP_ALLOC (NULL);
}

// Destroy an update list.

update_list::~update_list ()
{
  m_update_list.release ();
  BITMAP_FREE (m_propfail);
}

// Add BB to the list of blocks to update, unless it's already in the list.

void
update_list::add (basic_block bb)
{
  int i = bb->index;
  // If propagation has failed for BB, or its already in the list, don't
  // add it again.
  if ((unsigned)i >= m_update_list.length ())
    m_update_list.safe_grow_cleared (i + 64);
  if (!m_update_list[i] && !bitmap_bit_p (m_propfail, i))
    {
      if (empty_p ())
	{
	  m_update_head = i;
	  m_update_list[i] = -1;
	}
      else
	{
	  gcc_checking_assert (m_update_head > 0);
	  m_update_list[i] = m_update_head;
	  m_update_head = i;
	}
    }
}

// Remove a block from the list.

basic_block
update_list::pop ()
{
  gcc_checking_assert (!empty_p ());
  basic_block bb = BASIC_BLOCK_FOR_FN (cfun, m_update_head);
  int pop = m_update_head;
  m_update_head = m_update_list[pop];
  m_update_list[pop] = 0;
  return bb;
}

// --------------------------------------------------------------------------

ranger_cache::ranger_cache (int not_executable_flag)
						: m_gori (not_executable_flag)
{
  m_workback.create (0);
  m_workback.safe_grow_cleared (last_basic_block_for_fn (cfun));
  m_temporal = new temporal_cache;
  // If DOM info is available, spawn an oracle as well.
  if (dom_info_available_p (CDI_DOMINATORS))
      m_oracle = new dom_oracle ();
    else
      m_oracle = NULL;

  unsigned x, lim = last_basic_block_for_fn (cfun);
  // Calculate outgoing range info upfront.  This will fully populate the
  // m_maybe_variant bitmap which will help eliminate processing of names
  // which never have their ranges adjusted.
  for (x = 0; x < lim ; x++)
    {
      basic_block bb = BASIC_BLOCK_FOR_FN (cfun, x);
      if (bb)
	m_gori.exports (bb);
    }
  m_update = new update_list ();
}

ranger_cache::~ranger_cache ()
{
  delete m_update;
  if (m_oracle)
    delete m_oracle;
  delete m_temporal;
  m_workback.release ();
}

// Dump the global caches to file F.  if GORI_DUMP is true, dump the
// gori map as well.

void
ranger_cache::dump (FILE *f)
{
  m_globals.dump (f);
  fprintf (f, "\n");
}

// Dump the caches for basic block BB to file F.

void
ranger_cache::dump_bb (FILE *f, basic_block bb)
{
  m_gori.gori_map::dump (f, bb, false);
  m_on_entry.dump (f, bb);
  if (m_oracle)
    m_oracle->dump (f, bb);
}

// Get the global range for NAME, and return in R.  Return false if the
// global range is not set, and return the legacy global value in R.

bool
ranger_cache::get_global_range (irange &r, tree name) const
{
  if (m_globals.get_global_range (r, name))
    return true;
  r = gimple_range_global (name);
  return false;
}

// Get the global range for NAME, and return in R.  Return false if the
// global range is not set, and R will contain the legacy global value.
// CURRENT_P is set to true if the value was in cache and not stale.
// Otherwise, set CURRENT_P to false and mark as it always current.
// If the global cache did not have a value, initialize it as well.
// After this call, the global cache will have a value.

bool
ranger_cache::get_global_range (irange &r, tree name, bool &current_p)
{
  bool had_global = get_global_range (r, name);

  // If there was a global value, set current flag, otherwise set a value.
  current_p = false;
  if (had_global)
    current_p = r.singleton_p ()
		|| m_temporal->current_p (name, m_gori.depend1 (name),
					  m_gori.depend2 (name));
  else
    m_globals.set_global_range (name, r);

  // If the existing value was not current, mark it as always current.
  if (!current_p)
    m_temporal->set_always_current (name);
  return current_p;
}

//  Set the global range of NAME to R and give it a timestamp.

void
ranger_cache::set_global_range (tree name, const irange &r)
{
  if (m_globals.set_global_range (name, r))
    {
      // If there was already a range set, propagate the new value.
      basic_block bb = gimple_bb (SSA_NAME_DEF_STMT (name));
      if (!bb)
	bb = ENTRY_BLOCK_PTR_FOR_FN (cfun);

      if (DEBUG_RANGE_CACHE)
	fprintf (dump_file, "   GLOBAL :");

      propagate_updated_value (name, bb);
    }
  // Constants no longer need to tracked.  Any further refinement has to be
  // undefined. Propagation works better with constants. PR 100512.
  // Pointers which resolve to non-zero also do not need
  // tracking in the cache as they will never change.  See PR 98866.
  // Timestamp must always be updated, or dependent calculations may
  // not include this latest value. PR 100774.

  if (r.singleton_p ()
      || (POINTER_TYPE_P (TREE_TYPE (name)) && r.nonzero_p ()))
    m_gori.set_range_invariant (name);
  m_temporal->set_timestamp (name);
}

//  Provide lookup for the gori-computes class to access the best known range
//  of an ssa_name in any given basic block.  Note, this does no additonal
//  lookups, just accesses the data that is already known.

// Get the range of NAME when the def occurs in block BB.  If BB is NULL
// get the best global value available.

void
ranger_cache::range_of_def (irange &r, tree name, basic_block bb)
{
  gcc_checking_assert (gimple_range_ssa_p (name));
  gcc_checking_assert (!bb || bb == gimple_bb (SSA_NAME_DEF_STMT (name)));

  // Pick up the best global range available.
  if (!m_globals.get_global_range (r, name))
    {
      // If that fails, try to calculate the range using just global values.
      gimple *s = SSA_NAME_DEF_STMT (name);
      if (gimple_get_lhs (s) == name)
	fold_range (r, s, get_global_range_query ());
      else
	r = gimple_range_global (name);
    }
}

// Get the range of NAME as it occurs on entry to block BB.

void
ranger_cache::entry_range (irange &r, tree name, basic_block bb)
{
  if (bb == ENTRY_BLOCK_PTR_FOR_FN (cfun))
    {
      r = gimple_range_global (name);
      return;
    }

  // Look for the on-entry value of name in BB from the cache.
  // Otherwise pick up the best available global value.
  if (!m_on_entry.get_bb_range (r, name, bb))
    range_of_def (r, name);
}

// Get the range of NAME as it occurs on exit from block BB.

void
ranger_cache::exit_range (irange &r, tree name, basic_block bb)
{
  if (bb == ENTRY_BLOCK_PTR_FOR_FN (cfun))
    {
      r = gimple_range_global (name);
      return;
    }

  gimple *s = SSA_NAME_DEF_STMT (name);
  basic_block def_bb = gimple_bb (s);
  if (def_bb == bb)
    range_of_def (r, name, bb);
  else
    entry_range (r, name, bb);
 }


// Implement range_of_expr.

bool
ranger_cache::range_of_expr (irange &r, tree name, gimple *stmt)
{
  if (!gimple_range_ssa_p (name))
    {
      get_tree_range (r, name, stmt);
      return true;
    }

  basic_block bb = gimple_bb (stmt);
  gimple *def_stmt = SSA_NAME_DEF_STMT (name);
  basic_block def_bb = gimple_bb (def_stmt);

  if (bb == def_bb)
    range_of_def (r, name, bb);
  else
    entry_range (r, name, bb);
  return true;
}


// Implement range_on_edge.  Always return the best available range.

 bool
 ranger_cache::range_on_edge (irange &r, edge e, tree expr)
 {
   if (gimple_range_ssa_p (expr))
    {
      exit_range (r, expr, e->src);
      // If this is not an abnormal edge, check for a non-null exit.
      if ((e->flags & (EDGE_EH | EDGE_ABNORMAL)) == 0)
	m_non_null.adjust_range (r, expr, e->src, false);
      int_range_max edge_range;
      if (m_gori.outgoing_edge_range_p (edge_range, e, expr, *this))
	r.intersect (edge_range);
      return true;
    }

  return get_tree_range (r, expr, NULL);
}


// Return a static range for NAME on entry to basic block BB in R.  If
// calc is true, fill any cache entries required between BB and the
// def block for NAME.  Otherwise, return false if the cache is empty.

bool
ranger_cache::block_range (irange &r, basic_block bb, tree name, bool calc)
{
  gcc_checking_assert (gimple_range_ssa_p (name));

  // If there are no range calculations anywhere in the IL, global range
  // applies everywhere, so don't bother caching it.
  if (!m_gori.has_edge_range_p (name))
    return false;

  if (calc)
    {
      gimple *def_stmt = SSA_NAME_DEF_STMT (name);
      basic_block def_bb = NULL;
      if (def_stmt)
	def_bb = gimple_bb (def_stmt);;
      if (!def_bb)
	{
	  // If we get to the entry block, this better be a default def
	  // or range_on_entry was called for a block not dominated by
	  // the def.  
	  gcc_checking_assert (SSA_NAME_IS_DEFAULT_DEF (name));
	  def_bb = ENTRY_BLOCK_PTR_FOR_FN (cfun);
	}

      // There is no range on entry for the definition block.
      if (def_bb == bb)
	return false;

      // Otherwise, go figure out what is known in predecessor blocks.
      fill_block_cache (name, bb, def_bb);
      gcc_checking_assert (m_on_entry.bb_range_p (name, bb));
    }
  return m_on_entry.get_bb_range (r, name, bb);
}

// If there is anything in the propagation update_list, continue
// processing NAME until the list of blocks is empty.

void
ranger_cache::propagate_cache (tree name)
{
  basic_block bb;
  edge_iterator ei;
  edge e;
  int_range_max new_range;
  int_range_max current_range;
  int_range_max e_range;

  // Process each block by seeing if its calculated range on entry is
  // the same as its cached value. If there is a difference, update
  // the cache to reflect the new value, and check to see if any
  // successors have cache entries which may need to be checked for
  // updates.

  while (!m_update->empty_p ())
    {
      bb = m_update->pop ();
      gcc_checking_assert (m_on_entry.bb_range_p (name, bb));
      m_on_entry.get_bb_range (current_range, name, bb);

      if (DEBUG_RANGE_CACHE)
	{
	  fprintf (dump_file, "FWD visiting block %d for ", bb->index);
	  print_generic_expr (dump_file, name, TDF_SLIM);
	  fprintf (dump_file, "  starting range : ");
	  current_range.dump (dump_file);
	  fprintf (dump_file, "\n");
	}

      // Calculate the "new" range on entry by unioning the pred edges.
      new_range.set_undefined ();
      FOR_EACH_EDGE (e, ei, bb->preds)
	{
	  range_on_edge (e_range, e, name);
	  if (DEBUG_RANGE_CACHE)
	    {
	      fprintf (dump_file, "   edge %d->%d :", e->src->index, bb->index);
	      e_range.dump (dump_file);
	      fprintf (dump_file, "\n");
	    }
	  new_range.union_ (e_range);
	  if (new_range.varying_p ())
	    break;
	}

      // If the range on entry has changed, update it.
      if (new_range != current_range)
	{
	  bool ok_p = m_on_entry.set_bb_range (name, bb, new_range);
	  // If the cache couldn't set the value, mark it as failed.
	  if (!ok_p)
	    m_update->propagation_failed (bb);
	  if (DEBUG_RANGE_CACHE) 
	    {
	      if (!ok_p)
		{
		  fprintf (dump_file, "   Cache failure to store value:");
		  print_generic_expr (dump_file, name, TDF_SLIM);
		  fprintf (dump_file, "  ");
		}
	      else
		{
		  fprintf (dump_file, "      Updating range to ");
		  new_range.dump (dump_file);
		}
	      fprintf (dump_file, "\n      Updating blocks :");
	    }
	  // Mark each successor that has a range to re-check its range
	  FOR_EACH_EDGE (e, ei, bb->succs)
	    if (m_on_entry.bb_range_p (name, e->dest))
	      {
		if (DEBUG_RANGE_CACHE) 
		  fprintf (dump_file, " bb%d",e->dest->index);
		m_update->add (e->dest);
	      }
	  if (DEBUG_RANGE_CACHE) 
	    fprintf (dump_file, "\n");
	}
    }
  if (DEBUG_RANGE_CACHE)
    {
      fprintf (dump_file, "DONE visiting blocks for ");
      print_generic_expr (dump_file, name, TDF_SLIM);
      fprintf (dump_file, "\n");
    }
  m_update->clear_failures ();
}

// Check to see if an update to the value for NAME in BB has any effect
// on values already in the on-entry cache for successor blocks.
// If it does, update them.  Don't visit any blocks which dont have a cache
// entry.

void
ranger_cache::propagate_updated_value (tree name, basic_block bb)
{
  edge e;
  edge_iterator ei;

  // The update work list should be empty at this point.
  gcc_checking_assert (m_update->empty_p ());
  gcc_checking_assert (bb);

  if (DEBUG_RANGE_CACHE)
    {
      fprintf (dump_file, " UPDATE cache for ");
      print_generic_expr (dump_file, name, TDF_SLIM);
      fprintf (dump_file, " in BB %d : successors : ", bb->index);
    }
  FOR_EACH_EDGE (e, ei, bb->succs)
    {
      // Only update active cache entries.
      if (m_on_entry.bb_range_p (name, e->dest))
	{
	  m_update->add (e->dest);
	  if (DEBUG_RANGE_CACHE)
	    fprintf (dump_file, " UPDATE: bb%d", e->dest->index);
	}
    }
    if (!m_update->empty_p ())
      {
	if (DEBUG_RANGE_CACHE)
	  fprintf (dump_file, "\n");
	propagate_cache (name);
      }
    else
      {
	if (DEBUG_RANGE_CACHE)
	  fprintf (dump_file, "  : No updates!\n");
      }
}

// Make sure that the range-on-entry cache for NAME is set for block BB.
// Work back through the CFG to DEF_BB ensuring the range is calculated
// on the block/edges leading back to that point.

void
ranger_cache::fill_block_cache (tree name, basic_block bb, basic_block def_bb)
{
  edge_iterator ei;
  edge e;
  int_range_max block_result;
  int_range_max undefined;

  // At this point we shouldn't be looking at the def, entry or exit block.
  gcc_checking_assert (bb != def_bb && bb != ENTRY_BLOCK_PTR_FOR_FN (cfun) &&
		       bb != EXIT_BLOCK_PTR_FOR_FN (cfun));

  // If the block cache is set, then we've already visited this block.
  if (m_on_entry.bb_range_p (name, bb))
    return;

  // Visit each block back to the DEF.  Initialize each one to UNDEFINED.
  // m_visited at the end will contain all the blocks that we needed to set
  // the range_on_entry cache for.
  m_workback.truncate (0);
  m_workback.quick_push (bb);
  undefined.set_undefined ();
  m_on_entry.set_bb_range (name, bb, undefined);
  gcc_checking_assert (m_update->empty_p ());

  if (DEBUG_RANGE_CACHE)
    {
      fprintf (dump_file, "\n");
      print_generic_expr (dump_file, name, TDF_SLIM);
      fprintf (dump_file, " : ");
    }

  // If there are dominators, check if a dominators can supply the range.
  if (dom_info_available_p (CDI_DOMINATORS)
      && range_from_dom (block_result, name, bb))
    {
      m_on_entry.set_bb_range (name, bb, block_result);
      if (DEBUG_RANGE_CACHE)
	{
	  fprintf (dump_file, "Filled from dominator! :  ");
	  block_result.dump (dump_file);
	  fprintf (dump_file, "\n");
	}
      return;
    }

  while (m_workback.length () > 0)
    {
      basic_block node = m_workback.pop ();
      if (DEBUG_RANGE_CACHE)
	{
	  fprintf (dump_file, "BACK visiting block %d for ", node->index);
	  print_generic_expr (dump_file, name, TDF_SLIM);
	  fprintf (dump_file, "\n");
	}

      FOR_EACH_EDGE (e, ei, node->preds)
	{
	  basic_block pred = e->src;
	  int_range_max r;

	  if (DEBUG_RANGE_CACHE)
	    fprintf (dump_file, "  %d->%d ",e->src->index, e->dest->index);

	  // If the pred block is the def block add this BB to update list.
	  if (pred == def_bb)
	    {
	      m_update->add (node);
	      continue;
	    }

	  // If the pred is entry but NOT def, then it is used before
	  // defined, it'll get set to [] and no need to update it.
	  if (pred == ENTRY_BLOCK_PTR_FOR_FN (cfun))
	    {
	      if (DEBUG_RANGE_CACHE)
		fprintf (dump_file, "entry: bail.");
	      continue;
	    }

	  // Regardless of whether we have visited pred or not, if the
	  // pred has a non-null reference, revisit this block.
	  // Don't search the DOM tree.
	  if (m_non_null.non_null_deref_p (name, pred, false))
	    {
	      if (DEBUG_RANGE_CACHE)
		fprintf (dump_file, "nonnull: update ");
	      m_update->add (node);
	    }

	  // If the pred block already has a range, or if it can contribute
	  // something new. Ie, the edge generates a range of some sort.
	  if (m_on_entry.get_bb_range (r, name, pred))
	    {
	      if (DEBUG_RANGE_CACHE)
		{
		  fprintf (dump_file, "has cache, ");
		  r.dump (dump_file);
		  fprintf (dump_file, ", ");
		}
	      if (!r.undefined_p () || m_gori.has_edge_range_p (name, e))
		{
		  m_update->add (node);
		  if (DEBUG_RANGE_CACHE)
		    fprintf (dump_file, "update. ");
		}
	      continue;
	    }

	  if (DEBUG_RANGE_CACHE)
	    fprintf (dump_file, "pushing undefined pred block.\n");
	  // If the pred hasn't been visited (has no range), add it to
	  // the list.
	  gcc_checking_assert (!m_on_entry.bb_range_p (name, pred));
	  m_on_entry.set_bb_range (name, pred, undefined);
	  m_workback.quick_push (pred);
	}
    }

  if (DEBUG_RANGE_CACHE)
    fprintf (dump_file, "\n");

  // Now fill in the marked blocks with values.
  propagate_cache (name);
  if (DEBUG_RANGE_CACHE)
    fprintf (dump_file, "  Propagation update done.\n");
}


// Get the range of NAME from dominators of BB and return it in R.

bool
ranger_cache::range_from_dom (irange &r, tree name, basic_block start_bb)
{
  if (!dom_info_available_p (CDI_DOMINATORS))
    return false;

  // Search back to the definition block or entry block.
  basic_block def_bb = gimple_bb (SSA_NAME_DEF_STMT (name));
  if (def_bb == NULL)
    def_bb = ENTRY_BLOCK_PTR_FOR_FN (cfun);

  basic_block bb;
  basic_block prev_bb = start_bb;
  // Flag if we encounter a block with non-null set.
  bool non_null = false;

  // Range on entry to the DEF block should not be queried.
  gcc_checking_assert (start_bb != def_bb);
  m_workback.truncate (0);

  // Default value is global range.
  get_global_range (r, name);

  // Search until a value is found, pushing outgoing edges encountered.
  for (bb = get_immediate_dominator (CDI_DOMINATORS, start_bb);
       bb;
       prev_bb = bb, bb = get_immediate_dominator (CDI_DOMINATORS, bb))
    {
      if (!non_null)
	non_null |= m_non_null.non_null_deref_p (name, bb, false);

      // This block has an outgoing range.
      if (m_gori.has_edge_range_p (name, bb))
	{
	  // Only outgoing ranges to single_pred blocks are dominated by
	  // outgoing edge ranges, so only those need to be considered.
	  edge e = find_edge (bb, prev_bb);
	  if (e && single_pred_p (prev_bb))
	    m_workback.quick_push (prev_bb);
	}

      if (def_bb == bb)
	break;

      if (m_on_entry.get_bb_range (r, name, bb))
	break;
    }

  if (DEBUG_RANGE_CACHE)
    {
      fprintf (dump_file, "CACHE: BB %d DOM query, found ", start_bb->index);
      r.dump (dump_file);
      if (bb)
	fprintf (dump_file, " at BB%d\n", bb->index);
      else
	fprintf (dump_file, " at function top\n");
    }

  // Now process any outgoing edges that we seen along the way.
  while (m_workback.length () > 0)
    {
      int_range_max edge_range;
      prev_bb = m_workback.pop ();
      edge e = single_pred_edge (prev_bb);
      bb = e->src;

      if (m_gori.outgoing_edge_range_p (edge_range, e, name, *this))
	{
	  r.intersect (edge_range);
	  if (r.varying_p () && ((e->flags & (EDGE_EH | EDGE_ABNORMAL)) == 0))
	    {
	      if (m_non_null.non_null_deref_p (name, bb, false))
		{
		  gcc_checking_assert (POINTER_TYPE_P (TREE_TYPE (name)));
		  r.set_nonzero (TREE_TYPE (name));
		}
	    }
	  if (DEBUG_RANGE_CACHE)
	    {
	      fprintf (dump_file, "CACHE: Adjusted edge range for %d->%d : ",
		       bb->index, prev_bb->index);
	      r.dump (dump_file);
	      fprintf (dump_file, "\n");
	    }
	}
    }

  // Apply non-null if appropriate.
  if (non_null && r.varying_p ()
      && !has_abnormal_call_or_eh_pred_edge_p (start_bb))
    {
      gcc_checking_assert (POINTER_TYPE_P (TREE_TYPE (name)));
      r.set_nonzero (TREE_TYPE (name));
    }
  if (DEBUG_RANGE_CACHE)
    {
      fprintf (dump_file, "CACHE: Range for DOM returns : ");
      r.dump (dump_file);
      fprintf (dump_file, "\n");
    }
  return true;
}

// This routine will update NAME in block BB to the nonnull state.
// It will then update the on-entry cache for this block to be non-null
// if it isn't already.

void
ranger_cache::update_to_nonnull (basic_block bb, tree name)
{
  tree type = TREE_TYPE (name);
  if (gimple_range_ssa_p (name) && POINTER_TYPE_P (type))
    {
      m_non_null.set_nonnull (bb, name);
      // Update the on-entry cache for BB to be non-zero.  Note this can set
      // the on entry value in the DEF block, which can override the def.
      int_range_max r;
      exit_range (r, name, bb);
      if (r.varying_p ())
	{
	  r.set_nonzero (type);
	  m_on_entry.set_bb_range (name, bb, r);
	}
    }
}

// Adapted from infer_nonnull_range_by_dereference and check_loadstore
// to process nonnull ssa_name OP in S.  DATA contains the ranger_cache.

static bool
non_null_loadstore (gimple *s, tree op, tree, void *data)
{
  if (TREE_CODE (op) == MEM_REF || TREE_CODE (op) == TARGET_MEM_REF)
    {
      /* Some address spaces may legitimately dereference zero.  */
      addr_space_t as = TYPE_ADDR_SPACE (TREE_TYPE (op));
      if (!targetm.addr_space.zero_address_valid (as))
	{
	  tree ssa = TREE_OPERAND (op, 0);
	  basic_block bb = gimple_bb (s);
	  ((ranger_cache *)data)->update_to_nonnull (bb, ssa);
	}
    }
  return false;
}

// This routine is used during a block walk to move the state of non-null for
// any operands on stmt S to nonnull.

void
ranger_cache::block_apply_nonnull (gimple *s)
{
  if (!flag_delete_null_pointer_checks)
    return;
  if (is_a<gphi *> (s))
    return;
  if (gimple_code (s) == GIMPLE_ASM || gimple_clobber_p (s))
    return;
  if (is_a<gcall *> (s))
    {
      tree fntype = gimple_call_fntype (s);
      bitmap nonnullargs = get_nonnull_args (fntype);
      // Process any non-null arguments
      if (nonnullargs)
	{
	  basic_block bb = gimple_bb (s);
	  for (unsigned i = 0; i < gimple_call_num_args (s); i++)
	    {
	      if (bitmap_empty_p (nonnullargs) || bitmap_bit_p (nonnullargs, i))
		{
		  tree op = gimple_call_arg (s, i);
		  update_to_nonnull (bb, op);
		}
	    }
	  BITMAP_FREE (nonnullargs);
	}
      // Fallthru and walk load/store ops now.
    }
  walk_stmt_load_store_ops (s, (void *)this, non_null_loadstore,
			    non_null_loadstore);
}
