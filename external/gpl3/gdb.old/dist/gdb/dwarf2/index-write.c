/* DWARF index writing support for GDB.

   Copyright (C) 1994-2020 Free Software Foundation, Inc.

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

#include "dwarf2/index-write.h"

#include "addrmap.h"
#include "cli/cli-decode.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/gdb_unlinker.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/scoped_fd.h"
#include "complaints.h"
#include "dwarf2/index-common.h"
#include "dwarf2.h"
#include "dwarf2/read.h"
#include "dwarf2/dwz.h"
#include "gdb/gdb-index.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "psympriv.h"
#include "ada-lang.h"

#include <algorithm>
#include <cmath>
#include <forward_list>
#include <set>
#include <unordered_map>
#include <unordered_set>

/* Ensure only legit values are used.  */
#define DW2_GDB_INDEX_SYMBOL_STATIC_SET_VALUE(cu_index, value) \
  do { \
    gdb_assert ((unsigned int) (value) <= 1); \
    GDB_INDEX_SYMBOL_STATIC_SET_VALUE((cu_index), (value)); \
  } while (0)

/* Ensure only legit values are used.  */
#define DW2_GDB_INDEX_SYMBOL_KIND_SET_VALUE(cu_index, value) \
  do { \
    gdb_assert ((value) >= GDB_INDEX_SYMBOL_KIND_TYPE \
                && (value) <= GDB_INDEX_SYMBOL_KIND_OTHER); \
    GDB_INDEX_SYMBOL_KIND_SET_VALUE((cu_index), (value)); \
  } while (0)

/* Ensure we don't use more than the allotted number of bits for the CU.  */
#define DW2_GDB_INDEX_CU_SET_VALUE(cu_index, value) \
  do { \
    gdb_assert (((value) & ~GDB_INDEX_CU_MASK) == 0); \
    GDB_INDEX_CU_SET_VALUE((cu_index), (value)); \
  } while (0)

/* The "save gdb-index" command.  */

/* Write SIZE bytes from the buffer pointed to by DATA to FILE, with
   error checking.  */

static void
file_write (FILE *file, const void *data, size_t size)
{
  if (fwrite (data, 1, size, file) != size)
    error (_("couldn't data write to file"));
}

/* Write the contents of VEC to FILE, with error checking.  */

template<typename Elem, typename Alloc>
static void
file_write (FILE *file, const std::vector<Elem, Alloc> &vec)
{
  if (!vec.empty ())
    file_write (file, vec.data (), vec.size () * sizeof (vec[0]));
}

/* In-memory buffer to prepare data to be written later to a file.  */
class data_buf
{
public:
  /* Copy DATA to the end of the buffer.  */
  template<typename T>
  void append_data (const T &data)
  {
    std::copy (reinterpret_cast<const gdb_byte *> (&data),
	       reinterpret_cast<const gdb_byte *> (&data + 1),
	       grow (sizeof (data)));
  }

  /* Copy CSTR (a zero-terminated string) to the end of buffer.  The
     terminating zero is appended too.  */
  void append_cstr0 (const char *cstr)
  {
    const size_t size = strlen (cstr) + 1;
    std::copy (cstr, cstr + size, grow (size));
  }

  /* Store INPUT as ULEB128 to the end of buffer.  */
  void append_unsigned_leb128 (ULONGEST input)
  {
    for (;;)
      {
	gdb_byte output = input & 0x7f;
	input >>= 7;
	if (input)
	  output |= 0x80;
	append_data (output);
	if (input == 0)
	  break;
      }
  }

  /* Accept a host-format integer in VAL and append it to the buffer
     as a target-format integer which is LEN bytes long.  */
  void append_uint (size_t len, bfd_endian byte_order, ULONGEST val)
  {
    ::store_unsigned_integer (grow (len), len, byte_order, val);
  }

  /* Return the size of the buffer.  */
  size_t size () const
  {
    return m_vec.size ();
  }

  /* Return true iff the buffer is empty.  */
  bool empty () const
  {
    return m_vec.empty ();
  }

  /* Write the buffer to FILE.  */
  void file_write (FILE *file) const
  {
    ::file_write (file, m_vec);
  }

private:
  /* Grow SIZE bytes at the end of the buffer.  Returns a pointer to
     the start of the new block.  */
  gdb_byte *grow (size_t size)
  {
    m_vec.resize (m_vec.size () + size);
    return &*(m_vec.end () - size);
  }

  gdb::byte_vector m_vec;
};

/* An entry in the symbol table.  */
struct symtab_index_entry
{
  /* The name of the symbol.  */
  const char *name;
  /* The offset of the name in the constant pool.  */
  offset_type index_offset;
  /* A sorted vector of the indices of all the CUs that hold an object
     of this name.  */
  std::vector<offset_type> cu_indices;
};

/* The symbol table.  This is a power-of-2-sized hash table.  */
struct mapped_symtab
{
  mapped_symtab ()
  {
    data.resize (1024);
  }

  offset_type n_elements = 0;
  std::vector<symtab_index_entry> data;

  /* Temporary storage for Ada names.  */
  auto_obstack m_string_obstack;
};

/* Find a slot in SYMTAB for the symbol NAME.  Returns a reference to
   the slot.

   Function is used only during write_hash_table so no index format backward
   compatibility is needed.  */

static symtab_index_entry &
find_slot (struct mapped_symtab *symtab, const char *name)
{
  offset_type index, step, hash = mapped_index_string_hash (INT_MAX, name);

  index = hash & (symtab->data.size () - 1);
  step = ((hash * 17) & (symtab->data.size () - 1)) | 1;

  for (;;)
    {
      if (symtab->data[index].name == NULL
	  || strcmp (name, symtab->data[index].name) == 0)
	return symtab->data[index];
      index = (index + step) & (symtab->data.size () - 1);
    }
}

/* Expand SYMTAB's hash table.  */

static void
hash_expand (struct mapped_symtab *symtab)
{
  auto old_entries = std::move (symtab->data);

  symtab->data.clear ();
  symtab->data.resize (old_entries.size () * 2);

  for (auto &it : old_entries)
    if (it.name != NULL)
      {
	auto &ref = find_slot (symtab, it.name);
	ref = std::move (it);
      }
}

/* Add an entry to SYMTAB.  NAME is the name of the symbol.
   CU_INDEX is the index of the CU in which the symbol appears.
   IS_STATIC is one if the symbol is static, otherwise zero (global).  */

static void
add_index_entry (struct mapped_symtab *symtab, const char *name,
		 int is_static, gdb_index_symbol_kind kind,
		 offset_type cu_index)
{
  offset_type cu_index_and_attrs;

  ++symtab->n_elements;
  if (4 * symtab->n_elements / 3 >= symtab->data.size ())
    hash_expand (symtab);

  symtab_index_entry &slot = find_slot (symtab, name);
  if (slot.name == NULL)
    {
      slot.name = name;
      /* index_offset is set later.  */
    }

  cu_index_and_attrs = 0;
  DW2_GDB_INDEX_CU_SET_VALUE (cu_index_and_attrs, cu_index);
  DW2_GDB_INDEX_SYMBOL_STATIC_SET_VALUE (cu_index_and_attrs, is_static);
  DW2_GDB_INDEX_SYMBOL_KIND_SET_VALUE (cu_index_and_attrs, kind);

  /* We don't want to record an index value twice as we want to avoid the
     duplication.
     We process all global symbols and then all static symbols
     (which would allow us to avoid the duplication by only having to check
     the last entry pushed), but a symbol could have multiple kinds in one CU.
     To keep things simple we don't worry about the duplication here and
     sort and uniquify the list after we've processed all symbols.  */
  slot.cu_indices.push_back (cu_index_and_attrs);
}

/* Sort and remove duplicates of all symbols' cu_indices lists.  */

