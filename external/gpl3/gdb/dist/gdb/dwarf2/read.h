/* DWARF 2 debugging format support for GDB.

   Copyright (C) 1994-2025 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_READ_H
#define GDB_DWARF2_READ_H

#if CXX_STD_THREAD
#include <mutex>
#endif
#include <queue>
#include "dwarf2/abbrev.h"
#include "dwarf2/unit-head.h"
#include "dwarf2/file-and-dir.h"
#include "dwarf2/index-cache.h"
#include "dwarf2/mapped-index.h"
#include "dwarf2/section.h"
#include "dwarf2/cu.h"
#include "dwarf2/dwz.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/packed.h"

/* Hold 'maintenance (set|show) dwarf' commands.  */
extern struct cmd_list_element *set_dwarf_cmdlist;
extern struct cmd_list_element *show_dwarf_cmdlist;

struct tu_stats
{
  int nr_uniq_abbrev_tables = 0;
  int nr_symtabs = 0;
  int nr_symtab_sharers = 0;
  int nr_stmt_less_type_units = 0;
  int nr_all_type_units_reallocs = 0;
};

struct abbrev_table_cache;
struct dwarf2_cu;
struct dwarf2_debug_sections;
struct dwarf2_per_bfd;
struct dwarf2_per_cu;
struct mapped_index;
struct mapped_debug_names;
struct signatured_type;

/* One item on the queue of compilation units to read in full symbols
   for.  */
struct dwarf2_queue_item
{
  dwarf2_queue_item (dwarf2_per_cu *cu, dwarf2_per_objfile *per_objfile)
    : per_cu (cu),
      per_objfile (per_objfile)  {
  }

  ~dwarf2_queue_item ();

  DISABLE_COPY_AND_ASSIGN (dwarf2_queue_item);

  dwarf2_per_cu *per_cu;
  dwarf2_per_objfile *per_objfile;
};

/* A struct that can be used as a hash key for tables based on DW_AT_stmt_list.
   This includes type_unit_group and quick_file_names.  */

struct stmt_list_hash
{
  bool operator== (const stmt_list_hash &other) const noexcept;

  /* The DWO unit this table is from or NULL if there is none.  */
  struct dwo_unit *dwo_unit;

  /* Offset in .debug_line or .debug_line.dwo.  */
  sect_offset line_sect_off;
};

struct stmt_list_hash_hash
{
  using is_avalanching = void;

  std::uint64_t operator() (const stmt_list_hash &key) const noexcept;
};

/* A deleter for dwarf2_per_cu that knows to downcast to signatured_type as
   appropriate.  This approach lets us avoid a virtual destructor, which saves
   a bit of space.  */

struct dwarf2_per_cu_deleter
{
  void operator() (dwarf2_per_cu *data);
};

/* A specialization of unique_ptr for dwarf2_per_cu and subclasses.  */
using dwarf2_per_cu_up = std::unique_ptr<dwarf2_per_cu, dwarf2_per_cu_deleter>;

/* Persistent data held for a compilation unit, even when not
   processing it.  We put a pointer to this structure in the
   psymtab.  */

struct dwarf2_per_cu
{
  /* LENGTH is the length of the unit.  If the value is 0, it means it is not
     known, and may be set later using the set_length method.  */
  dwarf2_per_cu (dwarf2_per_bfd *per_bfd, dwarf2_section_info *section,
		 sect_offset sect_off, unsigned int length, bool is_dwz)
    : sect_off (sect_off),
      m_length (length),
      is_debug_types (false),
      is_dwz (is_dwz),
      reading_dwo_directly (false),
      tu_read (false),
      lto_artificial (false),
      queued (false),
      mark (false),
      files_read (false),
      scanned (false),
      section (section),
      per_bfd (per_bfd)
  {
    gdb_assert (per_bfd != nullptr);
    gdb_assert (section != nullptr);
  }

  /* The start offset of this unit.

     If the unit is located in a DWO file, and has a corresponding skeleton
     unit in the main file, this is the offset of the skeleton unit in its
     containing section in the main file.

     If the unit is located in a DWO file, but has no corresponding skeleton
     unit in the main file, this is the offset of the unit in its containing
     section in the DWO file.  */
  sect_offset sect_off;

private:
  unsigned int m_length = 0;

public:
  /* Non-zero if this CU is from .debug_types.
     Struct dwarf2_per_cu is contained in struct signatured_type iff
     this is non-zero.  */
  unsigned int is_debug_types : 1;

  /* Non-zero if this CU is from the .dwz file.  */
  unsigned int is_dwz : 1;

  /* Non-zero if reading a TU directly from a DWO file, bypassing the stub.
     This flag is only valid if is_debug_types is true.
     We can't read a CU directly from a DWO file: There are required
     attributes in the stub.  */
  unsigned int reading_dwo_directly : 1;

  /* Non-zero if the TU has been read.
     This is used to assist the "Stay in DWO Optimization" for Fission:
     When reading a DWO, it's faster to read TUs from the DWO instead of
     fetching them from random other DWOs (due to comdat folding).
     If the TU has already been read, the optimization is unnecessary
     (and unwise - we don't want to change where gdb thinks the TU lives
     "midflight").
     This flag is only valid if is_debug_types is true.  */
  unsigned int tu_read : 1;

  /* Non-zero if the CU is produced by GCC and has name "<artificial>".  GCC
     uses this to indicate that the CU does not correspond to a single source
     file.  GCC produces this type of CU during LTO.  */
  unsigned int lto_artificial : 1;

  /* Wrap the following in struct packed instead of bitfields to avoid
     data races when the bitfields end up on the same memory location
     (per C++ memory model).  */

  /* If addresses have been read for this CU (usually from
     .debug_aranges), then this flag is set.  */
  packed<bool, 1> addresses_seen = false;

  /* Flag indicating this compilation unit will be read in before
     any of the current compilation units are processed.  */
  packed<bool, 1> queued;

  /* A temporary mark bit used when iterating over all CUs in
     expand_symtabs_matching.  */
  packed<unsigned int, 1> mark;

