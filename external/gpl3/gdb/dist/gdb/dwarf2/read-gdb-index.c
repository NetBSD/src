/* Reading code for .gdb_index

   Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

#include "read-gdb-index.h"

#include "cli/cli-cmds.h"
#include "cli/cli-style.h"
#include "complaints.h"
#include "dwarf2/index-common.h"
#include "dwz.h"
#include "event-top.h"
#include "gdb/gdb-index.h"
#include "gdbsupport/gdb-checked-static-cast.h"
#include "mapped-index.h"
#include "read.h"
#include "extract-store-integer.h"
#include "cp-support.h"
#include "symtab.h"
#include "gdbsupport/selftest.h"

/* When true, do not reject deprecated .gdb_index sections.  */
static bool use_deprecated_index_sections = false;

/* This is a view into the index that converts from bytes to an
   offset_type, and allows indexing.  Unaligned bytes are specifically
   allowed here, and handled via unpacking.  */

class offset_view
{
public:
  offset_view () = default;

  explicit offset_view (gdb::array_view<const gdb_byte> bytes)
    : m_bytes (bytes)
  {
  }

  /* Extract the INDEXth offset_type from the array.  */
  offset_type operator[] (size_t index) const
  {
    const gdb_byte *bytes = &m_bytes[index * sizeof (offset_type)];
    return (offset_type) extract_unsigned_integer (bytes,
						   sizeof (offset_type),
						   BFD_ENDIAN_LITTLE);
  }

  /* Return the number of offset_types in this array.  */
  size_t size () const
  {
    return m_bytes.size () / sizeof (offset_type);
  }

  /* Return true if this view is empty.  */
  bool empty () const
  {
    return m_bytes.empty ();
  }

private:
  /* The underlying bytes.  */
  gdb::array_view<const gdb_byte> m_bytes;
};

/* An index into a (C++) symbol name component in a symbol name as
   recorded in the mapped_index's symbol table.  For each C++ symbol
   in the symbol table, we record one entry for the start of each
   component in the symbol in a table of name components, and then
   sort the table, in order to be able to binary search symbol names,
   ignoring leading namespaces, both completion and regular look up.
   For example, for symbol "A::B::C", we'll have an entry that points
   to "A::B::C", another that points to "B::C", and another for "C".
   Note that function symbols in GDB index have no parameter
   information, just the function/method names.  You can convert a
   name_component to a "const char *" using the
   'mapped_index::symbol_name_at(offset_type)' method.  */

struct name_component
{
  /* Offset in the symbol name where the component starts.  Stored as
     a (32-bit) offset instead of a pointer to save memory and improve
     locality on 64-bit architectures.  */
  offset_type name_offset;

  /* The symbol's index in the symbol and constant pool tables of a
     mapped_index.  */
  offset_type idx;
};

/* A description of .gdb_index index.  The file format is described in
   a comment by the code that writes the index.  */

struct mapped_gdb_index : public dwarf_scanner_base
{
  /* The name_component table (a sorted vector).  See name_component's
     description above.  */
  std::vector<name_component> name_components;

  /* How NAME_COMPONENTS is sorted.  */
  enum case_sensitivity name_components_casing;

  /* Index data format version.  */
  int version = 0;

  /* Compile units followed by type units, in the order as found in the
     index.  Indices found in index entries can index directly into this.  */
  std::vector<dwarf2_per_cu *> units;

  /* The address table data.  */
  gdb::array_view<const gdb_byte> address_table;

  /* The symbol table, implemented as a hash table.  */
  offset_view symbol_table;

  /* A pointer to the constant pool.  */
  gdb::array_view<const gdb_byte> constant_pool;

  /* The shortcut table data.  */
  gdb::array_view<const gdb_byte> shortcut_table;

  /* An address map that maps from PC to dwarf2_per_cu.  */
  addrmap_fixed *index_addrmap = nullptr;

  /* Return the index into the constant pool of the name of the IDXth
     symbol in the symbol table.  */
  offset_type symbol_name_index (offset_type idx) const
  {
    return symbol_table[2 * idx];
  }

  /* Return the index into the constant pool of the CU vector of the
     IDXth symbol in the symbol table.  */
  offset_type symbol_vec_index (offset_type idx) const
  {
    return symbol_table[2 * idx + 1];
  }

  /* Return whether the name at IDX in the symbol table should be
     ignored.  */
  virtual bool symbol_name_slot_invalid (offset_type idx) const
  {
    return (symbol_name_index (idx) == 0
	    && symbol_vec_index (idx) == 0);
  }

  /* Convenience method to get at the name of the symbol at IDX in the
     symbol table.  */
  virtual const char *symbol_name_at
    (offset_type idx, dwarf2_per_objfile *per_objfile) const
  {
    return (const char *) (this->constant_pool.data ()
			   + symbol_name_index (idx));
  }

  virtual size_t symbol_name_count () const
  { return this->symbol_table.size () / 2; }

  /* Build the symbol name component sorted vector, if we haven't
     yet.  */
  void build_name_components (dwarf2_per_objfile *per_objfile);

  /* Returns the lower (inclusive) and upper (exclusive) bounds of the
     possible matches for LN_NO_PARAMS in the name component
     vector.  */
  std::pair<std::vector<name_component>::const_iterator,
	    std::vector<name_component>::const_iterator>
    find_name_components_bounds (const lookup_name_info &ln_no_params,
				 enum language lang,
				 dwarf2_per_objfile *per_objfile) const;

  quick_symbol_functions_up make_quick_functions () const override;

  bool version_check () const override
  {
    return version >= 8;
  }

  dwarf2_per_cu *lookup (unrelocated_addr addr) override
  {
    if (index_addrmap == nullptr)
      return nullptr;

    void *obj = index_addrmap->find (static_cast<CORE_ADDR> (addr));
    return static_cast<dwarf2_per_cu *> (obj);
  }

  cooked_index *index_for_writing () override
  { return nullptr; }
};


/* Starting from a search name, return the string that finds the upper
   bound of all strings that start with SEARCH_NAME in a sorted name
   list.  Returns the empty string to indicate that the upper bound is
   the end of the list.  */