static void
uniquify_cu_indices (struct mapped_symtab *symtab)
{
  for (auto &entry : symtab->data)
    {
      if (entry.name != NULL && !entry.cu_indices.empty ())
	{
	  auto &cu_indices = entry.cu_indices;
	  std::sort (cu_indices.begin (), cu_indices.end ());
	  auto from = std::unique (cu_indices.begin (), cu_indices.end ());
	  cu_indices.erase (from, cu_indices.end ());
	}
    }
}

/* A form of 'const char *' suitable for container keys.  Only the
   pointer is stored.  The strings themselves are compared, not the
   pointers.  */
class c_str_view
{
public:
  c_str_view (const char *cstr)
    : m_cstr (cstr)
  {}

  bool operator== (const c_str_view &other) const
  {
    return strcmp (m_cstr, other.m_cstr) == 0;
  }

  /* Return the underlying C string.  Note, the returned string is
     only a reference with lifetime of this object.  */
  const char *c_str () const
  {
    return m_cstr;
  }

private:
  friend class c_str_view_hasher;
  const char *const m_cstr;
};

/* A std::unordered_map::hasher for c_str_view that uses the right
   hash function for strings in a mapped index.  */
class c_str_view_hasher
{
public:
  size_t operator () (const c_str_view &x) const
  {
    return mapped_index_string_hash (INT_MAX, x.m_cstr);
  }
};

/* A std::unordered_map::hasher for std::vector<>.  */
template<typename T>
class vector_hasher
{
public:
  size_t operator () (const std::vector<T> &key) const
  {
    return iterative_hash (key.data (),
			   sizeof (key.front ()) * key.size (), 0);
  }
};

/* Write the mapped hash table SYMTAB to the data buffer OUTPUT, with
   constant pool entries going into the data buffer CPOOL.  */

static void
write_hash_table (mapped_symtab *symtab, data_buf &output, data_buf &cpool)
{
  {
    /* Elements are sorted vectors of the indices of all the CUs that
       hold an object of this name.  */
    std::unordered_map<std::vector<offset_type>, offset_type,
		       vector_hasher<offset_type>>
      symbol_hash_table;

    /* We add all the index vectors to the constant pool first, to
       ensure alignment is ok.  */
    for (symtab_index_entry &entry : symtab->data)
      {
	if (entry.name == NULL)
	  continue;
	gdb_assert (entry.index_offset == 0);

	/* Finding before inserting is faster than always trying to
	   insert, because inserting always allocates a node, does the
	   lookup, and then destroys the new node if another node
	   already had the same key.  C++17 try_emplace will avoid
	   this.  */
	const auto found
	  = symbol_hash_table.find (entry.cu_indices);
	if (found != symbol_hash_table.end ())
	  {
	    entry.index_offset = found->second;
	    continue;
	  }

	symbol_hash_table.emplace (entry.cu_indices, cpool.size ());
	entry.index_offset = cpool.size ();
	cpool.append_data (MAYBE_SWAP (entry.cu_indices.size ()));
	for (const auto index : entry.cu_indices)
	  cpool.append_data (MAYBE_SWAP (index));
      }
  }

  /* Now write out the hash table.  */
  std::unordered_map<c_str_view, offset_type, c_str_view_hasher> str_table;
  for (const auto &entry : symtab->data)
    {
      offset_type str_off, vec_off;

      if (entry.name != NULL)
	{
	  const auto insertpair = str_table.emplace (entry.name, cpool.size ());
	  if (insertpair.second)
	    cpool.append_cstr0 (entry.name);
	  str_off = insertpair.first->second;
	  vec_off = entry.index_offset;
	}
      else
	{
	  /* While 0 is a valid constant pool index, it is not valid
	     to have 0 for both offsets.  */
	  str_off = 0;
	  vec_off = 0;
	}

      output.append_data (MAYBE_SWAP (str_off));
      output.append_data (MAYBE_SWAP (vec_off));
    }
}

typedef std::unordered_map<partial_symtab *, unsigned int> psym_index_map;

/* Helper struct for building the address table.  */
struct addrmap_index_data
{
  addrmap_index_data (data_buf &addr_vec_, psym_index_map &cu_index_htab_)
    : addr_vec (addr_vec_), cu_index_htab (cu_index_htab_)
  {}

  struct objfile *objfile;
  data_buf &addr_vec;
  psym_index_map &cu_index_htab;

  /* Non-zero if the previous_* fields are valid.
     We can't write an entry until we see the next entry (since it is only then
     that we know the end of the entry).  */
  int previous_valid;
  /* Index of the CU in the table of all CUs in the index file.  */
  unsigned int previous_cu_index;
  /* Start address of the CU.  */
  CORE_ADDR previous_cu_start;
};

/* Write an address entry to ADDR_VEC.  */

static void
add_address_entry (struct objfile *objfile, data_buf &addr_vec,
		   CORE_ADDR start, CORE_ADDR end, unsigned int cu_index)
{
  addr_vec.append_uint (8, BFD_ENDIAN_LITTLE, start);
  addr_vec.append_uint (8, BFD_ENDIAN_LITTLE, end);
  addr_vec.append_data (MAYBE_SWAP (cu_index));
}

/* Worker function for traversing an addrmap to build the address table.  */

static int
add_address_entry_worker (void *datap, CORE_ADDR start_addr, void *obj)
{
  struct addrmap_index_data *data = (struct addrmap_index_data *) datap;
  partial_symtab *pst = (partial_symtab *) obj;

  if (data->previous_valid)
    add_address_entry (data->objfile, data->addr_vec,
		       data->previous_cu_start, start_addr,
		       data->previous_cu_index);

  data->previous_cu_start = start_addr;
  if (pst != NULL)
    {
      const auto it = data->cu_index_htab.find (pst);
      gdb_assert (it != data->cu_index_htab.cend ());
      data->previous_cu_index = it->second;
      data->previous_valid = 1;
    }
  else
    data->previous_valid = 0;

  return 0;
}

/* Write OBJFILE's address map to ADDR_VEC.
   CU_INDEX_HTAB is used to map addrmap entries to their CU indices
   in the index file.  */

static void
write_address_map (struct objfile *objfile, data_buf &addr_vec,
		   psym_index_map &cu_index_htab)
{
  struct addrmap_index_data addrmap_index_data (addr_vec, cu_index_htab);

  /* When writing the address table, we have to cope with the fact that
     the addrmap iterator only provides the start of a region; we have to
     wait until the next invocation to get the start of the next region.  */

  addrmap_index_data.objfile = objfile;
  addrmap_index_data.previous_valid = 0;

  addrmap_foreach (objfile->partial_symtabs->psymtabs_addrmap,
		   add_address_entry_worker, &addrmap_index_data);

  /* It's highly unlikely the last entry (end address = 0xff...ff)
     is valid, but we should still handle it.
     The end address is recorded as the start of the next region, but that
     doesn't work here.  To cope we pass 0xff...ff, this is a rare situation
     anyway.  */
  if (addrmap_index_data.previous_valid)
    add_address_entry (objfile, addr_vec,
		       addrmap_index_data.previous_cu_start, (CORE_ADDR) -1,
		       addrmap_index_data.previous_cu_index);
}

/* Return the symbol kind of PSYM.  */

