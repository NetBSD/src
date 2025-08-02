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

#include "defs.h"
#include "dwarf2/cooked-index.h"
#include "dwarf2/read.h"
#include "cp-support.h"
#include "c-lang.h"
#include "ada-lang.h"
#include "split-name.h"
#include <algorithm>
#include "safe-ctype.h"
#include "gdbsupport/selftest.h"

/* See cooked-index.h.  */

bool
language_requires_canonicalization (enum language lang)
{
  return (lang == language_ada
	  || lang == language_c
	  || lang == language_cplus);
}

/* See cooked-index.h.  */

int
cooked_index_entry::compare (const char *stra, const char *strb,
			     comparison_mode mode)
{
  auto munge = [] (char c) -> unsigned char
    {
      /* We want to sort '<' before any other printable character.
	 So, rewrite '<' to something just before ' '.  */
      if (c == '<')
	return '\x1f';
      return TOLOWER ((unsigned char) c);
    };

  while (*stra != '\0'
	 && *strb != '\0'
	 && (munge (*stra) == munge (*strb)))
    {
      ++stra;
      ++strb;
    }

  unsigned char c1 = munge (*stra);
  unsigned char c2 = munge (*strb);

  if (c1 == c2)
    return 0;

  /* When completing, if STRB ends earlier than STRA, consider them as
     equal.  When comparing, if STRB ends earlier and STRA ends with
     '<', consider them as equal.  */
  if (mode == COMPLETE || (mode == MATCH && c1 == munge ('<')))
    {
      if (c2 == '\0')
	return 0;
    }

  return c1 < c2 ? -1 : 1;
}

#if GDB_SELF_TEST

namespace {

void
test_compare ()
{
  /* Convenience aliases.  */
  const auto mode_compare = cooked_index_entry::MATCH;
  const auto mode_sort = cooked_index_entry::SORT;
  const auto mode_complete = cooked_index_entry::COMPLETE;

  SELF_CHECK (cooked_index_entry::compare ("abcd", "abcd",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "abcd",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("abcd", "ABCDE",
					   mode_compare) < 0);
  SELF_CHECK (cooked_index_entry::compare ("ABCDE", "abcd",
					   mode_compare) > 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "ABCDE",
					   mode_complete) < 0);
  SELF_CHECK (cooked_index_entry::compare ("ABCDE", "abcd",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("name", "name<>",
					   mode_compare) < 0);
  SELF_CHECK (cooked_index_entry::compare ("name<>", "name",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name", "name<>",
					   mode_complete) < 0);
  SELF_CHECK (cooked_index_entry::compare ("name<>", "name",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<arg>",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<ag>",
					   mode_compare) > 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<arg>",
					   mode_complete) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<ag>",
					   mode_complete) > 0);

  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>",
					   "name<arg<more>>",
					   mode_compare) == 0);

  SELF_CHECK (cooked_index_entry::compare ("name", "name<arg<more>>",
					   mode_compare) < 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>", "name",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>", "name<arg<",
					   mode_compare) > 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>", "name<arg<",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("", "abcd", mode_compare) < 0);
  SELF_CHECK (cooked_index_entry::compare ("", "abcd", mode_complete) < 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "", mode_compare) > 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "", mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("func", "func<type>",
					   mode_sort) < 0);
  SELF_CHECK (cooked_index_entry::compare ("func<type>", "func1",
					   mode_sort) < 0);
}

} /* anonymous namespace */

#endif /* GDB_SELF_TEST */

/* See cooked-index.h.  */

const char *
cooked_index_entry::full_name (struct obstack *storage, bool for_main) const
{
  const char *local_name = for_main ? name : canonical;

  if ((flags & IS_LINKAGE) != 0 || parent_entry == nullptr)
    return local_name;

  const char *sep = nullptr;
  switch (per_cu->lang ())
    {
    case language_cplus:
    case language_rust:
      sep = "::";
      break;

    case language_go:
    case language_d:
    case language_ada:
      sep = ".";
      break;

    default:
      return local_name;
    }

  parent_entry->write_scope (storage, sep, for_main);
  obstack_grow0 (storage, local_name, strlen (local_name));
  return (const char *) obstack_finish (storage);
}

/* See cooked-index.h.  */