static std::string
make_sort_after_prefix_name (const char *search_name)
{
  /* When looking to complete "func", we find the upper bound of all
     symbols that start with "func" by looking for where we'd insert
     the closest string that would follow "func" in lexicographical
     order.  Usually, that's "func"-with-last-character-incremented,
     i.e. "fund".  Mind non-ASCII characters, though.  Usually those
     will be UTF-8 multi-byte sequences, but we can't be certain.
     Especially mind the 0xff character, which is a valid character in
     non-UTF-8 source character sets (e.g. Latin1 'ÿ'), and we can't
     rule out compilers allowing it in identifiers.  Note that
     conveniently, strcmp/strcasecmp are specified to compare
     characters interpreted as unsigned char.  So what we do is treat
     the whole string as a base 256 number composed of a sequence of
     base 256 "digits" and add 1 to it.  I.e., adding 1 to 0xff wraps
     to 0, and carries 1 to the following more-significant position.
     If the very first character in SEARCH_NAME ends up incremented
     and carries/overflows, then the upper bound is the end of the
     list.  The string after the empty string is also the empty
     string.

     Some examples of this operation:

       SEARCH_NAME  => "+1" RESULT

       "abc"              => "abd"
       "ab\xff"           => "ac"
       "\xff" "a" "\xff"  => "\xff" "b"
       "\xff"             => ""
       "\xff\xff"         => ""
       ""                 => ""

     Then, with these symbols for example:

      func
      func1
      fund

     completing "func" looks for symbols between "func" and
     "func"-with-last-character-incremented, i.e. "fund" (exclusive),
     which finds "func" and "func1", but not "fund".

     And with:

      funcÿ     (Latin1 'ÿ' [0xff])
      funcÿ1
      fund

     completing "funcÿ" looks for symbols between "funcÿ" and "fund"
     (exclusive), which finds "funcÿ" and "funcÿ1", but not "fund".

     And with:

      ÿÿ        (Latin1 'ÿ' [0xff])
      ÿÿ1

     completing "ÿ" or "ÿÿ" looks for symbols between between "ÿÿ" and
     the end of the list.
  */
  std::string after = search_name;
  while (!after.empty () && (unsigned char) after.back () == 0xff)
    after.pop_back ();
  if (!after.empty ())
    after.back () = (unsigned char) after.back () + 1;
  return after;
}

/* See declaration.  */

std::pair<std::vector<name_component>::const_iterator,
	  std::vector<name_component>::const_iterator>
mapped_gdb_index::find_name_components_bounds
  (const lookup_name_info &lookup_name_without_params, language lang,
   dwarf2_per_objfile *per_objfile) const
{
  auto *name_cmp
    = this->name_components_casing == case_sensitive_on ? strcmp : strcasecmp;

  const char *lang_name
    = lookup_name_without_params.language_lookup_name (lang);

  /* Comparison function object for lower_bound that matches against a
     given symbol name.  */
  auto lookup_compare_lower = [&] (const name_component &elem,
				   const char *name)
    {
      const char *elem_qualified = this->symbol_name_at (elem.idx, per_objfile);
      const char *elem_name = elem_qualified + elem.name_offset;
      return name_cmp (elem_name, name) < 0;
    };

  /* Comparison function object for upper_bound that matches against a
     given symbol name.  */
  auto lookup_compare_upper = [&] (const char *name,
				   const name_component &elem)
    {
      const char *elem_qualified = this->symbol_name_at (elem.idx, per_objfile);
      const char *elem_name = elem_qualified + elem.name_offset;
      return name_cmp (name, elem_name) < 0;
    };

  auto begin = this->name_components.begin ();
  auto end = this->name_components.end ();

  /* Find the lower bound.  */
  auto lower = [&] ()
    {
      if (lookup_name_without_params.completion_mode () && lang_name[0] == '\0')
	return begin;
      else
	return std::lower_bound (begin, end, lang_name, lookup_compare_lower);
    } ();

  /* Find the upper bound.  */
  auto upper = [&] ()
    {
      if (lookup_name_without_params.completion_mode ())
	{
	  /* In completion mode, we want UPPER to point past all
	     symbols names that have the same prefix.  I.e., with
	     these symbols, and completing "func":

	      function        << lower bound
	      function1
	      other_function  << upper bound

	     We find the upper bound by looking for the insertion
	     point of "func"-with-last-character-incremented,
	     i.e. "fund".  */
	  std::string after = make_sort_after_prefix_name (lang_name);
	  if (after.empty ())
	    return end;
	  return std::lower_bound (lower, end, after.c_str (),
				   lookup_compare_lower);
	}
      else
	return std::upper_bound (lower, end, lang_name, lookup_compare_upper);
    } ();

  return {lower, upper};
}

/* See declaration.  */

void
mapped_gdb_index::build_name_components (dwarf2_per_objfile *per_objfile)
{
  if (!this->name_components.empty ())
    return;

  this->name_components_casing = case_sensitivity;
  auto *name_cmp
    = this->name_components_casing == case_sensitive_on ? strcmp : strcasecmp;

  /* The code below only knows how to break apart components of C++
     symbol names (and other languages that use '::' as
     namespace/module separator) and Ada symbol names.  */
  auto count = this->symbol_name_count ();
  for (offset_type idx = 0; idx < count; idx++)
    {
      if (this->symbol_name_slot_invalid (idx))
	continue;

      const char *name = this->symbol_name_at (idx, per_objfile);

      /* Add each name component to the name component table.  */
      unsigned int previous_len = 0;

      if (strstr (name, "::") != nullptr)
	{
	  for (unsigned int current_len = cp_find_first_component (name);
	       name[current_len] != '\0';
	       current_len += cp_find_first_component (name + current_len))
	    {
	      gdb_assert (name[current_len] == ':');
	      this->name_components.push_back ({previous_len, idx});
	      /* Skip the '::'.  */
	      current_len += 2;
	      previous_len = current_len;
	    }
	}
      else
	{
	  /* Handle the Ada encoded (aka mangled) form here.  */
	  for (const char *iter = strstr (name, "__");
	       iter != nullptr;
	       iter = strstr (iter, "__"))
	    {
	      this->name_components.push_back ({previous_len, idx});
	      iter += 2;
	      previous_len = iter - name;
	    }
	}

      this->name_components.push_back ({previous_len, idx});
    }

  /* Sort name_components elements by name.  */
  auto name_comp_compare = [&] (const name_component &left,
				const name_component &right)
    {
      const char *left_qualified
	= this->symbol_name_at (left.idx, per_objfile);
      const char *right_qualified
	= this->symbol_name_at (right.idx, per_objfile);

      const char *left_name = left_qualified + left.name_offset;
      const char *right_name = right_qualified + right.name_offset;

      return name_cmp (left_name, right_name) < 0;
    };

  std::sort (this->name_components.begin (),
	     this->name_components.end (),
	     name_comp_compare);
}

/* Helper for dw2_expand_symtabs_matching that works with a
   mapped_index_base instead of the containing objfile.  This is split
   to a separate function in order to be able to unit test the
   name_components matching using a mock mapped_index_base.  For each
   symbol name that matches, calls MATCH_CALLBACK, passing it the
   symbol's index in the mapped_index_base symbol table.  */