static gdb_index_symbol_kind
symbol_kind (struct partial_symbol *psym)
{
  domain_enum domain = psym->domain;
  enum address_class aclass = psym->aclass;

  switch (domain)
    {
    case VAR_DOMAIN:
      switch (aclass)
	{
	case LOC_BLOCK:
	  return GDB_INDEX_SYMBOL_KIND_FUNCTION;
	case LOC_TYPEDEF:
	  return GDB_INDEX_SYMBOL_KIND_TYPE;
	case LOC_COMPUTED:
	case LOC_CONST_BYTES:
	case LOC_OPTIMIZED_OUT:
	case LOC_STATIC:
	  return GDB_INDEX_SYMBOL_KIND_VARIABLE;
	case LOC_CONST:
	  /* Note: It's currently impossible to recognize psyms as enum values
	     short of reading the type info.  For now punt.  */
	  return GDB_INDEX_SYMBOL_KIND_VARIABLE;
	default:
	  /* There are other LOC_FOO values that one might want to classify
	     as variables, but dwarf2read.c doesn't currently use them.  */
	  return GDB_INDEX_SYMBOL_KIND_OTHER;
	}
    case STRUCT_DOMAIN:
      return GDB_INDEX_SYMBOL_KIND_TYPE;
    default:
      return GDB_INDEX_SYMBOL_KIND_OTHER;
    }
}

/* Add a list of partial symbols to SYMTAB.  */

static void
write_psymbols (struct mapped_symtab *symtab,
		std::unordered_set<partial_symbol *> &psyms_seen,
		struct partial_symbol **psymp,
		int count,
		offset_type cu_index,
		int is_static)
{
  for (; count-- > 0; ++psymp)
    {
      struct partial_symbol *psym = *psymp;
      const char *name = psym->ginfo.search_name ();

      if (psym->ginfo.language () == language_ada)
	{
	  /* We want to ensure that the Ada main function's name appears
	     verbatim in the index.  However, this name will be of the
	     form "_ada_mumble", and will be rewritten by ada_decode.
	     So, recognize it specially here and add it to the index by
	     hand.  */
	  if (strcmp (main_name (), name) == 0)
	    {
	      gdb_index_symbol_kind kind = symbol_kind (psym);

	      add_index_entry (symtab, name, is_static, kind, cu_index);
	    }

	  /* In order for the index to work when read back into gdb, it
	     has to supply a funny form of the name: it should be the
	     encoded name, with any suffixes stripped.  Using the
	     ordinary encoded name will not work properly with the
	     searching logic in find_name_components_bounds; nor will
	     using the decoded name.  Furthermore, an Ada "verbatim"
	     name (of the form "<MumBle>") must be entered without the
	     angle brackets.  Note that the current index is unusual,
	     see PR symtab/24820 for details.  */
	  std::string decoded = ada_decode (name);
	  if (decoded[0] == '<')
	    name = (char *) obstack_copy0 (&symtab->m_string_obstack,
					   decoded.c_str () + 1,
					   decoded.length () - 2);
	  else
	    name = obstack_strdup (&symtab->m_string_obstack,
				   ada_encode (decoded.c_str ()));
	}

      /* Only add a given psymbol once.  */
      if (psyms_seen.insert (psym).second)
	{
	  gdb_index_symbol_kind kind = symbol_kind (psym);

	  add_index_entry (symtab, name, is_static, kind, cu_index);
	}
    }
}

/* A helper struct used when iterating over debug_types.  */
struct signatured_type_index_data
{
  signatured_type_index_data (data_buf &types_list_,
                              std::unordered_set<partial_symbol *> &psyms_seen_)
    : types_list (types_list_), psyms_seen (psyms_seen_)
  {}

  struct objfile *objfile;
  struct mapped_symtab *symtab;
  data_buf &types_list;
  std::unordered_set<partial_symbol *> &psyms_seen;
  int cu_index;
};

/* A helper function that writes a single signatured_type to an
   obstack.  */

static int
write_one_signatured_type (void **slot, void *d)
{
  struct signatured_type_index_data *info
    = (struct signatured_type_index_data *) d;
  struct signatured_type *entry = (struct signatured_type *) *slot;
  partial_symtab *psymtab = entry->per_cu.v.psymtab;

  write_psymbols (info->symtab,
		  info->psyms_seen,
		  (info->objfile->partial_symtabs->global_psymbols.data ()
		   + psymtab->globals_offset),
		  psymtab->n_global_syms, info->cu_index,
		  0);
  write_psymbols (info->symtab,
		  info->psyms_seen,
		  (info->objfile->partial_symtabs->static_psymbols.data ()
		   + psymtab->statics_offset),
		  psymtab->n_static_syms, info->cu_index,
		  1);

  info->types_list.append_uint (8, BFD_ENDIAN_LITTLE,
				to_underlying (entry->per_cu.sect_off));
  info->types_list.append_uint (8, BFD_ENDIAN_LITTLE,
				to_underlying (entry->type_offset_in_tu));
  info->types_list.append_uint (8, BFD_ENDIAN_LITTLE, entry->signature);

  ++info->cu_index;

  return 1;
}

/* Recurse into all "included" dependencies and count their symbols as
   if they appeared in this psymtab.  */

static void
recursively_count_psymbols (partial_symtab *psymtab,
			    size_t &psyms_seen)
{
  for (int i = 0; i < psymtab->number_of_dependencies; ++i)
    if (psymtab->dependencies[i]->user != NULL)
      recursively_count_psymbols (psymtab->dependencies[i],
				  psyms_seen);

  psyms_seen += psymtab->n_global_syms;
  psyms_seen += psymtab->n_static_syms;
}

/* Recurse into all "included" dependencies and write their symbols as
   if they appeared in this psymtab.  */

static void
recursively_write_psymbols (struct objfile *objfile,
			    partial_symtab *psymtab,
			    struct mapped_symtab *symtab,
			    std::unordered_set<partial_symbol *> &psyms_seen,
			    offset_type cu_index)
{
  int i;

  for (i = 0; i < psymtab->number_of_dependencies; ++i)
    if (psymtab->dependencies[i]->user != NULL)
      recursively_write_psymbols (objfile,
				  psymtab->dependencies[i],
				  symtab, psyms_seen, cu_index);

  write_psymbols (symtab,
		  psyms_seen,
		  (objfile->partial_symtabs->global_psymbols.data ()
		   + psymtab->globals_offset),
		  psymtab->n_global_syms, cu_index,
		  0);
  write_psymbols (symtab,
		  psyms_seen,
		  (objfile->partial_symtabs->static_psymbols.data ()
		   + psymtab->statics_offset),
		  psymtab->n_static_syms, cu_index,
		  1);
}

/* DWARF-5 .debug_names builder.  */
class debug_names
{
public:
  debug_names (dwarf2_per_objfile *per_objfile, bool is_dwarf64,
	       bfd_endian dwarf5_byte_order)
    : m_dwarf5_byte_order (dwarf5_byte_order),
      m_dwarf32 (dwarf5_byte_order),
      m_dwarf64 (dwarf5_byte_order),
      m_dwarf (is_dwarf64
	       ? static_cast<dwarf &> (m_dwarf64)
	       : static_cast<dwarf &> (m_dwarf32)),
      m_name_table_string_offs (m_dwarf.name_table_string_offs),
      m_name_table_entry_offs (m_dwarf.name_table_entry_offs),
      m_debugstrlookup (per_objfile)
  {}

  int dwarf5_offset_size () const
  {
    const bool dwarf5_is_dwarf64 = &m_dwarf == &m_dwarf64;
    return dwarf5_is_dwarf64 ? 8 : 4;
  }

  /* Is this symbol from DW_TAG_compile_unit or DW_TAG_type_unit?  */
  enum class unit_kind { cu, tu };