  /* True if we've tried to read the file table.  There will be no
     point in trying to read it again next time.  */
  packed<bool, 1> files_read;

private:
  /* The unit type of this CU.  */
  std::atomic<packed<dwarf_unit_type, 1>> m_unit_type {(dwarf_unit_type)0};

  /* The language of this CU.  */
  std::atomic<packed<language, LANGUAGE_BYTES>> m_lang {language_unknown};

  /* The original DW_LANG_* value of the CU, as provided to us by
     DW_AT_language.  It is interesting to keep this value around in cases where
     we can't use the values from the language enum, as the mapping to them is
     lossy, and, while that is usually fine, things like the index have an
     understandable bias towards not exposing internal GDB structures to the
     outside world, and so prefer to use DWARF constants in their stead. */
  std::atomic<packed<dwarf_source_language, 2>> m_dw_lang
       { (dwarf_source_language) 0 };

public:
  /* True if this CU has been scanned by the indexer; false if
     not.  */
  std::atomic<bool> scanned;

private:
  /* Sizes for an address, an offset, and a section offset.  These fields are
     set by cutu_reader when the unit is read.  */
  std::uint8_t m_addr_size = 0;
  std::uint8_t m_offset_size = 0;
  std::uint8_t m_ref_addr_size = 0;

public:
  /* Our index in the unshared "symtabs" vector.  */
  unsigned index = 0;

  /* The section this unit lives in.

     If the unit is located in a DWO file, and has a corresponding skeleton unit
     in the main file, this is the section of the skeleton unit in the main
     file.

     If the unit is located in a DWO file, but has no corresponding skeleton
     unit in the main file, this is the section of the unit in the DWO
     file.  */
  struct dwarf2_section_info *section = nullptr;

  /* Backlink to the owner of this.  */
  dwarf2_per_bfd *per_bfd;

  /* The file and directory for this CU.  This is cached so that we
     don't need to re-examine the DWO in some situations.  This may be
     nullptr, depending on the CU; for example a partial unit won't
     have one.  */
  file_and_directory_up fnd;

  /* The file table.  This can be NULL if there was no file table
     or it's currently not read in.
     NOTE: This points into dwarf2_per_objfile->per_bfd->quick_file_names_table.  */
  struct quick_file_names *file_names = nullptr;

  /* The CUs we import using DW_TAG_imported_unit.

     This is also used to work around a difference between the way gold
     generates .gdb_index version <=7 and the way gdb does.  Arguably this
     is a gold bug.  For symbols coming from TUs, gold records in the index
     the CU that includes the TU instead of the TU itself.  This breaks
     dw2_lookup_symbol: It assumes that if the index says symbol X lives
     in CU/TU Y, then one need only expand Y and a subsequent lookup in Y
     will find X.  Alas TUs live in their own symtab, so after expanding CU Y
     we need to look in TU Z to find X.  Fortunately, this is akin to
     DW_TAG_imported_unit, so we just use the same mechanism: For
     .gdb_index version <=7 this also records the TUs that the CU referred
     to.  Concurrently with this change gdb was modified to emit version 8
     indices so we only pay a price for gold generated indices.
     http://sourceware.org/bugzilla/show_bug.cgi?id=15021.  */
  std::vector<dwarf2_per_cu *> imported_symtabs;

  /* Return the address size given in the compilation unit header for
     this CU.  */
  std::uint8_t addr_size () const
  {
    gdb_assert (m_addr_size != 0);
    return m_addr_size;
  }

  /* Set the address size given in the compilation unit header for
     this CU.  */
  void set_addr_size (std::uint8_t addr_size)
  { m_addr_size = addr_size; }

  /* Return the offset size given in the compilation unit header for
     this CU.  */
  std::uint8_t offset_size () const
  {
    gdb_assert (m_offset_size != 0);
    return m_offset_size;
  }

  /* Set the offset size given in the compilation unit header for
     this CU.  */
  void set_offset_size (std::uint8_t offset_size)
  { m_offset_size = offset_size; }

  /* Return the DW_FORM_ref_addr size given in the compilation unit
     header for this CU.  */
  std::uint8_t ref_addr_size () const
  {
    gdb_assert (m_ref_addr_size != 0);
    return m_ref_addr_size;
  }

  /* Return the DW_FORM_ref_addr size given in the compilation unit
     header for this CU.  */
  void set_ref_addr_size (std::uint8_t ref_addr_size)
  { m_ref_addr_size = ref_addr_size; }

  /* Return length of this CU.  */
  unsigned int length () const
  {
    /* Make sure it's set already.  */
    gdb_assert (m_length != 0);
    return m_length;
  }

  /* Return true if the length of this CU has been set.  */
  bool length_is_set () const
  { return m_length != 0; }

  void set_length (unsigned int length, bool strict_p = true)
  {
    if (m_length == 0)
      /* Set if not set already.  */
      m_length = length;
    else if (strict_p)
      /* If already set, verify that it's the same value.  */
      gdb_assert (m_length == length);
  }

  dwarf_unit_type unit_type (bool strict_p = true) const
  {
    dwarf_unit_type ut = m_unit_type.load ();
    if (strict_p)
      gdb_assert (ut != 0);
    return ut;
  }

  void set_unit_type (dwarf_unit_type unit_type)
  {
    /* Set if not set already.  */
    packed<dwarf_unit_type, 1> nope = (dwarf_unit_type)0;
    if (m_unit_type.compare_exchange_strong (nope, unit_type))
      return;

    /* If already set, verify that it's the same value.  */
    nope = unit_type;
    if (m_unit_type.compare_exchange_strong (nope, unit_type))
      return;
    gdb_assert_not_reached ();
  }

  enum language lang (bool strict_p = true) const
  {
    enum language l = m_lang.load ();
    if (strict_p)
      gdb_assert (l != language_unknown);
    return l;
  }

  /* Make sure that m_lang != language_unknown.  */
  void ensure_lang (dwarf2_per_objfile *per_objfile);