static bool
dw2_expand_symtabs_matching_symbol
  (mapped_gdb_index &index,
   const lookup_name_info &lookup_name_in,
   expand_symtabs_symbol_matcher symbol_matcher,
   gdb::function_view<bool (offset_type)> match_callback,
   dwarf2_per_objfile *per_objfile,
   expand_symtabs_lang_matcher lang_matcher)
{
  lookup_name_info lookup_name_without_params
    = lookup_name_in.make_ignore_params ();

  /* Build the symbol name component sorted vector, if we haven't
     yet.  */
  index.build_name_components (per_objfile);

  /* The same symbol may appear more than once in the range though.
     E.g., if we're looking for symbols that complete "w", and we have
     a symbol named "w1::w2", we'll find the two name components for
     that same symbol in the range.  To be sure we only call the
     callback once per symbol, we first collect the symbol name
     indexes that matched in a temporary vector and ignore
     duplicates.  */
  std::vector<offset_type> matches;

  struct name_and_matcher
  {
    symbol_name_matcher_ftype *matcher;
    const char *name;

    bool operator== (const name_and_matcher &other) const
    {
      return matcher == other.matcher && strcmp (name, other.name) == 0;
    }
  };

  /* A vector holding all the different symbol name matchers, for all
     languages.  */
  std::vector<name_and_matcher> matchers;

  for (int i = 0; i < nr_languages; i++)
    {
      enum language lang_e = (enum language) i;
      if (lang_matcher != nullptr && !lang_matcher (lang_e))
	continue;

      const language_defn *lang = language_def (lang_e);
      symbol_name_matcher_ftype *name_matcher
	= lang->get_symbol_name_matcher (lookup_name_without_params);

      name_and_matcher key {
	 name_matcher,
	 lookup_name_without_params.language_lookup_name (lang_e)
      };

      /* Don't insert the same comparison routine more than once.
	 Note that we do this linear walk.  This is not a problem in
	 practice because the number of supported languages is
	 low.  */
      if (std::find (matchers.begin (), matchers.end (), key)
	  != matchers.end ())
	continue;
      matchers.push_back (std::move (key));

      auto bounds
	= index.find_name_components_bounds (lookup_name_without_params,
					     lang_e, per_objfile);

      /* Now for each symbol name in range, check to see if we have a name
	 match, and if so, call the MATCH_CALLBACK callback.  */

      for (; bounds.first != bounds.second; ++bounds.first)
	{
	  const char *qualified
	    = index.symbol_name_at (bounds.first->idx, per_objfile);

	  if (!name_matcher (qualified, lookup_name_without_params, NULL)
	      || (symbol_matcher != NULL && !symbol_matcher (qualified)))
	    continue;

	  matches.push_back (bounds.first->idx);
	}
    }

  std::sort (matches.begin (), matches.end ());

  /* Finally call the callback, once per match.  */
  ULONGEST prev = -1;
  bool result = true;
  for (offset_type idx : matches)
    {
      if (prev != idx)
	{
	  if (!match_callback (idx))
	    {
	      result = false;
	      break;
	    }
	  prev = idx;
	}
    }

  /* Above we use a type wider than idx's for 'prev', since 0 and
     (offset_type)-1 are both possible values.  */
  static_assert (sizeof (prev) > sizeof (offset_type), "");

  return result;
}

#if GDB_SELF_TEST

namespace selftests { namespace dw2_expand_symtabs_matching {

/* A mock .gdb_index/.debug_names-like name index table, enough to
   exercise dw2_expand_symtabs_matching_symbol, which works with the
   mapped_index_base interface.  Builds an index from the symbol list
   passed as parameter to the constructor.  */
class mock_mapped_index : public mapped_gdb_index
{
public:
  mock_mapped_index (gdb::array_view<const char *> symbols)
    : m_symbol_table (symbols)
  {}

  DISABLE_COPY_AND_ASSIGN (mock_mapped_index);

  bool symbol_name_slot_invalid (offset_type idx) const override
  { return false; }

  /* Return the number of names in the symbol table.  */
  size_t symbol_name_count () const override
  {
    return m_symbol_table.size ();
  }

  /* Get the name of the symbol at IDX in the symbol table.  */
  const char *symbol_name_at
    (offset_type idx, dwarf2_per_objfile *per_objfile) const override
  {
    return m_symbol_table[idx];
  }