  /* Insert one symbol.  */
  void insert (const partial_symbol *psym, int cu_index, bool is_static,
	       unit_kind kind)
  {
    const int dwarf_tag = psymbol_tag (psym);
    if (dwarf_tag == 0)
      return;
    const char *name = psym->ginfo.search_name ();

    if (psym->ginfo.language () == language_ada)
      {
	/* We want to ensure that the Ada main function's name appears
	   verbatim in the index.  However, this name will be of the
	   form "_ada_mumble", and will be rewritten by ada_decode.
	   So, recognize it specially here and add it to the index by
	   hand.  */
	if (strcmp (main_name (), name) == 0)
	  {
	    const auto insertpair
	      = m_name_to_value_set.emplace (c_str_view (name),
					     std::set<symbol_value> ());
	    std::set<symbol_value> &value_set = insertpair.first->second;
	    value_set.emplace (symbol_value (dwarf_tag, cu_index, is_static,
					     kind));
	  }

	/* In order for the index to work when read back into gdb, it
	   has to supply a funny form of the name: it should be the
	   encoded name, with any suffixes stripped.  Using the
	   ordinary encoded name will not work properly with the
	   searching logic in find_name_components_bounds; nor will
	   using the decoded name.  Furthermore, an Ada "verbatim"
	   name (of the form "<MumBle>") must be entered without the
	   angle brackets.  Note that the current index is unusual,
	   see PR symtab/24820 for details.  */
	std::string decoded = ada_decode (name);
	if (decoded[0] == '<')
	  name = (char *) obstack_copy0 (&m_string_obstack,
					 decoded.c_str () + 1,
					 decoded.length () - 2);
	else
	  name = obstack_strdup (&m_string_obstack,
				 ada_encode (decoded.c_str ()));
      }

    const auto insertpair
      = m_name_to_value_set.emplace (c_str_view (name),
				     std::set<symbol_value> ());
    std::set<symbol_value> &value_set = insertpair.first->second;
    value_set.emplace (symbol_value (dwarf_tag, cu_index, is_static, kind));
  }

  /* Build all the tables.  All symbols must be already inserted.
     This function does not call file_write, caller has to do it
     afterwards.  */
  void build ()
  {
    /* Verify the build method has not be called twice.  */
    gdb_assert (m_abbrev_table.empty ());
    const size_t name_count = m_name_to_value_set.size ();
    m_bucket_table.resize
      (std::pow (2, std::ceil (std::log2 (name_count * 4 / 3))));
    m_hash_table.reserve (name_count);
    m_name_table_string_offs.reserve (name_count);
    m_name_table_entry_offs.reserve (name_count);

    /* Map each hash of symbol to its name and value.  */
    struct hash_it_pair
    {
      uint32_t hash;
      decltype (m_name_to_value_set)::const_iterator it;
    };
    std::vector<std::forward_list<hash_it_pair>> bucket_hash;
    bucket_hash.resize (m_bucket_table.size ());
    for (decltype (m_name_to_value_set)::const_iterator it
	   = m_name_to_value_set.cbegin ();
	 it != m_name_to_value_set.cend ();
	 ++it)
      {
	const char *const name = it->first.c_str ();
	const uint32_t hash = dwarf5_djb_hash (name);
	hash_it_pair hashitpair;
	hashitpair.hash = hash;
	hashitpair.it = it;
	auto &slot = bucket_hash[hash % bucket_hash.size()];
	slot.push_front (std::move (hashitpair));
      }
    for (size_t bucket_ix = 0; bucket_ix < bucket_hash.size (); ++bucket_ix)
      {
	const std::forward_list<hash_it_pair> &hashitlist
	  = bucket_hash[bucket_ix];
	if (hashitlist.empty ())
	  continue;
	uint32_t &bucket_slot = m_bucket_table[bucket_ix];
	/* The hashes array is indexed starting at 1.  */
	store_unsigned_integer (reinterpret_cast<gdb_byte *> (&bucket_slot),
				sizeof (bucket_slot), m_dwarf5_byte_order,
				m_hash_table.size () + 1);
	for (const hash_it_pair &hashitpair : hashitlist)
	  {
	    m_hash_table.push_back (0);
	    store_unsigned_integer (reinterpret_cast<gdb_byte *>
							(&m_hash_table.back ()),
				    sizeof (m_hash_table.back ()),
				    m_dwarf5_byte_order, hashitpair.hash);
	    const c_str_view &name = hashitpair.it->first;
	    const std::set<symbol_value> &value_set = hashitpair.it->second;
	    m_name_table_string_offs.push_back_reorder
	      (m_debugstrlookup.lookup (name.c_str ()));
	    m_name_table_entry_offs.push_back_reorder (m_entry_pool.size ());
	    gdb_assert (!value_set.empty ());
	    for (const symbol_value &value : value_set)
	      {
		int &idx = m_indexkey_to_idx[index_key (value.dwarf_tag,
							value.is_static,
							value.kind)];
		if (idx == 0)
		  {
		    idx = m_idx_next++;
		    m_abbrev_table.append_unsigned_leb128 (idx);
		    m_abbrev_table.append_unsigned_leb128 (value.dwarf_tag);
		    m_abbrev_table.append_unsigned_leb128
			      (value.kind == unit_kind::cu ? DW_IDX_compile_unit
							   : DW_IDX_type_unit);
		    m_abbrev_table.append_unsigned_leb128 (DW_FORM_udata);
		    m_abbrev_table.append_unsigned_leb128 (value.is_static
							   ? DW_IDX_GNU_internal
							   : DW_IDX_GNU_external);
		    m_abbrev_table.append_unsigned_leb128 (DW_FORM_flag_present);

		    /* Terminate attributes list.  */
		    m_abbrev_table.append_unsigned_leb128 (0);
		    m_abbrev_table.append_unsigned_leb128 (0);
		  }

		m_entry_pool.append_unsigned_leb128 (idx);
		m_entry_pool.append_unsigned_leb128 (value.cu_index);
	      }

	    /* Terminate the list of CUs.  */
	    m_entry_pool.append_unsigned_leb128 (0);
	  }
      }
    gdb_assert (m_hash_table.size () == name_count);

    /* Terminate tags list.  */
    m_abbrev_table.append_unsigned_leb128 (0);
  }

  /* Return .debug_names bucket count.  This must be called only after
     calling the build method.  */
  uint32_t bucket_count () const
  {
    /* Verify the build method has been already called.  */
    gdb_assert (!m_abbrev_table.empty ());
    const uint32_t retval = m_bucket_table.size ();

    /* Check for overflow.  */
    gdb_assert (retval == m_bucket_table.size ());
    return retval;
  }

  /* Return .debug_names names count.  This must be called only after
     calling the build method.  */
  uint32_t name_count () const
  {
    /* Verify the build method has been already called.  */
    gdb_assert (!m_abbrev_table.empty ());
    const uint32_t retval = m_hash_table.size ();

    /* Check for overflow.  */
    gdb_assert (retval == m_hash_table.size ());
    return retval;
  }

  /* Return number of bytes of .debug_names abbreviation table.  This
     must be called only after calling the build method.  */
  uint32_t abbrev_table_bytes () const
  {
    gdb_assert (!m_abbrev_table.empty ());
    return m_abbrev_table.size ();
  }

  /* Recurse into all "included" dependencies and store their symbols
     as if they appeared in this psymtab.  */
  void recursively_write_psymbols
    (struct objfile *objfile,
     partial_symtab *psymtab,
     std::unordered_set<partial_symbol *> &psyms_seen,
     int cu_index)
  {
    for (int i = 0; i < psymtab->number_of_dependencies; ++i)
      if (psymtab->dependencies[i]->user != NULL)
	recursively_write_psymbols
	  (objfile, psymtab->dependencies[i], psyms_seen, cu_index);

    write_psymbols (psyms_seen,
		    (objfile->partial_symtabs->global_psymbols.data ()
		     + psymtab->globals_offset),
		    psymtab->n_global_syms, cu_index, false, unit_kind::cu);
    write_psymbols (psyms_seen,
		    (objfile->partial_symtabs->static_psymbols.data ()
		     + psymtab->statics_offset),
		    psymtab->n_static_syms, cu_index, true, unit_kind::cu);
  }

