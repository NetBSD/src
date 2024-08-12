/* DIE indexing 

   Copyright (C) 2022-2023 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef GDB_DWARF2_COOKED_INDEX_H
#define GDB_DWARF2_COOKED_INDEX_H

#include "dwarf2.h"
#include "gdbtypes.h"
#include "symtab.h"
#include "hashtab.h"
#include "dwarf2/index-common.h"
#include "gdbsupport/gdb_string_view.h"
#include "quick-symbol.h"
#include "gdbsupport/gdb_obstack.h"
#include "addrmap.h"
#include "gdbsupport/iterator-range.h"
#include "gdbsupport/thread-pool.h"
#include "dwarf2/mapped-index.h"
#include "dwarf2/tag.h"
#include "gdbsupport/range-chain.h"

struct dwarf2_per_cu_data;

/* Flags that describe an entry in the index.  */
enum cooked_index_flag_enum : unsigned char
{
  /* True if this entry is the program's "main".  */
  IS_MAIN = 1,
  /* True if this entry represents a "static" object.  */
  IS_STATIC = 2,
  /* True if this entry is an "enum class".  */
  IS_ENUM_CLASS = 4,
  /* True if this entry uses the linkage name.  */
  IS_LINKAGE = 8,
  /* True if this entry is just for the declaration of a type, not the
     definition.  */
  IS_TYPE_DECLARATION = 16,
};
DEF_ENUM_FLAGS_TYPE (enum cooked_index_flag_enum, cooked_index_flag);

/* Return true if LANG requires canonicalization.  This is used
   primarily to work around an issue computing the name of "main".
   This function must be kept in sync with
   cooked_index_shard::do_finalize.  */

extern bool language_requires_canonicalization (enum language lang);

/* A cooked_index_entry represents a single item in the index.  Note
   that two entries can be created for the same DIE -- one using the
   name, and another one using the linkage name, if any.

   This is an "open" class and the members are all directly
   accessible.  It is read-only after the index has been fully read
   and processed.  */
struct cooked_index_entry : public allocate_on_obstack
{
  cooked_index_entry (sect_offset die_offset_, enum dwarf_tag tag_,
		      cooked_index_flag flags_, const char *name_,
		      const cooked_index_entry *parent_entry_,
		      dwarf2_per_cu_data *per_cu_)
    : name (name_),
      tag (tag_),
      flags (flags_),
      die_offset (die_offset_),
      parent_entry (parent_entry_),
      per_cu (per_cu_)
  {
  }

  /* Return true if this entry matches SEARCH_FLAGS.  */
  bool matches (block_search_flags search_flags) const
  {
    /* Just reject type declarations.  */
    if ((flags & IS_TYPE_DECLARATION) != 0)
      return false;

    if ((search_flags & SEARCH_STATIC_BLOCK) != 0
	&& (flags & IS_STATIC) != 0)
      return true;
    if ((search_flags & SEARCH_GLOBAL_BLOCK) != 0
	&& (flags & IS_STATIC) == 0)
      return true;
    return false;
  }

  /* Return true if this entry matches DOMAIN.  */
  bool matches (domain_enum domain) const
  {
    /* Just reject type declarations.  */
    if ((flags & IS_TYPE_DECLARATION) != 0)
      return false;

    switch (domain)
      {
      case LABEL_DOMAIN:
	return false;

      case MODULE_DOMAIN:
	return tag == DW_TAG_module;

      case COMMON_BLOCK_DOMAIN:
	return tag == DW_TAG_common_block;
      }

    return true;
  }

  /* Return true if this entry matches KIND.  */
  bool matches (enum search_domain kind) const
  {
    /* Just reject type declarations.  */
    if ((flags & IS_TYPE_DECLARATION) != 0)
      return false;

    switch (kind)
      {
      case VARIABLES_DOMAIN:
	return (tag == DW_TAG_variable
		|| tag == DW_TAG_constant
		|| tag == DW_TAG_enumerator);
      case FUNCTIONS_DOMAIN:
	return tag == DW_TAG_subprogram;
      case TYPES_DOMAIN:
	return tag_is_type (tag);
      case MODULES_DOMAIN:
	return tag == DW_TAG_module;
      }

    return true;
  }

  /* Construct the fully-qualified name of this entry and return a
     pointer to it.  If allocation is needed, it will be done on
     STORAGE.  FOR_MAIN is true if we are computing the name of the
     "main" entry -- one marked DW_AT_main_subprogram.  This matters
     for avoiding name canonicalization and also a related race (if
     "main" computation is done during finalization).  */
  const char *full_name (struct obstack *storage, bool for_main = false) const;