  quick_symbol_functions_up make_quick_functions () const override
  {
    return nullptr;
  }

private:
  gdb::array_view<const char *> m_symbol_table;
};

/* Convenience function that converts a NULL pointer to a "<null>"
   string, to pass to print routines.  */

static const char *
string_or_null (const char *str)
{
  return str != NULL ? str : "<null>";
}

/* Check if a lookup_name_info built from
   NAME/MATCH_TYPE/COMPLETION_MODE matches the symbols in the mock
   index.  EXPECTED_LIST is the list of expected matches, in expected
   matching order.  If no match expected, then an empty list is
   specified.  Returns true on success.  On failure prints a warning
   indicating the file:line that failed, and returns false.  */

static bool
check_match (const char *file, int line,
	     mock_mapped_index &mock_index,
	     const char *name, symbol_name_match_type match_type,
	     bool completion_mode,
	     std::initializer_list<const char *> expected_list,
	     dwarf2_per_objfile *per_objfile)
{
  lookup_name_info lookup_name (name, match_type, completion_mode);

  bool matched = true;

  auto mismatch = [&] (const char *expected_str,
		       const char *got)
  {
    warning (_("%s:%d: match_type=%s, looking-for=\"%s\", "
	       "expected=\"%s\", got=\"%s\"\n"),
	     file, line,
	     (match_type == symbol_name_match_type::FULL
	      ? "FULL" : "WILD"),
	     name, string_or_null (expected_str), string_or_null (got));
    matched = false;
  };

  auto expected_it = expected_list.begin ();
  auto expected_end = expected_list.end ();

  dw2_expand_symtabs_matching_symbol (mock_index, lookup_name,
				      nullptr,
				      [&] (offset_type idx)
  {
    const char *matched_name = mock_index.symbol_name_at (idx, per_objfile);
    const char *expected_str
      = expected_it == expected_end ? NULL : *expected_it++;

    if (expected_str == NULL || strcmp (expected_str, matched_name) != 0)
      mismatch (expected_str, matched_name);
    return true;
  }, per_objfile, nullptr);

  const char *expected_str
  = expected_it == expected_end ? NULL : *expected_it++;
  if (expected_str != NULL)
    mismatch (expected_str, NULL);

  return matched;
}

/* The symbols added to the mock mapped_index for testing (in
   canonical form).  */
static const char *test_symbols[] = {
  "function",
  "std::bar",
  "std::zfunction",
  "std::zfunction2",
  "w1::w2",
  "ns::foo<char*>",
  "ns::foo<int>",
  "ns::foo<long>",
  "ns2::tmpl<int>::foo2",
  "(anonymous namespace)::A::B::C",

  /* These are used to check that the increment-last-char in the
     matching algorithm for completion doesn't match "t1_fund" when
     completing "t1_func".  */
  "t1_func",
  "t1_func1",
  "t1_fund",
  "t1_fund1",

  /* A UTF-8 name with multi-byte sequences to make sure that
     cp-name-parser understands this as a single identifier ("função"
     is "function" in PT).  */
  (const char *)u8"u8função",

  /* Test a symbol name that ends with a 0xff character, which is a
     valid character in non-UTF-8 source character sets (e.g. Latin1
     'ÿ'), and we can't rule out compilers allowing it in identifiers.
     We test this because the completion algorithm finds the upper
     bound of symbols by looking for the insertion point of
     "func"-with-last-character-incremented, i.e. "fund", and adding 1
     to 0xff should wraparound and carry to the previous character.
     See comments in make_sort_after_prefix_name.  */
  "yfunc\377",

  /* Some more symbols with \377 (0xff).  See above.  */
  "\377",
  "\377\377123",

  /* A name with all sorts of complications.  Starts with "z" to make
     it easier for the completion tests below.  */
#define Z_SYM_NAME \
  "z::std::tuple<(anonymous namespace)::ui*, std::bar<(anonymous namespace)::ui> >" \
    "::tuple<(anonymous namespace)::ui*, " \
    "std::default_delete<(anonymous namespace)::ui>, void>"

  Z_SYM_NAME
};

/* Returns true if the mapped_index_base::find_name_component_bounds
   method finds EXPECTED_SYMS in INDEX when looking for SEARCH_NAME,
   in completion mode.  */

static bool
check_find_bounds_finds (mapped_gdb_index &index,
			 const char *search_name,
			 gdb::array_view<const char *> expected_syms,
			 dwarf2_per_objfile *per_objfile)
{
  lookup_name_info lookup_name (search_name,
				symbol_name_match_type::FULL, true);

  auto bounds = index.find_name_components_bounds (lookup_name,
						   language_cplus,
						   per_objfile);

  size_t distance = std::distance (bounds.first, bounds.second);
  if (distance != expected_syms.size ())
    return false;

  for (size_t exp_elem = 0; exp_elem < distance; exp_elem++)
    {
      auto nc_elem = bounds.first + exp_elem;
      const char *qualified = index.symbol_name_at (nc_elem->idx, per_objfile);
      if (strcmp (qualified, expected_syms[exp_elem]) != 0)
	return false;
    }

  return true;
}

/* Test the lower-level mapped_index::find_name_component_bounds
   method.  */

static void
test_mapped_index_find_name_component_bounds ()
{
  mock_mapped_index mock_index (test_symbols);

  mock_index.build_name_components (NULL /* per_objfile */);

  /* Test the lower-level mapped_index::find_name_component_bounds
     method in completion mode.  */
  {
    static const char *expected_syms[] = {
      "t1_func",
      "t1_func1",
    };

    SELF_CHECK (check_find_bounds_finds
		  (mock_index, "t1_func", expected_syms,
		   NULL /* per_objfile */));
  }

  /* Check that the increment-last-char in the name matching algorithm
     for completion doesn't get confused with Ansi1 'ÿ' / 0xff.  See
     make_sort_after_prefix_name.  */
  {
    static const char *expected_syms1[] = {
      "\377",
      "\377\377123",
    };
    SELF_CHECK (check_find_bounds_finds
		  (mock_index, "\377", expected_syms1, NULL /* per_objfile */));

    static const char *expected_syms2[] = {
      "\377\377123",
    };
    SELF_CHECK (check_find_bounds_finds
		  (mock_index, "\377\377", expected_syms2,
		   NULL /* per_objfile */));
  }
}

/* Test dw2_expand_symtabs_matching_symbol.  */

static void
test_dw2_expand_symtabs_matching_symbol ()
{
  mock_mapped_index mock_index (test_symbols);

  /* We let all tests run until the end even if some fails, for debug
     convenience.  */
  bool any_mismatch = false;

  /* Create the expected symbols list (an initializer_list).  Needed
     because lists have commas, and we need to pass them to CHECK,
     which is a macro.  */
#define EXPECT(...) { __VA_ARGS__ }

  /* Wrapper for check_match that passes down the current
     __FILE__/__LINE__.  */
#define CHECK_MATCH(NAME, MATCH_TYPE, COMPLETION_MODE, EXPECTED_LIST)	\
  any_mismatch |= !check_match (__FILE__, __LINE__,			\
				mock_index,				\
				NAME, MATCH_TYPE, COMPLETION_MODE,	\
				EXPECTED_LIST, NULL)

  /* Identity checks.  */
  for (const char *sym : test_symbols)
    {
      /* Should be able to match all existing symbols.  */
      CHECK_MATCH (sym, symbol_name_match_type::FULL, false,
		   EXPECT (sym));

      /* Should be able to match all existing symbols with
	 parameters.  */
      std::string with_params = std::string (sym) + "(int)";
      CHECK_MATCH (with_params.c_str (), symbol_name_match_type::FULL, false,
		   EXPECT (sym));

      /* Should be able to match all existing symbols with
	 parameters and qualifiers.  */
      with_params = std::string (sym) + " ( int ) const";
      CHECK_MATCH (with_params.c_str (), symbol_name_match_type::FULL, false,
		   EXPECT (sym));

      /* This should really find sym, but cp-name-parser.y doesn't
	 know about lvalue/rvalue qualifiers yet.  */
      with_params = std::string (sym) + " ( int ) &&";
      CHECK_MATCH (with_params.c_str (), symbol_name_match_type::FULL, false,
		   {});
    }

  /* Check that the name matching algorithm for completion doesn't get
     confused with Latin1 'ÿ' / 0xff.  See
     make_sort_after_prefix_name.  */
  {
    static const char str[] = "\377";
    CHECK_MATCH (str, symbol_name_match_type::FULL, true,
		 EXPECT ("\377", "\377\377123"));
  }

  /* Check that the increment-last-char in the matching algorithm for
     completion doesn't match "t1_fund" when completing "t1_func".  */
  {
    static const char str[] = "t1_func";
    CHECK_MATCH (str, symbol_name_match_type::FULL, true,
		 EXPECT ("t1_func", "t1_func1"));
  }

  /* Check that completion mode works at each prefix of the expected
     symbol name.  */
  {
    static const char str[] = "function(int)";
    size_t len = strlen (str);
    std::string lookup;

    for (size_t i = 1; i < len; i++)
      {
	lookup.assign (str, i);
	CHECK_MATCH (lookup.c_str (), symbol_name_match_type::FULL, true,
		     EXPECT ("function"));
      }
  }

  /* While "w" is a prefix of both components, the match function
     should still only be called once.  */
  {
    CHECK_MATCH ("w", symbol_name_match_type::FULL, true,
		 EXPECT ("w1::w2"));
    CHECK_MATCH ("w", symbol_name_match_type::WILD, true,
		 EXPECT ("w1::w2"));
  }

  /* Same, with a "complicated" symbol.  */
  {
    static const char str[] = Z_SYM_NAME;
    size_t len = strlen (str);
    std::string lookup;

    for (size_t i = 1; i < len; i++)
      {
	lookup.assign (str, i);
	CHECK_MATCH (lookup.c_str (), symbol_name_match_type::FULL, true,
		     EXPECT (Z_SYM_NAME));
      }
  }

  /* In FULL mode, an incomplete symbol doesn't match.  */
  {
    CHECK_MATCH ("std::zfunction(int", symbol_name_match_type::FULL, false,
		 {});
  }

  /* A complete symbol with parameters matches any overload, since the
     index has no overload info.  */
  {
    CHECK_MATCH ("std::zfunction(int)", symbol_name_match_type::FULL, true,
		 EXPECT ("std::zfunction", "std::zfunction2"));
    CHECK_MATCH ("zfunction(int)", symbol_name_match_type::WILD, true,
		 EXPECT ("std::zfunction", "std::zfunction2"));
    CHECK_MATCH ("zfunc", symbol_name_match_type::WILD, true,
		 EXPECT ("std::zfunction", "std::zfunction2"));
  }

  /* Check that whitespace is ignored appropriately.  A symbol with a
     template argument list. */
  {
    static const char expected[] = "ns::foo<int>";
    CHECK_MATCH ("ns :: foo < int > ", symbol_name_match_type::FULL, false,
		 EXPECT (expected));
    CHECK_MATCH ("foo < int > ", symbol_name_match_type::WILD, false,
		 EXPECT (expected));
  }

  /* Check that whitespace is ignored appropriately.  A symbol with a
     template argument list that includes a pointer.  */
  {
    static const char expected[] = "ns::foo<char*>";
    /* Try both completion and non-completion modes.  */
    static const bool completion_mode[2] = {false, true};
    for (size_t i = 0; i < 2; i++)
      {
	CHECK_MATCH ("ns :: foo < char * >", symbol_name_match_type::FULL,
		     completion_mode[i], EXPECT (expected));
	CHECK_MATCH ("foo < char * >", symbol_name_match_type::WILD,
		     completion_mode[i], EXPECT (expected));

	CHECK_MATCH ("ns :: foo < char * > (int)", symbol_name_match_type::FULL,
		     completion_mode[i], EXPECT (expected));
	CHECK_MATCH ("foo < char * > (int)", symbol_name_match_type::WILD,
		     completion_mode[i], EXPECT (expected));
      }
  }

  {
    /* Check method qualifiers are ignored.  */
    static const char expected[] = "ns::foo<char*>";
    CHECK_MATCH ("ns :: foo < char * >  ( int ) const",
		 symbol_name_match_type::FULL, true, EXPECT (expected));
    CHECK_MATCH ("ns :: foo < char * >  ( int ) &&",
		 symbol_name_match_type::FULL, true, EXPECT (expected));
    CHECK_MATCH ("foo < char * >  ( int ) const",
		 symbol_name_match_type::WILD, true, EXPECT (expected));
    CHECK_MATCH ("foo < char * >  ( int ) &&",
		 symbol_name_match_type::WILD, true, EXPECT (expected));
  }

  /* Test lookup names that don't match anything.  */
  {
    CHECK_MATCH ("bar2", symbol_name_match_type::WILD, false,
		 {});

    CHECK_MATCH ("doesntexist", symbol_name_match_type::FULL, false,
		 {});
  }

  /* Some wild matching tests, exercising "(anonymous namespace)",
     which should not be confused with a parameter list.  */
  {
    static const char *syms[] = {
      "A::B::C",
      "B::C",
      "C",
      "A :: B :: C ( int )",
      "B :: C ( int )",
      "C ( int )",
    };

    for (const char *s : syms)
      {
	CHECK_MATCH (s, symbol_name_match_type::WILD, false,
		     EXPECT ("(anonymous namespace)::A::B::C"));
      }
  }

  {
    static const char expected[] = "ns2::tmpl<int>::foo2";
    CHECK_MATCH ("tmp", symbol_name_match_type::WILD, true,
		 EXPECT (expected));
    CHECK_MATCH ("tmpl<", symbol_name_match_type::WILD, true,
		 EXPECT (expected));
  }

  SELF_CHECK (!any_mismatch);

#undef EXPECT
#undef CHECK_MATCH
}

static void
run_test ()
{
  test_mapped_index_find_name_component_bounds ();
  test_dw2_expand_symtabs_matching_symbol ();
}

}} /* namespace selftests::dw2_expand_symtabs_matching */