  /* Return the language of this CU, as a DWARF DW_LANG_* value.  This
     may be 0 in some situations.  */
  dwarf_source_language dw_lang () const
  { return m_dw_lang.load (); }

  /* Set the language of this CU.  LANG is the language in gdb terms,
     and DW_LANG is the language as a DW_LANG_* value.  These may
     differ, as DW_LANG can be 0 for included units, whereas in this
     situation LANG would be set by the importing CU.  */
  void set_lang (enum language lang, dwarf_source_language dw_lang);

  /* Return true if the CU may be a multi-language CU.  */

  bool maybe_multi_language () const
  {
    enum language lang = this->lang ();

    if (!lto_artificial)
      /* Assume multi-language CUs are generated only by GCC LTO.  */
      return false;

    /* If GCC mixes different languages in an artificial LTO CU, it labels it C.
       The exception to this is when it mixes C and C++, which it labels it C++.
       For now, we don't consider the latter a multi-language CU.  */
    return lang == language_c;
  }

  /* Free any cached file names.  */
  void free_cached_file_names ();
};

/* Entry in the signatured_types hash table.  */

struct signatured_type : public dwarf2_per_cu
{
  signatured_type (dwarf2_per_bfd *per_bfd, dwarf2_section_info *section,
		   sect_offset sect_off, unsigned int length, bool is_dwz,
		   ULONGEST signature)
    : dwarf2_per_cu (per_bfd, section, sect_off, length, is_dwz),
      signature (signature)
  {
    this->is_debug_types = true;
  }

  /* The type's signature.  */
  ULONGEST signature;

  /* Offset in the TU of the type's DIE, as read from the TU header.
     If this TU is a DWO stub and the definition lives in a DWO file
     (specified by DW_AT_GNU_dwo_name), this value is unusable.  */
  cu_offset type_offset_in_tu {};

  /* Offset in the section of the type's DIE.
     If the definition lives in a DWO file, this is the offset in the
     .debug_types.dwo section.
     The value is zero until the actual value is known.
     Zero is otherwise not a valid section offset.  */
  sect_offset type_offset_in_section {};

  /* Type units are grouped by their DW_AT_stmt_list entry so that they
     can share them.  This is the key of the group this type unit is part
     of.  */
  std::optional<stmt_list_hash> type_unit_group_key;

  /* Containing DWO unit.
     This field is valid iff per_cu.reading_dwo_directly.  */
  struct dwo_unit *dwo_unit = nullptr;
};

using signatured_type_up = std::unique_ptr<signatured_type>;

/* Hash a signatured_type object based on its signature.  */

struct signatured_type_hash
{
  using is_transparent = void;

  std::size_t operator() (ULONGEST signature) const noexcept
  { return signature; }

  std::size_t operator() (const signatured_type *sig_type) const noexcept
  { return (*this) (sig_type->signature); }
};

/* Compare signatured_type objects based on their signatures.  */

struct signatured_type_eq
{
  using is_transparent = void;

  bool operator() (ULONGEST sig, const signatured_type *sig_type) const noexcept
  { return sig == sig_type->signature; }

  bool operator() (const signatured_type *sig_type_a,
		   const signatured_type *sig_type_b) const noexcept
  { return (*this) (sig_type_a->signature, sig_type_b); }
};

/* Unordered set of signatured_type objects using their signature as the
   key.  */

using signatured_type_set
  = gdb::unordered_set<signatured_type *, signatured_type_hash,
		       signatured_type_eq>;

struct dwo_file;

using dwo_file_up = std::unique_ptr<dwo_file>;

/* This is used when looking up entries in a dwo_file_set.  */

struct dwo_file_search
{
  /* Name of the DWO to look for.  */
  const char *dwo_name;

  /* Compilation directory to look for.  */
  const char *comp_dir;
};

/* Hash function for dwo_file objects, using their dwo_name and comp_dir as
   identity.  */

struct dwo_file_hash
{
  using is_transparent = void;

  std::size_t operator() (const dwo_file_search &search) const noexcept;
  std::size_t operator() (const dwo_file_up &file) const noexcept;
};

/* Equal function for dwo_file objects, using their dwo_name and comp_dir as
   identity.  */

struct dwo_file_eq
{
  using is_transparent = void;

  bool operator() (const dwo_file_search &search,
		   const dwo_file_up &dwo_file) const noexcept;
  bool operator() (const dwo_file_up &a, const dwo_file_up &b) const noexcept;
};

/* Set of dwo_file objects, using their dwo_name and comp_dir as identity.  */

using dwo_file_up_set
  = gdb::unordered_set<dwo_file_up, dwo_file_hash, dwo_file_eq>;

struct dwp_file;

using dwp_file_up = std::unique_ptr<dwp_file>;

/* Some DWARF data can be shared across objfiles who share the same BFD,
   this data is stored in this object.

   Two dwarf2_per_objfile objects representing objfiles sharing the same BFD
   will point to the same instance of dwarf2_per_bfd, unless the BFD requires
   relocation.  */

struct dwarf2_per_bfd
{
  /* Construct a dwarf2_per_bfd for OBFD.  NAMES points to the
     dwarf2 section names, or is NULL if the standard ELF names are
     used.  CAN_COPY is true for formats where symbol
     interposition is possible and so symbol values must follow copy
     relocation rules.  */
  dwarf2_per_bfd (bfd *obfd, const dwarf2_debug_sections *names, bool can_copy);

  ~dwarf2_per_bfd ();

  DISABLE_COPY_AND_ASSIGN (dwarf2_per_bfd);

  /* Return the filename of the BFD.  */
  const char *filename () const
  { return bfd_get_filename (this->obfd); }

  /* Return the unit given its index.  */
  dwarf2_per_cu *get_unit (int index) const
  {
    return this->all_units[index].get ();
  }

  /* Return the separate '.dwz' debug file.  If there is no
     .gnu_debugaltlink or .debug_sup section in the file, then the
     result depends on REQUIRE: if REQUIRE is true, error out; if
     REQUIRE is false, return nullptr.  */
  struct dwz_file *get_dwz_file (bool require = false)
  {
    gdb_assert (!require || this->dwz_file.has_value ());