void
cooked_index_entry::write_scope (struct obstack *storage,
				 const char *sep,
				 bool for_main) const
{
  if (parent_entry != nullptr)
    parent_entry->write_scope (storage, sep, for_main);
  const char *local_name = for_main ? name : canonical;
  obstack_grow (storage, local_name, strlen (local_name));
  obstack_grow (storage, sep, strlen (sep));
}

/* See cooked-index.h.  */

const cooked_index_entry *
cooked_index::add (sect_offset die_offset, enum dwarf_tag tag,
		   cooked_index_flag flags, const char *name,
		   const cooked_index_entry *parent_entry,
		   dwarf2_per_cu_data *per_cu)
{
  cooked_index_entry *result = create (die_offset, tag, flags, name,
				       parent_entry, per_cu);
  m_entries.push_back (result);

  /* An explicitly-tagged main program should always override the
     implicit "main" discovery.  */
  if ((flags & IS_MAIN) != 0)
    m_main = result;

  return result;
}

/* See cooked-index.h.  */

void
cooked_index::finalize ()
{
  m_future = gdb::thread_pool::g_thread_pool->post_task ([this] ()
    {
      do_finalize ();
    });
}

/* See cooked-index.h.  */

gdb::unique_xmalloc_ptr<char>
cooked_index::handle_gnat_encoded_entry (cooked_index_entry *entry,
					 htab_t gnat_entries)
{
  std::string canonical = ada_decode (entry->name, false, false);
  if (canonical.empty ())
    return {};
  std::vector<gdb::string_view> names = split_name (canonical.c_str (),
						    split_style::DOT);
  gdb::string_view tail = names.back ();
  names.pop_back ();

  const cooked_index_entry *parent = nullptr;
  for (const auto &name : names)
    {
      uint32_t hashval = dwarf5_djb_hash (name);
      void **slot = htab_find_slot_with_hash (gnat_entries, &name,
					      hashval, INSERT);
      /* CUs are processed in order, so we only need to check the most
	 recent entry.  */
      cooked_index_entry *last = (cooked_index_entry *) *slot;
      if (last == nullptr || last->per_cu != entry->per_cu)
	{
	  gdb::unique_xmalloc_ptr<char> new_name
	    = make_unique_xstrndup (name.data (), name.length ());
	  last = create (entry->die_offset, DW_TAG_namespace,
			 0, new_name.get (), parent,
			 entry->per_cu);
	  last->canonical = last->name;
	  m_names.push_back (std::move (new_name));
	  *slot = last;
	}

      parent = last;
    }

  entry->parent_entry = parent;
  return make_unique_xstrndup (tail.data (), tail.length ());
}

/* See cooked-index.h.  */

void
cooked_index::do_finalize ()
{
  auto hash_name_ptr = [] (const void *p)
    {
      const cooked_index_entry *entry = (const cooked_index_entry *) p;
      return htab_hash_pointer (entry->name);
    };

  auto eq_name_ptr = [] (const void *a, const void *b) -> int
    {
      const cooked_index_entry *ea = (const cooked_index_entry *) a;
      const cooked_index_entry *eb = (const cooked_index_entry *) b;
      return ea->name == eb->name;
    };

  /* We can use pointer equality here because names come from
     .debug_str, which will normally be unique-ified by the linker.
     Also, duplicates are relatively harmless -- they just mean a bit
     of extra memory is used.  */
  htab_up seen_names (htab_create_alloc (10, hash_name_ptr, eq_name_ptr,
					 nullptr, xcalloc, xfree));

  auto hash_entry = [] (const void *e)
    {
      const cooked_index_entry *entry = (const cooked_index_entry *) e;
      return dwarf5_djb_hash (entry->canonical);
    };

  auto eq_entry = [] (const void *a, const void *b) -> int
    {
      const cooked_index_entry *ae = (const cooked_index_entry *) a;
      const gdb::string_view *sv = (const gdb::string_view *) b;
      return (strlen (ae->canonical) == sv->length ()
	      && strncasecmp (ae->canonical, sv->data (), sv->length ()) == 0);
    };

  htab_up gnat_entries (htab_create_alloc (10, hash_entry, eq_entry,
					   nullptr, xcalloc, xfree));

  for (cooked_index_entry *entry : m_entries)
    {
      /* Note that this code must be kept in sync with
	 language_requires_canonicalization.  */
      gdb_assert (entry->canonical == nullptr);
      if ((entry->flags & IS_LINKAGE) != 0)
	entry->canonical = entry->name;
      else if (entry->per_cu->lang () == language_ada)
	{
	  gdb::unique_xmalloc_ptr<char> canon_name
	    = handle_gnat_encoded_entry (entry, gnat_entries.get ());
	  if (canon_name == nullptr)
	    entry->canonical = entry->name;
	  else
	    {
	      entry->canonical = canon_name.get ();
	      m_names.push_back (std::move (canon_name));
	    }
	}
      else if (entry->per_cu->lang () == language_cplus
	       || entry->per_cu->lang () == language_c)
	{
	  void **slot = htab_find_slot (seen_names.get (), entry,
					INSERT);
	  if (*slot == nullptr)
	    {
	      gdb::unique_xmalloc_ptr<char> canon_name
		= (entry->per_cu->lang () == language_cplus
		   ? cp_canonicalize_string (entry->name)
		   : c_canonicalize_name (entry->name));
	      if (canon_name == nullptr)
		entry->canonical = entry->name;
	      else
		{
		  entry->canonical = canon_name.get ();
		  m_names.push_back (std::move (canon_name));
		}
	    }
	  else
	    {
	      const cooked_index_entry *other
		= (const cooked_index_entry *) *slot;
	      entry->canonical = other->canonical;
	    }
	}
      else
	entry->canonical = entry->name;
    }

  m_names.shrink_to_fit ();
  m_entries.shrink_to_fit ();
  std::sort (m_entries.begin (), m_entries.end (),
	     [] (const cooked_index_entry *a, const cooked_index_entry *b)
	     {
	       return *a < *b;
	     });
}