  /* Return number of bytes the .debug_names section will have.  This
     must be called only after calling the build method.  */
  size_t bytes () const
  {
    /* Verify the build method has been already called.  */
    gdb_assert (!m_abbrev_table.empty ());
    size_t expected_bytes = 0;
    expected_bytes += m_bucket_table.size () * sizeof (m_bucket_table[0]);
    expected_bytes += m_hash_table.size () * sizeof (m_hash_table[0]);
    expected_bytes += m_name_table_string_offs.bytes ();
    expected_bytes += m_name_table_entry_offs.bytes ();
    expected_bytes += m_abbrev_table.size ();
    expected_bytes += m_entry_pool.size ();
    return expected_bytes;
  }

  /* Write .debug_names to FILE_NAMES and .debug_str addition to
     FILE_STR.  This must be called only after calling the build
     method.  */
  void file_write (FILE *file_names, FILE *file_str) const
  {
    /* Verify the build method has been already called.  */
    gdb_assert (!m_abbrev_table.empty ());
    ::file_write (file_names, m_bucket_table);
    ::file_write (file_names, m_hash_table);
    m_name_table_string_offs.file_write (file_names);
    m_name_table_entry_offs.file_write (file_names);
    m_abbrev_table.file_write (file_names);
    m_entry_pool.file_write (file_names);
    m_debugstrlookup.file_write (file_str);
  }

  /* A helper user data for write_one_signatured_type.  */
  class write_one_signatured_type_data
  {
  public:
    write_one_signatured_type_data (debug_names &nametable_,
                                    signatured_type_index_data &&info_)
    : nametable (nametable_), info (std::move (info_))
    {}
    debug_names &nametable;
    struct signatured_type_index_data info;
  };

  /* A helper function to pass write_one_signatured_type to
     htab_traverse_noresize.  */
  static int
  write_one_signatured_type (void **slot, void *d)
  {
    write_one_signatured_type_data *data = (write_one_signatured_type_data *) d;
    struct signatured_type_index_data *info = &data->info;
    struct signatured_type *entry = (struct signatured_type *) *slot;

    data->nametable.write_one_signatured_type (entry, info);

    return 1;
  }

private:

  /* Storage for symbol names mapping them to their .debug_str section
     offsets.  */
  class debug_str_lookup
  {
  public:

    /* Object constructor to be called for current DWARF2_PER_OBJFILE.
       All .debug_str section strings are automatically stored.  */
    debug_str_lookup (dwarf2_per_objfile *per_objfile)
      : m_abfd (per_objfile->objfile->obfd),
	m_per_objfile (per_objfile)
    {
      per_objfile->per_bfd->str.read (per_objfile->objfile);
      if (per_objfile->per_bfd->str.buffer == NULL)
	return;
      for (const gdb_byte *data = per_objfile->per_bfd->str.buffer;
	   data < (per_objfile->per_bfd->str.buffer
		   + per_objfile->per_bfd->str.size);)
	{
	  const char *const s = reinterpret_cast<const char *> (data);
	  const auto insertpair
	    = m_str_table.emplace (c_str_view (s),
				   data - per_objfile->per_bfd->str.buffer);
	  if (!insertpair.second)
	    complaint (_("Duplicate string \"%s\" in "
			 ".debug_str section [in module %s]"),
		       s, bfd_get_filename (m_abfd));
	  data += strlen (s) + 1;
	}
    }

    /* Return offset of symbol name S in the .debug_str section.  Add
       such symbol to the section's end if it does not exist there
       yet.  */
    size_t lookup (const char *s)
    {
      const auto it = m_str_table.find (c_str_view (s));
      if (it != m_str_table.end ())
	return it->second;
      const size_t offset = (m_per_objfile->per_bfd->str.size
			     + m_str_add_buf.size ());
      m_str_table.emplace (c_str_view (s), offset);
      m_str_add_buf.append_cstr0 (s);
      return offset;
    }

    /* Append the end of the .debug_str section to FILE.  */
    void file_write (FILE *file) const
    {
      m_str_add_buf.file_write (file);
    }

  private:
    std::unordered_map<c_str_view, size_t, c_str_view_hasher> m_str_table;
    bfd *const m_abfd;
    dwarf2_per_objfile *m_per_objfile;

    /* Data to add at the end of .debug_str for new needed symbol names.  */
    data_buf m_str_add_buf;
  };

  /* Container to map used DWARF tags to their .debug_names abbreviation
     tags.  */
  class index_key
  {
  public:
    index_key (int dwarf_tag_, bool is_static_, unit_kind kind_)
      : dwarf_tag (dwarf_tag_), is_static (is_static_), kind (kind_)
    {
    }

    bool
    operator== (const index_key &other) const
    {
      return (dwarf_tag == other.dwarf_tag && is_static == other.is_static
	      && kind == other.kind);
    }

    const int dwarf_tag;
    const bool is_static;
    const unit_kind kind;
  };

  /* Provide std::unordered_map::hasher for index_key.  */
  class index_key_hasher
  {
  public:
    size_t
    operator () (const index_key &key) const
    {
      return (std::hash<int>() (key.dwarf_tag) << 1) | key.is_static;
    }
  };

  /* Parameters of one symbol entry.  */
  class symbol_value
  {
  public:
    const int dwarf_tag, cu_index;
    const bool is_static;
    const unit_kind kind;

    symbol_value (int dwarf_tag_, int cu_index_, bool is_static_,
		  unit_kind kind_)
      : dwarf_tag (dwarf_tag_), cu_index (cu_index_), is_static (is_static_),
        kind (kind_)
    {}

    bool
    operator< (const symbol_value &other) const
    {
#define X(n) \
  do \
    { \
      if (n < other.n) \
	return true; \
      if (n > other.n) \
	return false; \
    } \
  while (0)
      X (dwarf_tag);
      X (is_static);
      X (kind);
      X (cu_index);
#undef X
      return false;
    }
  };

  /* Abstract base class to unify DWARF-32 and DWARF-64 name table
     output.  */
  class offset_vec
  {
  protected:
    const bfd_endian dwarf5_byte_order;
  public:
    explicit offset_vec (bfd_endian dwarf5_byte_order_)
      : dwarf5_byte_order (dwarf5_byte_order_)
    {}

    /* Call std::vector::reserve for NELEM elements.  */
    virtual void reserve (size_t nelem) = 0;

    /* Call std::vector::push_back with store_unsigned_integer byte
       reordering for ELEM.  */
    virtual void push_back_reorder (size_t elem) = 0;

    /* Return expected output size in bytes.  */
    virtual size_t bytes () const = 0;

    /* Write name table to FILE.  */
    virtual void file_write (FILE *file) const = 0;
  };

  /* Template to unify DWARF-32 and DWARF-64 output.  */
  template<typename OffsetSize>
  class offset_vec_tmpl : public offset_vec
  {
  public:
    explicit offset_vec_tmpl (bfd_endian dwarf5_byte_order_)
      : offset_vec (dwarf5_byte_order_)
    {}

    /* Implement offset_vec::reserve.  */
    void reserve (size_t nelem) override
    {
      m_vec.reserve (nelem);
    }

    /* Implement offset_vec::push_back_reorder.  */
    void push_back_reorder (size_t elem) override
    {
      m_vec.push_back (elem);
      /* Check for overflow.  */
      gdb_assert (m_vec.back () == elem);
      store_unsigned_integer (reinterpret_cast<gdb_byte *> (&m_vec.back ()),
			      sizeof (m_vec.back ()), dwarf5_byte_order, elem);
    }

    /* Implement offset_vec::bytes.  */
    size_t bytes () const override
    {
      return m_vec.size () * sizeof (m_vec[0]);
    }

    /* Implement offset_vec::file_write.  */
    void file_write (FILE *file) const override
    {
      ::file_write (file, m_vec);
    }