    struct dwz_file *result = nullptr;

    if (this->dwz_file.has_value ())
      {
	result = this->dwz_file->get ();
	if (require && result == nullptr)
	  error (_("could not find supplementary DWARF file"));
      }

    return result;
  }

  /* A convenience function to allocate a dwarf2_per_cu.  The returned object
     has its "index" field set properly.  The object is allocated on the
     dwarf2_per_bfd obstack.  */
  dwarf2_per_cu_up allocate_per_cu (dwarf2_section_info *section,
				    sect_offset sect_off, unsigned int length,
				    bool is_dwz);

  /* A convenience function to allocate a signatured_type.  The
     returned object has its "index" field set properly.  The object
     is allocated on the dwarf2_per_bfd obstack.  */
  signatured_type_up allocate_signatured_type (dwarf2_section_info *section,
					       sect_offset sect_off,
					       unsigned int length,
					       bool is_dwz,
					       ULONGEST signature);

  /* Map all the DWARF section data needed when scanning
     .debug_info.  */
  void map_info_sections (struct objfile *objfile);

  /* Set the 'index_table' member and then call start_reading on
     it.  */
  void start_reading (dwarf_scanner_base_up new_table);

private:
  /* This function is mapped across the sections and remembers the
     offset and size of each of the debugging sections we are
     interested in.  */
  void locate_sections (asection *sectp, const dwarf2_debug_sections &names);

public:
  /* The corresponding BFD.  */
  bfd *obfd;

  /* Objects that can be shared across objfiles may be stored in this
     obstack, while objects that are objfile-specific are stored on
     the objfile obstack.  */
  auto_obstack obstack;

  std::vector<dwarf2_section_info> infos;
  dwarf2_section_info abbrev {};
  dwarf2_section_info line {};
  dwarf2_section_info loc {};
  dwarf2_section_info loclists {};
  dwarf2_section_info macinfo {};
  dwarf2_section_info macro {};
  dwarf2_section_info str {};
  dwarf2_section_info str_offsets {};
  dwarf2_section_info line_str {};
  dwarf2_section_info ranges {};
  dwarf2_section_info rnglists {};
  dwarf2_section_info addr {};
  dwarf2_section_info frame {};
  dwarf2_section_info eh_frame {};
  dwarf2_section_info gdb_index {};
  dwarf2_section_info debug_names {};
  dwarf2_section_info debug_aranges {};

  std::vector<dwarf2_section_info> types;

  /* Table of all the compilation units.  This is used to locate
     the target compilation unit of a particular reference.  */
  std::vector<dwarf2_per_cu_up> all_units;

  /* Number of compilation and type units in the ALL_UNITS vector.  */
  unsigned int num_comp_units = 0;
  unsigned int num_type_units = 0;

  /* Set of signatured_types, used to look up by signature.  */
  signatured_type_set signatured_types;

  /* Type unit statistics, to see how well the scaling improvements
     are doing.  */
  struct tu_stats tu_stats;

  /* Set of dwo_file objects.  */
  dwo_file_up_set dwo_files;

#if CXX_STD_THREAD
  /* Mutex to synchronize access to DWO_FILES.  */
  std::mutex dwo_files_lock;
#endif

  /* The DWP file if there is one, or NULL.  */
  dwp_file_up dwp_file;

  /* The shared '.dwz' file, if one exists.  This is used when the
     original data was compressed using 'dwz -m'.  */
  std::optional<dwz_file_up> dwz_file;

  /* Whether copy relocations are supported by this object format.  */
  bool can_copy;

  /* A flag indicating whether this objfile has a section loaded at a
     VMA of 0.  */
  bool has_section_at_zero = false;

  /* The mapped index, or NULL in the readnow case.  */
  dwarf_scanner_base_up index_table;

  /* When using index_table, this keeps track of all quick_file_names entries.
     TUs typically share line table entries with a CU, so we maintain a
     separate table of all line table entries to support the sharing.
     Note that while there can be way more TUs than CUs, we've already
     sorted all the TUs into "type unit groups", grouped by their
     DW_AT_stmt_list value.  Therefore the only sharing done here is with a
     CU and its associated TU group if there is one.  */
  gdb::unordered_map<stmt_list_hash, quick_file_names *, stmt_list_hash_hash>
    quick_file_names_table;

  /* The CUs we recently read.  */
  std::vector<dwarf2_per_cu *> just_read_cus;

  /* If we loaded the index from an external file, this contains the
     resources associated to the open file, memory mapping, etc.  */
  index_cache_resource_up index_cache_res;

  /* Mapping from abstract origin DIE to concrete DIEs that reference it as
     DW_AT_abstract_origin.  */
  gdb::unordered_map<sect_offset, std::vector<sect_offset>>
    abstract_to_concrete;

  /* Current directory, captured at the moment that object this was
     created.  */
  std::string captured_cwd;
  /* Captured copy of debug_file_directory.  */
  std::string captured_debug_dir;
};

/* Scoped object to remove all units from PER_BFD and clear other associated
   fields in case of failure.  */

struct scoped_remove_all_units
{
  explicit scoped_remove_all_units (dwarf2_per_bfd &per_bfd)
    : m_per_bfd (&per_bfd)
  {}

  DISABLE_COPY_AND_ASSIGN (scoped_remove_all_units);

  ~scoped_remove_all_units ()
  {
    if (m_per_bfd == nullptr)
      return;

    m_per_bfd->all_units.clear ();
    m_per_bfd->num_comp_units = 0;
    m_per_bfd->num_type_units = 0;
  }

  /* Disable this object.  Call this to keep the units of M_PER_BFD on the
     success path.  */
  void disable () { m_per_bfd = nullptr; }

private:
  /* This is nullptr if the object is disabled.  */
  dwarf2_per_bfd *m_per_bfd;
};

