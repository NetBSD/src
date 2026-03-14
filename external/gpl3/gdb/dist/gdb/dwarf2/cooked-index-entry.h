/* Entry in the cooked index

   Copyright (C) 2022-2025 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_COOKED_INDEX_ENTRY_H
#define GDB_DWARF2_COOKED_INDEX_ENTRY_H

#include "dwarf2/parent-map.h"
#include "dwarf2/types.h"
#include "symtab.h"
#include "gdbsupport/gdb_obstack.h"
#include "quick-symbol.h"

/* Flags that describe an entry in the index.  */
enum cooked_index_flag_enum : unsigned char
{
  /* True if this entry is the program's "main".  */
  IS_MAIN = 1,
  /* True if this entry represents a "static" object.  */
  IS_STATIC = 2,
  /* True if this entry uses the linkage name.  */
  IS_LINKAGE = 4,
  /* True if this entry is just for the declaration of a type, not the
     definition.  */
  IS_TYPE_DECLARATION = 8,
  /* True is parent_entry.deferred has a value rather than parent_entry
     .resolved.  */
  IS_PARENT_DEFERRED = 16,
  /* True if this entry was synthesized by gdb (as opposed to coming
     directly from the DWARF).  */
  IS_SYNTHESIZED = 32,
};
DEF_ENUM_FLAGS_TYPE (enum cooked_index_flag_enum, cooked_index_flag);

/* Flags used when requesting the full name of an entry.  */
enum cooked_index_full_name_enum : unsigned char
{
  /* Set when requesting the name of "main".  See the method for the
     full description.  */
  FOR_MAIN = 1,
  /* Set when requesting the linkage name for an Ada entry.  */
  FOR_ADA_LINKAGE_NAME = 2,
};
DEF_ENUM_FLAGS_TYPE (enum cooked_index_full_name_enum, cooked_index_full_name_flag);

/* Type representing either a resolved or deferred cooked_index_entry.  */

union cooked_index_entry_ref
{
  cooked_index_entry_ref (parent_map::addr_type deferred_)
  {
    deferred = deferred_;
  }

  cooked_index_entry_ref (const cooked_index_entry *resolved_)
  {
    resolved = resolved_;
  }

  const cooked_index_entry *resolved;
  parent_map::addr_type deferred;
};

/* Return a string representation of FLAGS.  */

std::string to_string (cooked_index_flag flags);

/* A cooked_index_entry represents a single item in the index.  Note
   that two entries can be created for the same DIE -- one using the
   name, and another one using the linkage name, if any.

   This is an "open" class and the members are all directly
   accessible.  It is read-only after the index has been fully read
   and processed.  */
struct cooked_index_entry : public allocate_on_obstack<cooked_index_entry>
{
  cooked_index_entry (sect_offset die_offset_, enum dwarf_tag tag_,
		      cooked_index_flag flags_,
		      enum language lang_, const char *name_,
		      cooked_index_entry_ref parent_entry_,
		      dwarf2_per_cu *per_cu_)
    : name (name_),
      tag (tag_),
      flags (flags_),
      lang (lang_),
      die_offset (die_offset_),
      per_cu (per_cu_),
      m_parent_entry (parent_entry_)
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

  /* Return true if this entry matches KIND.  */
  bool matches (domain_search_flags kind) const;

  /* Construct the fully-qualified name of this entry and return a
     pointer to it.  If allocation is needed, it will be done on
     STORAGE.

     FLAGS affects the result.  If the FOR_MAIN flag is set, we are
     computing the name of the "main" entry -- one marked
     DW_AT_main_subprogram.  This matters for avoiding name
     canonicalization and also a related race (if "main" computation
     is done during finalization).

     If the FOR_ADA_LINKAGE_NAME flag is set, then Ada-language
     symbols will have their "linkage-style" name computed.  The
     default is source-style.

     If the language doesn't prescribe a separator, one can be
     specified using DEFAULT_SEP.  */
  const char *full_name (struct obstack *storage,
			 cooked_index_full_name_flag name_flags = 0,
			 const char *default_sep = nullptr) const;

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

  /* Set parent entry to PARENT.  */
  void set_parent (const cooked_index_entry *parent)
  {
    gdb_assert ((flags & IS_PARENT_DEFERRED) == 0);
    m_parent_entry.resolved = parent;
  }

  /* Resolve deferred parent entry to PARENT.  */
  void resolve_parent (const cooked_index_entry *parent)
  {
    gdb_assert ((flags & IS_PARENT_DEFERRED) != 0);
    flags = flags & ~IS_PARENT_DEFERRED;
    m_parent_entry.resolved = parent;
  }

  /* Return parent entry.  */
  const cooked_index_entry *get_parent () const
  {
    gdb_assert ((flags & IS_PARENT_DEFERRED) == 0);
    return m_parent_entry.resolved;
  }

  /* Return deferred parent entry.  */
  parent_map::addr_type get_deferred_parent () const
  {
    gdb_assert ((flags & IS_PARENT_DEFERRED) != 0);
    return m_parent_entry.deferred;
  }

  /* The name as it appears in DWARF.  This always points into one of
     the mapped DWARF sections.  Note that this may be the name or the
     linkage name -- two entries are created for DIEs which have both
     attributes.  */
  const char *name;
  /* The canonical name.  This may be equal to NAME.  */
  const char *canonical = nullptr;
  /* The DWARF tag.  */
  enum dwarf_tag tag;
  /* Any flags attached to this entry.  */
  cooked_index_flag flags;
  /* The language of this symbol.  */
  ENUM_BITFIELD (language) lang : LANGUAGE_BITS;
  /* The offset of this DIE.  */
  sect_offset die_offset;
  /* The CU from which this entry originates.  */
  dwarf2_per_cu *per_cu;

private:

  /* A helper method for full_name.  Emits the full scope of this
     object, followed by the separator, to STORAGE.  If this entry has
     a parent, its write_scope method is called first.  See full_name
     for a description of the FLAGS parameter.  */
  void write_scope (struct obstack *storage, const char *sep,
		    cooked_index_full_name_flag flags) const;

  /* The parent entry.  This is NULL for top-level entries.
     Otherwise, it points to the parent entry, such as a namespace or
     class.  */
  cooked_index_entry_ref m_parent_entry;
};

#endif /* GDB_DWARF2_COOKED_INDEX_ENTRY_H */