  private:
    std::vector<OffsetSize> m_vec;
  };

  /* Base class to unify DWARF-32 and DWARF-64 .debug_names output
     respecting name table width.  */
  class dwarf
  {
  public:
    offset_vec &name_table_string_offs, &name_table_entry_offs;

    dwarf (offset_vec &name_table_string_offs_,
	   offset_vec &name_table_entry_offs_)
      : name_table_string_offs (name_table_string_offs_),
	name_table_entry_offs (name_table_entry_offs_)
    {
    }
  };

  /* Template to unify DWARF-32 and DWARF-64 .debug_names output
     respecting name table width.  */
  template<typename OffsetSize>
  class dwarf_tmpl : public dwarf
  {
  public:
    explicit dwarf_tmpl (bfd_endian dwarf5_byte_order_)
      : dwarf (m_name_table_string_offs, m_name_table_entry_offs),
	m_name_table_string_offs (dwarf5_byte_order_),
	m_name_table_entry_offs (dwarf5_byte_order_)
    {}

  private:
    offset_vec_tmpl<OffsetSize> m_name_table_string_offs;
    offset_vec_tmpl<OffsetSize> m_name_table_entry_offs;
  };

  /* Try to reconstruct original DWARF tag for given partial_symbol.
     This function is not DWARF-5 compliant but it is sufficient for
     GDB as a DWARF-5 index consumer.  */
  static int psymbol_tag (const struct partial_symbol *psym)
  {
    domain_enum domain = psym->domain;
    enum address_class aclass = psym->aclass;

    switch (domain)
      {
      case VAR_DOMAIN:
	switch (aclass)
	  {
	  case LOC_BLOCK:
	    return DW_TAG_subprogram;
	  case LOC_TYPEDEF:
	    return DW_TAG_typedef;
	  case LOC_COMPUTED:
	  case LOC_CONST_BYTES:
	  case LOC_OPTIMIZED_OUT:
	  case LOC_STATIC:
	    return DW_TAG_variable;
	  case LOC_CONST:
	    /* Note: It's currently impossible to recognize psyms as enum values
	       short of reading the type info.  For now punt.  */
	    return DW_TAG_variable;
	  default:
	    /* There are other LOC_FOO values that one might want to classify
	       as variables, but dwarf2read.c doesn't currently use them.  */
	    return DW_TAG_variable;
	  }
      case STRUCT_DOMAIN:
	return DW_TAG_structure_type;
      case MODULE_DOMAIN:
	return DW_TAG_module;
      default:
	return 0;
      }
  }

  /* Call insert for all partial symbols and mark them in PSYMS_SEEN.  */
  void write_psymbols (std::unordered_set<partial_symbol *> &psyms_seen,
		       struct partial_symbol **psymp, int count, int cu_index,
		       bool is_static, unit_kind kind)
  {
    for (; count-- > 0; ++psymp)
      {
	struct partial_symbol *psym = *psymp;

	/* Only add a given psymbol once.  */
	if (psyms_seen.insert (psym).second)
	  insert (psym, cu_index, is_static, kind);
      }
  }

  /* A helper function that writes a single signatured_type
     to a debug_names.  */
  void
  write_one_signatured_type (struct signatured_type *entry,
			     struct signatured_type_index_data *info)
  {
    partial_symtab *psymtab = entry->per_cu.v.psymtab;

    write_psymbols (info->psyms_seen,
		    (info->objfile->partial_symtabs->global_psymbols.data ()
		     + psymtab->globals_offset),
		    psymtab->n_global_syms, info->cu_index, false,
		    unit_kind::tu);
    write_psymbols (info->psyms_seen,
		    (info->objfile->partial_symtabs->static_psymbols.data ()
		     + psymtab->statics_offset),
		    psymtab->n_static_syms, info->cu_index, true,
		    unit_kind::tu);

    info->types_list.append_uint (dwarf5_offset_size (), m_dwarf5_byte_order,
				  to_underlying (entry->per_cu.sect_off));

    ++info->cu_index;
  }

  /* Store value of each symbol.  */
  std::unordered_map<c_str_view, std::set<symbol_value>, c_str_view_hasher>
    m_name_to_value_set;

  /* Tables of DWARF-5 .debug_names.  They are in object file byte
     order.  */
  std::vector<uint32_t> m_bucket_table;
  std::vector<uint32_t> m_hash_table;

  const bfd_endian m_dwarf5_byte_order;
  dwarf_tmpl<uint32_t> m_dwarf32;
  dwarf_tmpl<uint64_t> m_dwarf64;
  dwarf &m_dwarf;
  offset_vec &m_name_table_string_offs, &m_name_table_entry_offs;
  debug_str_lookup m_debugstrlookup;

  /* Map each used .debug_names abbreviation tag parameter to its
     index value.  */
  std::unordered_map<index_key, int, index_key_hasher> m_indexkey_to_idx;

  /* Next unused .debug_names abbreviation tag for
     m_indexkey_to_idx.  */
  int m_idx_next = 1;

  /* .debug_names abbreviation table.  */
  data_buf m_abbrev_table;

  /* .debug_names entry pool.  */
  data_buf m_entry_pool;

  /* Temporary storage for Ada names.  */
  auto_obstack m_string_obstack;
};

/* Return iff any of the needed offsets does not fit into 32-bit
   .debug_names section.  */

static bool
check_dwarf64_offsets (dwarf2_per_objfile *per_objfile)
{
  for (dwarf2_per_cu_data *per_cu : per_objfile->per_bfd->all_comp_units)
    {
      if (to_underlying (per_cu->sect_off) >= (static_cast<uint64_t> (1) << 32))
	return true;
    }
  for (const signatured_type *sigtype : per_objfile->per_bfd->all_type_units)
    {
      const dwarf2_per_cu_data &per_cu = sigtype->per_cu;

      if (to_underlying (per_cu.sect_off) >= (static_cast<uint64_t> (1) << 32))
	return true;
    }
  return false;
}

/* The psyms_seen set is potentially going to be largish (~40k
   elements when indexing a -g3 build of GDB itself).  Estimate the
   number of elements in order to avoid too many rehashes, which
   require rebuilding buckets and thus many trips to
   malloc/free.  */

static size_t
psyms_seen_size (dwarf2_per_objfile *per_objfile)
{
  size_t psyms_count = 0;
  for (dwarf2_per_cu_data *per_cu : per_objfile->per_bfd->all_comp_units)
    {
      partial_symtab *psymtab = per_cu->v.psymtab;

      if (psymtab != NULL && psymtab->user == NULL)
	recursively_count_psymbols (psymtab, psyms_count);
    }
  /* Generating an index for gdb itself shows a ratio of
     TOTAL_SEEN_SYMS/UNIQUE_SYMS or ~5.  4 seems like a good bet.  */
  return psyms_count / 4;
}

/* Assert that FILE's size is EXPECTED_SIZE.  Assumes file's seek
   position is at the end of the file.  */

static void
assert_file_size (FILE *file, size_t expected_size)
{
  const auto file_size = ftell (file);
  if (file_size == -1)
    perror_with_name (("ftell"));
  gdb_assert (file_size == expected_size);
}

/* Write a gdb index file to OUT_FILE from all the sections passed as
   arguments.  */