/* An iterator for all_units that is based on index.  This
   approach makes it possible to iterate over all_units safely,
   when some caller in the loop may add new units.  */

class all_units_iterator
{
public:

  all_units_iterator (dwarf2_per_bfd *per_bfd, bool start)
    : m_per_bfd (per_bfd),
      m_index (start ? 0 : per_bfd->all_units.size ())
  {
  }

  all_units_iterator &operator++ ()
  {
    ++m_index;
    return *this;
  }

  dwarf2_per_cu *operator* () const
  {
    return m_per_bfd->get_unit (m_index);
  }

  bool operator== (const all_units_iterator &other) const
  {
    return m_index == other.m_index;
  }


  bool operator!= (const all_units_iterator &other) const
  {
    return m_index != other.m_index;
  }

private:

  dwarf2_per_bfd *m_per_bfd;
  size_t m_index;
};

/* A range adapter for the all_units_iterator.  */
class all_units_range
{
public:

  all_units_range (dwarf2_per_bfd *per_bfd)
    : m_per_bfd (per_bfd)
  {
  }

  all_units_iterator begin ()
  {
    return all_units_iterator (m_per_bfd, true);
  }

  all_units_iterator end ()
  {
    return all_units_iterator (m_per_bfd, false);
  }

private:

  dwarf2_per_bfd *m_per_bfd;
};

/* This is the per-objfile data associated with a type_unit_group.  */

struct type_unit_group_unshareable
{
  /* The compunit symtab.
     Type units in a group needn't all be defined in the same source file,
     so we create an essentially anonymous symtab as the compunit symtab.  */
  struct compunit_symtab *compunit_symtab = nullptr;

  /* The number of symtabs from the line header.
     The value here must match line_header.num_file_names.  */
  unsigned int num_symtabs = 0;

  /* The symbol tables for this TU (obtained from the files listed in
     DW_AT_stmt_list).
     WARNING: The order of entries here must match the order of entries
     in the line header.  After the first TU using this type_unit_group, the
     line header for the subsequent TUs is recreated from this.  This is done
     because we need to use the same symtabs for each TU using the same
     DW_AT_stmt_list value.  Also note that symtabs may be repeated here,
     there's no guarantee the line header doesn't have duplicate entries.  */
  struct symtab **symtabs = nullptr;
};

using type_unit_group_unshareable_up
  = std::unique_ptr<type_unit_group_unshareable>;

struct per_cu_and_offset
{
  dwarf2_per_cu *per_cu;
  sect_offset offset;

  bool operator== (const per_cu_and_offset &other) const noexcept
  {
    return this->per_cu == other.per_cu && this->offset == other.offset;
  }
};

struct per_cu_and_offset_hash
{
  std::uint64_t operator() (const per_cu_and_offset &key) const noexcept
  {
    return (std::hash<dwarf2_per_cu *> () (key.per_cu)
	    + std::hash<sect_offset> () (key.offset));
  }
};

/* Collection of data recorded per objfile.
   This hangs off of dwarf2_objfile_data_key.

   Some DWARF data cannot (currently) be shared across objfiles.  Such
   data is stored in this object.  */

struct dwarf2_per_objfile
{
  dwarf2_per_objfile (struct objfile *objfile, dwarf2_per_bfd *per_bfd)
    : objfile (objfile), per_bfd (per_bfd)
  {}

  ~dwarf2_per_objfile ();

  /* Return pointer to string at .debug_line_str offset as read from BUF.
     BUF is assumed to be in a compilation unit described by CU_HEADER.
     Return *BYTES_READ_PTR count of bytes read from BUF.  */
  const char *read_line_string (const gdb_byte *buf,
				const struct unit_head *unit_header,
				unsigned int *bytes_read_ptr);

  /* Return pointer to string at .debug_line_str offset as read from BUF.
     The offset_size is OFFSET_SIZE.  */
  const char *read_line_string (const gdb_byte *buf,
				unsigned int offset_size);

  /* Return true if the symtab corresponding to PER_CU has been set,
     false otherwise.  */
  bool symtab_set_p (const dwarf2_per_cu *per_cu) const;

  /* Return the compunit_symtab associated to PER_CU, if it has been created.  */
  compunit_symtab *get_symtab (const dwarf2_per_cu *per_cu) const;

  /* Set the compunit_symtab associated to PER_CU.  */
  void set_symtab (const dwarf2_per_cu *per_cu, compunit_symtab *symtab);

  /* Get the type_unit_group_unshareable corresponding to TU_GROUP_KEY.  If one
     does not exist, create it.  */
  type_unit_group_unshareable *get_type_unit_group_unshareable
    (stmt_list_hash tu_group_key);

  struct type *get_type_for_signatured_type (signatured_type *sig_type) const;

  void set_type_for_signatured_type (signatured_type *sig_type,
				     struct type *type);

  /* Get the dwarf2_cu matching PER_CU for this objfile.  */
  dwarf2_cu *get_cu (dwarf2_per_cu *per_cu);

  /* Set the dwarf2_cu matching PER_CU for this objfile.  */
  void set_cu (dwarf2_per_cu *per_cu, dwarf2_cu_up cu);

  /* Remove/free the dwarf2_cu matching PER_CU for this objfile.  */
  void remove_cu (dwarf2_per_cu *per_cu);

  /* Free all cached compilation units.  */
  void remove_all_cus ();

  /* Increase the age counter on each CU compilation unit and free
     any that are too old.  */
  void age_comp_units ();

  /* Apply any needed adjustments to ADDR and then relocate the
     address according to the objfile's section offsets, returning a
     relocated address.  */
  CORE_ADDR relocate (unrelocated_addr addr);

  /* Back link.  */
  struct objfile *objfile;

  /* Pointer to the data that is (possibly) shared between this objfile and
     other objfiles backed by the same BFD.  */
  struct dwarf2_per_bfd *per_bfd;