#endif /* GDB_SELF_TEST */

struct dwarf2_gdb_index : public dwarf2_base_index_functions
{
  /* This dumps minimal information about the index.
     It is called via "mt print objfiles".
     One use is to verify .gdb_index has been loaded by the
     gdb.dwarf2/gdb-index.exp testcase.  */
  void dump (struct objfile *objfile) override;

  bool expand_symtabs_matching
    (struct objfile *objfile,
     expand_symtabs_file_matcher file_matcher,
     const lookup_name_info *lookup_name,
     expand_symtabs_symbol_matcher symbol_matcher,
     expand_symtabs_expansion_listener expansion_notify,
     block_search_flags search_flags,
     domain_search_flags domain,
     expand_symtabs_lang_matcher lang_matcher) override;
};

/* This dumps minimal information about the index.
   It is called via "mt print objfiles".
   One use is to verify .gdb_index has been loaded by the
   gdb.dwarf2/gdb-index.exp testcase.  */

void
dwarf2_gdb_index::dump (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  mapped_gdb_index *index = (gdb::checked_static_cast<mapped_gdb_index *>
			     (per_objfile->per_bfd->index_table.get ()));
  gdb_printf (".gdb_index: version %d\n", index->version);
  gdb_printf ("\n");
}

/* Helper for dw2_expand_matching symtabs.  Called on each symbol
   matched, to expand corresponding CUs that were marked.  IDX is the
   index of the symbol name that matched.  */