static void
write_gdbindex_1 (FILE *out_file,
		  const data_buf &cu_list,
		  const data_buf &types_cu_list,
		  const data_buf &addr_vec,
		  const data_buf &symtab_vec,
		  const data_buf &constant_pool)
{
  data_buf contents;
  const offset_type size_of_header = 6 * sizeof (offset_type);
  offset_type total_len = size_of_header;

  /* The version number.  */
  contents.append_data (MAYBE_SWAP (8));

  /* The offset of the CU list from the start of the file.  */
  contents.append_data (MAYBE_SWAP (total_len));
  total_len += cu_list.size ();

  /* The offset of the types CU list from the start of the file.  */
  contents.append_data (MAYBE_SWAP (total_len));
  total_len += types_cu_list.size ();

  /* The offset of the address table from the start of the file.  */
  contents.append_data (MAYBE_SWAP (total_len));
  total_len += addr_vec.size ();

  /* The offset of the symbol table from the start of the file.  */
  contents.append_data (MAYBE_SWAP (total_len));
  total_len += symtab_vec.size ();

  /* The offset of the constant pool from the start of the file.  */
  contents.append_data (MAYBE_SWAP (total_len));
  total_len += constant_pool.size ();

  gdb_assert (contents.size () == size_of_header);

  contents.file_write (out_file);
  cu_list.file_write (out_file);
  types_cu_list.file_write (out_file);
  addr_vec.file_write (out_file);
  symtab_vec.file_write (out_file);
  constant_pool.file_write (out_file);

  assert_file_size (out_file, total_len);
}

/* Write contents of a .gdb_index section for OBJFILE into OUT_FILE.
   If OBJFILE has an associated dwz file, write contents of a .gdb_index
   section for that dwz file into DWZ_OUT_FILE.  If OBJFILE does not have an
   associated dwz file, DWZ_OUT_FILE must be NULL.  */

static void
write_gdbindex (dwarf2_per_objfile *per_objfile, FILE *out_file,
		FILE *dwz_out_file)
{
  struct objfile *objfile = per_objfile->objfile;
  mapped_symtab symtab;
  data_buf objfile_cu_list;
  data_buf dwz_cu_list;

  /* While we're scanning CU's create a table that maps a psymtab pointer
     (which is what addrmap records) to its index (which is what is recorded
     in the index file).  This will later be needed to write the address
     table.  */
  psym_index_map cu_index_htab;
  cu_index_htab.reserve (per_objfile->per_bfd->all_comp_units.size ());

  /* The CU list is already sorted, so we don't need to do additional
     work here.  Also, the debug_types entries do not appear in
     all_comp_units, but only in their own hash table.  */

  std::unordered_set<partial_symbol *> psyms_seen
    (psyms_seen_size (per_objfile));
  for (int i = 0; i < per_objfile->per_bfd->all_comp_units.size (); ++i)
    {
      dwarf2_per_cu_data *per_cu = per_objfile->per_bfd->all_comp_units[i];
      partial_symtab *psymtab = per_cu->v.psymtab;

      if (psymtab != NULL)
	{
	  if (psymtab->user == NULL)
	    recursively_write_psymbols (objfile, psymtab, &symtab,
					psyms_seen, i);

	  const auto insertpair = cu_index_htab.emplace (psymtab, i);
	  gdb_assert (insertpair.second);
	}

      /* The all_comp_units list contains CUs read from the objfile as well as
	 from the eventual dwz file.  We need to place the entry in the
	 corresponding index.  */
      data_buf &cu_list = per_cu->is_dwz ? dwz_cu_list : objfile_cu_list;
      cu_list.append_uint (8, BFD_ENDIAN_LITTLE,
			   to_underlying (per_cu->sect_off));
      cu_list.append_uint (8, BFD_ENDIAN_LITTLE, per_cu->length);
    }

  /* Dump the address map.  */
  data_buf addr_vec;
  write_address_map (objfile, addr_vec, cu_index_htab);

  /* Write out the .debug_type entries, if any.  */
  data_buf types_cu_list;
  if (per_objfile->per_bfd->signatured_types)
    {
      signatured_type_index_data sig_data (types_cu_list,
					   psyms_seen);

      sig_data.objfile = objfile;
      sig_data.symtab = &symtab;
      sig_data.cu_index = per_objfile->per_bfd->all_comp_units.size ();
      htab_traverse_noresize (per_objfile->per_bfd->signatured_types.get (),
			      write_one_signatured_type, &sig_data);
    }

  /* Now that we've processed all symbols we can shrink their cu_indices
     lists.  */
  uniquify_cu_indices (&symtab);

  data_buf symtab_vec, constant_pool;
  write_hash_table (&symtab, symtab_vec, constant_pool);

  write_gdbindex_1(out_file, objfile_cu_list, types_cu_list, addr_vec,
		   symtab_vec, constant_pool);

  if (dwz_out_file != NULL)
    write_gdbindex_1 (dwz_out_file, dwz_cu_list, {}, {}, {}, {});
  else
    gdb_assert (dwz_cu_list.empty ());
}

/* DWARF-5 augmentation string for GDB's DW_IDX_GNU_* extension.  */
static const gdb_byte dwarf5_gdb_augmentation[] = { 'G', 'D', 'B', 0 };

/* Write a new .debug_names section for OBJFILE into OUT_FILE, write
   needed addition to .debug_str section to OUT_FILE_STR.  Return how
   many bytes were expected to be written into OUT_FILE.  */

static void
write_debug_names (dwarf2_per_objfile *per_objfile,
		   FILE *out_file, FILE *out_file_str)
{
  const bool dwarf5_is_dwarf64 = check_dwarf64_offsets (per_objfile);
  struct objfile *objfile = per_objfile->objfile;
  const enum bfd_endian dwarf5_byte_order
    = gdbarch_byte_order (objfile->arch ());

  /* The CU list is already sorted, so we don't need to do additional
     work here.  Also, the debug_types entries do not appear in
     all_comp_units, but only in their own hash table.  */
  data_buf cu_list;
  debug_names nametable (per_objfile, dwarf5_is_dwarf64, dwarf5_byte_order);
  std::unordered_set<partial_symbol *>
    psyms_seen (psyms_seen_size (per_objfile));
  for (int i = 0; i < per_objfile->per_bfd->all_comp_units.size (); ++i)
    {
      const dwarf2_per_cu_data *per_cu = per_objfile->per_bfd->all_comp_units[i];
      partial_symtab *psymtab = per_cu->v.psymtab;

      /* CU of a shared file from 'dwz -m' may be unused by this main
	 file.  It may be referenced from a local scope but in such
	 case it does not need to be present in .debug_names.  */
      if (psymtab == NULL)
	continue;

      if (psymtab->user == NULL)
	nametable.recursively_write_psymbols (objfile, psymtab, psyms_seen, i);

      cu_list.append_uint (nametable.dwarf5_offset_size (), dwarf5_byte_order,
			   to_underlying (per_cu->sect_off));
    }

  /* Write out the .debug_type entries, if any.  */
  data_buf types_cu_list;
  if (per_objfile->per_bfd->signatured_types)
    {
      debug_names::write_one_signatured_type_data sig_data (nametable,
			signatured_type_index_data (types_cu_list, psyms_seen));

      sig_data.info.objfile = objfile;
      /* It is used only for gdb_index.  */
      sig_data.info.symtab = nullptr;
      sig_data.info.cu_index = 0;
      htab_traverse_noresize (per_objfile->per_bfd->signatured_types.get (),
			      debug_names::write_one_signatured_type,
			      &sig_data);
    }

  nametable.build ();

  /* No addr_vec - DWARF-5 uses .debug_aranges generated by GCC.  */

  const offset_type bytes_of_header
    = ((dwarf5_is_dwarf64 ? 12 : 4)
       + 2 + 2 + 7 * 4
       + sizeof (dwarf5_gdb_augmentation));
  size_t expected_bytes = 0;
  expected_bytes += bytes_of_header;
  expected_bytes += cu_list.size ();
  expected_bytes += types_cu_list.size ();
  expected_bytes += nametable.bytes ();
  data_buf header;

  if (!dwarf5_is_dwarf64)
    {
      const uint64_t size64 = expected_bytes - 4;
      gdb_assert (size64 < 0xfffffff0);
      header.append_uint (4, dwarf5_byte_order, size64);
    }
  else
    {
      header.append_uint (4, dwarf5_byte_order, 0xffffffff);
      header.append_uint (8, dwarf5_byte_order, expected_bytes - 12);
    }

  /* The version number.  */
  header.append_uint (2, dwarf5_byte_order, 5);

  /* Padding.  */
  header.append_uint (2, dwarf5_byte_order, 0);

  /* comp_unit_count - The number of CUs in the CU list.  */
  header.append_uint (4, dwarf5_byte_order,
		      per_objfile->per_bfd->all_comp_units.size ());

  /* local_type_unit_count - The number of TUs in the local TU
     list.  */
  header.append_uint (4, dwarf5_byte_order,
		      per_objfile->per_bfd->all_type_units.size ());

  /* foreign_type_unit_count - The number of TUs in the foreign TU
     list.  */
  header.append_uint (4, dwarf5_byte_order, 0);

  /* bucket_count - The number of hash buckets in the hash lookup
     table.  */
  header.append_uint (4, dwarf5_byte_order, nametable.bucket_count ());

  /* name_count - The number of unique names in the index.  */
  header.append_uint (4, dwarf5_byte_order, nametable.name_count ());

  /* abbrev_table_size - The size in bytes of the abbreviations
     table.  */
  header.append_uint (4, dwarf5_byte_order, nametable.abbrev_table_bytes ());

  /* augmentation_string_size - The size in bytes of the augmentation
     string.  This value is rounded up to a multiple of 4.  */
  static_assert (sizeof (dwarf5_gdb_augmentation) % 4 == 0, "");
  header.append_uint (4, dwarf5_byte_order, sizeof (dwarf5_gdb_augmentation));
  header.append_data (dwarf5_gdb_augmentation);

  gdb_assert (header.size () == bytes_of_header);

  header.file_write (out_file);
  cu_list.file_write (out_file);
  types_cu_list.file_write (out_file);
  nametable.file_write (out_file, out_file_str);

  assert_file_size (out_file, expected_bytes);
}