  /* A mapping of (CU "per_cu" pointer, DIE offset) to GDB type pointer.

     We store these in a hash table separate from the DIEs, and preserve them
     when the DIEs are flushed out of cache.

     The CU "per_cu" pointer is needed because offset alone is not enough to
     uniquely identify the type.  A file may have multiple .debug_types sections,
     or the type may come from a DWO file.  Furthermore, while it's more logical
     to use per_cu->section+offset, with Fission the section with the data is in
     the DWO file but we don't know that section at the point we need it.
     We have to use something in dwarf2_per_cu (or the pointer to it)
     because we can enter the lookup routine, get_die_type_at_offset, from
     outside this file, and thus won't necessarily have PER_CU->cu.
     Fortunately, PER_CU is stable for the life of the objfile.  */
  gdb::unordered_map<per_cu_and_offset, type *, per_cu_and_offset_hash>
    die_type_hash;

  /* Table containing line_header indexed by offset and offset_in_dwz.  */
  htab_up line_header_hash;

  /* The CU containing the m_builder in scope.  */
  dwarf2_cu *sym_cu = nullptr;

  /* CUs that are queued to be read.  */
  std::optional<std::queue<dwarf2_queue_item>> queue;

private:
  /* Hold the corresponding compunit_symtab for each CU or TU.  This is indexed
     by dwarf2_per_cu::index.  A NULL value means that the CU/TU has not been
     expanded yet.  */
  std::vector<compunit_symtab *> m_symtabs;

  /* Map from a type unit group key to the corresponding unshared
     structure.  */
  gdb::unordered_map<stmt_list_hash, type_unit_group_unshareable_up,
		     stmt_list_hash_hash>
    m_type_units;

  /* Map from signatured types to the corresponding struct type.  */
  gdb::unordered_map<signatured_type *, struct type *> m_type_map;

  /* Map from the objfile-independent dwarf2_per_cu instances to the
     corresponding objfile-dependent dwarf2_cu instances.  */
  gdb::unordered_map<dwarf2_per_cu *, dwarf2_cu_up> m_dwarf2_cus;
};

class cutu_reader
{
public:

  cutu_reader (dwarf2_per_cu &this_cu,
	       dwarf2_per_objfile &per_objfile,
	       const struct abbrev_table *abbrev_table,
	       dwarf2_cu *existing_cu,
	       bool skip_partial,
	       enum language pretend_language,
	       const abbrev_table_cache *abbrev_cache = nullptr);

  cutu_reader (dwarf2_per_cu &this_cu,
	       dwarf2_per_objfile &per_objfile,
	       enum language pretend_language,
	       struct dwarf2_cu &parent_cu,
	       struct dwo_file &dwo_file);

  DISABLE_COPY_AND_ASSIGN (cutu_reader);

  /* Return true if either:

      - the unit is empty (just a header without any DIE)
      - the unit is a partial unit and this cutu_reader was built with SKIP
	PARTIAL true.  */
  bool is_dummy () const { return m_dummy_p; }

  dwarf2_cu *cu () const { return m_cu; }

  die_info *top_level_die () const { return m_top_level_die; }

  const gdb_byte *info_ptr () const { return m_info_ptr; }

  bfd *abfd () const { return m_abfd; }

  const gdb_byte *buffer () const { return m_buffer; }

  const gdb_byte *buffer_end () const { return m_buffer_end; }

  const dwarf2_section_info *section () const { return m_die_section; }

  /* Release the CU created by this cutu_reader.  */
  dwarf2_cu_up release_cu ();

  /* Release the abbrev table, transferring ownership to the
     caller.  */
  abbrev_table_up release_abbrev_table ()
  {
    return std::move (m_abbrev_table_holder);
  }

  /* Read all DIES of the debug info section in memory.  */
  void read_all_dies ();

  const gdb_byte *read_attribute (attribute *attr, const attr_abbrev *abbrev,
				  const gdb_byte *info_ptr,
				  bool allow_reprocess = true);

  const abbrev_info *peek_die_abbrev (const gdb_byte *info_ptr,
				      unsigned int *bytes_read);

  const gdb_byte *skip_one_die (const gdb_byte *info_ptr,
				const abbrev_info *abbrev,
				bool do_skip_children = true);

  const gdb_byte *skip_children (const gdb_byte *info_ptr);

private:
  /* Skip the attribute at INFO_PTR, knowing it has form FORM.  Return a pointer
     just past the attribute.  */
  const gdb_byte *skip_one_attribute (dwarf_form form,
				      const gdb_byte *info_ptr);

  void init_cu_die_reader (dwarf2_cu *cu, dwarf2_section_info *section,
			   struct dwo_file *dwo_file,
			   const struct abbrev_table *abbrev_table);

  void init_tu_and_read_dwo_dies (dwarf2_per_cu *this_cu,
				  dwarf2_per_objfile *per_objfile,
				  dwarf2_cu *existing_cu,
				  enum language pretend_language);

  void read_cutu_die_from_dwo (dwarf2_cu *cu, dwo_unit *dwo_unit,
			       die_info *stub_comp_unit_die,
			       const char *stub_comp_dir);

  void prepare_one_comp_unit (struct dwarf2_cu *cu,
			      enum language pretend_language);

  /* Helpers to build the in-memory DIE tree.  */

  die_info *read_toplevel_die (gdb::array_view<attribute *> extra_attrs = {});

  die_info *read_die_and_siblings (die_info *parent);

  die_info *read_die_and_children (die_info *parent);

  die_info *read_full_die (int num_extra_attrs, bool allow_reprocess);

  const gdb_byte *read_attribute_value (attribute *attr, unsigned form,
					LONGEST implicit_const,
					const gdb_byte *info_ptr,
					bool allow_reprocess);

  void read_attribute_reprocess (attribute *attr,
				 dwarf_tag tag = DW_TAG_padding);

  const char *read_dwo_str_index (ULONGEST str_index);

  gdb_bfd_ref_ptr open_dwo_file (dwarf2_per_bfd *per_bfd, const char *file_name,
				 const char *comp_dir);

  dwo_file_up open_and_init_dwo_file (dwarf2_cu *cu, const char *dwo_name,
				      const char *comp_dir);