static bool
dw2_expand_marked_cus (dwarf2_per_objfile *per_objfile, offset_type idx,
		       expand_symtabs_file_matcher file_matcher,
		       expand_symtabs_expansion_listener expansion_notify,
		       block_search_flags search_flags,
		       domain_search_flags kind,
		       expand_symtabs_lang_matcher lang_matcher)
{
  offset_type vec_len, vec_idx;
  bool global_seen = false;
  mapped_gdb_index &index
    = *(gdb::checked_static_cast<mapped_gdb_index *>
	(per_objfile->per_bfd->index_table.get ()));

  offset_view vec (index.constant_pool.slice (index.symbol_vec_index (idx)));
  vec_len = vec[0];
  for (vec_idx = 0; vec_idx < vec_len; ++vec_idx)
    {
      offset_type cu_index_and_attrs = vec[vec_idx + 1];
      /* This value is only valid for index versions >= 7.  */
      int is_static = GDB_INDEX_SYMBOL_STATIC_VALUE (cu_index_and_attrs);
      gdb_index_symbol_kind symbol_kind =
	GDB_INDEX_SYMBOL_KIND_VALUE (cu_index_and_attrs);
      int cu_index = GDB_INDEX_CU_VALUE (cu_index_and_attrs);
      /* Only check the symbol attributes if they're present.
	 Indices prior to version 7 don't record them,
	 and indices >= 7 may elide them for certain symbols
	 (gold does this).  */
      int attrs_valid =
	(index.version >= 7
	 && symbol_kind != GDB_INDEX_SYMBOL_KIND_NONE);

      /* Work around gold/15646.  */
      if (attrs_valid
	  && !is_static
	  && symbol_kind == GDB_INDEX_SYMBOL_KIND_TYPE)
	{
	  if (global_seen)
	    continue;

	  global_seen = true;
	}

      /* Only check the symbol's kind if it has one.  */
      if (attrs_valid)
	{
	  if (is_static)
	    {
	      if ((search_flags & SEARCH_STATIC_BLOCK) == 0)
		continue;
	    }
	  else
	    {
	      if ((search_flags & SEARCH_GLOBAL_BLOCK) == 0)
		continue;
	    }

	  domain_search_flags mask = 0;
	  switch (symbol_kind)
	    {
	    case GDB_INDEX_SYMBOL_KIND_VARIABLE:
	      mask = SEARCH_VAR_DOMAIN;
	      break;
	    case GDB_INDEX_SYMBOL_KIND_FUNCTION:
	      mask = SEARCH_FUNCTION_DOMAIN;
	      break;
	    case GDB_INDEX_SYMBOL_KIND_TYPE:
	      mask = SEARCH_TYPE_DOMAIN | SEARCH_STRUCT_DOMAIN;
	      break;
	    case GDB_INDEX_SYMBOL_KIND_OTHER:
	      mask = SEARCH_MODULE_DOMAIN;
	      break;
	    }
	  if ((kind & mask) == 0)
	    continue;
	}

      /* Don't crash on bad data.  */
      if (cu_index >= index.units.size ())
	{
	  complaint (_(".gdb_index entry has bad CU index"
		       " [in module %s]"), objfile_name (per_objfile->objfile));
	  continue;
	}

      dwarf2_per_cu *per_cu = index.units[cu_index];

      if (!dw2_expand_symtabs_matching_one (per_cu, per_objfile, file_matcher,
					    expansion_notify, lang_matcher))
	return false;
    }

  return true;
}

bool
dwarf2_gdb_index::expand_symtabs_matching
  (objfile *objfile,
   expand_symtabs_file_matcher file_matcher,
   const lookup_name_info *lookup_name,
   expand_symtabs_symbol_matcher symbol_matcher,
   expand_symtabs_expansion_listener expansion_notify,
   block_search_flags search_flags,
   domain_search_flags domain,
   expand_symtabs_lang_matcher lang_matcher)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  dw_expand_symtabs_matching_file_matcher (per_objfile, file_matcher);

  /* This invariant is documented in quick-functions.h.  */
  gdb_assert (lookup_name != nullptr || symbol_matcher == nullptr);
  if (lookup_name == nullptr)
    {
      for (dwarf2_per_cu *per_cu : all_units_range (per_objfile->per_bfd))
	{
	  QUIT;

	  if (!dw2_expand_symtabs_matching_one (per_cu, per_objfile,
						file_matcher,
						expansion_notify,
						lang_matcher))
	    return false;
	}
      return true;
    }

  mapped_gdb_index &index
    = *(gdb::checked_static_cast<mapped_gdb_index *>
	(per_objfile->per_bfd->index_table.get ()));

  bool result
    = dw2_expand_symtabs_matching_symbol (index, *lookup_name,
					  symbol_matcher,
					  [&] (offset_type idx)
    {
      if (!dw2_expand_marked_cus (per_objfile, idx, file_matcher,
				  expansion_notify, search_flags, domain,
				  lang_matcher))
	return false;
      return true;
    }, per_objfile, lang_matcher);

  return result;
}

quick_symbol_functions_up
mapped_gdb_index::make_quick_functions () const
{
  return quick_symbol_functions_up (new dwarf2_gdb_index);
}

/* A helper function that reads the .gdb_index from BUFFER and fills
   in MAP.  FILENAME is the name of the file containing the data;
   it is used for error reporting.  DEPRECATED_OK is true if it is
   ok to use deprecated sections.

   CU_LIST, CU_LIST_ELEMENTS, TYPES_LIST, and TYPES_LIST_ELEMENTS are
   out parameters that are filled in with information about the CU and
   TU lists in the section.

   Returns true if all went well, false otherwise.  */