  /* Comparison modes for the 'compare' function.  See the function
     for a description.  */
  enum comparison_mode
  {
    MATCH,
    SORT,
    COMPLETE,
  };

  /* Compare two strings, case-insensitively.  Return -1 if STRA is
     less than STRB, 0 if they are equal, and 1 if STRA is greater.

     When comparing, '<' is considered to be less than all other
     printable characters.  This ensures that "t<x>" sorts before
     "t1", which is necessary when looking up "t".  This '<' handling
     is to ensure that certain C++ lookups work correctly.  It is
     inexact, and applied regardless of the search language, but this
     is ok because callers of this code do more precise filtering
     according to their needs.  This is also why using a
     case-insensitive comparison works even for languages that are
     case sensitive.

     MODE controls how the comparison proceeds.

     MODE==SORT is used when sorting and the only special '<' handling
     that it does is to ensure that '<' sorts before all other
     printable characters.  This ensures that the resulting ordering
     will be binary-searchable.

     MODE==MATCH is used when searching for a symbol.  In this case,
     STRB must always be the search name, and STRA must be the name in
     the index that is under consideration.  In compare mode, early
     termination of STRB may match STRA -- for example, "t<int>" and
     "t" will be considered to be equal.  (However, if A=="t" and
     B=="t<int>", then this will not consider them as equal.)

     MODE==COMPLETE is used when searching for a symbol for
     completion.  In this case, STRB must always be the search name,
     and STRA must be the name in the index that is under
     consideration.  In completion mode, early termination of STRB
     always results in a match.  */
  static int compare (const char *stra, const char *strb,
		      comparison_mode mode);

  /* Compare two entries by canonical name.  */
  bool operator< (const cooked_index_entry &other) const
  {
    return compare (canonical, other.canonical, SORT) < 0;
  }

  /* The name as it appears in DWARF.  This always points into one of
     the mapped DWARF sections.  Note that this may be the name or the
     linkage name -- two entries are created for DIEs which have both
     attributes.  */
  const char *name;
  /* The canonical name.  For C++ names, this may differ from NAME.
     In all other cases, this is equal to NAME.  */
  const char *canonical = nullptr;
  /* The DWARF tag.  */
  enum dwarf_tag tag;
  /* Any flags attached to this entry.  */
  cooked_index_flag flags;
  /* The offset of this DIE.  */
  sect_offset die_offset;
  /* The parent entry.  This is NULL for top-level entries.
     Otherwise, it points to the parent entry, such as a namespace or
     class.  */
  const cooked_index_entry *parent_entry;
  /* The CU from which this entry originates.  */
  dwarf2_per_cu_data *per_cu;

private:

  /* A helper method for full_name.  Emits the full scope of this
     object, followed by the separator, to STORAGE.  If this entry has
     a parent, its write_scope method is called first.  */
  void write_scope (struct obstack *storage, const char *sep,
		    bool for_name) const;
};

class cooked_index_vector;

/* An index of interesting DIEs.  This is "cooked", in contrast to a
   mapped .debug_names or .gdb_index, which are "raw".  An entry in
   the index is of type cooked_index_entry.

   Operations on the index are described below.  They are chosen to
   make it relatively simple to implement the symtab "quick"
   methods.  */
class cooked_index
{
public:
  cooked_index () = default;
  DISABLE_COPY_AND_ASSIGN (cooked_index);

  /* Create a new cooked_index_entry and register it with this object.
     Entries are owned by this object.  The new item is returned.  */
  const cooked_index_entry *add (sect_offset die_offset, enum dwarf_tag tag,
				 cooked_index_flag flags,
				 const char *name,
				 const cooked_index_entry *parent_entry,
				 dwarf2_per_cu_data *per_cu);

  /* Install a new fixed addrmap from the given mutable addrmap.  */
  void install_addrmap (addrmap_mutable *map)
  {
    gdb_assert (m_addrmap == nullptr);
    m_addrmap = new (&m_storage) addrmap_fixed (&m_storage, map);
  }

  /* Finalize the index.  This should be called a single time, when
     the index has been fully populated.  It enters all the entries
     into the internal table.  */
  void finalize ();

  /* Wait for this index's finalization to be complete.  */
  void wait ()
  {
    m_future.wait ();
  }

  friend class cooked_index_vector;

  /* A simple range over part of m_entries.  */
  typedef iterator_range<std::vector<cooked_index_entry *>::iterator> range;

  /* Return a range of all the entries.  */
  range all_entries ()
  {
    wait ();
    return { m_entries.begin (), m_entries.end () };
  }

  /* Look up an entry by name.  Returns a range of all matching
     results.  If COMPLETING is true, then a larger range, suitable
     for completion, will be returned.  */
  range find (const std::string &name, bool completing);

private:

  /* Return the entry that is believed to represent the program's
     "main".  This will return NULL if no such entry is available.  */
  const cooked_index_entry *get_main () const
  {
    return m_main;
  }

  /* Look up ADDR in the address map, and return either the
     corresponding CU, or nullptr if the address could not be
     found.  */
  dwarf2_per_cu_data *lookup (CORE_ADDR addr)
  {
    return (dwarf2_per_cu_data *) m_addrmap->find (addr);
  }

  /* Create a new cooked_index_entry and register it with this object.
     Entries are owned by this object.  The new item is returned.  */
  cooked_index_entry *create (sect_offset die_offset,
			      enum dwarf_tag tag,
			      cooked_index_flag flags,
			      const char *name,
			      const cooked_index_entry *parent_entry,
			      dwarf2_per_cu_data *per_cu)
  {
    return new (&m_storage) cooked_index_entry (die_offset, tag, flags,
						name, parent_entry,
						per_cu);
  }

  /* GNAT only emits mangled ("encoded") names in the DWARF, and does
     not emit the module structure.  However, we need this structure
     to do lookups.  This function recreates that structure for an
     existing entry.  It returns the base name (last element) of the
     full decoded name.  */
  gdb::unique_xmalloc_ptr<char> handle_gnat_encoded_entry
       (cooked_index_entry *entry, htab_t gnat_entries);

  /* A helper method that does the work of 'finalize'.  */
  void do_finalize ();

  /* Storage for the entries.  */
  auto_obstack m_storage;
  /* List of all entries.  */
  std::vector<cooked_index_entry *> m_entries;
  /* If we found an entry with 'is_main' set, store it here.  */
  cooked_index_entry *m_main = nullptr;
  /* The addrmap.  This maps address ranges to dwarf2_per_cu_data
     objects.  */
  addrmap *m_addrmap = nullptr;
  /* Storage for canonical names.  */
  std::vector<gdb::unique_xmalloc_ptr<char>> m_names;
  /* A future that tracks when the 'finalize' method is done.  Note
     that the 'get' method is never called on this future, only
     'wait'.  */
  gdb::future<void> m_future;
};

/* The main index of DIEs.  The parallel DIE indexers create
   cooked_index objects.  Then, these are all handled to a
   cooked_index_vector for storage and final indexing.  The index is
   made by iterating over the entries previously created.  */

class cooked_index_vector : public dwarf_scanner_base
{
public:

  /* A convenience typedef for the vector that is contained in this
     object.  */
  typedef std::vector<std::unique_ptr<cooked_index>> vec_type;

  explicit cooked_index_vector (vec_type &&vec);
  DISABLE_COPY_AND_ASSIGN (cooked_index_vector);

  /* Wait until the finalization of the entire cooked_index_vector is
     done.  */
  void wait ()
  {
    for (auto &item : m_vector)
      item->wait ();
  }

  ~cooked_index_vector ()
  {
    /* The 'finalize' methods may be run in a different thread.  If
       this object is destroyed before these complete, then one will
       end up writing to freed memory.  Waiting for finalization to
       complete avoids this problem; and the cost seems ignorable
       because creating and immediately destroying the debug info is a
       relatively rare thing to do.  */
    wait ();
  }

  /* A range over a vector of subranges.  */
  typedef range_chain<cooked_index::range> range;

  /* Look up an entry by name.  Returns a range of all matching
     results.  If COMPLETING is true, then a larger range, suitable
     for completion, will be returned.  */
  range find (const std::string &name, bool completing);

  /* Return a range of all the entries.  */
  range all_entries ()
  {
    std::vector<cooked_index::range> result_range;
    result_range.reserve (m_vector.size ());
    for (auto &entry : m_vector)
      result_range.push_back (entry->all_entries ());
    return range (std::move (result_range));
  }

  /* Look up ADDR in the address map, and return either the
     corresponding CU, or nullptr if the address could not be
     found.  */
  dwarf2_per_cu_data *lookup (CORE_ADDR addr);

  /* Return a new vector of all the addrmaps used by all the indexes
     held by this object.  */
  std::vector<addrmap *> get_addrmaps ();

  /* Return the entry that is believed to represent the program's
     "main".  This will return NULL if no such entry is available.  */
  const cooked_index_entry *get_main () const;

  cooked_index_vector *index_for_writing () override
  {
    return this;
  }

  quick_symbol_functions_up make_quick_functions () const override;

private:

  /* The vector of cooked_index objects.  This is stored because the
     entries are stored on the obstacks in those objects.  */
  vec_type m_vector;
};

#endif /* GDB_DWARF2_COOKED_INDEX_H */