/* See cooked-index.h.  */

cooked_index::range
cooked_index::find (const std::string &name, bool completing)
{
  wait ();

  cooked_index_entry::comparison_mode mode = (completing
					      ? cooked_index_entry::COMPLETE
					      : cooked_index_entry::MATCH);

  auto lower = std::lower_bound (m_entries.begin (), m_entries.end (), name,
				 [=] (const cooked_index_entry *entry,
				      const std::string &n)
  {
    return cooked_index_entry::compare (entry->canonical, n.c_str (), mode) < 0;
  });

  auto upper = std::upper_bound (m_entries.begin (), m_entries.end (), name,
				 [=] (const std::string &n,
				      const cooked_index_entry *entry)
  {
    return cooked_index_entry::compare (entry->canonical, n.c_str (), mode) > 0;
  });

  return range (lower, upper);
}

cooked_index_vector::cooked_index_vector (vec_type &&vec)
  : m_vector (std::move (vec))
{
  for (auto &idx : m_vector)
    idx->finalize ();
}

/* See cooked-index.h.  */

dwarf2_per_cu_data *
cooked_index_vector::lookup (CORE_ADDR addr)
{
  for (const auto &index : m_vector)
    {
      dwarf2_per_cu_data *result = index->lookup (addr);
      if (result != nullptr)
	return result;
    }
  return nullptr;
}

/* See cooked-index.h.  */

std::vector<addrmap *>
cooked_index_vector::get_addrmaps ()
{
  std::vector<addrmap *> result;
  for (const auto &index : m_vector)
    result.push_back (index->m_addrmap);
  return result;
}

/* See cooked-index.h.  */

cooked_index_vector::range
cooked_index_vector::find (const std::string &name, bool completing)
{
  std::vector<cooked_index::range> result_range;
  result_range.reserve (m_vector.size ());
  for (auto &entry : m_vector)
    result_range.push_back (entry->find (name, completing));
  return range (std::move (result_range));
}

/* See cooked-index.h.  */

const cooked_index_entry *
cooked_index_vector::get_main () const
{
  const cooked_index_entry *result = nullptr;

  for (const auto &index : m_vector)
    {
      const cooked_index_entry *entry = index->get_main ();
      /* Choose the first "main" we see.  The choice among several is
	 arbitrary.  See the comment by the sole caller to understand
	 the rationale for filtering by language.  */
      if (entry != nullptr
	  && !language_requires_canonicalization (entry->per_cu->lang ()))
	{
	  result = entry;
	  break;
	}
    }

  return result;
}

void _initialize_cooked_index ();
void
_initialize_cooked_index ()
{
#if GDB_SELF_TEST
  selftests::register_test ("cooked_index_entry::compare", test_compare);
#endif
}