static bool
read_gdb_index_from_buffer (const char *filename,
			    bool deprecated_ok,
			    gdb::array_view<const gdb_byte> buffer,
			    mapped_gdb_index *map,
			    const gdb_byte **cu_list,
			    offset_type *cu_list_elements,
			    const gdb_byte **types_list,
			    offset_type *types_list_elements)
{
  const gdb_byte *addr = &buffer[0];
  offset_view metadata (buffer);

  /* Version check.  */
  offset_type version = metadata[0];
  /* Versions earlier than 3 emitted every copy of a psymbol.  This
     causes the index to behave very poorly for certain requests.  Version 3
     contained incomplete addrmap.  So, it seems better to just ignore such
     indices.  */
  if (version < 4)
    {
      static int warning_printed = 0;
      if (!warning_printed)
	{
	  warning (_("Skipping obsolete .gdb_index section in %s."),
		   filename);
	  warning_printed = 1;
	}
      return 0;
    }
  /* Index version 4 uses a different hash function than index version
     5 and later.

     Versions earlier than 6 did not emit psymbols for inlined
     functions.  Using these files will cause GDB not to be able to
     set breakpoints on inlined functions by name, so we ignore these
     indices unless the user has done
     "set use-deprecated-index-sections on".  */
  if (version < 6 && !deprecated_ok)
    {
      static int warning_printed = 0;
      if (!warning_printed)
	{
	  warning (_("\
Skipping deprecated .gdb_index section in %s.\n\
Do \"%ps\" before the file is read\n\
to use the section anyway."),
		   filename,
		   styled_string (command_style.style (),
				  "set use-deprecated-index-sections on"));
	  warning_printed = 1;
	}
      return 0;
    }
  /* Version 7 indices generated by gold refer to the CU for a symbol instead
     of the TU (for symbols coming from TUs),
     http://sourceware.org/bugzilla/show_bug.cgi?id=15021.
     Plus gold-generated indices can have duplicate entries for global symbols,
     http://sourceware.org/bugzilla/show_bug.cgi?id=15646.
     These are just performance bugs, and we can't distinguish gdb-generated
     indices from gold-generated ones, so issue no warning here.  */

  /* Indexes with higher version than the one supported by GDB may be no
     longer backward compatible.  */
  if (version > 9)
    return 0;

  map->version = version;

  int i = 1;
  *cu_list = addr + metadata[i];
  *cu_list_elements = (metadata[i + 1] - metadata[i]) / 8;
  ++i;

  *types_list = addr + metadata[i];
  *types_list_elements = (metadata[i + 1] - metadata[i]) / 8;
  ++i;

  const gdb_byte *address_table = addr + metadata[i];
  const gdb_byte *address_table_end = addr + metadata[i + 1];
  map->address_table
    = gdb::array_view<const gdb_byte> (address_table, address_table_end);
  ++i;

  const gdb_byte *symbol_table = addr + metadata[i];
  const gdb_byte *symbol_table_end = addr + metadata[i + 1];
  map->symbol_table
    = offset_view (gdb::array_view<const gdb_byte> (symbol_table,
						    symbol_table_end));

  ++i;

  if (version >= 9)
    {
      const gdb_byte *shortcut_table = addr + metadata[i];
      const gdb_byte *shortcut_table_end = addr + metadata[i + 1];
      map->shortcut_table
	= gdb::array_view<const gdb_byte> (shortcut_table, shortcut_table_end);
      ++i;
    }

  map->constant_pool = buffer.slice (metadata[i]);

  if (map->constant_pool.empty () && !map->symbol_table.empty ())
    {
      /* An empty constant pool implies that all symbol table entries are
	 empty.  Make map->symbol_table.empty () == true.  */
      map->symbol_table
	= offset_view (gdb::array_view<const gdb_byte> (symbol_table,
							symbol_table));
    }

  return 1;
}

/* A helper for create_cus_from_gdb_index that handles a given list of
   CUs.  */

static void
create_cus_from_gdb_index_list (dwarf2_per_bfd *per_bfd,
				const gdb_byte *cu_list, offset_type n_elements,
				struct dwarf2_section_info *section,
				int is_dwz, std::vector<dwarf2_per_cu *> &units)
{
  for (offset_type i = 0; i < n_elements; i += 2)
    {
      static_assert (sizeof (ULONGEST) >= 8);

      sect_offset sect_off
	= (sect_offset) extract_unsigned_integer (cu_list, 8, BFD_ENDIAN_LITTLE);
      ULONGEST length = extract_unsigned_integer (cu_list + 8, 8, BFD_ENDIAN_LITTLE);
      cu_list += 2 * 8;

      dwarf2_per_cu_up per_cu = per_bfd->allocate_per_cu (section, sect_off,
							  length, is_dwz);
      units.emplace_back (per_cu.get ());
      per_bfd->all_units.emplace_back (std::move (per_cu));
    }
}

/* Read the CU list from the mapped index, and use it to create all
   the CU objects for PER_BFD.  */

static void
create_cus_from_gdb_index (dwarf2_per_bfd *per_bfd,
			   const gdb_byte *cu_list, offset_type cu_list_elements,
			   std::vector<dwarf2_per_cu *> &units,
			   const gdb_byte *dwz_list, offset_type dwz_elements)
{
  gdb_assert (per_bfd->all_units.empty ());
  per_bfd->all_units.reserve ((cu_list_elements + dwz_elements) / 2);

  create_cus_from_gdb_index_list (per_bfd, cu_list, cu_list_elements,
				  &per_bfd->infos[0], 0, units);

  if (dwz_elements == 0)
    return;

  dwz_file *dwz = per_bfd->get_dwz_file ();
  create_cus_from_gdb_index_list (per_bfd, dwz_list, dwz_elements,
				  &dwz->info, 1, units);
}

/* Create the signatured type hash table from the index.  */

static void
create_signatured_type_table_from_gdb_index
  (dwarf2_per_bfd *per_bfd, struct dwarf2_section_info *section,
   const gdb_byte *bytes, offset_type elements,
   std::vector<dwarf2_per_cu *> &units)
{
  signatured_type_set sig_types_hash;

  for (offset_type i = 0; i < elements; i += 3)
    {
      static_assert (sizeof (ULONGEST) >= 8);
      sect_offset sect_off
	= (sect_offset) extract_unsigned_integer (bytes, 8, BFD_ENDIAN_LITTLE);
	cu_offset type_offset_in_tu
	= (cu_offset) extract_unsigned_integer (bytes + 8, 8,
						BFD_ENDIAN_LITTLE);
      ULONGEST signature
	= extract_unsigned_integer (bytes + 16, 8, BFD_ENDIAN_LITTLE);
      bytes += 3 * 8;

      /* The length of the type unit is unknown at this time.  It gets
	 (presumably) set by a cutu_reader when it gets expanded later.  */
      signatured_type_up sig_type
	= per_bfd->allocate_signatured_type (section, sect_off, 0 /* length */,
					     false /* is_dwz */, signature);
      sig_type->type_offset_in_tu = type_offset_in_tu;

      sig_types_hash.emplace (sig_type.get ());
      units.emplace_back (sig_type.get ());
      per_bfd->all_units.emplace_back (sig_type.release ());
    }

  per_bfd->signatured_types = std::move (sig_types_hash);
}

/* Read the address map data from the mapped GDB index.  Return true if no
   errors were found, otherwise return false.  */