  void locate_dwo_sections (objfile *objfile, dwo_file &dwo_file);

  void create_dwo_unit_hash_tables (dwo_file &dwo_file, dwarf2_cu &skeleton_cu,
				    dwarf2_section_info &section,
				    ruh_kind section_kind);

  dwo_unit *lookup_dwo_cutu (dwarf2_cu *cu, const char *dwo_name,
			     const char *comp_dir, ULONGEST signature,
			     int is_debug_types);

  dwo_unit *lookup_dwo_comp_unit (dwarf2_cu *cu, const char *dwo_name,
				  const char *comp_dir, ULONGEST signature);

  dwo_unit *lookup_dwo_type_unit (dwarf2_cu *cu, const char *dwo_name,
				  const char *comp_dir);

  dwo_unit *lookup_dwo_unit (dwarf2_cu *cu, die_info *comp_unit_die,
			     const char *dwo_name);

  /* The bfd of die_section.  */
  bfd *m_abfd;

  /* The CU of the DIE we are parsing.  */
  struct dwarf2_cu *m_cu;

  /* Non-NULL if reading a DWO file (including one packaged into a DWP).  */
  struct dwo_file *m_dwo_file;

  /* The section the die comes from.
    This is either .debug_info or .debug_types, or the .dwo variants.  */
  struct dwarf2_section_info *m_die_section;

  /* die_section->buffer.  */
  const gdb_byte *m_buffer;

  /* The end of the buffer.  */
  const gdb_byte *m_buffer_end;

  /* The abbreviation table to use when reading the DIEs.  */
  const struct abbrev_table *m_abbrev_table;

  const gdb_byte *m_info_ptr = nullptr;
  struct die_info *m_top_level_die = nullptr;
  bool m_dummy_p = false;

  dwarf2_cu_up m_new_cu;

  /* The ordinary abbreviation table.  */
  abbrev_table_up m_abbrev_table_holder;

  /* The DWO abbreviation table.  */
  abbrev_table_up m_dwo_abbrev_table;
};

/* Converts DWARF language names to GDB language names.  */

enum language dwarf_lang_to_enum_language (ULONGEST lang);

/* Get the dwarf2_per_objfile associated to OBJFILE.  */

dwarf2_per_objfile *get_dwarf2_per_objfile (struct objfile *objfile);

/* Return the type of the DIE at DIE_OFFSET in the CU named by
   PER_CU.  */

struct type *dwarf2_get_die_type (cu_offset die_offset, dwarf2_per_cu *per_cu,
				  dwarf2_per_objfile *per_objfile);

/* Given an index in .debug_addr, fetch the value.
   NOTE: This can be called during dwarf expression evaluation,
   long after the debug information has been read, and thus per_cu->cu
   may no longer exist.  */

unrelocated_addr dwarf2_read_addr_index (dwarf2_per_cu *per_cu,
					 dwarf2_per_objfile *per_objfile,
					 unsigned int addr_index);

/* Return DWARF block referenced by DW_AT_location of DIE at SECT_OFF at PER_CU.
   Returned value is intended for DW_OP_call*.  Returned
   dwarf2_locexpr_baton->data has lifetime of
   PER_CU->DWARF2_PER_OBJFILE->OBJFILE.  */

struct dwarf2_locexpr_baton dwarf2_fetch_die_loc_sect_off
  (sect_offset sect_off, dwarf2_per_cu *per_cu,
   dwarf2_per_objfile *per_objfile,
   gdb::function_view<CORE_ADDR ()> get_frame_pc,
   bool resolve_abstract_p = false);

/* Like dwarf2_fetch_die_loc_sect_off, but take a CU
   offset.  */

struct dwarf2_locexpr_baton dwarf2_fetch_die_loc_cu_off
  (cu_offset offset_in_cu, dwarf2_per_cu *per_cu,
   dwarf2_per_objfile *per_objfile,
   gdb::function_view<CORE_ADDR ()> get_frame_pc);

/* If the DIE at SECT_OFF in PER_CU has a DW_AT_const_value, return a
   pointer to the constant bytes and set LEN to the length of the
   data.  If memory is needed, allocate it on OBSTACK.  If the DIE
   does not have a DW_AT_const_value, return NULL.  */

extern const gdb_byte *dwarf2_fetch_constant_bytes
  (sect_offset sect_off, dwarf2_per_cu *per_cu,
   dwarf2_per_objfile *per_objfile, obstack *obstack,
   LONGEST *len);

/* Return the type of the die at SECT_OFF in PER_CU.  Return NULL if no
   valid type for this die is found.  If VAR_NAME is non-null, and if
   the DIE in question is a variable declaration (definitions are
   excluded), then *VAR_NAME is set to the variable's name.  */

type *dwarf2_fetch_die_type_sect_off (sect_offset sect_off,
				      dwarf2_per_cu *per_cu,
				      dwarf2_per_objfile *per_objfile,
				      const char **var_name = nullptr);

/* When non-zero, dump line number entries as they are read in.  */
extern unsigned int dwarf_line_debug;

/* Dwarf2 sections that can be accessed by dwarf2_get_section_info.  */
enum dwarf2_section_enum {
  DWARF2_DEBUG_FRAME,
  DWARF2_EH_FRAME
};

extern void dwarf2_get_section_info (struct objfile *,
				     enum dwarf2_section_enum,
				     asection **, const gdb_byte **,
				     bfd_size_type *);

/* Interface for DWARF indexing methods.  */

struct dwarf2_base_index_functions : public quick_symbol_functions
{
  bool has_symbols (struct objfile *objfile) override;

  bool has_unexpanded_symtabs (struct objfile *objfile) override;

  struct symtab *find_last_source_symtab (struct objfile *objfile) override;

  void forget_cached_source_info (struct objfile *objfile) override;

  enum language lookup_global_symbol_language (struct objfile *objfile,
					       const char *name,
					       domain_search_flags domain,
					       bool *symbol_found_p) override
  {
    *symbol_found_p = false;
    return language_unknown;
  }

