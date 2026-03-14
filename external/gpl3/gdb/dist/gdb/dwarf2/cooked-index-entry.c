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

#include "dwarf2/cooked-index-entry.h"
#include "dwarf2/tag.h"
#include "gdbsupport/gdb-safe-ctype.h"
#include "gdbsupport/selftest.h"

/* See cooked-index-entry.h.  */

std::string
to_string (cooked_index_flag flags)
{
  static constexpr cooked_index_flag::string_mapping mapping[] = {
    MAP_ENUM_FLAG (IS_MAIN),
    MAP_ENUM_FLAG (IS_STATIC),
    MAP_ENUM_FLAG (IS_LINKAGE),
    MAP_ENUM_FLAG (IS_TYPE_DECLARATION),
    MAP_ENUM_FLAG (IS_PARENT_DEFERRED),
    MAP_ENUM_FLAG (IS_SYNTHESIZED),
  };

  return flags.to_string (mapping);
}

/* See cooked-index-entry.h.  */

int
cooked_index_entry::compare (const char *stra, const char *strb,
			     comparison_mode mode)
{
#if defined (__GNUC__) && !defined (__clang__) && __GNUC__ <= 7
  /* Work around error with gcc 7.5.0.  */
  auto munge = [] (char c) -> unsigned char
#else
  auto munge = [] (char c) constexpr -> unsigned char
#endif
    {
      /* Treat '<' as if it ended the string.  This lets something
	 like "func<t>" match "func<t<int>>".  See the "Breakpoints in
	 template functions" section in the manual.  */
      if (c == '<')
	return '\0';
      return TOLOWER ((unsigned char) c);
    };

  unsigned char a = munge (*stra);
  unsigned char b = munge (*strb);

  while (a != '\0' && b != '\0' && a == b)
    {
      a = munge (*++stra);
      b = munge (*++strb);
    }

  if (a == b)
    return 0;

  /* When completing, if STRB ends earlier than STRA, consider them as
     equal.  */
  if (mode == COMPLETE && b == '\0')
    return 0;

  return a < b ? -1 : 1;
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
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<>", "name",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name", "name<>",
					   mode_complete) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<>", "name",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<arg>",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<ag>",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<arg>",
					   mode_complete) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>", "name<ag>",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>",
					   "name<arg<more>>",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg>",
					   "name<arg<more>>",
					   mode_compare) == 0);

  SELF_CHECK (cooked_index_entry::compare ("name", "name<arg<more>>",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>", "name",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>", "name<arg<",
					   mode_compare) == 0);
  SELF_CHECK (cooked_index_entry::compare ("name<arg<more>>", "name<arg<",
					   mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("", "abcd", mode_compare) < 0);
  SELF_CHECK (cooked_index_entry::compare ("", "abcd", mode_complete) < 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "", mode_compare) > 0);
  SELF_CHECK (cooked_index_entry::compare ("abcd", "", mode_complete) == 0);

  SELF_CHECK (cooked_index_entry::compare ("func", "func<type>",
					   mode_sort) == 0);
  SELF_CHECK (cooked_index_entry::compare ("func<type>", "func1",
					   mode_sort) < 0);
}

} /* anonymous namespace */

#endif /* GDB_SELF_TEST */

/* See cooked-index-entry.h.  */

bool
cooked_index_entry::matches (domain_search_flags kind) const
{
  /* Just reject type declarations.  */
  if ((flags & IS_TYPE_DECLARATION) != 0)
    return false;

  return tag_matches_domain (tag, kind, lang);
}

/* See cooked-index-entry.h.  */

const char *
cooked_index_entry::full_name (struct obstack *storage,
			       cooked_index_full_name_flag name_flags,
			       const char *default_sep) const
{
  const char *local_name = ((name_flags & FOR_MAIN) != 0) ? name : canonical;

  if ((flags & IS_LINKAGE) != 0 || get_parent () == nullptr)
    return local_name;

  const char *sep = default_sep;
  switch (lang)
    {
    case language_cplus:
    case language_rust:
    case language_fortran:
      sep = "::";
      break;

    case language_ada:
      if ((name_flags & FOR_ADA_LINKAGE_NAME) != 0)
	{
	  sep = "__";
	  break;
	}
      [[fallthrough]];
    case language_go:
    case language_d:
      sep = ".";
      break;

    default:
      if (sep == nullptr)
	return local_name;
      break;
    }

  /* The FOR_ADA_LINKAGE_NAME flag should only affect Ada entries, so
     disable it here if we don't need it.  */
  if (lang != language_ada)
    name_flags &= ~FOR_ADA_LINKAGE_NAME;

  get_parent ()->write_scope (storage, sep, name_flags);
  obstack_grow0 (storage, local_name, strlen (local_name));
  return (const char *) obstack_finish (storage);
}

/* See cooked-index-entry.h.  */

void
cooked_index_entry::write_scope (struct obstack *storage,
				 const char *sep,
				 cooked_index_full_name_flag flags) const
{
  if (get_parent () != nullptr)
    get_parent ()->write_scope (storage, sep, flags);
  /* When computing the Ada linkage name, the entry might not have
     been canonicalized yet, so use the 'name'.  */
  const char *local_name = ((flags & (FOR_MAIN | FOR_ADA_LINKAGE_NAME)) != 0
			    ? name
			    : canonical);
  obstack_grow (storage, local_name, strlen (local_name));
  obstack_grow (storage, sep, strlen (sep));
}

INIT_GDB_FILE (dwarf2_entry)
{
#if GDB_SELF_TEST
  selftests::register_test ("cooked_index_entry::compare", test_compare);
#endif
}