static bool
create_addrmap_from_gdb_index (dwarf2_per_objfile *per_objfile,
			       mapped_gdb_index *index)
{
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  const gdb_byte *iter, *end;

  addrmap_mutable mutable_map;

  iter = index->address_table.data ();
  end = iter + index->address_table.size ();

  while (iter < end)
    {
      ULONGEST hi, lo, cu_index;
      lo = extract_unsigned_integer (iter, 8, BFD_ENDIAN_LITTLE);
      iter += 8;
      hi = extract_unsigned_integer (iter, 8, BFD_ENDIAN_LITTLE);
      iter += 8;
      cu_index = extract_unsigned_integer (iter, 4, BFD_ENDIAN_LITTLE);
      iter += 4;

      if (lo >= hi)
	{
	  warning (_(".gdb_index address table has invalid range (%s - %s),"
		     " ignoring .gdb_index"),
		   hex_string (lo), hex_string (hi));
	  return false;
	}

      if (cu_index >= index->units.size ())
	{
	  warning (_(".gdb_index address table has invalid CU number %u,"
		     " ignoring .gdb_index"),
		   (unsigned) cu_index);
	  return false;
	}

      bool full_range_p
	= mutable_map.set_empty (lo, hi - 1, index->units[cu_index]);
      if (!full_range_p)
	{
	  warning (_(".gdb_index address table has a range (%s - %s) that"
		     " overlaps with an earlier range, ignoring .gdb_index"),
		     hex_string (lo), hex_string (hi));
	  return false;
	}
    }

  index->index_addrmap
    = new (&per_bfd->obstack) addrmap_fixed (&per_bfd->obstack, &mutable_map);

  return true;
}

/* Sets the name and language of the main function from the shortcut table.  */

static void
set_main_name_from_gdb_index (dwarf2_per_objfile *per_objfile,
			      mapped_gdb_index *index)
{
  const auto expected_size = 2 * sizeof (offset_type);
  if (index->shortcut_table.size () < expected_size)
    /* The data in the section is not present, is corrupted or is in a version
       we don't know about.  Regardless, we can't make use of it.  */
    return;

  auto ptr = index->shortcut_table.data ();
  const auto dw_lang = extract_unsigned_integer (ptr, 4, BFD_ENDIAN_LITTLE);
  if (dw_lang >= DW_LANG_hi_user)
    {
      complaint (_(".gdb_index shortcut table has invalid main language %u"),
		   (unsigned) dw_lang);
      return;
    }
  if (dw_lang == 0)
    {
      /* Don't bother if the language for the main symbol was not known or if
	 there was no main symbol at all when the index was built.  */
      return;
    }
  ptr += 4;

  const auto lang = dwarf_lang_to_enum_language (dw_lang);
  const auto name_offset = extract_unsigned_integer (ptr,
						     sizeof (offset_type),
						     BFD_ENDIAN_LITTLE);
  const auto name = (const char *) (index->constant_pool.data () + name_offset);

  set_objfile_main_name (per_objfile->objfile, name, (enum language) lang);
}

/* See read-gdb-index.h.  */

bool
dwarf2_read_gdb_index
  (dwarf2_per_objfile *per_objfile,
   get_gdb_index_contents_ftype get_gdb_index_contents,
   get_gdb_index_contents_dwz_ftype get_gdb_index_contents_dwz)
{
  const gdb_byte *cu_list, *types_list, *dwz_list = NULL;
  offset_type cu_list_elements, types_list_elements, dwz_list_elements = 0;
  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  scoped_remove_all_units remove_all_units (*per_bfd);

  gdb::array_view<const gdb_byte> main_index_contents
    = get_gdb_index_contents (objfile, per_bfd);

  if (main_index_contents.empty ())
    return false;

  auto map = std::make_unique<mapped_gdb_index> ();
  if (!read_gdb_index_from_buffer (objfile_name (objfile),
				   use_deprecated_index_sections,
				   main_index_contents, map.get (), &cu_list,
				   &cu_list_elements, &types_list,
				   &types_list_elements))
    return false;

  /* Don't use the index if it's empty.  */
  if (map->symbol_table.empty ())
    return false;

  /* If there is a .dwz file, read it so we can get its CU list as
     well.  */
  dwz_file *dwz = per_bfd->get_dwz_file ();
  if (dwz != NULL)
    {
      mapped_gdb_index dwz_map;
      const gdb_byte *dwz_types_ignore;
      offset_type dwz_types_elements_ignore;

      gdb::array_view<const gdb_byte> dwz_index_content
	= get_gdb_index_contents_dwz (objfile, dwz);

      if (dwz_index_content.empty ())
	return false;

      if (!read_gdb_index_from_buffer (dwz->filename (),
				       1, dwz_index_content, &dwz_map,
				       &dwz_list, &dwz_list_elements,
				       &dwz_types_ignore,
				       &dwz_types_elements_ignore))
	{
	  warning (_("could not read '.gdb_index' section from %s; skipping"),
		   dwz->filename ());
	  return false;
	}
    }

  create_cus_from_gdb_index (per_bfd, cu_list, cu_list_elements, map->units,
			     dwz_list, dwz_list_elements);

  if (types_list_elements)
    {
      /* We can only handle a single .debug_info and .debug_types when we have
	 an index.  */
      if (per_bfd->infos.size () > 1
	  || per_bfd->types.size () > 1)
	return false;

      dwarf2_section_info *section
	= (per_bfd->types.size () == 1
	   ? &per_bfd->types[0]
	   : &per_bfd->infos[0]);

      create_signatured_type_table_from_gdb_index (per_bfd, section, types_list,
						   types_list_elements,
						   map->units);
    }

  finalize_all_units (per_bfd);

  if (!create_addrmap_from_gdb_index (per_objfile, map.get ()))
    return false;

  set_main_name_from_gdb_index (per_objfile, map.get ());

  per_bfd->index_table = std::move (map);
  remove_all_units.disable ();

  return true;
}

INIT_GDB_FILE (read_gdb_index)
{
  add_setshow_boolean_cmd ("use-deprecated-index-sections",
			   no_class, &use_deprecated_index_sections, _("\
Set whether to use deprecated gdb_index sections."), _("\
Show whether to use deprecated gdb_index sections."), _("\
When enabled, deprecated .gdb_index sections are used anyway.\n\
Normally they are ignored either because of a missing feature or\n\
performance issue.\n\
Warning: This option must be enabled before gdb reads the file."),
			   NULL,
			   NULL,
			   &setlist, &showlist);

#if GDB_SELF_TEST
  selftests::register_test ("dw2_expand_symtabs_matching",
			    selftests::dw2_expand_symtabs_matching::run_test);
#endif
}