  void print_stats (struct objfile *objfile, bool print_bcache) override;

  void expand_all_symtabs (struct objfile *objfile) override;

  struct compunit_symtab *find_pc_sect_compunit_symtab
    (struct objfile *objfile, bound_minimal_symbol msymbol,
     CORE_ADDR pc, struct obj_section *section, int warn_if_readin)
       override;

  struct compunit_symtab *find_compunit_symtab_by_address
    (struct objfile *objfile, CORE_ADDR address) override
  {
    return nullptr;
  }

  void map_symbol_filenames (objfile *objfile, symbol_filename_listener fun,
			     bool need_fullname) override;
};

/* If FILE_MATCHER is NULL or if PER_CU has
   dwarf2_per_cu_quick_data::MARK set (see
   dw_expand_symtabs_matching_file_matcher), expand the CU and call
   EXPANSION_NOTIFY on it.  */

extern bool dw2_expand_symtabs_matching_one
  (dwarf2_per_cu *per_cu,
   dwarf2_per_objfile *per_objfile,
   expand_symtabs_file_matcher file_matcher,
   expand_symtabs_expansion_listener expansion_notify,
   expand_symtabs_lang_matcher lang_matcher);

/* If FILE_MATCHER is non-NULL, set all the
   dwarf2_per_cu_quick_data::MARK of the current DWARF2_PER_OBJFILE
   that match FILE_MATCHER.  */

extern void dw_expand_symtabs_matching_file_matcher
  (dwarf2_per_objfile *per_objfile,
   expand_symtabs_file_matcher file_matcher);

/* Return pointer to string at .debug_str offset STR_OFFSET.  */

extern const char *read_indirect_string_at_offset
  (dwarf2_per_objfile *per_objfile, LONGEST str_offset);

/* Finalize the all_units vector.  */

extern void finalize_all_units (dwarf2_per_bfd *per_bfd);

/* Create a list of all compilation units in OBJFILE.  */

extern void create_all_units (dwarf2_per_objfile *per_objfile);

/* Find the base address of the compilation unit for range lists and
   location lists.  It will normally be specified by DW_AT_low_pc.
   In DWARF-3 draft 4, the base address could be overridden by
   DW_AT_entry_pc.  It's been removed, but GCC still uses this for
   compilation units with discontinuous ranges.  */

extern void dwarf2_find_base_address (die_info *die, dwarf2_cu *cu);

/* How dwarf2_get_pc_bounds constructed its *LOWPC and *HIGHPC return
   values.  Keep the items ordered with increasing constraints compliance.  */

enum pc_bounds_kind
{
  /* No attribute DW_AT_low_pc, DW_AT_high_pc or DW_AT_ranges was found.  */
  PC_BOUNDS_NOT_PRESENT,

  /* Some of the attributes DW_AT_low_pc, DW_AT_high_pc or DW_AT_ranges
	were present but they do not form a valid range of PC addresses.  */
  PC_BOUNDS_INVALID,

  /* Discontiguous range was found - that is DW_AT_ranges was found.  */
  PC_BOUNDS_RANGES,

  /* Contiguous range was found - DW_AT_low_pc and DW_AT_high_pc were found.  */
  PC_BOUNDS_HIGH_LOW,
};

/* Get low and high pc attributes from a die.  See enum pc_bounds_kind
   definition for the return value.  *LOWPC and *HIGHPC are set iff
   neither PC_BOUNDS_NOT_PRESENT nor PC_BOUNDS_INVALID are returned.  */

extern pc_bounds_kind dwarf2_get_pc_bounds (die_info *die,
					    unrelocated_addr *lowpc,
					    unrelocated_addr *highpc,
					    dwarf2_cu *cu, addrmap_mutable *map,
					    void *datum);

/* Locate the unit in PER_OBJFILE which contains the DIE at TARGET.  Raises an
   error on failure.  */

extern dwarf2_per_cu *dwarf2_find_containing_unit
  (const section_and_offset &target, dwarf2_per_objfile *per_objfile);

/* Locate the unit starting at START in PER_BFD.  Return nullptr if not
   found.  */

extern dwarf2_per_cu *dwarf2_find_unit (const section_and_offset &start,
					dwarf2_per_bfd *per_bfd);

/* Decode simple location descriptions.

   Given a pointer to a DWARF block that defines a location, compute
   the location.  Returns true if the expression was computable by
   this function, false otherwise.  On a true return, *RESULT is set.

   Note that this function does not implement a full DWARF expression
   evaluator.  Instead, it is used for a few limited purposes:

   - Getting the address of a symbol that has a constant address.  For
   example, if a symbol has a location like "DW_OP_addr", the address
   can be extracted.

   - Getting the offset of a virtual function in its vtable.  There
   are two forms of this, one of which involves DW_OP_deref -- so this
   function handles derefs in a peculiar way to make this 'work'.
   (Probably this area should be rewritten.)

   - Getting the offset of a field, when it is constant.

   Opcodes that cannot be part of a constant expression, for example
   those involving registers, simply result in a return of
   'false'.

   This function may emit a complaint.  */

extern bool decode_locdesc (dwarf_block *blk, dwarf2_cu *cu, CORE_ADDR *result);

/* Get low and high pc attributes from DW_AT_ranges attribute value OFFSET.
   Return 1 if the attributes are present and valid, otherwise, return 0.
   TAG is passed to dwarf2_ranges_process.  If MAP is not NULL, then
   ranges in MAP are set, using DATUM as the value.  */

extern int dwarf2_ranges_read (unsigned offset, unrelocated_addr *low_return,
			       unrelocated_addr *high_return, dwarf2_cu *cu,
			       addrmap_mutable *map, void *datum,
			       dwarf_tag tag);

extern file_and_directory &find_file_and_directory (die_info *die,
						    dwarf2_cu *cu);


/* Return the section that ATTR, an attribute with ref form, references.  */

extern const dwarf2_section_info &get_section_for_ref
  (const attribute &attr, dwarf2_cu *cu);

#endif /* GDB_DWARF2_READ_H */