/* This represents an index file being written (work-in-progress).

   The data is initially written to a temporary file.  When the finalize method
   is called, the file is closed and moved to its final location.

   On failure (if this object is being destroyed with having called finalize),
   the temporary file is closed and deleted.  */

struct index_wip_file
{
  index_wip_file (const char *dir, const char *basename,
		  const char *suffix)
  {
    filename = (std::string (dir) + SLASH_STRING + basename
    		+ suffix);

    filename_temp = make_temp_filename (filename);

    scoped_fd out_file_fd (gdb_mkostemp_cloexec (filename_temp.data (),
						 O_BINARY));
    if (out_file_fd.get () == -1)
      perror_with_name (("mkstemp"));

    out_file = out_file_fd.to_file ("wb");

    if (out_file == nullptr)
      error (_("Can't open `%s' for writing"), filename_temp.data ());

    unlink_file.emplace (filename_temp.data ());
  }

  void finalize ()
  {
    /* We want to keep the file.  */
    unlink_file->keep ();

    /* Close and move the str file in place.  */
    unlink_file.reset ();
    if (rename (filename_temp.data (), filename.c_str ()) != 0)
      perror_with_name (("rename"));
  }

  std::string filename;
  gdb::char_vector filename_temp;

  /* Order matters here; we want FILE to be closed before
     FILENAME_TEMP is unlinked, because on MS-Windows one cannot
     delete a file that is still open.  So, we wrap the unlinker in an
     optional and emplace it once we know the file name.  */
  gdb::optional<gdb::unlinker> unlink_file;

  gdb_file_up out_file;
};

/* See dwarf-index-write.h.  */

void
write_psymtabs_to_index (dwarf2_per_objfile *per_objfile, const char *dir,
			 const char *basename, const char *dwz_basename,
			 dw_index_kind index_kind)
{
  struct objfile *objfile = per_objfile->objfile;

  if (per_objfile->per_bfd->using_index)
    error (_("Cannot use an index to create the index"));

  if (per_objfile->per_bfd->types.size () > 1)
    error (_("Cannot make an index when the file has multiple .debug_types sections"));

  if (!objfile->partial_symtabs->psymtabs
      || !objfile->partial_symtabs->psymtabs_addrmap)
    return;

  struct stat st;
  if (stat (objfile_name (objfile), &st) < 0)
    perror_with_name (objfile_name (objfile));

  const char *index_suffix = (index_kind == dw_index_kind::DEBUG_NAMES
			      ? INDEX5_SUFFIX : INDEX4_SUFFIX);

  index_wip_file objfile_index_wip (dir, basename, index_suffix);
  gdb::optional<index_wip_file> dwz_index_wip;

  if (dwz_basename != NULL)
      dwz_index_wip.emplace (dir, dwz_basename, index_suffix);

  if (index_kind == dw_index_kind::DEBUG_NAMES)
    {
      index_wip_file str_wip_file (dir, basename, DEBUG_STR_SUFFIX);

      write_debug_names (per_objfile, objfile_index_wip.out_file.get (),
			 str_wip_file.out_file.get ());

      str_wip_file.finalize ();
    }
  else
    write_gdbindex (per_objfile, objfile_index_wip.out_file.get (),
		    (dwz_index_wip.has_value ()
		     ? dwz_index_wip->out_file.get () : NULL));

  objfile_index_wip.finalize ();

  if (dwz_index_wip.has_value ())
    dwz_index_wip->finalize ();
}

/* Implementation of the `save gdb-index' command.

   Note that the .gdb_index file format used by this command is
   documented in the GDB manual.  Any changes here must be documented
   there.  */

static void
save_gdb_index_command (const char *arg, int from_tty)
{
  const char dwarf5space[] = "-dwarf-5 ";
  dw_index_kind index_kind = dw_index_kind::GDB_INDEX;

  if (!arg)
    arg = "";

  arg = skip_spaces (arg);
  if (strncmp (arg, dwarf5space, strlen (dwarf5space)) == 0)
    {
      index_kind = dw_index_kind::DEBUG_NAMES;
      arg += strlen (dwarf5space);
      arg = skip_spaces (arg);
    }

  if (!*arg)
    error (_("usage: save gdb-index [-dwarf-5] DIRECTORY"));

  for (objfile *objfile : current_program_space->objfiles ())
    {
      struct stat st;

      /* If the objfile does not correspond to an actual file, skip it.  */
      if (stat (objfile_name (objfile), &st) < 0)
	continue;

      dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

      if (per_objfile != NULL)
	{
	  try
	    {
	      const char *basename = lbasename (objfile_name (objfile));
	      const dwz_file *dwz = dwarf2_get_dwz_file (per_objfile->per_bfd);
	      const char *dwz_basename = NULL;

	      if (dwz != NULL)
		dwz_basename = lbasename (dwz->filename ());

	      write_psymtabs_to_index (per_objfile, arg, basename, dwz_basename,
				       index_kind);
	    }
	  catch (const gdb_exception_error &except)
	    {
	      exception_fprintf (gdb_stderr, except,
				 _("Error while writing index for `%s': "),
				 objfile_name (objfile));
	    }
	    }

    }
}

void _initialize_dwarf_index_write ();
void
_initialize_dwarf_index_write ()
{
  cmd_list_element *c = add_cmd ("gdb-index", class_files,
				 save_gdb_index_command, _("\
Save a gdb-index file.\n\
Usage: save gdb-index [-dwarf-5] DIRECTORY\n\
\n\
No options create one file with .gdb-index extension for pre-DWARF-5\n\
compatible .gdb_index section.  With -dwarf-5 creates two files with\n\
extension .debug_names and .debug_str for DWARF-5 .debug_names section."),
	       &save_cmdlist);
  set_cmd_completer (c, filename_completer);
}
