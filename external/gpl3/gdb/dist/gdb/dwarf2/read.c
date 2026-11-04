/* DWARF 2 debugging format support for GDB.

   Copyright (C) 1994-2025 Free Software Foundation, Inc.

   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support.

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

/* FIXME: Various die-reading functions need to be more careful with
   reading off the end of the section.  */

#include "dwarf2/read.h"
#include "dwarf2/abbrev.h"
#include "dwarf2/aranges.h"
#include "dwarf2/attribute.h"
#include "dwarf2/unit-head.h"
#include "dwarf2/cooked-index-worker.h"
#include "dwarf2/cooked-indexer.h"
#include "dwarf2/cu.h"
#include "dwarf2/index-cache.h"
#include "dwarf2/leb.h"
#include "dwarf2/line-header.h"
#include "dwarf2/dwz.h"
#include "dwarf2/macro.h"
#include "dwarf2/die.h"
#include "dwarf2/read-debug-names.h"
#include "dwarf2/read-gdb-index.h"
#include "dwarf2/sect-names.h"
#include "dwarf2/stringify.h"
#include "dwarf2/tag.h"
#include "dwarf2/public.h"
#include "bfd.h"
#include "elf-bfd.h"
#include "event-top.h"
#include "exceptions.h"
#include "gdbsupport/task-group.h"
#include "maint.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "objfiles.h"
#include "dwarf2.h"
#include "demangle.h"
#include "gdb-demangle.h"
#include "filenames.h"
#include "language.h"
#include "complaints.h"
#include "dwarf2/expr.h"
#include "dwarf2/loc.h"
#include "cp-support.h"
#include "hashtab.h"
#include "command.h"
#include "cli/cli-cmds.h"
#include "block.h"
#include "addrmap.h"
#include "typeprint.h"
#include "c-lang.h"
#include "go-lang.h"
#include "valprint.h"
#include "gdbcore.h"
#include "gdb_bfd.h"
#include "f-lang.h"
#include "source.h"
#include "build-id.h"
#include "namespace.h"
#include "gdbsupport/function-view.h"
#include <optional>
#include "gdbsupport/underlying.h"
#include "filename-seen-cache.h"
#include <fcntl.h>
#include <algorithm>
#include "gdbsupport/selftest.h"
#include "rust-lang.h"
#include "gdbsupport/pathstuff.h"
#include "count-one-bits.h"
#include "dwarf2/abbrev-table-cache.h"
#include "cooked-index.h"
#include "gdbsupport/thread-pool.h"
#include "run-on-main-thread.h"
#include "dwarf2/parent-map.h"
#include "dwarf2/error.h"
#include "gdbsupport/unordered_set.h"
#include "extract-store-integer.h"

/* When == 1, print basic high level tracing messages.
   When > 1, be more verbose.
   This is in contrast to the low level DIE reading of dwarf_die_debug.  */
static unsigned int dwarf_read_debug = 0;

/* Print a "dwarf-read" debug statement if dwarf_read_debug is >= 1.  */

#define dwarf_read_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (dwarf_read_debug >= 1, "dwarf-read", fmt, \
			      ##__VA_ARGS__)

/* Print a "dwarf-read" debug statement if dwarf_read_debug is >= 2.  */

#define dwarf_read_debug_printf_v(fmt, ...) \
  debug_prefixed_printf_cond (dwarf_read_debug >= 2, "dwarf-read", fmt, \
			      ##__VA_ARGS__)

/* Print "dwarf-read" start/end debug statements.  */

#define DWARF_READ_SCOPED_DEBUG_START_END(fmt, ...)                           \
  scoped_debug_start_end ([] { return dwarf_read_debug >= 1; }, "dwarf-read", \
			  fmt, ##__VA_ARGS__)

/* When non-zero, dump DIEs after they are read in.  */
static unsigned int dwarf_die_debug = 0;

/* When non-zero, dump line number entries as they are read in.  */
unsigned int dwarf_line_debug = 0;

/* When true, cross-check physname against demangler.  */
static bool check_physname = false;

/* This is used to store the data that is always per objfile.  */
static const registry<objfile>::key<dwarf2_per_objfile>
     dwarf2_objfile_data_key;

/* These are used to store the dwarf2_per_bfd objects.

   objfiles having the same BFD, which doesn't require relocations, are going to
   share a dwarf2_per_bfd object, which is held in the _bfd_data_key version.

   Other objfiles are not going to share a dwarf2_per_bfd with any other
   objfiles, so they'll have their own version kept in the _objfile_data_key
   version.  */
static const registry<bfd>::key<dwarf2_per_bfd> dwarf2_per_bfd_bfd_data_key;
static const registry<objfile>::key<dwarf2_per_bfd>
  dwarf2_per_bfd_objfile_data_key;

/* The "loc_class" indices for various kinds of computed DWARF symbols.  */

static int dwarf2_locexpr_index;
static int dwarf2_loclist_index;
static int ada_imported_index;
static int dwarf2_locexpr_block_index;
static int dwarf2_loclist_block_index;
static int ada_block_index;

/* Size of .debug_loclists section header for 32-bit DWARF format.  */
#define LOCLIST_HEADER_SIZE32 12

/* Size of .debug_loclists section header for 64-bit DWARF format.  */
#define LOCLIST_HEADER_SIZE64 20

/* Size of .debug_rnglists section header for 32-bit DWARF format.  */
#define RNGLIST_HEADER_SIZE32 12

/* Size of .debug_rnglists section header for 64-bit DWARF format.  */
#define RNGLIST_HEADER_SIZE64 20

/* See dwarf2/read.h.  */

dwarf2_per_objfile *
get_dwarf2_per_objfile (struct objfile *objfile)
{
  return dwarf2_objfile_data_key.get (objfile);
}

/* Default names of the debugging sections.  */

/* Note that if the debugging section has been compressed, it might
   have a name like .zdebug_info.  */

const struct dwarf2_debug_sections dwarf2_elf_names =
{
  { ".debug_info", ".zdebug_info" },
  { ".debug_abbrev", ".zdebug_abbrev" },
  { ".debug_line", ".zdebug_line" },
  { ".debug_loc", ".zdebug_loc" },
  { ".debug_loclists", ".zdebug_loclists" },
  { ".debug_macinfo", ".zdebug_macinfo" },
  { ".debug_macro", ".zdebug_macro" },
  { ".debug_str", ".zdebug_str" },
  { ".debug_str_offsets", ".zdebug_str_offsets" },
  { ".debug_line_str", ".zdebug_line_str" },
  { ".debug_ranges", ".zdebug_ranges" },
  { ".debug_rnglists", ".zdebug_rnglists" },
  { ".debug_types", ".zdebug_types" },
  { ".debug_addr", ".zdebug_addr" },
  { ".debug_frame", ".zdebug_frame" },
  { ".eh_frame", NULL },
  { ".gdb_index", ".zgdb_index" },
  { ".debug_names", ".zdebug_names" },
  { ".debug_aranges", ".zdebug_aranges" },
  23
};

/* List of DWO/DWP sections.  */

static const struct dwop_section_names
{
  struct dwarf2_section_names abbrev_dwo;
  struct dwarf2_section_names info_dwo;
  struct dwarf2_section_names line_dwo;
  struct dwarf2_section_names loc_dwo;
  struct dwarf2_section_names loclists_dwo;
  struct dwarf2_section_names macinfo_dwo;
  struct dwarf2_section_names macro_dwo;
  struct dwarf2_section_names rnglists_dwo;
  struct dwarf2_section_names str_dwo;
  struct dwarf2_section_names str_offsets_dwo;
  struct dwarf2_section_names types_dwo;
  struct dwarf2_section_names cu_index;
  struct dwarf2_section_names tu_index;
}
dwop_section_names =
{
  { ".debug_abbrev.dwo", ".zdebug_abbrev.dwo" },
  { ".debug_info.dwo", ".zdebug_info.dwo" },
  { ".debug_line.dwo", ".zdebug_line.dwo" },
  { ".debug_loc.dwo", ".zdebug_loc.dwo" },
  { ".debug_loclists.dwo", ".zdebug_loclists.dwo" },
  { ".debug_macinfo.dwo", ".zdebug_macinfo.dwo" },
  { ".debug_macro.dwo", ".zdebug_macro.dwo" },
  { ".debug_rnglists.dwo", ".zdebug_rnglists.dwo" },
  { ".debug_str.dwo", ".zdebug_str.dwo" },
  { ".debug_str_offsets.dwo", ".zdebug_str_offsets.dwo" },
  { ".debug_types.dwo", ".zdebug_types.dwo" },
  { ".debug_cu_index", ".zdebug_cu_index" },
  { ".debug_tu_index", ".zdebug_tu_index" },
};

/* local data types */

/* The location list and range list sections (.debug_loclists & .debug_rnglists)
   begin with a header,  which contains the following information.  */
struct loclists_rnglists_header
{
  /* A 4-byte or 12-byte length containing the length of the
     set of entries for this compilation unit, not including the
     length field itself.  */
  unsigned int length;

  /* A 2-byte version identifier.  */
  short version;

  /* A 1-byte unsigned integer containing the size in bytes of an address on
     the target system.  */
  unsigned char addr_size;

  /* A 1-byte unsigned integer containing the size in bytes of a segment selector
     on the target system.  */
  unsigned char segment_collector_size;

  /* A 4-byte count of the number of offsets that follow the header.  */
  unsigned int offset_entry_count;
};

/* These sections are what may appear in a (real or virtual) DWO file.  */

struct dwo_sections
{
  struct dwarf2_section_info abbrev;
  struct dwarf2_section_info line;
  struct dwarf2_section_info loc;
  struct dwarf2_section_info loclists;
  struct dwarf2_section_info macinfo;
  struct dwarf2_section_info macro;
  struct dwarf2_section_info rnglists;
  struct dwarf2_section_info str;
  struct dwarf2_section_info str_offsets;
  /* In the case of a virtual DWO file, these two are unused.  */
  std::vector<dwarf2_section_info> infos;
  std::vector<dwarf2_section_info> types;
};

/* CUs/TUs in DWP/DWO files.  */

struct dwo_unit
{
  /* Backlink to the containing struct dwo_file.  */
  struct dwo_file *dwo_file = nullptr;

  /* The "id" that distinguishes this CU/TU.
     .debug_info calls this "dwo_id", .debug_types calls this "signature".
     Since signatures came first, we stick with it for consistency.  */
  ULONGEST signature = 0;

  /* The section this CU/TU lives in, in the DWO file.  */
  dwarf2_section_info *section = nullptr;

  /* This is set if SECTION is owned by this dwo_unit.  */
  dwarf2_section_info_up section_holder;

  /* Same as dwarf2_per_cu::{sect_off,length} but in the DWO section.  */
  sect_offset sect_off {};
  unsigned int length = 0;

  /* For types, offset in the type's DIE of the type defined by this TU.  */
  cu_offset type_offset_in_tu;
};

using dwo_unit_up = std::unique_ptr<dwo_unit>;

/* Hash function for dwo_unit objects, based on the signature.  */

struct dwo_unit_hash
{
  using is_transparent = void;

  std::size_t operator() (ULONGEST signature) const noexcept
  { return signature; }

  std::size_t operator() (const dwo_unit_up &unit) const noexcept
  { return (*this) (unit->signature); }
};

/* Equal function for dwo_unit objects, based on the signature.

   The signature is assumed to be unique within the DWO file.  So while object
   file CU dwo_id's always have the value zero, that's OK, assuming each object
   file DWO file has only one CU, and that's the rule for now.  */

struct dwo_unit_eq
{
  using is_transparent = void;

  bool operator() (ULONGEST sig, const dwo_unit_up  &unit) const noexcept
  { return sig == unit->signature; }

  bool operator() (const dwo_unit_up &a, const dwo_unit_up &b) const noexcept
  { return (*this) (a->signature, b); }
};

/* Set of dwo_unit object, using their signature as identity.  */

using dwo_unit_set = gdb::unordered_set<dwo_unit_up, dwo_unit_hash, dwo_unit_eq>;

/* include/dwarf2.h defines the DWP section codes.
   It defines a max value but it doesn't define a min value, which we
   use for error checking, so provide one.  */

enum dwp_v2_section_ids
{
  DW_SECT_MIN = 1
};

/* Data for one DWO file.

   This includes virtual DWO files (a virtual DWO file is a DWO file as it
   appears in a DWP file).  DWP files don't really have DWO files per se -
   comdat folding of types "loses" the DWO file they came from, and from
   a high level view DWP files appear to contain a mass of random types.
   However, to maintain consistency with the non-DWP case we pretend DWP
   files contain virtual DWO files, and we assign each TU with one virtual
   DWO file (generally based on the line and abbrev section offsets -
   a heuristic that seems to work in practice).  */

struct dwo_file
{
  dwo_file () = default;
  DISABLE_COPY_AND_ASSIGN (dwo_file);

  /* The DW_AT_GNU_dwo_name or DW_AT_dwo_name attribute.
     For virtual DWO files the name is constructed from the section offsets
     of abbrev,line,loc,str_offsets so that we combine virtual DWO files
     from related CU+TUs.  */
  std::string dwo_name;

  /* The DW_AT_comp_dir attribute.  */
  const char *comp_dir = nullptr;

  /* The bfd, when the file is open.  Otherwise this is NULL.
     This is unused(NULL) for virtual DWO files where we use dwp_file.dbfd.  */
  gdb_bfd_ref_ptr dbfd;

  /* The sections that make up this DWO file.
     Remember that for virtual DWO files in DWP V2 or DWP V5, these are virtual
     sections (for lack of a better name).  */
  struct dwo_sections sections {};

  /* The CUs in the file.

     Multiple CUs per DWO are supported as an extension to handle LLVM's Link
     Time Optimization output (where multiple source files may be compiled into
     a single object/dwo pair).  */
  dwo_unit_set cus;

  /* Table of TUs in the file.  */
  dwo_unit_set tus;
};

/* See dwarf2/read.h.  */

std::size_t
dwo_file_hash::operator() (const dwo_file_search &search) const noexcept
{
  hashval_t hash = htab_hash_string (search.dwo_name);

  if (search.comp_dir != nullptr)
    hash += htab_hash_string (search.comp_dir);

  return hash;
}

/* See dwarf2/read.h.  */

std::size_t
dwo_file_hash::operator() (const dwo_file_up &file) const noexcept
{
  return (*this) ({ file->dwo_name.c_str (), file->comp_dir });
}

/* See dwarf2/read.h.  */

bool
dwo_file_eq::operator() (const dwo_file_search &search,
			 const dwo_file_up &dwo_file) const noexcept
{
  if (search.dwo_name != dwo_file->dwo_name)
    return false;

  if (search.comp_dir == nullptr || dwo_file->comp_dir == nullptr)
    return search.comp_dir == dwo_file->comp_dir;

  return std::strcmp (search.comp_dir, dwo_file->comp_dir) == 0;
}

/* See dwarf2/read.h.  */

bool
dwo_file_eq::operator() (const dwo_file_up &a,
			 const dwo_file_up &b) const noexcept
{ return (*this) ({ a->dwo_name.c_str (), a->comp_dir }, b); }

/* These sections are what may appear in a DWP file.  */

struct dwp_sections
{
  /* These are used by all DWP versions (1, 2 and 5).  */
  struct dwarf2_section_info str;
  struct dwarf2_section_info cu_index;
  struct dwarf2_section_info tu_index;

  /* These are only used by DWP version 2 and version 5 files.
     In DWP version 1 the .debug_info.dwo, .debug_types.dwo, and other
     sections are referenced by section number, and are not recorded here.
     In DWP version 2 or 5 there is at most one copy of all these sections,
     each section being (effectively) comprised of the concatenation of all of
     the individual sections that exist in the version 1 format.
     To keep the code simple we treat each of these concatenated pieces as a
     section itself (a virtual section?).  */
  struct dwarf2_section_info abbrev;
  struct dwarf2_section_info info;
  struct dwarf2_section_info line;
  struct dwarf2_section_info loc;
  struct dwarf2_section_info loclists;
  struct dwarf2_section_info macinfo;
  struct dwarf2_section_info macro;
  struct dwarf2_section_info rnglists;
  struct dwarf2_section_info str_offsets;
  struct dwarf2_section_info types;
};

/* These sections are what may appear in a virtual DWO file in DWP version 1.
   A virtual DWO file is a DWO file as it appears in a DWP file.  */

struct virtual_v1_dwo_sections
{
  struct dwarf2_section_info abbrev;
  struct dwarf2_section_info line;
  struct dwarf2_section_info loc;
  struct dwarf2_section_info macinfo;
  struct dwarf2_section_info macro;
  struct dwarf2_section_info str_offsets;
  /* Each DWP hash table entry records one CU or one TU.
     That is recorded here, and copied to dwo_unit.section.  */
  struct dwarf2_section_info info_or_types;
};

/* Similar to virtual_v1_dwo_sections, but for DWP version 2 or 5.
   In version 2, the sections of the DWO files are concatenated together
   and stored in one section of that name.  Thus each ELF section contains
   several "virtual" sections.  */

struct virtual_v2_or_v5_dwo_sections
{
  bfd_size_type abbrev_offset;
  bfd_size_type abbrev_size;

  bfd_size_type line_offset;
  bfd_size_type line_size;

  bfd_size_type loc_offset;
  bfd_size_type loc_size;

  bfd_size_type loclists_offset;
  bfd_size_type loclists_size;

  bfd_size_type macinfo_offset;
  bfd_size_type macinfo_size;

  bfd_size_type macro_offset;
  bfd_size_type macro_size;

  bfd_size_type rnglists_offset;
  bfd_size_type rnglists_size;

  bfd_size_type str_offsets_offset;
  bfd_size_type str_offsets_size;

  /* Each DWP hash table entry records one CU or one TU.
     That is recorded here, and copied to dwo_unit.section.  */
  bfd_size_type info_or_types_offset;
  bfd_size_type info_or_types_size;
};

/* Contents of DWP hash tables.  */

struct dwp_hash_table
{
  uint32_t version, nr_columns;
  uint32_t nr_units, nr_slots;
  const gdb_byte *hash_table, *unit_table;
  union
  {
    struct
    {
      const gdb_byte *indices;
    } v1;
    struct
    {
      /* This is indexed by column number and gives the id of the section
	 in that column.  */
#define MAX_NR_V2_DWO_SECTIONS \
  (1 /* .debug_info or .debug_types */ \
   + 1 /* .debug_abbrev */ \
   + 1 /* .debug_line */ \
   + 1 /* .debug_loc */ \
   + 1 /* .debug_str_offsets */ \
   + 1 /* .debug_macro or .debug_macinfo */)
      int section_ids[MAX_NR_V2_DWO_SECTIONS];
      const gdb_byte *offsets;
      const gdb_byte *sizes;
    } v2;
    struct
    {
      /* This is indexed by column number and gives the id of the section
	 in that column.  */
#define MAX_NR_V5_DWO_SECTIONS \
  (1 /* .debug_info */ \
   + 1 /* .debug_abbrev */ \
   + 1 /* .debug_line */ \
   + 1 /* .debug_loclists */ \
   + 1 /* .debug_str_offsets */ \
   + 1 /* .debug_macro */ \
   + 1 /* .debug_rnglists */)
      int section_ids[MAX_NR_V5_DWO_SECTIONS];
      const gdb_byte *offsets;
      const gdb_byte *sizes;
    } v5;
  } section_pool;
};

/* Data for one DWP file.  */

struct dwp_file
{
  dwp_file (const char *name_, gdb_bfd_ref_ptr &&abfd)
    : name (name_),
      dbfd (std::move (abfd))
  {
  }

  /* Name of the file.  */
  const char *name;

  /* File format version.  */
  int version = 0;

  /* The bfd.  */
  gdb_bfd_ref_ptr dbfd;

  /* Section info for this file.  */
  struct dwp_sections sections {};

  /* Table of CUs in the file.  */
  const struct dwp_hash_table *cus = nullptr;

  /* Table of TUs in the file.  */
  const struct dwp_hash_table *tus = nullptr;

  /* Tables of loaded CUs/TUs.  */
  dwo_unit_set loaded_cus;
  dwo_unit_set loaded_tus;

#if CXX_STD_THREAD
  /* Mutex to synchronize access to LOADED_CUS and LOADED_TUS.  */
  std::mutex loaded_cutus_lock;
#endif

  /* Table to map ELF section numbers to their sections.
     This is only needed for the DWP V1 file format.  */
  unsigned int num_sections = 0;
  asection **elf_sections = nullptr;
};

/* FIXME: We might want to set this from BFD via bfd_arch_bits_per_byte,
   but this would require a corresponding change in unpack_field_as_long
   and friends.  */
static int bits_per_byte = 8;

struct variant_part_builder;

/* When reading a variant, we track a bit more information about the
   field, and store it in an object of this type.  */

struct variant_field
{
  int first_field = -1;
  int last_field = -1;

  /* A variant can contain other variant parts.  */
  std::vector<variant_part_builder> variant_parts;

  /* If we see a DW_AT_discr_value, then this will be the discriminant
     value.  Just the attribute is stored here, because we have to
     defer deciding whether the value is signed or unsigned until the
     end.  */
  const attribute *discriminant_attr = nullptr;
  /* If we see a DW_AT_discr_list, then this is a pointer to the list
     data.  */
  struct dwarf_block *discr_list_data = nullptr;

  /* If both DW_AT_discr_value and DW_AT_discr_list are absent, then
     this is the default branch.  */
  bool is_default () const
  {
    return discriminant_attr == nullptr && discr_list_data == nullptr;
  }
};

/* This represents a DW_TAG_variant_part.  */

struct variant_part_builder
{
  /* The offset of the discriminant field.  */
  sect_offset discriminant_offset {};

  /* Variants that are direct children of this variant part.  */
  std::vector<variant_field> variants;

  /* True if we're currently reading a variant.  */
  bool processing_variant = false;
};

struct nextfield
{
  /* Variant parts need to find the discriminant, which is a DIE
     reference.  We track the section offset of each field to make
     this link.  */
  sect_offset offset;
  struct field field {};
};

struct fnfieldlist
{
  const char *name = nullptr;
  std::vector<struct fn_field> fnfields;
};

/* The routines that read and process dies for a C struct or C++ class
   pass lists of data member fields and lists of member function fields
   in an instance of a field_info structure, as defined below.  */
struct field_info
{
  /* List of data member and baseclasses fields.  */
  std::vector<struct nextfield> fields;
  std::vector<struct nextfield> baseclasses;

  /* Member function fieldlist array, contains name of possibly overloaded
     member function, number of overloaded member functions and a pointer
     to the head of the member function field chain.  */
  std::vector<struct fnfieldlist> fnfieldlists;

  /* typedefs defined inside this class.  TYPEDEF_FIELD_LIST contains head of
     a NULL terminated list of TYPEDEF_FIELD_LIST_COUNT elements.  */
  std::vector<struct decl_field> typedef_field_list;

  /* Nested types defined by this class and the number of elements in this
     list.  */
  std::vector<struct decl_field> nested_types_list;

  /* If non-null, this is the variant part we are currently
     reading.  */
  variant_part_builder *current_variant_part = nullptr;
  /* This holds all the top-level variant parts attached to the type
     we're reading.  */
  std::vector<variant_part_builder> variant_parts;

  /* Return the total number of fields (including baseclasses).  */
  int nfields () const
  {
    return fields.size () + baseclasses.size ();
  }
};

/* Loaded secondary compilation units are kept in memory until they
   have not been referenced for the processing of this many
   compilation units.  Set this to zero to disable caching.  Cache
   sizes of up to at least twenty will improve startup time for
   typical inter-CU-reference binaries, at an obvious memory cost.  */
static int dwarf_max_cache_age = 5;
static void
show_dwarf_max_cache_age (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("The upper bound on the age of cached "
		      "DWARF compilation units is %s.\n"),
	      value);
}

/* When true, wait for DWARF reading to be complete.  */
static bool dwarf_synchronous = false;

/* "Show" callback for "maint set dwarf synchronous".  */
static void
show_dwarf_synchronous (struct ui_file *file, int from_tty,
			struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Whether DWARF reading is synchronous is %s.\n"),
	      value);
}

/* local function prototypes */

static void var_decode_location (struct attribute *attr,
				 struct symbol *sym,
				 struct dwarf2_cu *cu);

static unsigned int peek_abbrev_code (bfd *, const gdb_byte *);

static unrelocated_addr read_addr_index (struct dwarf2_cu *cu,
					 unsigned int addr_index);

static sect_offset read_abbrev_offset (dwarf2_per_objfile *per_objfile,
				       dwarf2_section_info *, sect_offset);

static const char *read_indirect_string (dwarf2_per_objfile *per_objfile, bfd *,
					 const gdb_byte *, const unit_head *,
					 unsigned int *);

static unrelocated_addr read_addr_index_from_leb128 (struct dwarf2_cu *,
						     const gdb_byte *,
						     unsigned int *);

static const char *read_stub_str_index (struct dwarf2_cu *cu,
					ULONGEST str_index);

static struct attribute *dwarf2_attr (struct die_info *, unsigned int,
				      struct dwarf2_cu *);

static const char *dwarf2_string_attr (struct die_info *die, unsigned int name,
				       struct dwarf2_cu *cu);

static const char *dwarf2_dwo_name (struct die_info *die, struct dwarf2_cu *cu);

static int dwarf2_flag_true_p (struct die_info *die, unsigned name,
			       struct dwarf2_cu *cu);

static int die_is_declaration (struct die_info *, struct dwarf2_cu *cu);

static struct die_info *die_specification (struct die_info *die,
					   struct dwarf2_cu **);

static line_header_up dwarf_decode_line_header (sect_offset sect_off,
						struct dwarf2_cu *cu,
						const char *comp_dir);

static void dwarf_decode_lines (struct line_header *,
				struct dwarf2_cu *,
				unrelocated_addr, int decode_mapping);

static void dwarf2_start_subfile (dwarf2_cu *cu, const file_entry &fe,
				  const line_header &lh);

static struct symbol *new_symbol (struct die_info *, struct type *,
				  struct dwarf2_cu *, struct symbol * = NULL);

static void dwarf2_const_value (const struct attribute *, struct symbol *,
				struct dwarf2_cu *);

static void dwarf2_const_value_attr (const struct attribute *attr,
				     struct type *type,
				     const char *name,
				     struct obstack *obstack,
				     struct dwarf2_cu *cu, LONGEST *value,
				     const gdb_byte **bytes,
				     struct dwarf2_locexpr_baton **baton);

static struct type *read_subrange_index_type (struct die_info *die,
					      struct dwarf2_cu *cu);

static struct type *die_type (struct die_info *, struct dwarf2_cu *);

static int need_gnat_info (struct dwarf2_cu *);

static struct type *die_descriptive_type (struct die_info *,
					  struct dwarf2_cu *);

static void set_descriptive_type (struct type *, struct die_info *,
				  struct dwarf2_cu *);

static struct type *die_containing_type (struct die_info *,
					 struct dwarf2_cu *);

static struct type *lookup_die_type (struct die_info *, const struct attribute *,
				     struct dwarf2_cu *);

static struct type *read_type_die (struct die_info *, struct dwarf2_cu *);

static struct type *read_type_die_1 (struct die_info *, struct dwarf2_cu *);

static const char *determine_prefix (struct die_info *die, struct dwarf2_cu *);

static gdb::unique_xmalloc_ptr<char> typename_concat (const char *prefix,
						      const char *suffix,
						      int physname,
						      struct dwarf2_cu *cu);

static void read_file_scope (struct die_info *, struct dwarf2_cu *);

static void read_type_unit_scope (struct die_info *, struct dwarf2_cu *);

static void read_func_scope (struct die_info *, struct dwarf2_cu *);

static void read_lexical_block_scope (struct die_info *, struct dwarf2_cu *);

static void read_call_site_scope (struct die_info *die, struct dwarf2_cu *cu);

static void read_variable (struct die_info *die, struct dwarf2_cu *cu);

/* Return the .debug_loclists section to use for cu.  */
static struct dwarf2_section_info *cu_debug_loc_section (struct dwarf2_cu *cu);

/* Return the .debug_rnglists section to use for cu.  */
static struct dwarf2_section_info *cu_debug_rnglists_section
  (struct dwarf2_cu *cu, dwarf_tag tag);

static void get_scope_pc_bounds (struct die_info *,
				 unrelocated_addr *, unrelocated_addr *,
				 struct dwarf2_cu *);

static void dwarf2_record_block_ranges (struct die_info *, struct block *,
					struct dwarf2_cu *);

static void dwarf2_add_field (struct field_info *, struct die_info *,
			      struct dwarf2_cu *);

static void dwarf2_attach_fields_to_type (struct field_info *,
					  struct type *, struct dwarf2_cu *);

static void dwarf2_add_member_fn (struct field_info *,
				  struct die_info *, struct type *,
				  struct dwarf2_cu *);

static void dwarf2_attach_fn_fields_to_type (struct field_info *,
					     struct type *,
					     struct dwarf2_cu *);

static void process_structure_scope (struct die_info *, struct dwarf2_cu *);

static void read_common_block (struct die_info *, struct dwarf2_cu *);

static void read_namespace (struct die_info *die, struct dwarf2_cu *);

static void read_module (struct die_info *die, struct dwarf2_cu *cu);

static struct using_direct **using_directives (struct dwarf2_cu *cu);

static void read_import_statement (struct die_info *die, struct dwarf2_cu *);

static bool read_alias (struct die_info *die, struct dwarf2_cu *cu);

static struct type *read_module_type (struct die_info *die,
				      struct dwarf2_cu *cu);

static const char *namespace_name (struct die_info *die,
				   int *is_anonymous, struct dwarf2_cu *);

static void process_enumeration_scope (struct die_info *, struct dwarf2_cu *);

static enum dwarf_array_dim_ordering read_array_order (struct die_info *,
						       struct dwarf2_cu *);

static void process_die (struct die_info *, struct dwarf2_cu *);

static const char *dwarf2_canonicalize_name (const char *, struct dwarf2_cu *,
					     struct objfile *);

static const char *dwarf2_name (struct die_info *die, struct dwarf2_cu *);

static const char *dwarf2_full_name (const char *name,
				     struct die_info *die,
				     struct dwarf2_cu *cu);

static const char *dwarf2_physname (const char *name, struct die_info *die,
				    struct dwarf2_cu *cu);

static struct die_info *dwarf2_extension (struct die_info *die,
					  struct dwarf2_cu **);

static struct die_info *follow_die_ref_or_sig (struct die_info *,
					       const struct attribute *,
					       struct dwarf2_cu **);

static struct die_info *follow_die_ref (struct die_info *,
					const struct attribute *,
					struct dwarf2_cu **);

static struct die_info *follow_die_sig (struct die_info *,
					const struct attribute *,
					struct dwarf2_cu **);

static struct type *get_signatured_type (struct die_info *, ULONGEST,
					 struct dwarf2_cu *);

static struct type *get_DW_AT_signature_type (struct die_info *,
					      const struct attribute *,
					      struct dwarf2_cu *);

static void load_full_type_unit (dwarf2_per_cu *per_cu,
				 dwarf2_per_objfile *per_objfile);

static void read_signatured_type (signatured_type *sig_type,
				  dwarf2_per_objfile *per_objfile);

static int attr_to_dynamic_prop (const struct attribute *attr,
				 struct die_info *die, struct dwarf2_cu *cu,
				 struct dynamic_prop *prop, struct type *type);

/* memory allocation interface */

static struct dwarf_block *dwarf_alloc_block (struct dwarf2_cu *);

static void dwarf_decode_macros (struct dwarf2_cu *, unsigned int, int);

static void fill_in_loclist_baton (struct dwarf2_cu *cu,
				   struct dwarf2_loclist_baton *baton,
				   const struct attribute *attr);

static void dwarf2_symbol_mark_computed (const struct attribute *attr,
					 struct symbol *sym,
					 struct dwarf2_cu *cu,
					 int is_block);

static struct type *set_die_type (struct die_info *, struct type *,
				  struct dwarf2_cu *, bool = false);

static void load_full_comp_unit (dwarf2_per_cu *per_cu,
				 dwarf2_per_objfile *per_objfile,
				 bool skip_partial,
				 enum language pretend_language);

static void process_full_comp_unit (dwarf2_cu *cu);

static void process_full_type_unit (dwarf2_cu *cu);

static struct type *get_die_type_at_offset (sect_offset,
					    dwarf2_per_cu *per_cu,
					    dwarf2_per_objfile *per_objfile);

static struct type *get_die_type (struct die_info *die, struct dwarf2_cu *cu);

static void queue_comp_unit (dwarf2_per_cu *per_cu,
			     dwarf2_per_objfile *per_objfile);

static void process_queue (dwarf2_per_objfile *per_objfile);

static bool is_ada_import_or_export (dwarf2_cu *cu, const char *name,
				     const char *linkagename);

/* Class, the destructor of which frees all allocated queue entries.  This
   will only have work to do if an error was thrown while processing the
   dwarf.  If no error was thrown then the queue entries should have all
   been processed, and freed, as we went along.  */

class dwarf2_queue_guard
{
public:
  explicit dwarf2_queue_guard (dwarf2_per_objfile *per_objfile)
    : m_per_objfile (per_objfile)
  {
    gdb_assert (!m_per_objfile->queue.has_value ());

    m_per_objfile->queue.emplace ();
  }

  /* Free any entries remaining on the queue.  There should only be
     entries left if we hit an error while processing the dwarf.  */
  ~dwarf2_queue_guard ()
  {
    gdb_assert (m_per_objfile->queue.has_value ());

    m_per_objfile->queue.reset ();
  }

  DISABLE_COPY_AND_ASSIGN (dwarf2_queue_guard);

private:
  dwarf2_per_objfile *m_per_objfile;
};

dwarf2_queue_item::~dwarf2_queue_item ()
{
  /* Anything still marked queued is likely to be in an
     inconsistent state, so discard it.  */
  if (per_cu->queued)
    {
      per_objfile->remove_cu (per_cu);
      per_cu->queued = 0;
    }
}

/* See dwarf2/read.h.  */

void
dwarf2_per_cu_deleter::operator() (dwarf2_per_cu *data)
{
  if (data->is_debug_types)
    delete static_cast<signatured_type *> (data);
  else
    delete data;
}

static const char *compute_include_file_name
     (const struct line_header *lh,
      const file_entry &fe,
      const file_and_directory &cu_info,
      std::string &name_holder);

static struct dwo_unit *lookup_dwo_unit_in_dwp
  (dwarf2_per_bfd *per_bfd, struct dwp_file *dwp_file,
   const char *comp_dir, ULONGEST signature, int is_debug_types);

static void open_and_init_dwp_file (dwarf2_per_objfile *per_objfile);

static void queue_and_load_all_dwo_tus (dwarf2_cu *cu);

static void process_cu_includes (dwarf2_per_objfile *per_objfile);


/* Various complaints about symbol reading that don't abort the process.  */

static void
dwarf2_debug_line_missing_file_complaint (void)
{
  complaint (_(".debug_line section has line data without a file"));
}

static void
dwarf2_debug_line_missing_end_sequence_complaint (void)
{
  complaint (_(".debug_line section has line "
	       "program sequence without an end"));
}

static void
dwarf2_complex_location_expr_complaint (void)
{
  complaint (_("location expression too complex"));
}

static void
dwarf2_const_value_length_mismatch_complaint (const char *arg1, int arg2,
					      int arg3)
{
  complaint (_("const value length mismatch for '%s', got %d, expected %d"),
	     arg1, arg2, arg3);
}

static void
dwarf2_invalid_attrib_class_complaint (const char *arg1, const char *arg2)
{
  complaint (_("invalid attribute class or form for '%s' in '%s'"),
	     arg1, arg2);
}

/* See read.h.  */

CORE_ADDR
dwarf2_per_objfile::relocate (unrelocated_addr addr)
{
  CORE_ADDR baseaddr = objfile->text_section_offset ();
  CORE_ADDR tem = (CORE_ADDR) addr + baseaddr;
  return gdbarch_adjust_dwarf2_addr (objfile->arch (), tem);
}

/* Hash function for line_header_hash.  */

static hashval_t
line_header_hash (const struct line_header *ofs)
{
  return to_underlying (ofs->sect_off) ^ ofs->offset_in_dwz;
}

/* Hash function for htab_create_alloc_ex for line_header_hash.  */

static hashval_t
line_header_hash_voidp (const void *item)
{
  const struct line_header *ofs = (const struct line_header *) item;

  return line_header_hash (ofs);
}

/* Equality function for line_header_hash.  */

static int
line_header_eq_voidp (const void *item_lhs, const void *item_rhs)
{
  const struct line_header *ofs_lhs = (const struct line_header *) item_lhs;
  const struct line_header *ofs_rhs = (const struct line_header *) item_rhs;

  return (ofs_lhs->sect_off == ofs_rhs->sect_off
	  && ofs_lhs->offset_in_dwz == ofs_rhs->offset_in_dwz);
}

/* See declaration.  */

dwarf2_per_bfd::dwarf2_per_bfd (bfd *obfd, const dwarf2_debug_sections *names,
				bool can_copy_)
  : obfd (obfd),
    can_copy (can_copy_),
    captured_cwd (current_directory),
    captured_debug_dir (debug_file_directory)
{
  if (names == NULL)
    names = &dwarf2_elf_names;

  for (asection *sec = obfd->sections; sec != NULL; sec = sec->next)
    this->locate_sections (sec, *names);
}

dwarf2_per_bfd::~dwarf2_per_bfd ()
{
  /* Data from the per-BFD may be needed when finalizing the cooked
     index table, so wait here while this happens.  */
  if (index_table != nullptr)
    index_table->wait_completely ();

  for (auto &per_cu : all_units)
    per_cu->free_cached_file_names ();

  /* Everything else should be on this->obstack.  */
}

/* See read.h.  */

void
dwarf2_per_objfile::remove_all_cus ()
{
  gdb_assert (!queue.has_value ());

  m_dwarf2_cus.clear ();
}

/* A helper class that calls free_cached_comp_units on
   destruction.  */

class free_cached_comp_units
{
public:

  explicit free_cached_comp_units (dwarf2_per_objfile *per_objfile)
    : m_per_objfile (per_objfile)
  {
  }

  ~free_cached_comp_units ()
  {
    m_per_objfile->remove_all_cus ();
  }

  DISABLE_COPY_AND_ASSIGN (free_cached_comp_units);

private:

  dwarf2_per_objfile *m_per_objfile;
};

/* See read.h.  */

bool
dwarf2_per_objfile::symtab_set_p (const dwarf2_per_cu *per_cu) const
{
  if (per_cu->index < m_symtabs.size ())
    return m_symtabs[per_cu->index] != nullptr;
  return false;
}

/* See read.h.  */

compunit_symtab *
dwarf2_per_objfile::get_symtab (const dwarf2_per_cu *per_cu) const
{
  if (per_cu->index < m_symtabs.size ())
    return m_symtabs[per_cu->index];
  return nullptr;
}

/* See read.h.  */

void
dwarf2_per_objfile::set_symtab (const dwarf2_per_cu *per_cu,
				compunit_symtab *symtab)
{
  if (per_cu->index >= m_symtabs.size ())
    m_symtabs.resize (per_cu->index + 1);
  gdb_assert (m_symtabs[per_cu->index] == nullptr);
  m_symtabs[per_cu->index] = symtab;
}

/* Helper function for dwarf2_initialize_objfile that creates the
   per-BFD object.  */

static bool
dwarf2_has_info (struct objfile *objfile,
		 const struct dwarf2_debug_sections *names,
		 bool can_copy)
{
  if (objfile->flags & OBJF_READNEVER)
    return false;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  bool just_created = false;

  if (per_objfile == NULL)
    {
      dwarf2_per_bfd *per_bfd;

      /* We can share a "dwarf2_per_bfd" with other objfiles if the
	 BFD doesn't require relocations.

	 We don't share with objfiles for which -readnow was requested,
	 because it would complicate things when loading the same BFD with
	 -readnow and then without -readnow.  */
      if (!gdb_bfd_requires_relocations (objfile->obfd.get ())
	  && (objfile->flags & OBJF_READNOW) == 0)
	{
	  /* See if one has been created for this BFD yet.  */
	  per_bfd = dwarf2_per_bfd_bfd_data_key.get (objfile->obfd.get ());

	  if (per_bfd == nullptr)
	    {
	      /* No, create it now.  */
	      per_bfd = new dwarf2_per_bfd (objfile->obfd.get (), names,
					    can_copy);
	      dwarf2_per_bfd_bfd_data_key.set (objfile->obfd.get (), per_bfd);
	      just_created = true;
	    }
	}
      else
	{
	  /* No sharing possible, create one specifically for this objfile.  */
	  per_bfd = new dwarf2_per_bfd (objfile->obfd.get (), names, can_copy);
	  dwarf2_per_bfd_objfile_data_key.set (objfile, per_bfd);
	  just_created = true;
	}

      per_objfile = dwarf2_objfile_data_key.emplace (objfile, objfile, per_bfd);
    }

  /* Virtual sections are created from DWP files.  It's not clear those
     can occur here, so perhaps the is_virtual checks here are dead code.  */
  const bool has_info = (!per_objfile->per_bfd->infos.empty ()
			 && !per_objfile->per_bfd->infos[0].is_virtual
			 && per_objfile->per_bfd->infos[0].s.section != nullptr
			 && !per_objfile->per_bfd->abbrev.is_virtual
			 && per_objfile->per_bfd->abbrev.s.section != nullptr);

  if (just_created && has_info)
    {
      /* Try to fetch any potential dwz file early, while still on
	 the main thread.  Also, be sure to do it just once per
	 BFD, to avoid races.  */
      try
	{
	  dwz_file::read_dwz_file (per_objfile);
	}
      catch (const gdb_exception_error &err)
	{
	  warning (_("%s"), err.what ());
	}

      try
	{
	  open_and_init_dwp_file (per_objfile);
	}
      catch (const gdb_exception_error &err)
	{
	  warning (_("%s"), err.what ());
	}
    }

  return has_info;
}

/* See declaration.  */

void
dwarf2_per_bfd::locate_sections (asection *sectp,
				 const dwarf2_debug_sections &names)
{
  flagword aflag = bfd_section_flags (sectp);

  if ((aflag & SEC_HAS_CONTENTS) == 0)
    {
    }
  else if (bfd_section_size_insane (this->obfd, sectp))
    {
      bfd_size_type size = sectp->size;
      warning (_("Discarding section %s which has an invalid size (%s) "
		 "[in module %s]"),
	       bfd_section_name (sectp), phex_nz (size),
	       this->filename ());
    }
  else if (names.info.matches (sectp->name))
    {
      struct dwarf2_section_info info_section;
      memset (&info_section, 0, sizeof (info_section));
      info_section.s.section = sectp;
      info_section.size = bfd_section_size (sectp);
      this->infos.push_back (info_section);
    }
  else if (names.abbrev.matches (sectp->name))
    {
      this->abbrev.s.section = sectp;
      this->abbrev.size = bfd_section_size (sectp);
    }
  else if (names.line.matches (sectp->name))
    {
      this->line.s.section = sectp;
      this->line.size = bfd_section_size (sectp);
    }
  else if (names.loc.matches (sectp->name))
    {
      this->loc.s.section = sectp;
      this->loc.size = bfd_section_size (sectp);
    }
  else if (names.loclists.matches (sectp->name))
    {
      this->loclists.s.section = sectp;
      this->loclists.size = bfd_section_size (sectp);
    }
  else if (names.macinfo.matches (sectp->name))
    {
      this->macinfo.s.section = sectp;
      this->macinfo.size = bfd_section_size (sectp);
    }
  else if (names.macro.matches (sectp->name))
    {
      this->macro.s.section = sectp;
      this->macro.size = bfd_section_size (sectp);
    }
  else if (names.str.matches (sectp->name))
    {
      this->str.s.section = sectp;
      this->str.size = bfd_section_size (sectp);
    }
  else if (names.str_offsets.matches (sectp->name))
    {
      this->str_offsets.s.section = sectp;
      this->str_offsets.size = bfd_section_size (sectp);
    }
  else if (names.line_str.matches (sectp->name))
    {
      this->line_str.s.section = sectp;
      this->line_str.size = bfd_section_size (sectp);
    }
  else if (names.addr.matches (sectp->name))
    {
      this->addr.s.section = sectp;
      this->addr.size = bfd_section_size (sectp);
    }
  else if (names.frame.matches (sectp->name))
    {
      this->frame.s.section = sectp;
      this->frame.size = bfd_section_size (sectp);
    }
  else if (names.eh_frame.matches (sectp->name))
    {
      this->eh_frame.s.section = sectp;
      this->eh_frame.size = bfd_section_size (sectp);
    }
  else if (names.ranges.matches (sectp->name))
    {
      this->ranges.s.section = sectp;
      this->ranges.size = bfd_section_size (sectp);
    }
  else if (names.rnglists.matches (sectp->name))
    {
      this->rnglists.s.section = sectp;
      this->rnglists.size = bfd_section_size (sectp);
    }
  else if (names.types.matches (sectp->name))
    {
      struct dwarf2_section_info type_section;

      memset (&type_section, 0, sizeof (type_section));
      type_section.s.section = sectp;
      type_section.size = bfd_section_size (sectp);

      this->types.push_back (type_section);
    }
  else if (names.gdb_index.matches (sectp->name))
    {
      this->gdb_index.s.section = sectp;
      this->gdb_index.size = bfd_section_size (sectp);
    }
  else if (names.debug_names.matches (sectp->name))
    {
      this->debug_names.s.section = sectp;
      this->debug_names.size = bfd_section_size (sectp);
    }
  else if (names.debug_aranges.matches (sectp->name))
    {
      this->debug_aranges.s.section = sectp;
      this->debug_aranges.size = bfd_section_size (sectp);
    }

  if ((bfd_section_flags (sectp) & (SEC_LOAD | SEC_ALLOC))
      && bfd_section_vma (sectp) == 0)
    this->has_section_at_zero = true;
}

/* Fill in SECTP, BUFP and SIZEP with section info, given OBJFILE and
   SECTION_NAME.  */

void
dwarf2_get_section_info (struct objfile *objfile,
			 enum dwarf2_section_enum sect,
			 asection **sectp, const gdb_byte **bufp,
			 bfd_size_type *sizep)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  struct dwarf2_section_info *info;

  /* We may see an objfile without any DWARF, in which case we just
     return nothing.  */
  if (per_objfile == NULL)
    {
      *sectp = NULL;
      *bufp = NULL;
      *sizep = 0;
      return;
    }
  switch (sect)
    {
    case DWARF2_DEBUG_FRAME:
      info = &per_objfile->per_bfd->frame;
      break;
    case DWARF2_EH_FRAME:
      info = &per_objfile->per_bfd->eh_frame;
      break;
    default:
      gdb_assert_not_reached ("unexpected section");
    }

  info->read (objfile);

  *sectp = info->get_bfd_section ();
  *bufp = info->buffer;
  *sizep = info->size;
}

/* See dwarf2/read.h.  */

void
dwarf2_per_bfd::map_info_sections (struct objfile *objfile)
{
  for (auto &section : infos)
    section.read (objfile);

  abbrev.read (objfile);
  line.read (objfile);
  str.read (objfile);
  str_offsets.read (objfile);
  line_str.read (objfile);
  ranges.read (objfile);
  rnglists.read (objfile);
  addr.read (objfile);
  debug_aranges.read (objfile);

  for (auto &section : types)
    section.read (objfile);
}

/* See dwarf2/read.h.  */

void
dwarf2_per_bfd::start_reading (dwarf_scanner_base_up new_table)
{
  gdb_assert (index_table == nullptr);
  index_table = std::move (new_table);
  index_table->start_reading ();
}


/* DWARF quick_symbol_functions support.  */

/* TUs can share .debug_line entries, and there can be a lot more TUs than
   unique line tables, so we maintain a separate table of all .debug_line
   derived entries to support the sharing.
   All the quick functions need is the list of file names.  We discard the
   line_header when we're done and don't need to record it here.  */
struct quick_file_names
{
  /* The number of entries in file_names, real_names.  */
  unsigned int num_file_names;

  /* The CU directory, as given by DW_AT_comp_dir.  May be
     nullptr.  */
  const char *comp_dir;

  /* The file names from the line table, after being run through
     file_full_name.  */
  const char **file_names;

  /* The file names from the line table after being run through
     gdb_realpath.  These are computed lazily.  */
  const char **real_names;
};

/* With OBJF_READNOW, the DWARF reader expands all CUs immediately.
   It's handy in this case to have an empty implementation of the
   quick symbol functions, to avoid special cases in the rest of the
   code.  */

struct readnow_functions : public dwarf2_base_index_functions
{
  void dump (struct objfile *objfile) override
  {
  }

  bool expand_symtabs_matching
    (struct objfile *objfile,
     expand_symtabs_file_matcher file_matcher,
     const lookup_name_info *lookup_name,
     expand_symtabs_symbol_matcher symbol_matcher,
     expand_symtabs_expansion_listener expansion_notify,
     block_search_flags search_flags,
     domain_search_flags domain,
     expand_symtabs_lang_matcher lang_matcher) override
  {
    return true;
  }
};

/* See read.h.  */

std::uint64_t
stmt_list_hash_hash::operator() (const stmt_list_hash &key) const noexcept
{
  std::uint64_t v = 0;

  if (key.dwo_unit != nullptr)
    v += ankerl::unordered_dense::hash<dwo_file *> () (key.dwo_unit->dwo_file);

  v += (ankerl::unordered_dense::hash<std::uint64_t> ()
	(to_underlying (key.line_sect_off)));
  return v;
}

/* See read.h.  */

bool
stmt_list_hash::operator== (const stmt_list_hash &rhs) const noexcept
{
  if ((this->dwo_unit != nullptr) != (rhs.dwo_unit != nullptr))
    return false;

  if (this->dwo_unit != nullptr
      && this->dwo_unit->dwo_file != rhs.dwo_unit->dwo_file)
    return false;

  return this->line_sect_off == rhs.line_sect_off;
}

/* Read in CU (dwarf2_cu object) for PER_CU in the context of PER_OBJFILE.  This
   function is unrelated to symtabs, symtab would have to be created afterwards.
   You should call age_cached_comp_units after processing the CU.  */

static dwarf2_cu *
load_cu (dwarf2_per_cu *per_cu, dwarf2_per_objfile *per_objfile,
	 bool skip_partial)
{
  if (per_cu->is_debug_types)
    load_full_type_unit (per_cu, per_objfile);
  else
    load_full_comp_unit (per_cu, per_objfile, skip_partial, language_minimal);

  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu == nullptr)
    return nullptr;  /* Dummy CU.  */

  dwarf2_find_base_address (cu->dies, cu);

  return cu;
}

/* Read in the symbols for PER_CU in the context of PER_OBJFILE.  */

static void
dw2_do_instantiate_symtab (dwarf2_per_cu *per_cu,
			   dwarf2_per_objfile *per_objfile, bool skip_partial)
{
  {
    /* The destructor of dwarf2_queue_guard frees any entries left on
       the queue.  After this point we're guaranteed to leave this function
       with the dwarf queue empty.  */
    dwarf2_queue_guard q_guard (per_objfile);

    if (!per_objfile->symtab_set_p (per_cu))
      {
	queue_comp_unit (per_cu, per_objfile);
	dwarf2_cu *cu = per_objfile->get_cu (per_cu);

	if (cu == nullptr)
	  cu = load_cu (per_cu, per_objfile, skip_partial);

	/* If we just loaded a CU from a DWO, and we're working with an index
	   that may badly handle TUs, load all the TUs in that DWO as well.
	   http://sourceware.org/bugzilla/show_bug.cgi?id=15021  */
	if (!per_cu->is_debug_types
	    && cu != NULL
	    && cu->dwo_unit != NULL
	    && per_objfile->per_bfd->index_table != NULL
	    && !per_objfile->per_bfd->index_table->version_check ()
	    /* DWP files aren't supported yet.  */
	    && per_objfile->per_bfd->dwp_file == nullptr)
	  queue_and_load_all_dwo_tus (cu);
      }

    process_queue (per_objfile);
  }

  /* Age the cache, releasing compilation units that have not
     been used recently.  */
  per_objfile->age_comp_units ();
}

/* Ensure that the symbols for PER_CU have been read in.  DWARF2_PER_OBJFILE is
   the per-objfile for which this symtab is instantiated.

   Returns the resulting symbol table.  */

static struct compunit_symtab *
dw2_instantiate_symtab (dwarf2_per_cu *per_cu, dwarf2_per_objfile *per_objfile,
			bool skip_partial)
{
  if (!per_objfile->symtab_set_p (per_cu))
    {
      free_cached_comp_units freer (per_objfile);
      scoped_restore decrementer = increment_reading_symtab ();
      dw2_do_instantiate_symtab (per_cu, per_objfile, skip_partial);
      process_cu_includes (per_objfile);
    }

  return per_objfile->get_symtab (per_cu);
}

/* See read.h.  */

dwarf2_per_cu_up
dwarf2_per_bfd::allocate_per_cu (dwarf2_section_info *section,
				 sect_offset sect_off, unsigned int length,
				 bool is_dwz)
{
  dwarf2_per_cu_up result (new dwarf2_per_cu (this, section, sect_off,
					      length, is_dwz));
  result->index = all_units.size ();
  this->num_comp_units++;
  return result;
}

/* See read.h.  */

signatured_type_up
dwarf2_per_bfd::allocate_signatured_type (dwarf2_section_info *section,
					  sect_offset sect_off,
					  unsigned int length,
					  bool is_dwz,
					  ULONGEST signature)
{
  auto result
    = std::make_unique<signatured_type> (this, section, sect_off, length,
					 is_dwz, signature);
  result->index = all_units.size ();
  this->num_type_units++;
  return result;
}

/* die_reader_func for dw2_get_file_names.  */

static void
dw2_get_file_names_reader (dwarf2_cu *cu, die_info *comp_unit_die)
{
  dwarf2_per_cu *this_cu = cu->per_cu;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  gdb_assert (! this_cu->is_debug_types);

  this_cu->files_read = true;
  /* Our callers never want to match partial units -- instead they
     will match the enclosing full CU.  */
  if (comp_unit_die->tag == DW_TAG_partial_unit)
    return;

  line_header_up lh;

  file_and_directory &fnd = find_file_and_directory (comp_unit_die, cu);
  std::optional<stmt_list_hash> stmt_list_hash_key;
  attribute *attr = dwarf2_attr (comp_unit_die, DW_AT_stmt_list, cu);

  if (attr != nullptr && attr->form_is_unsigned ())
    {
      sect_offset line_offset = (sect_offset) attr->as_unsigned ();

      /* We may have already read in this line header (TU line header sharing).
	 If we have we're done.  */
      stmt_list_hash_key = {cu->dwo_unit, line_offset};

      if (auto it = per_bfd->quick_file_names_table.find (*stmt_list_hash_key);
	  it != per_bfd->quick_file_names_table.end ())
	{
	  this_cu->file_names = it->second;
	  return;
	}

      lh = dwarf_decode_line_header (line_offset, cu, fnd.get_comp_dir ());
    }

  int offset = 0;
  if (!fnd.is_unknown ())
    ++offset;
  else if (lh == nullptr)
    return;

  auto *qfn = XOBNEW (&per_bfd->obstack, quick_file_names);

  /* There may not be a DW_AT_stmt_list.  */
  if (stmt_list_hash_key.has_value ())
    per_bfd->quick_file_names_table.emplace (*stmt_list_hash_key, qfn);

  std::vector<const char *> include_names;
  if (lh != nullptr)
    {
      for (const auto &entry : lh->file_names ())
	{
	  std::string name_holder;
	  const char *include_name =
	    compute_include_file_name (lh.get (), entry, fnd, name_holder);
	  if (include_name != nullptr)
	    {
	      include_name = per_objfile->objfile->intern (include_name);
	      include_names.push_back (include_name);
	    }
	}
    }

  qfn->num_file_names = offset + include_names.size ();
  qfn->comp_dir = fnd.intern_comp_dir (per_objfile->objfile);
  qfn->file_names
    = XOBNEWVEC (&per_bfd->obstack, const char *, qfn->num_file_names);
  if (offset != 0)
    qfn->file_names[0] = per_objfile->objfile->intern (fnd.get_name ());

  if (!include_names.empty ())
    memcpy (&qfn->file_names[offset], include_names.data (),
	    include_names.size () * sizeof (const char *));

  qfn->real_names = NULL;

  this_cu->file_names = qfn;
}

/* A helper for the "quick" functions which attempts to read the line
   table for THIS_CU.  */

static struct quick_file_names *
dw2_get_file_names (dwarf2_per_cu *this_cu, dwarf2_per_objfile *per_objfile)
{
  /* This should never be called for TUs.  */
  gdb_assert (! this_cu->is_debug_types);

  if (this_cu->files_read)
    return this_cu->file_names;

  cutu_reader reader (*this_cu, *per_objfile, nullptr,
		      per_objfile->get_cu (this_cu), true, language_minimal,
		      nullptr);
  if (!reader.is_dummy ())
    dw2_get_file_names_reader (reader.cu (), reader.top_level_die ());

  return this_cu->file_names;
}

/* A helper for the "quick" functions which computes and caches the
   real path for a given file name from the line table.  */

static const char *
dw2_get_real_path (dwarf2_per_objfile *per_objfile,
		   struct quick_file_names *qfn, int index)
{
  if (qfn->real_names == NULL)
    qfn->real_names = OBSTACK_CALLOC (&per_objfile->per_bfd->obstack,
				      qfn->num_file_names, const char *);

  if (qfn->real_names[index] == NULL)
    {
      const char *dirname = nullptr;

      if (!IS_ABSOLUTE_PATH (qfn->file_names[index]))
	dirname = qfn->comp_dir;

      gdb::unique_xmalloc_ptr<char> fullname;
      fullname = find_source_or_rewrite (qfn->file_names[index], dirname);

      qfn->real_names[index] = fullname.release ();
    }

  return qfn->real_names[index];
}

struct symtab *
dwarf2_base_index_functions::find_last_source_symtab (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  dwarf2_per_cu *dwarf_cu = per_objfile->per_bfd->all_units.back ().get ();
  compunit_symtab *cust = dw2_instantiate_symtab (dwarf_cu, per_objfile, false);

  if (cust == NULL)
    return NULL;

  return cust->primary_filetab ();
}

/* See read.h.  */

void
dwarf2_per_cu::free_cached_file_names ()
{
  if (fnd != nullptr)
    fnd->forget_fullname ();

  if (per_bfd == nullptr)
    return;

  struct quick_file_names *file_data = file_names;
  if (file_data != nullptr && file_data->real_names != nullptr)
    {
      for (int i = 0; i < file_data->num_file_names; ++i)
	{
	  xfree ((void *) file_data->real_names[i]);
	  file_data->real_names[i] = nullptr;
	}
    }
}

void
dwarf2_base_index_functions::forget_cached_source_info
     (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  for (auto &per_cu : per_objfile->per_bfd->all_units)
    per_cu->free_cached_file_names ();
}

void
dwarf2_base_index_functions::print_stats (struct objfile *objfile,
					  bool print_bcache)
{
  if (print_bcache)
    return;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  int total = per_objfile->per_bfd->all_units.size ();
  int count = 0;

  for (int i = 0; i < total; ++i)
    {
      dwarf2_per_cu *per_cu = per_objfile->per_bfd->get_unit (i);

      if (!per_objfile->symtab_set_p (per_cu))
	++count;
    }
  gdb_printf (_("  Number of read units: %d\n"), total - count);
  gdb_printf (_("  Number of unread units: %d\n"), count);
}

void
dwarf2_base_index_functions::expand_all_symtabs (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  for (dwarf2_per_cu *per_cu : all_units_range (per_objfile->per_bfd))
    {
      /* We don't want to directly expand a partial CU, because if we
	 read it with the wrong language, then assertion failures can
	 be triggered later on.  See PR symtab/23010.  So, tell
	 dw2_instantiate_symtab to skip partial CUs -- any important
	 partial CU will be read via DW_TAG_imported_unit anyway.  */
      dw2_instantiate_symtab (per_cu, per_objfile, true);
    }
}

/* See read.h.  */

bool
dw2_expand_symtabs_matching_one
  (dwarf2_per_cu *per_cu,
   dwarf2_per_objfile *per_objfile,
   expand_symtabs_file_matcher file_matcher,
   expand_symtabs_expansion_listener expansion_notify,
   expand_symtabs_lang_matcher lang_matcher)
{
  if (file_matcher != nullptr && !per_cu->mark)
    return true;

  if (lang_matcher != nullptr)
    {
      /* Try to skip CUs with non-matching language.  */
      per_cu->ensure_lang (per_objfile);
      if (!per_cu->maybe_multi_language ()
	  && !lang_matcher (per_cu->lang ()))
	return true;
    }

  bool symtab_was_null = !per_objfile->symtab_set_p (per_cu);
  compunit_symtab *symtab
    = dw2_instantiate_symtab (per_cu, per_objfile, false);
  gdb_assert (symtab != nullptr);

  if (expansion_notify != NULL && symtab_was_null)
    return expansion_notify (symtab);

  return true;
}

/* See read.h.  */

void
dw_expand_symtabs_matching_file_matcher
  (dwarf2_per_objfile *per_objfile, expand_symtabs_file_matcher file_matcher)
{
  if (file_matcher == NULL)
    return;

  gdb::unordered_set<quick_file_names *> visited_found;
  gdb::unordered_set<quick_file_names *> visited_not_found;

  /* The rule is CUs specify all the files, including those used by
     any TU, so there's no need to scan TUs here.  */

  for (const auto &per_cu : per_objfile->per_bfd->all_units)
    {
      QUIT;

      if (per_cu->is_debug_types)
	continue;
      per_cu->mark = 0;

      /* We only need to look at symtabs not already expanded.  */
      if (per_objfile->symtab_set_p (per_cu.get ()))
	continue;

      if (per_cu->fnd != nullptr)
	{
	  file_and_directory *fnd = per_cu->fnd.get ();

	  if (file_matcher (fnd->get_name (), false))
	    {
	      per_cu->mark = 1;
	      continue;
	    }

	  /* Before we invoke realpath, which can get expensive when many
	     files are involved, do a quick comparison of the basenames.  */
	  if ((basenames_may_differ
	       || file_matcher (lbasename (fnd->get_name ()), true))
	      && file_matcher (fnd->get_fullname (), false))
	    {
	      per_cu->mark = 1;
	      continue;
	    }
	}

      quick_file_names *file_data = dw2_get_file_names (per_cu.get (),
							per_objfile);
      if (file_data == NULL)
	continue;

      if (visited_not_found.contains (file_data))
	continue;
      else if (visited_found.contains (file_data))
	{
	  per_cu->mark = 1;
	  continue;
	}

      for (int j = 0; j < file_data->num_file_names; ++j)
	{
	  const char *this_real_name;

	  if (file_matcher (file_data->file_names[j], false))
	    {
	      per_cu->mark = 1;
	      break;
	    }

	  /* Before we invoke realpath, which can get expensive when many
	     files are involved, do a quick comparison of the basenames.  */
	  if (!basenames_may_differ
	      && !file_matcher (lbasename (file_data->file_names[j]),
				true))
	    continue;

	  this_real_name = dw2_get_real_path (per_objfile, file_data, j);
	  if (file_matcher (this_real_name, false))
	    {
	      per_cu->mark = 1;
	      break;
	    }
	}

      if (per_cu->mark)
	visited_found.insert (file_data);
      else
	visited_not_found.insert (file_data);
    }
}


/* A helper for dw2_find_pc_sect_compunit_symtab which finds the most specific
   symtab.  */

static struct compunit_symtab *
recursively_find_pc_sect_compunit_symtab (struct compunit_symtab *cust,
					  CORE_ADDR pc)
{
  int i;

  if (cust->blockvector () != nullptr
      && blockvector_contains_pc (cust->blockvector (), pc))
    return cust;

  if (cust->includes == NULL)
    return NULL;

  for (i = 0; cust->includes[i]; ++i)
    {
      struct compunit_symtab *s = cust->includes[i];

      s = recursively_find_pc_sect_compunit_symtab (s, pc);
      if (s != NULL)
	return s;
    }

  return NULL;
}

struct compunit_symtab *
dwarf2_base_index_functions::find_pc_sect_compunit_symtab
     (struct objfile *objfile,
      bound_minimal_symbol msymbol,
      CORE_ADDR pc,
      struct obj_section *section,
      int warn_if_readin)
{
  struct compunit_symtab *result;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  if (per_bfd->index_table == nullptr)
    return nullptr;

  CORE_ADDR baseaddr = objfile->text_section_offset ();
  dwarf2_per_cu *data
    = per_bfd->index_table->lookup ((unrelocated_addr) (pc - baseaddr));
  if (data == nullptr)
    return nullptr;

  if (warn_if_readin && per_objfile->symtab_set_p (data))
    warning (_("(Internal error: pc %s in read in CU, but not in symtab.)"),
	     paddress (objfile->arch (), pc));

  result = recursively_find_pc_sect_compunit_symtab
    (dw2_instantiate_symtab (data, per_objfile, false), pc);

  if (warn_if_readin && result == nullptr)
    warning (_("(Error: pc %s in address map, but not in symtab.)"),
	     paddress (objfile->arch (), pc));

  return result;
}

void
dwarf2_base_index_functions::map_symbol_filenames (objfile *objfile,
						   symbol_filename_listener fun,
						   bool need_fullname)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  /* Use caches to ensure we only call FUN once for each filename.  */
  filename_seen_cache filenames_cache;
  gdb::unordered_set<quick_file_names *> qfn_cache;

  /* The rule is CUs specify all the files, including those used by any TU,
     so there's no need to scan TUs here.  We can ignore file names coming
     from already-expanded CUs.  It is possible that an expanded CU might
     reuse the file names data from a currently unexpanded CU, in this
     case we don't want to report the files from the unexpanded CU.  */

  for (const auto &per_cu : per_objfile->per_bfd->all_units)
    {
      if (!per_cu->is_debug_types
	  && per_objfile->symtab_set_p (per_cu.get ()))
	{
	  if (per_cu->file_names != nullptr)
	    qfn_cache.insert (per_cu->file_names);
	}
    }

  for (dwarf2_per_cu *per_cu : all_units_range (per_objfile->per_bfd))
    {
      /* We only need to look at symtabs not already expanded.  */
      if (per_cu->is_debug_types || per_objfile->symtab_set_p (per_cu))
	continue;

      if (per_cu->fnd != nullptr)
	{
	  file_and_directory *fnd = per_cu->fnd.get ();

	  const char *filename = fnd->get_name ();
	  const char *key = filename;
	  const char *fullname = nullptr;

	  if (need_fullname)
	    {
	      fullname = fnd->get_fullname ();
	      key = fullname;
	    }

	  if (!filenames_cache.seen (key))
	    fun (filename, fullname);
	}

      quick_file_names *file_data = dw2_get_file_names (per_cu, per_objfile);
      if (file_data == nullptr
	  || qfn_cache.find (file_data) != qfn_cache.end ())
	continue;

      for (int j = 0; j < file_data->num_file_names; ++j)
	{
	  const char *filename = file_data->file_names[j];
	  const char *key = filename;
	  const char *fullname = nullptr;

	  if (need_fullname)
	    {
	      fullname = dw2_get_real_path (per_objfile, file_data, j);
	      key = fullname;
	    }

	  if (!filenames_cache.seen (key))
	    fun (filename, fullname);
	}
    }
}

bool
dwarf2_base_index_functions::has_symbols (struct objfile *objfile)
{
  return true;
}

/* See quick_symbol_functions::has_unexpanded_symtabs in quick-symbol.h.  */

bool
dwarf2_base_index_functions::has_unexpanded_symtabs (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  for (const auto &per_cu : per_objfile->per_bfd->all_units)
    {
      /* Is this already expanded?  */
      if (per_objfile->symtab_set_p (per_cu.get ()))
	continue;

      /* It has not yet been expanded.  */
      return true;
    }

  return false;
}

/* Get the content of the .gdb_index section of OBJ.  SECTION_OWNER should point
   to either a dwarf2_per_bfd or dwz_file object.  */

template <typename T>
static gdb::array_view<const gdb_byte>
get_gdb_index_contents_from_section (objfile *obj, T *section_owner)
{
  dwarf2_section_info *section = &section_owner->gdb_index;

  if (section->empty ())
    return {};

  /* Older elfutils strip versions could keep the section in the main
     executable while splitting it for the separate debug info file.  */
  if ((section->get_flags () & SEC_HAS_CONTENTS) == 0)
    return {};

  section->read (obj);

  /* dwarf2_section_info::size is a bfd_size_type, while
     gdb::array_view works with size_t.  On 32-bit hosts, with
     --enable-64-bit-bfd, bfd_size_type is a 64-bit type, while size_t
     is 32-bit.  So we need an explicit narrowing conversion here.
     This is fine, because it's impossible to allocate or mmap an
     array/buffer larger than what size_t can represent.  */
  return gdb::make_array_view (section->buffer, section->size);
}

/* Lookup the index cache for the contents of the index associated to
   DWARF2_OBJ.  */

static gdb::array_view<const gdb_byte>
get_gdb_index_contents_from_cache (objfile *obj, dwarf2_per_bfd *dwarf2_per_bfd)
{
  const bfd_build_id *build_id = build_id_bfd_get (obj->obfd.get ());
  if (build_id == nullptr)
    return {};

  return global_index_cache.lookup_gdb_index (build_id,
					      &dwarf2_per_bfd->index_cache_res);
}

/* Same as the above, but for DWZ.  */

static gdb::array_view<const gdb_byte>
get_gdb_index_contents_from_cache_dwz (objfile *obj, dwz_file *dwz)
{
  const bfd_build_id *build_id = build_id_bfd_get (dwz->dwz_bfd.get ());
  if (build_id == nullptr)
    return {};

  return global_index_cache.lookup_gdb_index (build_id, &dwz->index_cache_res);
}

static void start_debug_info_reader (dwarf2_per_objfile *);

/* See dwarf2/public.h.  */

bool
dwarf2_initialize_objfile (struct objfile *objfile,
			   const struct dwarf2_debug_sections *names,
			   bool can_copy)
{
  if (!dwarf2_has_info (objfile, names, can_copy))
    return false;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  dwarf_read_debug_printf ("called");

  /* If we're about to read full symbols, don't bother with the
     indices.  In this case we also don't care if some other debug
     format is making psymtabs, because they are all about to be
     expanded anyway.  */
  if ((objfile->flags & OBJF_READNOW))
    {
      dwarf_read_debug_printf ("readnow requested");

      create_all_units (per_objfile);
      objfile->qf.emplace_front (new readnow_functions);
    }
  /* Was a GDB index already read when we processed an objfile sharing
     PER_BFD?  */
  else if (per_bfd->index_table != nullptr)
    dwarf_read_debug_printf ("re-using symbols");
  else if (dwarf2_read_debug_names (per_objfile))
    dwarf_read_debug_printf ("found debug names");
  else if (dwarf2_read_gdb_index (per_objfile,
				  get_gdb_index_contents_from_section<struct dwarf2_per_bfd>,
				  get_gdb_index_contents_from_section<dwz_file>))
    dwarf_read_debug_printf ("found gdb index from file");
  /* ... otherwise, try to find the index in the index cache.  */
  else if (dwarf2_read_gdb_index (per_objfile,
			     get_gdb_index_contents_from_cache,
			     get_gdb_index_contents_from_cache_dwz))
    {
      dwarf_read_debug_printf ("found gdb index from cache");
      global_index_cache.hit ();
    }
  else
    {
      global_index_cache.miss ();
      start_debug_info_reader (per_objfile);
    }

  if (per_bfd->index_table != nullptr)
    {
      if (dwarf_synchronous)
	per_bfd->index_table->wait_completely ();
      objfile->qf.push_front (per_bfd->index_table->make_quick_functions ());
    }

  return true;
}

/* See read.h.  */

void
dwarf2_find_base_address (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  cu->base_address.reset ();

  attr = dwarf2_attr (die, DW_AT_entry_pc, cu);
  if (attr != nullptr)
    cu->base_address = attr->as_address ();
  else
    {
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
      if (attr != nullptr)
	cu->base_address = attr->as_address ();
    }
}

/* Helper function that returns the proper abbrev section for
   THIS_CU.  */

static struct dwarf2_section_info *
get_abbrev_section_for_cu (dwarf2_per_cu *this_cu)
{
  struct dwarf2_section_info *abbrev;
  dwarf2_per_bfd *per_bfd = this_cu->per_bfd;

  if (this_cu->is_dwz)
    abbrev = &per_bfd->get_dwz_file (true)->abbrev;
  else
    abbrev = &per_bfd->abbrev;

  return abbrev;
}

/* "less than" function used to both sort and bisect units in the
   `dwarf2_per_bfd::all_units` vector.  Return true if the LHS CU comes before
   (is "less" than) the section and offset in RHS.

   For simplicity, sort sections by their pointer.  This is not ideal, because
   it can cause the behavior to change across runs, making some bugs harder to
   investigate.  An improvement would be for sections to be sorted by their
   properties.  */

static bool
all_units_less_than (const dwarf2_per_cu &lhs, const section_and_offset &rhs)
{
  if (lhs.section != rhs.section)
    return lhs.section < rhs.section;

  return lhs.sect_off < rhs.offset;
}

/* Fetch the abbreviation table offset from a comp or type unit header.  */

static sect_offset
read_abbrev_offset (dwarf2_per_objfile *per_objfile,
		    struct dwarf2_section_info *section,
		    sect_offset sect_off)
{
  bfd *abfd = section->get_bfd_owner ();
  const gdb_byte *info_ptr;
  unsigned int initial_length_size, offset_size;
  uint16_t version;

  section->read (per_objfile->objfile);
  info_ptr = section->buffer + to_underlying (sect_off);
  read_initial_length (abfd, info_ptr, &initial_length_size);
  offset_size = initial_length_size == 4 ? 4 : 8;
  info_ptr += initial_length_size;

  version = read_2_bytes (abfd, info_ptr);
  info_ptr += 2;
  if (version >= 5)
    {
      /* Skip unit type and address size.  */
      info_ptr += 2;
    }

  return (sect_offset) read_offset (abfd, info_ptr, offset_size);
}

/* Add an entry for signature SIG to per_bfd->signatured_types.

   This functions leaves PER_BFD::ALL_UNITS unsorted.  The caller must call
   finalize_all_units after adding one or more type units.  */

static signatured_type_set::iterator
add_type_unit (dwarf2_per_bfd *per_bfd, dwarf2_section_info *section,
	       sect_offset sect_off, unsigned int length, ULONGEST sig)
{
  if (per_bfd->all_units.size () == per_bfd->all_units.capacity ())
    ++per_bfd->tu_stats.nr_all_type_units_reallocs;

  signatured_type_up sig_type_holder
    = per_bfd->allocate_signatured_type (section, sect_off, length,
					 false /* is_dwz */, sig);
  signatured_type *sig_type = sig_type_holder.get ();

  per_bfd->all_units.emplace_back (sig_type_holder.release ());
  auto emplace_ret = per_bfd->signatured_types.emplace (sig_type);

  /* Assert that an insertion took place - that there wasn't a type unit with
     that signature already.  */
  gdb_assert (emplace_ret.second);

  /* The rest of sig_type must be filled in by the caller.  */
  return emplace_ret.first;
}

/* Subroutine of lookup_dwo_signatured_type and lookup_dwp_signatured_type.
   Fill in SIG_ENTRY with DWO_ENTRY.  */

static void
fill_in_sig_entry_from_dwo_entry (dwarf2_per_objfile *per_objfile,
				  struct signatured_type *sig_entry,
				  struct dwo_unit *dwo_entry)
{
  /* Make sure we're not clobbering something we don't expect to.  */
  gdb_assert (! sig_entry->queued);
  gdb_assert (per_objfile->get_cu (sig_entry) == NULL);
  gdb_assert (!per_objfile->symtab_set_p (sig_entry));
  gdb_assert (sig_entry->signature == dwo_entry->signature);
  gdb_assert (to_underlying (sig_entry->type_offset_in_section) == 0
	      || (to_underlying (sig_entry->type_offset_in_section)
		  == to_underlying (dwo_entry->type_offset_in_tu)));
  gdb_assert (!sig_entry->type_unit_group_key.has_value ());
  gdb_assert (sig_entry->dwo_unit == NULL
	      || sig_entry->dwo_unit == dwo_entry);

  sig_entry->reading_dwo_directly = 1;
  sig_entry->type_offset_in_tu = dwo_entry->type_offset_in_tu;
  sig_entry->dwo_unit = dwo_entry;
}

/* Subroutine of lookup_signatured_type.
   If we haven't read the TU yet, create the signatured_type data structure
   for a TU to be read in directly from a DWO file, bypassing the stub.
   This is the "Stay in DWO Optimization": When there is no DWP file and we're
   using .gdb_index, then when reading a CU we want to stay in the DWO file
   containing that CU.  Otherwise we could end up reading several other DWO
   files (due to comdat folding) to process the transitive closure of all the
   mentioned TUs, and that can be slow.  The current DWO file will have every
   type signature that it needs.
   We only do this for .gdb_index because in the psymtab case we already have
   to read all the DWOs to build the type unit groups.  */

static struct signatured_type *
lookup_dwo_signatured_type (struct dwarf2_cu *cu, ULONGEST sig)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  gdb_assert (cu->dwo_unit);

  /* We only ever need to read in one copy of a signatured type.
     Use the global signatured_types array to do our own comdat-folding
     of types.  If this is the first time we're reading this TU, and
     the TU has an entry in .gdb_index, replace the recorded data from
     .gdb_index with this TU.  */

  auto sig_type_it = per_bfd->signatured_types.find (sig);

  /* We can get here with the TU already read, *or* in the process of being
     read.  Don't reassign the global entry to point to this DWO if that's
     the case.  Also note that if the TU is already being read, it may not
     have come from a DWO, the program may be a mix of Fission-compiled
     code and non-Fission-compiled code.  */

  /* Have we already tried to read this TU?
     Note: sig_entry can be NULL if the skeleton TU was removed (thus it
     needn't exist in the global table yet).  */
  if (sig_type_it != per_bfd->signatured_types.end ()
      && (*sig_type_it)->tu_read)
    return *sig_type_it;

  /* Note: cu->dwo_unit is the dwo_unit that references this TU, not the
     dwo_unit of the TU itself.  */
  dwo_file *dwo_file = cu->dwo_unit->dwo_file;
  auto it = dwo_file->tus.find (sig);
  if (it == dwo_file->tus.end ())
    return nullptr;

  dwo_unit *dwo_entry = it->get ();

  /* If the global table doesn't have an entry for this TU, add one.  */
  if (sig_type_it == per_bfd->signatured_types.end ())
    {
      sig_type_it = add_type_unit (per_bfd, dwo_entry->section,
				   dwo_entry->sect_off, dwo_entry->length, sig);
      finalize_all_units (per_bfd);
    }

  if ((*sig_type_it)->dwo_unit == nullptr)
    fill_in_sig_entry_from_dwo_entry (per_objfile, *sig_type_it, dwo_entry);

  (*sig_type_it)->tu_read = 1;
  return *sig_type_it;
}

/* Subroutine of lookup_signatured_type.
   Look up the type for signature SIG, and if we can't find SIG in .gdb_index
   then try the DWP file.  If the TU stub (skeleton) has been removed then
   it won't be in .gdb_index.  */

static struct signatured_type *
lookup_dwp_signatured_type (struct dwarf2_cu *cu, ULONGEST sig)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  dwp_file *dwp_file = per_objfile->per_bfd->dwp_file.get ();

  gdb_assert (cu->dwo_unit);
  gdb_assert (dwp_file != NULL);

  auto sig_type_it = per_bfd->signatured_types.find (sig);

  /* Have we already tried to read this TU?
     Note: sig_entry can be NULL if the skeleton TU was removed (thus it
     needn't exist in the global table yet).  */
  if (sig_type_it != per_bfd->signatured_types.end ())
    return *sig_type_it;

  if (dwp_file->tus == NULL)
    return NULL;

  auto dwo_entry = lookup_dwo_unit_in_dwp (per_bfd, dwp_file, NULL, sig,
					   1 /* is_debug_types */);
  if (dwo_entry == NULL)
    return NULL;

  sig_type_it = add_type_unit (per_bfd, dwo_entry->section,
			       dwo_entry->sect_off, dwo_entry->length, sig);
  finalize_all_units (per_bfd);
  fill_in_sig_entry_from_dwo_entry (per_objfile, *sig_type_it, dwo_entry);

  return *sig_type_it;
}

/* Lookup a signature based type for DW_FORM_ref_sig8.
   Returns NULL if signature SIG is not present in the table.
   It is up to the caller to complain about this.  */

static struct signatured_type *
lookup_signatured_type (struct dwarf2_cu *cu, ULONGEST sig)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  if (cu->dwo_unit)
    {
      /* We're in a DWO/DWP file, and we're using .gdb_index.
	 These cases require special processing.  */
      if (per_objfile->per_bfd->dwp_file == nullptr)
	return lookup_dwo_signatured_type (cu, sig);
      else
	return lookup_dwp_signatured_type (cu, sig);
    }
  else
    {
      auto sig_type_it = per_bfd->signatured_types.find (sig);

      if (sig_type_it != per_bfd->signatured_types.end ())
	return *sig_type_it;

      return nullptr;
    }
}

/* Low level DIE reading support.  */

/* Initialize a cutu_reader from a dwarf2_cu.  */

void
cutu_reader::init_cu_die_reader (dwarf2_cu *cu, dwarf2_section_info *section,
				 struct dwo_file *dwo_file,
				 const struct abbrev_table *abbrev_table)
{
  gdb_assert (section->readin && section->buffer != NULL);
  m_abfd = section->get_bfd_owner ();
  m_cu = cu;
  m_dwo_file = dwo_file;
  m_die_section = section;
  m_buffer = section->buffer;
  m_buffer_end = section->buffer + section->size;
  m_abbrev_table = abbrev_table;
}

/* Subroutine of cutu_reader to simplify it.
   Read in the rest of a CU/TU top level DIE from DWO_UNIT.
   There's just a lot of work to do, and cutu_reader is big enough
   already.

   STUB_COMP_UNIT_DIE is for the stub DIE, we copy over certain attributes
   from it to the DIE in the DWO.  If NULL we are skipping the stub.

   STUB_COMP_DIR is similar to STUB_COMP_UNIT_DIE: When reading a TU directly
   from the DWO file, bypassing the stub, it contains the DW_AT_comp_dir
   attribute of the referencing CU.  At most one of STUB_COMP_UNIT_DIE and
   STUB_COMP_DIR may be non-NULL.

   *RESULT_READER and *RESULT_TOP_LEVEL_DIE are filled in with the info of the
   DIE from the DWO file.

   *RESULT_DWO_ABBREV_TABLE will be filled in with the abbrev table allocated
   from the dwo.  Since *RESULT_READER references this abbrev table, it must be
   kept around for at least as long as *RESULT_READER.  */

void
cutu_reader::read_cutu_die_from_dwo (dwarf2_cu *cu, dwo_unit *dwo_unit,
				     die_info *stub_comp_unit_die,
				     const char *stub_comp_dir)
{
  dwarf2_per_cu *per_cu = cu->per_cu;
  struct objfile *objfile = cu->per_objfile->objfile;
  bfd *abfd;
  struct dwarf2_section_info *dwo_abbrev_section;

  /* At most one of these may be provided.  */
  gdb_assert ((stub_comp_unit_die != NULL) + (stub_comp_dir != NULL) <= 1);

  /* These attributes aren't processed until later: DW_AT_stmt_list,
     DW_AT_low_pc, DW_AT_high_pc, DW_AT_ranges, DW_AT_comp_dir.
     However, these attributes are found in the stub which we won't
     have later.  In order to not impose this complication on the rest
     of the code, we read them here and copy them to the DWO CU/TU
     die.  */

  /* We store them all in an array.  */
  struct attribute *attributes[5] {};
  /* Next available element of the attributes array.  */
  int next_attr_idx = 0;

  /* Push an element into ATTRIBUTES.  */
  auto push_back = [&] (struct attribute *attr)
    {
      gdb_assert (next_attr_idx < ARRAY_SIZE (attributes));
      if (attr != nullptr)
	attributes[next_attr_idx++] = attr;
    };

  if (stub_comp_unit_die != NULL)
    {
      /* For TUs in DWO files, the DW_AT_stmt_list attribute lives in the
	 DWO file.  */
      if (!per_cu->is_debug_types)
	push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_stmt_list, cu));
      push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_low_pc, cu));
      push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_high_pc, cu));
      push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_ranges, cu));
      push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_comp_dir, cu));

      cu->addr_base = stub_comp_unit_die->addr_base ();

      /* There should be a DW_AT_GNU_ranges_base attribute here (if needed).
	 We need the value before we can process DW_AT_ranges values from the
	 DWO.  */
      cu->gnu_ranges_base = stub_comp_unit_die->gnu_ranges_base ();

      /* For DWARF5: record the DW_AT_rnglists_base value from the skeleton.  If
	 there are attributes of form DW_FORM_rnglistx in the skeleton, they'll
	 need the rnglists base.  Attributes of form DW_FORM_rnglistx in the
	 split unit don't use it, as the DWO has its own .debug_rnglists.dwo
	 section.  */
      cu->rnglists_base = stub_comp_unit_die->rnglists_base ();
    }
  else if (stub_comp_dir != NULL)
    {
      /* Reconstruct the comp_dir attribute to simplify the code below.  */
      struct attribute *comp_dir = OBSTACK_ZALLOC (&cu->comp_unit_obstack,
						   struct attribute);
      comp_dir->name = DW_AT_comp_dir;
      comp_dir->form = DW_FORM_string;
      comp_dir->set_string_noncanonical (stub_comp_dir);
      push_back (comp_dir);
    }

  /* Set up for reading the DWO CU/TU.  */
  cu->dwo_unit = dwo_unit;
  dwarf2_section_info *section = dwo_unit->section;
  section->read (objfile);
  abfd = section->get_bfd_owner ();
  m_info_ptr = section->buffer + to_underlying (dwo_unit->sect_off);
  const gdb_byte *begin_info_ptr = m_info_ptr;
  dwo_abbrev_section = &dwo_unit->dwo_file->sections.abbrev;

  if (per_cu->is_debug_types)
    {
      signatured_type *sig_type = (struct signatured_type *) per_cu;

      m_info_ptr = read_and_check_unit_head (&cu->header, section,
					     dwo_abbrev_section, m_info_ptr,
					     ruh_kind::TYPE);

      /* This is not an assert because it can be caused by bad debug info.  */
      if (sig_type->signature != cu->header.signature)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "signature mismatch %s vs %s while reading TU at offset %s"
		   " [in module %s]"),
		 hex_string (sig_type->signature),
		 hex_string (cu->header.signature),
		 sect_offset_str (dwo_unit->sect_off),
		 bfd_get_filename (abfd));
	}
      gdb_assert (dwo_unit->sect_off == cu->header.sect_off);
      /* For DWOs coming from DWP files, we don't know the CU length
	 nor the type's offset in the TU until now.  */
      dwo_unit->length = cu->header.get_length_with_initial ();
      dwo_unit->type_offset_in_tu = cu->header.type_offset_in_tu;

      /* Establish the type offset that can be used to lookup the type.
	 For DWO files, we don't know it until now.  */
      sig_type->type_offset_in_section
	= dwo_unit->sect_off + to_underlying (dwo_unit->type_offset_in_tu);
    }
  else
    {
      m_info_ptr = read_and_check_unit_head (&cu->header, section,
					     dwo_abbrev_section, m_info_ptr,
					     ruh_kind::COMPILE);
      gdb_assert (dwo_unit->sect_off == cu->header.sect_off);
      /* For DWOs coming from DWP files, we don't know the CU length
	 until now.  */
      dwo_unit->length = cu->header.get_length_with_initial ();
    }

  /* Record some information found in the header.  This will be needed when
     evaluating DWARF expressions in the context of this unit, for instance.  */
  per_cu->set_addr_size (cu->header.addr_size);
  per_cu->set_offset_size (cu->header.offset_size);
  per_cu->set_ref_addr_size (cu->header.version == 2
			     ? cu->header.addr_size
			     : cu->header.offset_size);

  dwo_abbrev_section->read (objfile);
  m_dwo_abbrev_table
    = abbrev_table::read (dwo_abbrev_section, cu->header.abbrev_sect_off);
  this->init_cu_die_reader (cu, section, dwo_unit->dwo_file,
			    m_dwo_abbrev_table.get ());

  /* Skip dummy compilation units.  */
  if (m_info_ptr >= begin_info_ptr + dwo_unit->length
      || peek_abbrev_code (abfd, m_info_ptr) == 0)
    {
      m_dummy_p = true;
      return;
    }

  /* Read in the die, filling in the attributes from the stub.  This
     has the benefit of simplifying the rest of the code - all the
     work to maintain the illusion of a single
     DW_TAG_{compile,type}_unit DIE is done here.  */
  m_top_level_die
    = this->read_toplevel_die (gdb::make_array_view (attributes,
						     next_attr_idx));
}

/* Return the signature of the compile unit, if found. In DWARF 4 and before,
   the signature is in the DW_AT_GNU_dwo_id attribute. In DWARF 5 and later, the
   signature is part of the header.  */
static std::optional<ULONGEST>
lookup_dwo_id (struct dwarf2_cu *cu, struct die_info* comp_unit_die)
{
  if (cu->header.version >= 5)
    return cu->header.signature;
  struct attribute *attr;
  attr = dwarf2_attr (comp_unit_die, DW_AT_GNU_dwo_id, cu);
  if (attr == nullptr || !attr->form_is_unsigned ())
    return std::optional<ULONGEST> ();
  return attr->as_unsigned ();
}

/* Subroutine of cutu_reader to simplify it.
   Look up the DWO unit specified by COMP_UNIT_DIE of CU.

   DWO_NAME is the name (DW_AT_dwo_name) of the DWO unit already read from
   COMP_UNIT_DIE.

   Returns nullptr if the specified DWO unit cannot be found.  */

dwo_unit *
cutu_reader::lookup_dwo_unit (dwarf2_cu *cu, die_info *comp_unit_die,
			      const char *dwo_name)
{
  dwarf2_per_cu *per_cu = cu->per_cu;
  struct dwo_unit *dwo_unit;
  const char *comp_dir;

  gdb_assert (cu != NULL);

  comp_dir = dwarf2_string_attr (comp_unit_die, DW_AT_comp_dir, cu);

  if (per_cu->is_debug_types)
    dwo_unit = lookup_dwo_type_unit (cu, dwo_name, comp_dir);
  else
    {
      std::optional<ULONGEST> signature = lookup_dwo_id (cu, comp_unit_die);

      if (!signature.has_value ())
	error (_(DWARF_ERROR_PREFIX
		 "missing dwo_id for dwo_name %s"
		 " [in module %s]"),
	       dwo_name, per_cu->per_bfd->filename ());

      dwo_unit = lookup_dwo_comp_unit (cu, dwo_name, comp_dir, *signature);
    }

  return dwo_unit;
}

/* Subroutine of cutu_reader to simplify it.
   See it for a description of the parameters.
   Read a TU directly from a DWO file, bypassing the stub.  */

void
cutu_reader::init_tu_and_read_dwo_dies (dwarf2_per_cu *this_cu,
					dwarf2_per_objfile *per_objfile,
					dwarf2_cu *existing_cu,
					enum language pretend_language)
{
  struct signatured_type *sig_type;

  /* Verify we can do the following downcast, and that we have the
     data we need.  */
  gdb_assert (this_cu->is_debug_types && this_cu->reading_dwo_directly);
  sig_type = (struct signatured_type *) this_cu;
  gdb_assert (sig_type->dwo_unit != NULL);

  dwarf2_cu *cu;

  if (existing_cu != nullptr)
    {
      cu = existing_cu;
      gdb_assert (cu->dwo_unit == sig_type->dwo_unit);
      /* There's no need to do the rereading_dwo_cu handling that
	 cutu_reader does since we don't read the stub.  */
    }
  else
    {
      /* If an existing_cu is provided, a dwarf2_cu must not exist for this_cu
	 in per_objfile yet.  */
      gdb_assert (per_objfile->get_cu (this_cu) == nullptr);
      m_new_cu = std::make_unique<dwarf2_cu> (this_cu, per_objfile);
      cu = m_new_cu.get ();
    }

  /* A future optimization, if needed, would be to use an existing
     abbrev table.  When reading DWOs with skeletonless TUs, all the TUs
     could share abbrev tables.  */

  read_cutu_die_from_dwo (cu, sig_type->dwo_unit, NULL /* stub_comp_unit_die */,
			  sig_type->dwo_unit->dwo_file->comp_dir);
  prepare_one_comp_unit (cu, pretend_language);
}

/* Initialize a CU (or TU) and read its DIEs.
   If the CU defers to a DWO file, read the DWO file as well.

   ABBREV_TABLE, if non-NULL, is the abbreviation table to use.
   Otherwise the table specified in the comp unit header is read in and used.
   This is an optimization for when we already have the abbrev table.

   If EXISTING_CU is non-NULL, then use it.  Otherwise, a new CU is
   allocated.  */

cutu_reader::cutu_reader (dwarf2_per_cu &this_cu,
			  dwarf2_per_objfile &per_objfile,
			  const struct abbrev_table *abbrev_table,
			  dwarf2_cu *existing_cu,
			  bool skip_partial,
			  enum language pretend_language,
			  const abbrev_table_cache *abbrev_cache)
{
  struct objfile *objfile = per_objfile.objfile;
  struct dwarf2_section_info *section = this_cu.section;
  bfd *abfd = section->get_bfd_owner ();
  const gdb_byte *begin_info_ptr;
  struct signatured_type *sig_type = NULL;
  struct dwarf2_section_info *abbrev_section;
  /* Non-zero if CU currently points to a DWO file and we need to
     reread it.  When this happens we need to reread the skeleton die
     before we can reread the DWO file (this only applies to CUs, not TUs).  */
  int rereading_dwo_cu = 0;

  if (dwarf_die_debug)
    gdb_printf (gdb_stdlog, "Reading %s unit at offset %s\n",
		this_cu.is_debug_types ? "type" : "comp",
		sect_offset_str (this_cu.sect_off));

  /* If we're reading a TU directly from a DWO file, including a virtual DWO
     file (instead of going through the stub), short-circuit all of this.  */
  if (this_cu.reading_dwo_directly)
    {
      /* Narrow down the scope of possibilities to have to understand.  */
      gdb_assert (this_cu.is_debug_types);
      gdb_assert (abbrev_table == NULL);
      init_tu_and_read_dwo_dies (&this_cu, &per_objfile, existing_cu,
				 pretend_language);
      return;
    }

  /* This is cheap if the section is already read in.  */
  section->read (objfile);

  begin_info_ptr = m_info_ptr
    = section->buffer + to_underlying (this_cu.sect_off);

  abbrev_section = get_abbrev_section_for_cu (&this_cu);

  dwarf2_cu *cu;

  if (existing_cu != nullptr)
    {
      cu = existing_cu;
      /* If this CU is from a DWO file we need to start over, we need to
	 refetch the attributes from the skeleton CU.
	 This could be optimized by retrieving those attributes from when we
	 were here the first time: the previous comp_unit_die was stored in
	 comp_unit_obstack.  But there's no data yet that we need this
	 optimization.  */
      if (cu->dwo_unit != NULL)
	rereading_dwo_cu = 1;
    }
  else
    {
      /* If an existing_cu is provided, a dwarf2_cu must not exist for
	 this_cu in per_objfile yet.  Here, CACHE doubles as a flag to
	 let us know that the CU is being scanned using the parallel
	 indexer.  This assert is avoided in this case because (1) it
	 is irrelevant, and (2) the get_cu method is not
	 thread-safe.  */
      gdb_assert (abbrev_cache != nullptr
		  || per_objfile.get_cu (&this_cu) == nullptr);
      m_new_cu = std::make_unique<dwarf2_cu> (&this_cu, &per_objfile);
      cu = m_new_cu.get ();
    }

  /* Get the header.  */
  if (to_underlying (cu->header.first_die_offset_in_unit) != 0
      && !rereading_dwo_cu)
    {
      /* We already have the header, there's no need to read it in again.  */
      m_info_ptr += to_underlying (cu->header.first_die_offset_in_unit);
    }
  else
    {
      if (this_cu.is_debug_types)
	{
	  m_info_ptr = read_and_check_unit_head (&cu->header, section,
						 abbrev_section, m_info_ptr,
						 ruh_kind::TYPE);

	  /* Since per_cu is the first member of struct signatured_type,
	     we can go from a pointer to one to a pointer to the other.  */
	  sig_type = (struct signatured_type *) &this_cu;
	  gdb_assert (sig_type->signature == cu->header.signature);
	  gdb_assert (sig_type->type_offset_in_tu
		      == cu->header.type_offset_in_tu);
	  gdb_assert (this_cu.sect_off == cu->header.sect_off);

	  /* LENGTH has not been set yet for type units if we're
	     using .gdb_index.  */
	  this_cu.set_length (cu->header.get_length_with_initial ());

	  /* Establish the type offset that can be used to lookup the type.  */
	  sig_type->type_offset_in_section =
	    this_cu.sect_off + to_underlying (sig_type->type_offset_in_tu);
	}
      else
	{
	  m_info_ptr = read_and_check_unit_head (&cu->header, section,
						 abbrev_section, m_info_ptr,
						 ruh_kind::COMPILE);

	  gdb_assert (this_cu.sect_off == cu->header.sect_off);
	  this_cu.set_length (cu->header.get_length_with_initial ());
	}

	/* Record some information found in the header.  This will be needed
	   when evaluating DWARF expressions in the context of this unit, for
	   instance.  */
	this_cu.set_addr_size (cu->header.addr_size);
	this_cu.set_offset_size (cu->header.offset_size);
	this_cu.set_ref_addr_size (cu->header.version == 2
				   ? cu->header.addr_size
				   : cu->header.offset_size);
    }

  /* Skip dummy compilation units.  */
  if (m_info_ptr >= begin_info_ptr + this_cu.length ()
      || peek_abbrev_code (abfd, m_info_ptr) == 0)
    m_dummy_p = true;
  else
    {
      /* If we don't have them yet, read the abbrevs for this
	 compilation unit.  And if we need to read them now, make sure
	 they're freed when we're done.  */
      if (abbrev_table != NULL)
	gdb_assert (cu->header.abbrev_sect_off == abbrev_table->sect_off);
      else
	{
	  if (abbrev_cache != nullptr)
	    abbrev_table = abbrev_cache->find (abbrev_section,
					       cu->header.abbrev_sect_off);
	  if (abbrev_table == nullptr)
	    {
	      abbrev_section->read (objfile);
	      m_abbrev_table_holder
		= abbrev_table::read (abbrev_section,
				      cu->header.abbrev_sect_off);
	      abbrev_table = m_abbrev_table_holder.get ();
	    }
	}

      /* Read the top level CU/TU die.  */
      this->init_cu_die_reader (cu, section, NULL, abbrev_table);
      m_top_level_die = this->read_toplevel_die ();

      if (skip_partial && m_top_level_die->tag == DW_TAG_partial_unit)
	m_dummy_p = true;
      else
	{
	  /* If we are in a DWO stub, process it and then read in the
	     "real" CU/TU from the DWO file.  read_cutu_die_from_dwo
	     will allocate the abbreviation table from the DWO file
	     and pass the ownership over to us.  It will be referenced
	     from READER, so we must make sure to free it after we're
	     done with READER.

	     Note that if USE_EXISTING_OK != 0, and THIS_CU->cu
	     already contains a DWO CU, that this test will fail (the
	     attribute will not be present).  */
	  const char *dwo_name = dwarf2_dwo_name (m_top_level_die, cu);
	  if (dwo_name != nullptr)
	    {
	      struct dwo_unit *dwo_unit;

	      if (m_top_level_die->has_children)
		{
		  complaint (_("compilation unit with DW_AT_GNU_dwo_name"
			       " has children (offset %s) [in module %s]"),
			     sect_offset_str (this_cu.sect_off),
			     bfd_get_filename (abfd));
		}

	      dwo_unit = lookup_dwo_unit (cu, m_top_level_die, dwo_name);
	      if (dwo_unit != NULL)
		read_cutu_die_from_dwo (cu, dwo_unit, m_top_level_die, nullptr);
	      else
		{
		  /* Yikes, we couldn't find the rest of the DIE, we only have
		     the stub.  A complaint has already been logged.  There's
		     not much more we can do except pass on the stub DIE to
		     die_reader_func.  We don't want to throw an error on bad
		     debug info.  */
		}
	    }
	}
    }

  /* Only a dummy unit can be missing the compunit DIE.  */
  gdb_assert (m_dummy_p || m_top_level_die != nullptr);
  prepare_one_comp_unit (cu, pretend_language);
}

/* See read.h.  */

dwarf2_cu_up
cutu_reader::release_cu ()
{
  gdb_assert (!m_dummy_p);
  gdb_assert (m_new_cu != nullptr);

  return std::move (m_new_cu);
}

/* This constructor exists for the special case of reading many units in a row
   from a given known DWO file.

   THIS_CU is a special dwarf2_per_cu to represent where to read the unit from,
   in the DWO file.  The caller is required to fill THIS_CU::SECTION,
   THIS_CU::SECT_OFF, and THIS_CU::IS_DEBUG_TYPES.  This constructor will fill
   in the length.  THIS_CU::SECTION must point to a section from the DWO file,
   which is normally not the case for regular dwarf2_per_cu uses.

   PARENT_CU is the CU created when reading the skeleton unit, and is used to
   provide a default value for str_offsets_base and addr_base.  */

cutu_reader::cutu_reader (dwarf2_per_cu &this_cu,
			  dwarf2_per_objfile &per_objfile,
			  language pretend_language, dwarf2_cu &parent_cu,
			  dwo_file &dwo_file)
{
  struct objfile *objfile = per_objfile.objfile;
  struct dwarf2_section_info *section = this_cu.section;
  bfd *abfd = section->get_bfd_owner ();

  if (dwarf_die_debug)
    gdb_printf (gdb_stdlog, "Reading %s unit at offset %s\n",
		this_cu.is_debug_types ? "type" : "comp",
		sect_offset_str (this_cu.sect_off));

  gdb_assert (per_objfile.get_cu (&this_cu) == nullptr);

  dwarf2_section_info *abbrev_section = &dwo_file.sections.abbrev;

  /* This is cheap if the section is already read in.  */
  section->read (objfile);

  m_new_cu = std::make_unique<dwarf2_cu> (&this_cu, &per_objfile);

  m_info_ptr = section->buffer + to_underlying (this_cu.sect_off);
  const gdb_byte *begin_info_ptr = m_info_ptr;
  m_info_ptr = read_and_check_unit_head (&m_new_cu->header, section,
					 abbrev_section, m_info_ptr,
					 (this_cu.is_debug_types
					  ? ruh_kind::TYPE
					  : ruh_kind::COMPILE));

  m_new_cu->str_offsets_base = parent_cu.str_offsets_base;
  m_new_cu->addr_base = parent_cu.addr_base;

  this_cu.set_length (m_new_cu->header.get_length_with_initial ());

  /* Skip dummy compilation units.  */
  if (m_info_ptr >= begin_info_ptr + this_cu.length ()
      || peek_abbrev_code (abfd, m_info_ptr) == 0)
    m_dummy_p = true;
  else
    {
      abbrev_section->read (objfile);
      m_abbrev_table_holder
	= abbrev_table::read (abbrev_section,
			      m_new_cu->header.abbrev_sect_off);

      this->init_cu_die_reader (m_new_cu.get (), section, &dwo_file,
				m_abbrev_table_holder.get ());
      m_top_level_die = this->read_toplevel_die ();
    }

  prepare_one_comp_unit (m_new_cu.get (), pretend_language);
}


/* Type Unit Groups.

   Type Unit Groups are a way to collapse the set of all TUs (type units) into
   a more manageable set.  The grouping is done by DW_AT_stmt_list entry
   so that all types coming from the same compilation (.o file) are grouped
   together.  A future step could be to put the types in the same symtab as
   the CU the types ultimately came from.  */

/* Type units that don't have DW_AT_stmt_list are grouped into their own
   partial symtabs.  We combine several TUs per psymtab to not let the size
   of any one psymtab grow too big.  */
#define NO_STMT_LIST_TYPE_UNIT_PSYMTAB (1 << 31)
#define NO_STMT_LIST_TYPE_UNIT_PSYMTAB_SIZE 10

/* Get the type unit group key for type unit CU.  STMT_LIST is a DW_AT_stmt_list
   attribute.  */

static stmt_list_hash
get_type_unit_group_key (struct dwarf2_cu *cu, const struct attribute *stmt_list)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct tu_stats *tu_stats = &per_objfile->per_bfd->tu_stats;
  unsigned int line_offset;

  /* Do we need to create a new group, or can we use an existing one?  */

  if (stmt_list != nullptr && stmt_list->form_is_unsigned ())
    {
      line_offset = stmt_list->as_unsigned ();
      ++tu_stats->nr_symtab_sharers;
    }
  else
    {
      /* Ugh, no stmt_list.  Rare, but we have to handle it.
	 We can do various things here like create one group per TU or
	 spread them over multiple groups to split up the expansion work.
	 To avoid worst case scenarios (too many groups or too large groups)
	 we, umm, group them in bunches.  */
      line_offset = (NO_STMT_LIST_TYPE_UNIT_PSYMTAB
		     | (tu_stats->nr_stmt_less_type_units
			/ NO_STMT_LIST_TYPE_UNIT_PSYMTAB_SIZE));
      ++tu_stats->nr_stmt_less_type_units;
    }

  return {cu->dwo_unit, static_cast<sect_offset> (line_offset)};
}

/* A subclass of cooked_index_worker that handles scanning
   .debug_info.  */

class cooked_index_worker_debug_info : public cooked_index_worker
{
public:
  cooked_index_worker_debug_info (dwarf2_per_objfile *per_objfile)
    : cooked_index_worker (per_objfile)
  {
    gdb_assert (is_main_thread ());

    struct objfile *objfile = per_objfile->objfile;
    dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

    dwarf_read_debug_printf ("Building psymtabs of objfile %s ...",
			     objfile_name (objfile));

    per_bfd->map_info_sections (objfile);
  }

private:
  void do_reading () override;

  /* Print collected type unit statistics.  */

  void print_tu_stats (dwarf2_per_objfile *per_objfile)
  {
    dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
    tu_stats *tu_stats = &per_bfd->tu_stats;

    dwarf_read_debug_printf ("Type unit statistics:");
    dwarf_read_debug_printf ("  %d TUs", per_bfd->num_type_units);
    dwarf_read_debug_printf ("  %d uniq abbrev tables",
			     tu_stats->nr_uniq_abbrev_tables);
    dwarf_read_debug_printf ("  %d symtabs from stmt_list entries",
			     tu_stats->nr_symtabs);
    dwarf_read_debug_printf ("  %d symtab sharers",
			     tu_stats->nr_symtab_sharers);
    dwarf_read_debug_printf ("  %d type units without a stmt_list",
			     tu_stats->nr_stmt_less_type_units);
    dwarf_read_debug_printf ("  %d all_type_units reallocs",
			     tu_stats->nr_all_type_units_reallocs);
  }

  void print_stats () override
  {
    if (dwarf_read_debug > 0)
      print_tu_stats (m_per_objfile);

    if (dwarf_read_debug > 1)
      {
	dwarf_read_debug_printf_v ("Final m_all_parents_map:");
	m_all_parents_map.dump (m_per_objfile->per_bfd);
      }
  }

  /* After the last DWARF-reading task has finished, this function
     does the remaining work to finish the scan.  */
  void done_reading () override;

  /* An iterator for the comp units.  */
  using unit_iterator = std::vector<dwarf2_per_cu_up>::iterator;

  /* Process a batch of CUs.  This may be called multiple times in
     separate threads.  TASK_NUMBER indicates which task this is --
     the result is stored in that slot of M_RESULTS.  */
  void process_units (size_t task_number, unit_iterator first,
		      unit_iterator end);

  /* Process unit THIS_CU.  */
  void process_unit (dwarf2_per_cu *this_cu, dwarf2_per_objfile *per_objfile,
		     cooked_index_worker_result *storage);

  /* Process all type units existing in PER_OBJFILE::PER_BFD::ALL_UNITS.  */
  void process_type_units (dwarf2_per_objfile *per_objfile,
			   cooked_index_worker_result *storage);

  /* Process the type unit wrapped in READER.  */
  void process_type_unit (cutu_reader *reader,
			  cooked_index_worker_result *storage);

  /* Process all type units of all DWO files.

     This is needed in case a TU was emitted without its skeleton.
     Note: This can't be done until we know what all the DWO files are.  */
  void process_skeletonless_type_units (dwarf2_per_objfile *per_objfile,
					cooked_index_worker_result *storage);

  /* Process the type unit represented by DWO_UNIT.  */
  void process_skeletonless_type_unit (dwo_unit *dwo_unit,
				       dwarf2_per_objfile *per_objfile,
				       cooked_index_worker_result *storage);

  /* A storage object for "leftovers" -- see the 'start' method, but
     essentially things not parsed during the normal CU parsing
     passes.  */
  cooked_index_worker_result m_index_storage;
};

void
cooked_index_worker_debug_info::process_unit
  (dwarf2_per_cu *this_cu, dwarf2_per_objfile *per_objfile,
   cooked_index_worker_result *storage)
{
  cutu_reader *reader = storage->get_reader (this_cu);
  if (reader == nullptr)
    {
      const abbrev_table_cache &abbrev_table_cache
	= storage->get_abbrev_table_cache ();
      auto new_reader = std::make_unique<cutu_reader> (*this_cu, *per_objfile,
						       nullptr, nullptr, false,
						       language_minimal,
						       &abbrev_table_cache);

      if (new_reader->is_dummy ())
	return;

      reader = storage->preserve (std::move (new_reader));
    }

  if (reader->is_dummy ())
    return;

  if (this_cu->is_debug_types)
    process_type_unit (reader, storage);
  else if (reader->top_level_die ()->tag != DW_TAG_partial_unit)
    {
      bool nope = false;
      if (this_cu->scanned.compare_exchange_strong (nope, true))
	{
	  gdb_assert (storage != nullptr);
	  cooked_indexer indexer (storage, this_cu, reader->cu ()->lang ());
	  indexer.make_index (reader);
	}
    }
}

void
cooked_index_worker_debug_info::process_type_unit
  (cutu_reader *reader, cooked_index_worker_result *storage)
{
  struct dwarf2_cu *cu = reader->cu ();
  dwarf2_per_cu *per_cu = cu->per_cu;
  die_info *type_unit_die = reader->top_level_die ();

  gdb_assert (per_cu->is_debug_types);

  if (! type_unit_die->has_children)
    return;

  gdb_assert (storage != nullptr);
  cooked_indexer indexer (storage, per_cu, cu->lang ());
  indexer.make_index (reader);
}

/* Struct used to sort TUs by their abbreviation table offset.  */

struct tu_abbrev_offset
{
  tu_abbrev_offset (signatured_type *sig_type_, sect_offset abbrev_offset_)
  : sig_type (sig_type_), abbrev_offset (abbrev_offset_)
  {}

  /* This is used when sorting.  */
  bool operator< (const tu_abbrev_offset &other) const
  {
    return abbrev_offset < other.abbrev_offset;
  }

  signatured_type *sig_type;
  sect_offset abbrev_offset;
};

void
cooked_index_worker_debug_info::process_type_units
  (dwarf2_per_objfile *per_objfile, cooked_index_worker_result *storage)
{
  struct tu_stats *tu_stats = &per_objfile->per_bfd->tu_stats;
  abbrev_table_up abbrev_table;
  sect_offset abbrev_offset;

  if (per_objfile->per_bfd->num_type_units == 0)
    return;

  /* TUs typically share abbrev tables, and there can be way more TUs than
     abbrev tables.  Sort by abbrev table to reduce the number of times we
     read each abbrev table in.
     Alternatives are to punt or to maintain a cache of abbrev tables.
     This is simpler and efficient enough for now.

     Later we group TUs by their DW_AT_stmt_list value (as this defines the
     symtab to use).  Typically TUs with the same abbrev offset have the same
     stmt_list value too so in practice this should work well.

     The basic algorithm here is:

      sort TUs by abbrev table
      for each TU with same abbrev table:
	read abbrev table if first user
	read TU top level DIE
	  [IWBN if DWO skeletons had DW_AT_stmt_list]
	call FUNC  */

  dwarf_read_debug_printf ("Building type unit groups ...");

  /* Sort in a separate table to maintain the order of all_units
     for .gdb_index: TU indices directly index all_type_units.  */
  std::vector<tu_abbrev_offset> sorted_by_abbrev;
  sorted_by_abbrev.reserve (per_objfile->per_bfd->num_type_units);

  for (const auto &cu : per_objfile->per_bfd->all_units)
    if (cu->is_debug_types)
      {
	auto sig_type = static_cast<signatured_type *> (cu.get ());
	sorted_by_abbrev.emplace_back (sig_type,
				       read_abbrev_offset (per_objfile,
							   sig_type->section,
							   sig_type->sect_off));
      }

  std::sort (sorted_by_abbrev.begin (), sorted_by_abbrev.end ());

  abbrev_offset = (sect_offset) ~(unsigned) 0;

  for (const tu_abbrev_offset &tu : sorted_by_abbrev)
    {
      /* Switch to the next abbrev table if necessary.  */
      if (abbrev_table == NULL
	  || tu.abbrev_offset != abbrev_offset)
	{
	  abbrev_offset = tu.abbrev_offset;
	  per_objfile->per_bfd->abbrev.read (per_objfile->objfile);
	  abbrev_table =
	    abbrev_table::read (&per_objfile->per_bfd->abbrev, abbrev_offset);
	  ++tu_stats->nr_uniq_abbrev_tables;
	}

      cutu_reader reader (*tu.sig_type, *per_objfile,
			  abbrev_table.get (), nullptr, false,
			  language_minimal);
      if (!reader.is_dummy ())
	storage->catch_error ([&] ()
	  {
	    process_type_unit (&reader, storage);
	  });
    }
}

void
cooked_index_worker_debug_info::process_skeletonless_type_unit
  (dwo_unit *dwo_unit, dwarf2_per_objfile *per_objfile,
   cooked_index_worker_result *storage)
{
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  /* If this TU doesn't exist in the global table, add it and read it in.  */
  auto sig_type_it = per_bfd->signatured_types.find (dwo_unit->signature);

  /* If we've already seen this type there's nothing to do.  What's happening
     is we're doing our own version of comdat-folding here.  */
  if (sig_type_it != per_bfd->signatured_types.end ())
    return;

  /* This does the job that create_all_units would have done for
     this TU.  */
  sig_type_it = add_type_unit (per_bfd, dwo_unit->section, dwo_unit->sect_off,
			       dwo_unit->length, dwo_unit->signature);
  /* finalize_all_units is called just once by process_skeletonless_type_units
     after going through all skeletonless type units.  */
  fill_in_sig_entry_from_dwo_entry (per_objfile, *sig_type_it, dwo_unit);

  /* This does the job that build_type_psymtabs would have done.  */
  cutu_reader reader (**sig_type_it, *per_objfile, nullptr, nullptr, false,
		      language_minimal);
  if (!reader.is_dummy ())
    process_type_unit (&reader, storage);
}

void
cooked_index_worker_debug_info::process_skeletonless_type_units
  (dwarf2_per_objfile *per_objfile, cooked_index_worker_result *storage)
{
  scoped_time_it time_it ("DWARF skeletonless type units", m_per_command_time);
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  /* Skeletonless TUs in DWP files without .gdb_index is not supported yet.  */
  if (per_bfd->dwp_file == nullptr)
    {
      for (const dwo_file_up &file : per_bfd->dwo_files)
	for (const dwo_unit_up &unit : file->tus)
	  storage->catch_error ([&] ()
	    {
	      process_skeletonless_type_unit (unit.get (), per_objfile, storage);
	    });

      finalize_all_units (per_bfd);
    }
}

void
cooked_index_worker_debug_info::process_units (size_t task_number,
					       unit_iterator first,
					       unit_iterator end)
{
  SCOPE_EXIT { bfd_thread_cleanup (); };

  /* Ensure that complaints are handled correctly.  */
  complaint_interceptor complaint_handler;

  std::vector<gdb_exception> errors;
  cooked_index_worker_result thread_storage;
  for (auto inner = first; inner != end; ++inner)
    {
      dwarf2_per_cu *per_cu = inner->get ();

      thread_storage.catch_error ([&] ()
	{
	  process_unit (per_cu, m_per_objfile, &thread_storage);
	});
    }

  thread_storage.done_reading (complaint_handler.release ());
  m_results[task_number] = std::move (thread_storage);
}

void
cooked_index_worker_debug_info::done_reading ()
{
  /* This has to wait until we read the CUs, we need the list of DWOs.  */
  process_skeletonless_type_units (m_per_objfile, &m_index_storage);

  m_results.push_back (std::move (m_index_storage));

  /* Call into the base class.  */
  cooked_index_worker::done_reading ();
}

void
cooked_index_worker_debug_info::do_reading ()
{
  dwarf2_per_bfd *per_bfd = m_per_objfile->per_bfd;

  create_all_units (m_per_objfile);
  process_type_units (m_per_objfile, &m_index_storage);

  if (!per_bfd->debug_aranges.empty ())
    read_addrmap_from_aranges (m_per_objfile, &per_bfd->debug_aranges,
			       m_index_storage.get_addrmap (),
			       &m_warnings);

  /* We want to balance the load between the worker threads.  This is
     done by using the size of each CU as a rough estimate of how
     difficult it will be to operate on.  This isn't ideal -- for
     example if dwz is used, the early CUs will all tend to be
     "included" and won't be parsed independently.  However, this
     heuristic works well for typical compiler output.  */

  size_t total_size = 0;
  for (const auto &per_cu : per_bfd->all_units)
    total_size += per_cu->length ();

  /* How many worker threads we plan to use.  We may not actually use
     this many.  We use 1 as the minimum to avoid division by zero,
     and anyway in the N==0 case the work will be done
     synchronously.  */
  const size_t n_worker_threads
    = std::max (gdb::thread_pool::g_thread_pool->thread_count (), (size_t) 1);

  /* How much effort should be put into each worker.  */
  const size_t size_per_thread
    = std::max (total_size / n_worker_threads, (size_t) 1);

  /* Work is done in a task group.  */
  gdb::task_group workers ([this] ()
  {
    this->done_reading ();
  });

  auto end = per_bfd->all_units.end ();
  size_t task_count = 0;
  for (auto iter = per_bfd->all_units.begin (); iter != end; )
    {
      auto last = iter;
      /* Put all remaining CUs into the last task.  */
      if (task_count == n_worker_threads - 1)
	last = end;
      else
	{
	  size_t chunk_size = 0;
	  for (; last != end && chunk_size < size_per_thread; ++last)
	    chunk_size += (*last)->length ();
	}

      gdb_assert (iter != last);
      workers.add_task ([this, task_count, iter, last] ()
	{
	  scoped_time_it time_it ("DWARF indexing worker", m_per_command_time);
	  process_units (task_count, iter, last);
	});

      ++task_count;
      iter = last;
    }

  m_results.resize (task_count);
  workers.start ();
}

static void
read_comp_units_from_section (dwarf2_per_objfile *per_objfile,
			      struct dwarf2_section_info *section,
			      struct dwarf2_section_info *abbrev_section,
			      unsigned int is_dwz,
			      signatured_type_set &sig_types,
			      ruh_kind section_kind)
{
  const gdb_byte *info_ptr;
  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  dwarf_read_debug_printf ("Reading %s for %s",
			   section->get_name (),
			   section->get_file_name ());

  section->read (objfile);

  info_ptr = section->buffer;

  while (info_ptr < section->buffer + section->size)
    {
      dwarf2_per_cu_up this_cu;

      sect_offset sect_off = (sect_offset) (info_ptr - section->buffer);

      unit_head cu_header;
      read_and_check_unit_head (&cu_header, section, abbrev_section, info_ptr,
				section_kind);

      unsigned int length = cu_header.get_length_with_initial ();

      /* Save the compilation unit for later lookup.  */
      if (cu_header.unit_type != DW_UT_type)
	this_cu = per_bfd->allocate_per_cu (section, sect_off, length, is_dwz);
      else
	{
	  auto sig_type
	    = per_bfd->allocate_signatured_type (section, sect_off, length,
						 is_dwz, cu_header.signature);
	  signatured_type *sig_ptr = sig_type.get ();
	  sig_type->type_offset_in_tu = cu_header.type_offset_in_tu;
	  this_cu.reset (sig_type.release ());

	  auto inserted = sig_types.emplace (sig_ptr).second;

	  if (!inserted)
	    complaint (_("debug type entry at offset %s is duplicate to"
			 " the entry at offset %s, signature %s"),
		       sect_offset_str (sect_off),
		       sect_offset_str (sig_ptr->sect_off),
		       hex_string (sig_ptr->signature));
	}

      info_ptr = info_ptr + this_cu->length ();
      per_bfd->all_units.push_back (std::move (this_cu));
    }
}

/* See read.h.  */

void
finalize_all_units (dwarf2_per_bfd *per_bfd)
{
  /* Sanity check.  */
  gdb_assert (per_bfd->all_units.size ()
	      == per_bfd->num_comp_units + per_bfd->num_type_units);

  /* Ensure that the all_units vector is in the expected order for
     dwarf2_find_containing_unit to be able to perform a binary search.  */
  std::sort (per_bfd->all_units.begin (), per_bfd->all_units.end (),
	     [] (const dwarf2_per_cu_up &a, const dwarf2_per_cu_up &b)
	       {
		 return all_units_less_than (*a, { b->section, b->sect_off });
	       });
}

/* See read.h.  */

void
create_all_units (dwarf2_per_objfile *per_objfile)
{
  gdb_assert (per_objfile->per_bfd->all_units.empty ());
  scoped_remove_all_units remove_all_units (*per_objfile->per_bfd);

  signatured_type_set sig_types;

  for (dwarf2_section_info &section : per_objfile->per_bfd->infos)
    read_comp_units_from_section (per_objfile, &section,
				  &per_objfile->per_bfd->abbrev, 0, sig_types,
				  ruh_kind::COMPILE);
  for (dwarf2_section_info &section : per_objfile->per_bfd->types)
    read_comp_units_from_section (per_objfile, &section,
				  &per_objfile->per_bfd->abbrev, 0, sig_types,
				  ruh_kind::TYPE);

  dwz_file *dwz = per_objfile->per_bfd->get_dwz_file ();
  if (dwz != NULL)
    {
      read_comp_units_from_section (per_objfile, &dwz->info, &dwz->abbrev, 1,
				    sig_types, ruh_kind::COMPILE);

      if (!dwz->types.empty ())
	{
	  /* See enhancement PR symtab/30838.  */
	  error (_(DWARF_ERROR_PREFIX
		   ".debug_types section not supported in dwz file"));
	}
    }

  per_objfile->per_bfd->signatured_types = std::move (sig_types);

  finalize_all_units (per_objfile->per_bfd);
  remove_all_units.disable ();
}

/* Return the initial uleb128 in the die at INFO_PTR.  */

static unsigned int
peek_abbrev_code (bfd *abfd, const gdb_byte *info_ptr)
{
  unsigned int bytes_read;

  return read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
}

/* Read the initial uleb128 in the die at INFO_PTR in compilation unit
   READER::CU.  Use READER::ABBREV_TABLE to lookup any abbreviation.

   Return the corresponding abbrev, or NULL if the number is zero (indicating
   an empty DIE).  In either case *BYTES_READ will be set to the length of
   the initial number.  */

const abbrev_info *
cutu_reader::peek_die_abbrev (const gdb_byte *info_ptr,
			      unsigned int *bytes_read)
{
  unsigned int abbrev_number
    = read_unsigned_leb128 (m_abfd, info_ptr, bytes_read);

  if (abbrev_number == 0)
    return NULL;

  const abbrev_info *abbrev = m_abbrev_table->lookup_abbrev (abbrev_number);
  if (!abbrev)
    {
      error (_(DWARF_ERROR_PREFIX
	       "Could not find abbrev number %d in %s at offset %s"
	       " [in module %s]"),
	     abbrev_number, m_cu->per_cu->is_debug_types ? "TU" : "CU",
	     sect_offset_str (m_cu->header.sect_off),
	     bfd_get_filename (m_abfd));
    }

  return abbrev;
}

/* Scan the debug information for CU starting at INFO_PTR in buffer BUFFER.
   Returns a pointer to the end of a series of DIEs, terminated by an empty
   DIE.  Any children of the skipped DIEs will also be skipped.  */

const gdb_byte *
cutu_reader::skip_children (const gdb_byte *info_ptr)
{
  while (1)
    {
      unsigned int bytes_read;
      const abbrev_info *abbrev = this->peek_die_abbrev (info_ptr, &bytes_read);

      if (abbrev == NULL)
	return info_ptr + bytes_read;
      else
	info_ptr = this->skip_one_die (info_ptr + bytes_read, abbrev);
    }
}

/* See read.h.  */

const gdb_byte *
cutu_reader::skip_one_attribute (dwarf_form form, const gdb_byte *info_ptr)
{
  unsigned int bytes_read;

  switch (form)
    {
    case DW_FORM_ref_addr:
      return info_ptr + m_cu->per_cu->ref_addr_size ();

    case DW_FORM_GNU_ref_alt:
      return info_ptr + m_cu->header.offset_size;

    case DW_FORM_addr:
      return info_ptr + m_cu->header.addr_size;

    case DW_FORM_data1:
    case DW_FORM_ref1:
    case DW_FORM_flag:
    case DW_FORM_strx1:
      return info_ptr + 1;

    case DW_FORM_flag_present:
    case DW_FORM_implicit_const:
      return info_ptr;

    case DW_FORM_data2:
    case DW_FORM_ref2:
    case DW_FORM_strx2:
      return info_ptr + 2;

    case DW_FORM_strx3:
      return info_ptr + 3;

    case DW_FORM_data4:
    case DW_FORM_ref4:
    case DW_FORM_strx4:
    case DW_FORM_ref_sup4:
      return info_ptr + 4;

    case DW_FORM_data8:
    case DW_FORM_ref8:
    case DW_FORM_ref_sig8:
    case DW_FORM_ref_sup8:
      return info_ptr + 8;

    case DW_FORM_data16:
      return info_ptr + 16;

    case DW_FORM_string:
      read_direct_string (m_abfd, info_ptr, &bytes_read);
      return info_ptr + bytes_read;

    case DW_FORM_sec_offset:
    case DW_FORM_strp:
    case DW_FORM_GNU_strp_alt:
    case DW_FORM_strp_sup:
      return info_ptr + m_cu->header.offset_size;

    case DW_FORM_exprloc:
    case DW_FORM_block:
      info_ptr += read_unsigned_leb128 (m_abfd, info_ptr, &bytes_read);
      return info_ptr + bytes_read;

    case DW_FORM_block1:
      return info_ptr + 1 + read_1_byte (m_abfd, info_ptr);

    case DW_FORM_block2:
      return info_ptr + 2 + read_2_bytes (m_abfd, info_ptr);

    case DW_FORM_block4:
      return info_ptr + 4 + read_4_bytes (m_abfd, info_ptr);

    case DW_FORM_addrx:
    case DW_FORM_strx:
    case DW_FORM_sdata:
    case DW_FORM_udata:
    case DW_FORM_ref_udata:
    case DW_FORM_GNU_addr_index:
    case DW_FORM_GNU_str_index:
    case DW_FORM_rnglistx:
    case DW_FORM_loclistx:
      return safe_skip_leb128 (info_ptr, m_buffer_end);

    case DW_FORM_indirect:
      form = static_cast<dwarf_form> (read_unsigned_leb128 (m_abfd, info_ptr,
							    &bytes_read));
      return this->skip_one_attribute(form, info_ptr + bytes_read);

    default:
      error (_ (DWARF_ERROR_PREFIX
		"Cannot handle %s in DWARF reader [in module %s]"),
	     dwarf_form_name (form), bfd_get_filename (m_abfd));
    }
}

/* Scan the debug information for CU starting at INFO_PTR in buffer BUFFER.
   INFO_PTR should point just after the initial uleb128 of a DIE, and the
   abbrev corresponding to that skipped uleb128 should be passed in
   ABBREV.
   
   If DO_SKIP_CHILDREN is true, or if the DIE has no children, this
   returns a pointer to this DIE's sibling, skipping any children.
   Otherwise, returns a pointer to the DIE's first child.  */

const gdb_byte *
cutu_reader::skip_one_die (const gdb_byte *info_ptr, const abbrev_info *abbrev,
			   bool do_skip_children)
{
  if (do_skip_children && abbrev->sibling_offset != (unsigned short) -1)
    {
      /* We only handle DW_FORM_ref4 here.  */
      const gdb_byte *sibling_data = info_ptr + abbrev->sibling_offset;
      unsigned int offset = read_4_bytes (m_abfd, sibling_data);
      const gdb_byte *sibling_ptr
	= m_buffer + to_underlying (m_cu->header.sect_off) + offset;
      if (sibling_ptr >= info_ptr && sibling_ptr < m_buffer_end)
	return sibling_ptr;
      /* Fall through to the slow way.  */
    }
  else if (abbrev->size_if_constant != 0)
    {
      info_ptr += abbrev->size_if_constant;
      if (do_skip_children && abbrev->has_children)
	return this->skip_children (info_ptr);
      return info_ptr;
    }

  for (unsigned int i = 0; i < abbrev->num_attrs; i++)
    {
      /* The only abbrev we care about is DW_AT_sibling.  */
      if (do_skip_children && abbrev->attrs[i].name == DW_AT_sibling)
	{
	  /* Note there is no need for the extra work of
	     "reprocessing" here, so we pass false for that
	     argument.  */
	  attribute attr;
	  this->read_attribute (&attr, &abbrev->attrs[i], info_ptr, false);
	  if (attr.form == DW_FORM_ref_addr)
	    complaint (_("ignoring absolute DW_AT_sibling"));
	  else
	    {
	      sect_offset off = attr.get_ref_die_offset ();
	      const gdb_byte *sibling_ptr = m_buffer + to_underlying (off);

	      if (sibling_ptr < info_ptr)
		complaint (_("DW_AT_sibling points backwards"));
	      else if (sibling_ptr > m_buffer_end)
		m_die_section->overflow_complaint ();
	      else
		return sibling_ptr;
	    }
	}

      /* If it isn't DW_AT_sibling, skip this attribute.  */
      info_ptr = this->skip_one_attribute (abbrev->attrs[i].form, info_ptr);
    }

  if (do_skip_children && abbrev->has_children)
    return this->skip_children (info_ptr);
  else
    return info_ptr;
}

/* Reading in full CUs.  */

/* Add PER_CU to the queue.  */

static void
queue_comp_unit (dwarf2_per_cu *per_cu, dwarf2_per_objfile *per_objfile)
{
  per_cu->queued = 1;

  gdb_assert (per_objfile->queue.has_value ());
  per_objfile->queue->emplace (per_cu, per_objfile);
}

/* If PER_CU is not yet expanded of queued for expansion, add it to the queue.

   If DEPENDENT_CU is non-NULL, it has a reference to PER_CU so add a
   dependency.

   Return true if maybe_queue_comp_unit requires the caller to load the CU's
   DIEs, false otherwise.

   Explanation: there is an invariant that if a CU is queued for expansion
   (present in `dwarf2_per_bfd::queue`), then its DIEs are loaded
   (a dwarf2_cu object exists for this CU, and `dwarf2_per_objfile::get_cu`
   returns non-nullptr).  If the CU gets enqueued by this function but its DIEs
   are not yet loaded, the the caller must load the CU's DIEs to ensure the
   invariant is respected.

   The caller is therefore not required to load the CU's DIEs (we return false)
   if:

     - the CU is already expanded, and therefore does not get enqueued
     - the CU gets enqueued for expansion, but its DIEs are already loaded

   Note that the caller should not use this function's return value as an
   indicator of whether the CU's DIEs are loaded right now, it should check
   that by calling `dwarf2_per_objfile::get_cu` instead.  */

static bool
maybe_queue_comp_unit (struct dwarf2_cu *dependent_cu, dwarf2_per_cu *per_cu,
		       dwarf2_per_objfile *per_objfile)
{
  /* Mark the dependence relation so that we don't flush PER_CU
     too early.  */
  if (dependent_cu != NULL)
    dependent_cu->add_dependence (per_cu);

  /* If it's already on the queue, we have nothing to do.  */
  if (per_cu->queued)
    {
      /* Verify the invariant that if a CU is queued for expansion, its DIEs are
	 loaded.  */
      gdb_assert (per_objfile->get_cu (per_cu) != nullptr);

      /* If the CU is queued for expansion, it should not already be
	 expanded.  */
      gdb_assert (!per_objfile->symtab_set_p (per_cu));

      /* The DIEs are already loaded, the caller doesn't need to do it.  */
      return false;
    }

  bool queued = false;
  if (!per_objfile->symtab_set_p (per_cu))
    {
      /* Add it to the queue.  */
      queue_comp_unit (per_cu, per_objfile);
      queued = true;

      dwarf_read_debug_printf ("Queuing CU for expansion: "
			       "section offset = 0x%" PRIx64 ", "
			       "queue size = %zu",
			       to_underlying (per_cu->sect_off),
			       per_objfile->queue->size ());
    }

  /* If the compilation unit is already loaded, just mark it as
     used.  */
  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu != nullptr)
    cu->last_used = 0;

  /* Ask the caller to load the CU's DIEs if the CU got enqueued for expansion
     and the DIEs are not already loaded.  */
  return queued && cu == nullptr;
}

/* Process the queue.  */

static void
process_queue (dwarf2_per_objfile *per_objfile)
{
  DWARF_READ_SCOPED_DEBUG_START_END
    ("Expanding one or more symtabs of objfile %s ...",
     objfile_name (per_objfile->objfile));

  unsigned int expanded_count = 0;

  /* The queue starts out with one item, but following a DIE reference
     may load a new CU, adding it to the end of the queue.  */
  while (!per_objfile->queue->empty ())
    {
      dwarf2_queue_item &item = per_objfile->queue->front ();
      dwarf2_per_cu *per_cu = item.per_cu;

      if (!per_objfile->symtab_set_p (per_cu))
	{
	  dwarf2_cu *cu = per_objfile->get_cu (per_cu);

	  /* Skip dummy CUs.  */
	  if (cu != nullptr)
	    {
	      namespace chr = std::chrono;

	      unsigned int debug_print_threshold;
	      char buf[100];
	      std::optional<chr::time_point<chr::steady_clock>> start_time;

	      if (per_cu->is_debug_types)
		{
		  struct signatured_type *sig_type =
		    (struct signatured_type *) per_cu;

		  sprintf (buf, "TU %s at offset %s",
			   hex_string (sig_type->signature),
			   sect_offset_str (per_cu->sect_off));
		  /* There can be 100s of TUs.
		     Only print them in verbose mode.  */
		  debug_print_threshold = 2;
		}
	      else
		{
		  sprintf (buf, "CU at offset %s",
			   sect_offset_str (per_cu->sect_off));
		  debug_print_threshold = 1;
		}

	      if (dwarf_read_debug >= debug_print_threshold)
		{
		  dwarf_read_debug_printf ("Expanding symtab of %s", buf);
		  start_time = chr::steady_clock::now ();
		}

	      ++expanded_count;

	      if (per_cu->is_debug_types)
		process_full_type_unit (cu);
	      else
		process_full_comp_unit (cu);

	      if (dwarf_read_debug >= debug_print_threshold)
		{
		  const auto end_time = chr::steady_clock::now ();
		  const auto time_spent = end_time - *start_time;
		  const auto ms
		    = chr::duration_cast<chr::milliseconds> (time_spent);

		  dwarf_read_debug_printf ("Done expanding %s, took %.3fs", buf,
					   ms.count () / 1000.0);
		}
	    }
	}

      per_cu->queued = 0;
      per_objfile->queue->pop ();
    }

  dwarf_read_debug_printf ("Done expanding %u symtabs.", expanded_count);
}

/* Load the DIEs associated with PER_CU into memory.  */

static void
load_full_comp_unit (dwarf2_per_cu *this_cu, dwarf2_per_objfile *per_objfile,
		     bool skip_partial, enum language pretend_language)
{
  gdb_assert (! this_cu->is_debug_types);
  gdb_assert (per_objfile->get_cu (this_cu) == nullptr);

  cutu_reader reader (*this_cu, *per_objfile, nullptr, nullptr, skip_partial,
		      pretend_language);
  if (reader.is_dummy ())
    return;

  reader.read_all_dies ();

  /* Save this dwarf2_cu in the per_objfile.  The per_objfile owns it
     now.  */
  per_objfile->set_cu (this_cu, reader.release_cu ());
}

/* Add a DIE to the delayed physname list.  */

static void
add_to_method_list (struct type *type, int fnfield_index, int index,
		    const char *name, struct die_info *die,
		    struct dwarf2_cu *cu)
{
  struct delayed_method_info mi;
  mi.type = type;
  mi.fnfield_index = fnfield_index;
  mi.index = index;
  mi.name = name;
  mi.die = die;
  cu->method_list.push_back (mi);
}

/* Check whether [PHYSNAME, PHYSNAME+LEN) ends with a modifier like
   "const" / "volatile".  If so, decrements LEN by the length of the
   modifier and return true.  Otherwise return false.  */

template<size_t N>
static bool
check_modifier (const char *physname, size_t &len, const char (&mod)[N])
{
  size_t mod_len = sizeof (mod) - 1;
  if (len > mod_len && startswith (physname + (len - mod_len), mod))
    {
      len -= mod_len;
      return true;
    }
  return false;
}

/* Compute the physnames of any methods on the CU's method list.

   The computation of method physnames is delayed in order to avoid the
   (bad) condition that one of the method's formal parameters is of an as yet
   incomplete type.  */

static void
compute_delayed_physnames (struct dwarf2_cu *cu)
{
  /* Only C++ delays computing physnames.  */
  if (cu->method_list.empty ())
    return;
  gdb_assert (cu->lang () == language_cplus);

  for (const delayed_method_info &mi : cu->method_list)
    {
      const char *physname;
      struct fn_fieldlist *fn_flp
	= &TYPE_FN_FIELDLIST (mi.type, mi.fnfield_index);
      physname = dwarf2_physname (mi.name, mi.die, cu);
      TYPE_FN_FIELD_PHYSNAME (fn_flp->fn_fields, mi.index)
	= physname ? physname : "";

      /* Since there's no tag to indicate whether a method is a
	 const/volatile overload, extract that information out of the
	 demangled name.  */
      if (physname != NULL)
	{
	  size_t len = strlen (physname);

	  while (1)
	    {
	      if (physname[len - 1] == ')') /* shortcut */
		break;
	      else if (check_modifier (physname, len, " const"))
		TYPE_FN_FIELD_CONST (fn_flp->fn_fields, mi.index) = 1;
	      else if (check_modifier (physname, len, " volatile"))
		TYPE_FN_FIELD_VOLATILE (fn_flp->fn_fields, mi.index) = 1;
	      else
		break;
	    }
	}
    }

  /* The list is no longer needed.  */
  cu->method_list.clear ();
}

/* Go objects should be embedded in a DW_TAG_module DIE,
   and it's not clear if/how imported objects will appear.
   To keep Go support simple until that's worked out,
   go back through what we've read and create something usable.
   We could do this while processing each DIE, and feels kinda cleaner,
   but that way is more invasive.
   This is to, for example, allow the user to type "p var" or "b main"
   without having to specify the package name, and allow lookups
   of module.object to work in contexts that use the expression
   parser.  */

static void
fixup_go_packaging (struct dwarf2_cu *cu)
{
  gdb::unique_xmalloc_ptr<char> package_name;
  struct pending *list;
  int i;

  for (list = *cu->get_builder ()->get_global_symbols ();
       list != NULL;
       list = list->next)
    {
      for (i = 0; i < list->nsyms; ++i)
	{
	  struct symbol *sym = list->symbol[i];

	  if (sym->language () == language_go
	      && sym->loc_class () == LOC_BLOCK)
	    {
	      gdb::unique_xmalloc_ptr<char> this_package_name
		= go_symbol_package_name (sym);

	      if (this_package_name == NULL)
		continue;
	      if (package_name == NULL)
		package_name = std::move (this_package_name);
	      else
		{
		  struct objfile *objfile = cu->per_objfile->objfile;
		  if (strcmp (package_name.get (), this_package_name.get ()) != 0)
		    complaint (_("Symtab %s has objects from two different Go packages: %s and %s"),
			       (sym->symtab () != NULL
				? symtab_to_filename_for_display
				(sym->symtab ())
				: objfile_name (objfile)),
			       this_package_name.get (), package_name.get ());
		}
	    }
	}
    }

  if (package_name != NULL)
    {
      struct objfile *objfile = cu->per_objfile->objfile;
      const char *saved_package_name = objfile->intern (package_name.get ());
      struct type *type
	= type_allocator (objfile, cu->lang ()).new_type (TYPE_CODE_MODULE, 0,
							  saved_package_name);
      struct symbol *sym;

      sym = new (&objfile->objfile_obstack) symbol;
      sym->set_language (language_go, &objfile->objfile_obstack);
      sym->compute_and_set_names (saved_package_name, false, objfile->per_bfd);
      sym->set_domain (TYPE_DOMAIN);
      sym->set_loc_class_index (LOC_TYPEDEF);
      sym->set_type (type);

      add_symbol_to_list (sym, cu->get_builder ()->get_global_symbols ());
    }
}

/* Allocate a fully-qualified name consisting of the two parts on the
   obstack.  */

static const char *
rust_fully_qualify (struct obstack *obstack, const char *p1, const char *p2)
{
  return obconcat (obstack, p1, "::", p2, (char *) NULL);
}

/* A helper that allocates a variant part to attach to a Rust enum
   type.  OBSTACK is where the results should be allocated.  TYPE is
   the type we're processing.  DISCRIMINANT_INDEX is the index of the
   discriminant.  It must be the index of one of the fields of TYPE,
   or -1 to mean there is no discriminant (univariant enum).
   DEFAULT_INDEX is the index of the default field; or -1 if there is
   no default.  RANGES is indexed by "effective" field number (the
   field index, but omitting the discriminant and default fields) and
   must hold the discriminant values used by the variants.  Note that
   RANGES must have a lifetime at least as long as OBSTACK -- either
   already allocated on it, or static.  */

static void
alloc_rust_variant (struct obstack *obstack, struct type *type,
		    int discriminant_index, int default_index,
		    gdb::array_view<discriminant_range> ranges)
{
  /* When DISCRIMINANT_INDEX == -1, we have a univariant enum.  */
  gdb_assert (discriminant_index == -1
	      || (discriminant_index >= 0
		  && discriminant_index < type->num_fields ()));
  gdb_assert (default_index == -1
	      || (default_index >= 0 && default_index < type->num_fields ()));

  /* We have one variant for each non-discriminant field.  */
  int n_variants = type->num_fields ();
  if (discriminant_index != -1)
    --n_variants;

  variant *variants = new (obstack) variant[n_variants];
  int var_idx = 0;
  int range_idx = 0;
  for (int i = 0; i < type->num_fields (); ++i)
    {
      if (i == discriminant_index)
	continue;

      variants[var_idx].first_field = i;
      variants[var_idx].last_field = i + 1;

      /* The default field does not need a range, but other fields do.
	 We skipped the discriminant above.  */
      if (i != default_index)
	{
	  variants[var_idx].discriminants = ranges.slice (range_idx, 1);
	  ++range_idx;
	}

      ++var_idx;
    }

  gdb_assert (range_idx == ranges.size ());
  gdb_assert (var_idx == n_variants);

  variant_part *part = new (obstack) variant_part;
  part->discriminant_index = discriminant_index;
  /* If there is no discriminant, then whether it is signed is of no
     consequence.  */
  part->is_unsigned
    = (discriminant_index == -1
       ? false
       : type->field (discriminant_index).type ()->is_unsigned ());
  part->variants = gdb::array_view<variant> (variants, n_variants);

  void *storage = obstack_alloc (obstack, sizeof (gdb::array_view<variant_part>));
  gdb::array_view<variant_part> *prop_value
    = new (storage) gdb::array_view<variant_part> (part, 1);

  struct dynamic_prop prop;
  prop.set_variant_parts (prop_value);

  type->add_dyn_prop (DYN_PROP_VARIANT_PARTS, prop);
}

/* Some versions of rustc emitted enums in an unusual way.

   Ordinary enums were emitted as unions.  The first element of each
   structure in the union was named "RUST$ENUM$DISR".  This element
   held the discriminant.

   These versions of Rust also implemented the "non-zero"
   optimization.  When the enum had two values, and one is empty and
   the other holds a pointer that cannot be zero, the pointer is used
   as the discriminant, with a zero value meaning the empty variant.
   Here, the union's first member is of the form
   RUST$ENCODED$ENUM$<fieldno>$<fieldno>$...$<variantname>
   where the fieldnos are the indices of the fields that should be
   traversed in order to find the field (which may be several fields deep)
   and the variantname is the name of the variant of the case when the
   field is zero.

   This function recognizes whether TYPE is of one of these forms,
   and, if so, smashes it to be a variant type.  */

static void
quirk_rust_enum (struct type *type, struct objfile *objfile)
{
  gdb_assert (type->code () == TYPE_CODE_UNION);

  /* We don't need to deal with empty enums.  */
  if (type->num_fields () == 0)
    return;

#define RUST_ENUM_PREFIX "RUST$ENCODED$ENUM$"
  if (type->num_fields () == 1
      && startswith (type->field (0).name (), RUST_ENUM_PREFIX))
    {
      const char *name = type->field (0).name () + strlen (RUST_ENUM_PREFIX);

      /* Decode the field name to find the offset of the
	 discriminant.  */
      ULONGEST bit_offset = 0;
      struct type *field_type = type->field (0).type ();
      while (name[0] >= '0' && name[0] <= '9')
	{
	  char *tail;
	  unsigned long index = strtoul (name, &tail, 10);
	  name = tail;
	  if (*name != '$'
	      || index >= field_type->num_fields ()
	      || (field_type->field (index).loc_kind ()
		  != FIELD_LOC_KIND_BITPOS))
	    {
	      complaint (_("Could not parse Rust enum encoding string \"%s\""
			   "[in module %s]"),
			 type->field (0).name (),
			 objfile_name (objfile));
	      return;
	    }
	  ++name;

	  bit_offset += field_type->field (index).loc_bitpos ();
	  field_type = field_type->field (index).type ();
	}

      /* Smash this type to be a structure type.  We have to do this
	 because the type has already been recorded.  */
      type->set_code (TYPE_CODE_STRUCT);
      /* Save the field we care about.  */
      struct field saved_field = type->field (0);
      type->alloc_fields (3);

      /* Put the discriminant at index 0.  */
      type->field (0).set_type (field_type);
      type->field (0).set_is_artificial (true);
      type->field (0).set_name ("<<discriminant>>");
      type->field (0).set_loc_bitpos (bit_offset);

      /* The order of fields doesn't really matter, so put the real
	 field at index 1 and the data-less field at index 2.  */
      type->field (1) = saved_field;
      type->field (1).set_name
	(rust_last_path_segment (type->field (1).type ()->name ()));
      type->field (1).type ()->set_name
	(rust_fully_qualify (&objfile->objfile_obstack, type->name (),
			     type->field (1).name ()));

      const char *dataless_name
	= rust_fully_qualify (&objfile->objfile_obstack, type->name (),
			      name);
      struct type *dataless_type
	= type_allocator (type).new_type (TYPE_CODE_VOID, 0,
					  dataless_name);
      type->field (2).set_type (dataless_type);
      /* NAME points into the original discriminant name, which
	 already has the correct lifetime.  */
      type->field (2).set_name (name);
      type->field (2).set_loc_bitpos (0);

      /* Indicate that this is a variant type.  */
      static discriminant_range ranges[1] = { { 0, 0 } };
      alloc_rust_variant (&objfile->objfile_obstack, type, 0, 1, ranges);
    }
  /* A union with a single anonymous field is probably an old-style
     univariant enum.  */
  else if (type->num_fields () == 1 && streq (type->field (0).name (), ""))
    {
      /* Smash this type to be a structure type.  We have to do this
	 because the type has already been recorded.  */
      type->set_code (TYPE_CODE_STRUCT);

      struct type *field_type = type->field (0).type ();
      const char *variant_name
	= rust_last_path_segment (field_type->name ());
      type->field (0).set_name (variant_name);
      field_type->set_name
	(rust_fully_qualify (&objfile->objfile_obstack,
			     type->name (), variant_name));

      alloc_rust_variant (&objfile->objfile_obstack, type, -1, 0, {});
    }
  else
    {
      struct type *disr_type = nullptr;
      for (const auto &field : type->fields ())
	{
	  disr_type = field.type ();

	  if (disr_type->code () != TYPE_CODE_STRUCT)
	    {
	      /* All fields of a true enum will be structs.  */
	      return;
	    }
	  else if (disr_type->num_fields () == 0)
	    {
	      /* Could be data-less variant, so keep going.  */
	      disr_type = nullptr;
	    }
	  else if (strcmp (disr_type->field (0).name (),
			   "RUST$ENUM$DISR") != 0)
	    {
	      /* Not a Rust enum.  */
	      return;
	    }
	  else
	    {
	      /* Found one.  */
	      break;
	    }
	}

      /* If we got here without a discriminant, then it's probably
	 just a union.  */
      if (disr_type == nullptr)
	return;

      /* Smash this type to be a structure type.  We have to do this
	 because the type has already been recorded.  */
      type->set_code (TYPE_CODE_STRUCT);

      /* Make space for the discriminant field.  */
      struct field *disr_field = &disr_type->field (0);
      field *new_fields
	= (struct field *) TYPE_ZALLOC (type, ((type->num_fields () + 1)
					       * sizeof (struct field)));
      memcpy (new_fields + 1, type->fields ().data (),
	      type->num_fields () * sizeof (struct field));
      type->set_fields (new_fields);
      type->set_num_fields (type->num_fields () + 1);

      /* Install the discriminant at index 0 in the union.  */
      type->field (0) = *disr_field;
      type->field (0).set_is_artificial (true);
      type->field (0).set_name ("<<discriminant>>");

      /* We need a way to find the correct discriminant given a
	 variant name.  For convenience we build a map here.  */
      struct type *enum_type = disr_field->type ();
      gdb::unordered_map<std::string_view, ULONGEST> discriminant_map;
      for (const auto &field : enum_type->fields ())
	{
	  if (field.loc_kind () == FIELD_LOC_KIND_ENUMVAL)
	    {
	      const char *name = rust_last_path_segment (field.name ());
	      discriminant_map[name] = field.loc_enumval ();
	    }
	}

      int n_fields = type->num_fields ();
      /* We don't need a range entry for the discriminant, but we do
	 need one for every other field, as there is no default
	 variant.  */
      discriminant_range *ranges = XOBNEWVEC (&objfile->objfile_obstack,
					      discriminant_range,
					      n_fields - 1);
      /* Skip the discriminant here.  */
      for (int i = 1; i < n_fields; ++i)
	{
	  /* Find the final word in the name of this variant's type.
	     That name can be used to look up the correct
	     discriminant.  */
	  const char *variant_name
	    = rust_last_path_segment (type->field (i).type ()->name ());

	  auto iter = discriminant_map.find (variant_name);
	  if (iter != discriminant_map.end ())
	    {
	      ranges[i - 1].low = iter->second;
	      ranges[i - 1].high = iter->second;
	    }

	  /* In Rust, each element should have the size of the
	     enclosing enum.  */
	  type->field (i).type ()->set_length (type->length ());

	  /* Remove the discriminant field, if it exists.  */
	  struct type *sub_type = type->field (i).type ();
	  if (sub_type->num_fields () > 0)
	    {
	      sub_type->set_num_fields (sub_type->num_fields () - 1);
	      sub_type->set_fields (sub_type->fields ().data () + 1);
	    }
	  type->field (i).set_name (variant_name);
	  sub_type->set_name
	    (rust_fully_qualify (&objfile->objfile_obstack,
				 type->name (), variant_name));
	}

      /* Indicate that this is a variant type.  */
      alloc_rust_variant (&objfile->objfile_obstack, type, 0, -1,
			  gdb::array_view<discriminant_range> (ranges,
							       n_fields - 1));
    }
}

/* Rewrite some Rust unions to be structures with variants parts.  */

static void
rust_union_quirks (struct dwarf2_cu *cu)
{
  gdb_assert (cu->lang () == language_rust);
  for (type *type_ : cu->rust_unions)
    quirk_rust_enum (type_, cu->per_objfile->objfile);
  /* We don't need this any more.  */
  cu->rust_unions.clear ();
}

/* See read.h.  */

type_unit_group_unshareable *
dwarf2_per_objfile::get_type_unit_group_unshareable
  (stmt_list_hash tu_group_key)
{
  auto [it, inserted] = m_type_units.emplace (tu_group_key, nullptr);

  if (inserted)
    it->second = std::make_unique<type_unit_group_unshareable> ();

  return it->second.get ();
}

struct type *
dwarf2_per_objfile::get_type_for_signatured_type
  (signatured_type *sig_type) const
{
  auto iter = m_type_map.find (sig_type);
  if (iter == m_type_map.end ())
    return nullptr;

  return iter->second;
}

void dwarf2_per_objfile::set_type_for_signatured_type
  (signatured_type *sig_type, struct type *type)
{
  gdb_assert (m_type_map.find (sig_type) == m_type_map.end ());

  m_type_map[sig_type] = type;
}

/* A helper function for computing the list of all symbol tables
   included by PER_CU.  */

static void
recursively_compute_inclusions
     (std::vector<compunit_symtab *> *result,
      gdb::unordered_set<dwarf2_per_cu *> &all_children,
      gdb::unordered_set<compunit_symtab *> &all_type_symtabs,
      dwarf2_per_cu *per_cu,
      dwarf2_per_objfile *per_objfile,
      struct compunit_symtab *immediate_parent)
{
  if (bool inserted = all_children.emplace (per_cu).second;
      !inserted)
    {
      /* This inclusion and its children have been processed.  */
      return;
    }

  /* Only add a CU if it has a symbol table.  */
  compunit_symtab *cust = per_objfile->get_symtab (per_cu);
  if (cust != NULL)
    {
      /* If this is a type unit only add its symbol table if we haven't
	 seen it yet (type unit per_cu's can share symtabs).  */
      if (per_cu->is_debug_types)
	{
	  if (bool inserted = all_type_symtabs.insert (cust).second;
	      inserted)
	    {
	      result->push_back (cust);
	      if (cust->user == NULL)
		cust->user = immediate_parent;
	    }
	}
      else
	{
	  result->push_back (cust);
	  if (cust->user == NULL)
	    cust->user = immediate_parent;
	}
    }

  for (dwarf2_per_cu *ptr : per_cu->imported_symtabs)
    recursively_compute_inclusions (result, all_children,
				    all_type_symtabs, ptr, per_objfile,
				    cust);
}

/* Compute the compunit_symtab 'includes' fields for the compunit_symtab of
   PER_CU.  */

static void
compute_compunit_symtab_includes (dwarf2_per_cu *per_cu,
				  dwarf2_per_objfile *per_objfile)
{
  gdb_assert (! per_cu->is_debug_types);

  if (!per_cu->imported_symtabs.empty ())
    {
      int len;
      std::vector<compunit_symtab *> result_symtabs;
      compunit_symtab *cust = per_objfile->get_symtab (per_cu);

      /* If we don't have a symtab, we can just skip this case.  */
      if (cust == NULL)
	return;

      gdb::unordered_set<dwarf2_per_cu *> all_children;
      gdb::unordered_set<compunit_symtab *> all_type_symtabs;

      for (dwarf2_per_cu *ptr : per_cu->imported_symtabs)
	recursively_compute_inclusions (&result_symtabs, all_children,
					all_type_symtabs, ptr,
					per_objfile, cust);

      /* Now we have a transitive closure of all the included symtabs.  */
      len = result_symtabs.size ();
      cust->includes
	= XOBNEWVEC (&per_objfile->objfile->objfile_obstack,
		     struct compunit_symtab *, len + 1);
      memcpy (cust->includes, result_symtabs.data (),
	      len * sizeof (compunit_symtab *));
      cust->includes[len] = NULL;
    }
}

/* Compute the 'includes' field for the symtabs of all the CUs we just
   read.  */

static void
process_cu_includes (dwarf2_per_objfile *per_objfile)
{
  for (dwarf2_per_cu *iter : per_objfile->per_bfd->just_read_cus)
    {
      if (! iter->is_debug_types)
	compute_compunit_symtab_includes (iter, per_objfile);
    }

  per_objfile->per_bfd->just_read_cus.clear ();
}

/* Generate full symbol information for CU, whose DIEs have
   already been loaded into memory.  */

static void
process_full_comp_unit (dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  unrelocated_addr lowpc, highpc;
  struct compunit_symtab *cust;
  struct block *static_block;
  CORE_ADDR addr;

  /* Clear the list here in case something was left over.  */
  cu->method_list.clear ();

  dwarf2_find_base_address (cu->dies, cu);

  /* Before we start reading the top-level DIE, ensure it has a valid tag
     type.  */
  switch (cu->dies->tag)
    {
    case DW_TAG_compile_unit:
    case DW_TAG_partial_unit:
    case DW_TAG_type_unit:
      break;
    default:
      error (_(DWARF_ERROR_PREFIX
	       "unexpected tag '%s' at offset %s [in module %s]"),
	     dwarf_tag_name (cu->dies->tag),
	     sect_offset_str (cu->per_cu->sect_off),
	     objfile_name (per_objfile->objfile));
    }

  /* Do line number decoding in read_file_scope () */
  process_die (cu->dies, cu);

  /* For now fudge the Go package.  */
  if (cu->lang () == language_go)
    fixup_go_packaging (cu);

  /* Now that we have processed all the DIEs in the CU, all the types
     should be complete, and it should now be safe to compute all of the
     physnames.  */
  compute_delayed_physnames (cu);

  if (cu->lang () == language_rust)
    rust_union_quirks (cu);

  /* Some compilers don't define a DW_AT_high_pc attribute for the
     compilation unit.  If the DW_AT_high_pc is missing, synthesize
     it, by scanning the DIE's below the compilation unit.  */
  get_scope_pc_bounds (cu->dies, &lowpc, &highpc, cu);

  addr = per_objfile->relocate (highpc);
  static_block
    = cu->get_builder ()->end_compunit_symtab_get_static_block (addr, 0, 1);

  /* If the comp unit has DW_AT_ranges, it may have discontiguous ranges.
     Also, DW_AT_ranges may record ranges not belonging to any child DIEs
     (such as virtual method tables).  Record the ranges in STATIC_BLOCK's
     addrmap to help ensure it has an accurate map of pc values belonging to
     this comp unit.  */
  dwarf2_record_block_ranges (cu->dies, static_block, cu);

  cust = cu->get_builder ()->end_compunit_symtab_from_static_block
    (static_block, 0);

  if (cust != NULL)
    {
      /* Set symtab language to language from DW_AT_language.  If the
	 compilation is from a C file generated by language preprocessors, do
	 not set the language if it was already deduced by start_subfile.  */
      if (!(cu->lang () == language_c
	    && cust->primary_filetab ()->language () != language_unknown))
	cust->primary_filetab ()->set_language (cu->lang ());

      /* GCC-4.0 has started to support -fvar-tracking.  GCC-3.x still can
	 produce DW_AT_location with location lists but it can be possibly
	 invalid without -fvar-tracking.  Still up to GCC-4.4.x incl. 4.4.0
	 there were bugs in prologue debug info, fixed later in GCC-4.5
	 by "unwind info for epilogues" patch (which is not directly related).

	 For -gdwarf-4 type units LOCATIONS_VALID indication is fortunately not
	 needed, it would be wrong due to missing DW_AT_producer there.

	 Still one can confuse GDB by using non-standard GCC compilation
	 options - this waits on GCC PR other/32998 (-frecord-gcc-switches).
	 */
      /* Note that this code traditionally did not accept non-GCC
	 compilers; that is preserved but seems potentially wrong.  */
      if (cu->has_loclist && cu->producer_is_gcc ()
	  && !cu->producer_is_gcc_lt_4_5 ())
	cust->set_locations_valid (true);

      if (cu->producer_is_gcc_lt_4_5 ())
	/* Don't trust gcc < 4.5.x.  */
	cust->set_epilogue_unwind_valid (false);
      else
	cust->set_epilogue_unwind_valid (true);

      cust->set_call_site_htab (std::move (cu->call_site_htab));
    }

  per_objfile->set_symtab (cu->per_cu, cust);

  /* Push it for inclusion processing later.  */
  per_objfile->per_bfd->just_read_cus.push_back (cu->per_cu);

  /* Not needed any more.  */
  cu->reset_builder ();
}

/* Generate full symbol information for type unit CU, whose DIEs have
   already been loaded into memory.  */

static void
process_full_type_unit (dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct compunit_symtab *cust;
  struct signatured_type *sig_type;

  gdb_assert (cu->per_cu->is_debug_types);
  sig_type = (struct signatured_type *) cu->per_cu;

  /* Clear the list here in case something was left over.  */
  cu->method_list.clear ();

  /* The symbol tables are set up in read_type_unit_scope.  */
  process_die (cu->dies, cu);

  /* For now fudge the Go package.  */
  if (cu->lang () == language_go)
    fixup_go_packaging (cu);

  /* Now that we have processed all the DIEs in the CU, all the types
     should be complete, and it should now be safe to compute all of the
     physnames.  */
  compute_delayed_physnames (cu);

  if (cu->lang () == language_rust)
    rust_union_quirks (cu);

  /* TUs share symbol tables.
     If this is the first TU to use this symtab, complete the construction
     of it with end_expandable_symtab.  Otherwise, complete the addition of
     this TU's symbols to the existing symtab.  */
  type_unit_group_unshareable *tug_unshare =
    per_objfile->get_type_unit_group_unshareable (*sig_type->type_unit_group_key);
  if (tug_unshare->compunit_symtab == NULL)
    {
      buildsym_compunit *builder = cu->get_builder ();
      cust = builder->end_expandable_symtab (0);
      tug_unshare->compunit_symtab = cust;

      if (cust != NULL)
	{
	  /* Set symtab language to language from DW_AT_language.  If the
	     compilation is from a C file generated by language preprocessors,
	     do not set the language if it was already deduced by
	     start_subfile.  */
	  if (!(cu->lang () == language_c
		&& cust->primary_filetab ()->language () != language_c))
	    cust->primary_filetab ()->set_language (cu->lang ());
	}
    }
  else
    {
      cu->get_builder ()->augment_type_symtab ();
      cust = tug_unshare->compunit_symtab;
    }

  per_objfile->set_symtab (cu->per_cu, cust);

  /* Not needed any more.  */
  cu->reset_builder ();
}

/* See read.h.  */

const dwarf2_section_info &
get_section_for_ref (const attribute &attr, dwarf2_cu *cu)
{
  gdb_assert (attr.form_is_ref ());

  if (attr.form_is_alt ())
    return cu->per_cu->per_bfd->get_dwz_file (true)->info;

  /* If the source is already in the supplementary (dwz) file, then CU->SECTION
     already represents the section in the supplementary file.  */
  return cu->section ();
}

/* Process an imported unit DIE.  */

static void
process_imported_unit_die (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  /* For now we don't handle imported units in type units.  */
  if (cu->per_cu->is_debug_types)
    {
      error (_(DWARF_ERROR_PREFIX
	       "DW_TAG_imported_unit is not supported in type units"
	       " [in module %s]"),
	     objfile_name (cu->per_objfile->objfile));
    }

  attr = dwarf2_attr (die, DW_AT_import, cu);
  if (attr != NULL)
    {
      const dwarf2_section_info &section = get_section_for_ref (*attr, cu);
      sect_offset sect_off = attr->get_ref_die_offset ();
      dwarf2_per_objfile *per_objfile = cu->per_objfile;
      dwarf2_per_cu *per_cu
	= dwarf2_find_containing_unit ({ &section, sect_off }, per_objfile);

      /* We're importing a C++ compilation unit with tag DW_TAG_compile_unit
	 into another compilation unit, at root level.  Regard this as a hint,
	 and ignore it.  This is a best effort, it only works if unit_type and
	 lang are already set.  */
      if (die->parent && die->parent->parent == NULL
	  && per_cu->unit_type (false) == DW_UT_compile
	  && per_cu->lang (false) == language_cplus)
	return;

      /* If necessary, add it to the queue and load its DIEs.  */
      if (maybe_queue_comp_unit (cu, per_cu, per_objfile))
	load_full_comp_unit (per_cu, per_objfile, false, cu->lang ());

      cu->per_cu->imported_symtabs.push_back (per_cu);
    }
}

/* RAII object that represents a process_die scope: i.e.,
   starts/finishes processing a DIE.  */
class process_die_scope
{
public:
  process_die_scope (die_info *die, dwarf2_cu *cu)
    : m_die (die), m_cu (cu)
  {
    /* We should only be processing DIEs not already in process.  */
    gdb_assert (!m_die->in_process);
    m_die->in_process = true;
  }

  ~process_die_scope ()
  {
    m_die->in_process = false;

    /* If we're done processing the DIE for the CU that owns the line
       header, we don't need the line header anymore.  */
    if (m_cu->line_header_die_owner == m_die)
      {
	delete m_cu->line_header;
	m_cu->line_header = NULL;
	m_cu->line_header_die_owner = NULL;
      }
  }

private:
  die_info *m_die;
  dwarf2_cu *m_cu;
};

/* Process a die and its children.  */

static void
process_die (struct die_info *die, struct dwarf2_cu *cu)
{
  process_die_scope scope (die, cu);

  switch (die->tag)
    {
    case DW_TAG_padding:
      break;
    case DW_TAG_compile_unit:
    case DW_TAG_partial_unit:
      read_file_scope (die, cu);
      break;
    case DW_TAG_type_unit:
      read_type_unit_scope (die, cu);
      break;
    case DW_TAG_subprogram:
      /* Nested subprograms in Fortran get a prefix.  */
      if (cu->lang () == language_fortran
	  && die->parent != NULL
	  && die->parent->tag == DW_TAG_subprogram)
	cu->processing_has_namespace_info = true;
      [[fallthrough]];
      /* Fall through.  */
    case DW_TAG_entry_point:
    case DW_TAG_inlined_subroutine:
      read_func_scope (die, cu);
      break;
    case DW_TAG_lexical_block:
    case DW_TAG_try_block:
    case DW_TAG_catch_block:
      read_lexical_block_scope (die, cu);
      break;
    case DW_TAG_call_site:
    case DW_TAG_GNU_call_site:
      read_call_site_scope (die, cu);
      break;
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_namelist:
      process_structure_scope (die, cu);
      break;
    case DW_TAG_enumeration_type:
      process_enumeration_scope (die, cu);
      break;

    /* These dies have a type, but processing them does not create
       a symbol or recurse to process the children.  Therefore we can
       read them on-demand through read_type_die.  */
    case DW_TAG_subroutine_type:
    case DW_TAG_set_type:
    case DW_TAG_pointer_type:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_reference_type:
    case DW_TAG_rvalue_reference_type:
    case DW_TAG_string_type:
      break;

    case DW_TAG_array_type:
      /* We only need to handle this case for Ada -- in other
	 languages, it's normal for the compiler to emit a typedef
	 instead.  */
      if (cu->lang () != language_ada)
	break;
      [[fallthrough]];
    case DW_TAG_base_type:
    case DW_TAG_subrange_type:
    case DW_TAG_generic_subrange:
    case DW_TAG_typedef:
    case DW_TAG_unspecified_type:
      /* Add a typedef symbol for the type definition, if it has a
	 DW_AT_name.  */
      new_symbol (die, read_type_die (die, cu), cu);
      break;
    case DW_TAG_common_block:
      read_common_block (die, cu);
      break;
    case DW_TAG_common_inclusion:
      break;
    case DW_TAG_namespace:
      cu->processing_has_namespace_info = true;
      read_namespace (die, cu);
      break;
    case DW_TAG_module:
      cu->processing_has_namespace_info = true;
      read_module (die, cu);
      break;
    case DW_TAG_imported_declaration:
      cu->processing_has_namespace_info = true;
      if (read_alias (die, cu))
	break;
      /* The declaration is neither a global namespace nor a variable
	 alias.  */
      [[fallthrough]];
    case DW_TAG_imported_module:
      cu->processing_has_namespace_info = true;
      if (die->child != NULL && (die->tag == DW_TAG_imported_declaration
				 || cu->lang () != language_fortran))
	complaint (_("Tag '%s' has unexpected children"),
		   dwarf_tag_name (die->tag));
      read_import_statement (die, cu);
      break;

    case DW_TAG_imported_unit:
      process_imported_unit_die (die, cu);
      break;

    case DW_TAG_variable:
      read_variable (die, cu);
      break;

    default:
      new_symbol (die, NULL, cu);
      break;
    }
}

/* DWARF name computation.  */

/* A helper function for dwarf2_compute_name which determines whether DIE
   needs to have the name of the scope prepended to the name listed in the
   die.  */

static int
die_needs_namespace (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  if (tag_is_type (die->tag) && die->tag != DW_TAG_template_type_param)
    {
      /* Historically GNAT emitted some types in funny scopes.  For
	 example, in one test case, where the first use of Natural was
	 as the type of a field in a record, GNAT emitted:

	  <2>: DW_TAG_structure_type
	  ... variant parts and whatnot
	  <5>: DW_TAG_subrange_type
	  .    DW_AT_name: natural

	  To detect this, we look up the DIE tree for a node that has
	  a name; and if that name is fully qualified, we return 0
	  here.  */
      if (cu->lang () == language_ada)
	{
	  for (die_info *iter = die->parent;
	       iter != nullptr;
	       iter = iter->parent)
	    {
	      if (tag_is_type (iter->tag))
		{
		  const char *name = dwarf2_name (iter, cu);
		  if (name != nullptr)
		    return strstr (name, "__") == nullptr;
		}
	    }
	}
      return 1;
    }

  switch (die->tag)
    {
    case DW_TAG_enumerator:
    case DW_TAG_subprogram:
    case DW_TAG_inlined_subroutine:
    case DW_TAG_entry_point:
    case DW_TAG_member:
    case DW_TAG_imported_declaration:
      return 1;

    case DW_TAG_module:
      /* We don't need the namespace for Fortran modules, but we do
	 for Ada packages.  */
      return cu->lang () == language_ada;

    case DW_TAG_variable:
    case DW_TAG_constant:
      /* We only need to prefix "globally" visible variables.  These include
	 any variable marked with DW_AT_external or any variable that
	 lives in a namespace.  [Variables in anonymous namespaces
	 require prefixing, but they are not DW_AT_external.]  */

      if (dwarf2_attr (die, DW_AT_specification, cu))
	{
	  struct dwarf2_cu *spec_cu = cu;

	  return die_needs_namespace (die_specification (die, &spec_cu),
				      spec_cu);
	}

      attr = dwarf2_attr (die, DW_AT_external, cu);
      if (attr == NULL && die->parent->tag != DW_TAG_namespace
	  && die->parent->tag != DW_TAG_module)
	return 0;
      /* A variable in a lexical block of some kind does not need a
	 namespace, even though in C++ such variables may be external
	 and have a mangled name.  */
      if (die->parent->tag ==  DW_TAG_lexical_block
	  || die->parent->tag ==  DW_TAG_try_block
	  || die->parent->tag ==  DW_TAG_catch_block
	  || die->parent->tag == DW_TAG_subprogram)
	return 0;
      return 1;

    default:
      return 0;
    }
}

/* Return the DIE's linkage name attribute, either DW_AT_linkage_name
   or DW_AT_MIPS_linkage_name.  Returns NULL if the attribute is not
   defined for the given DIE.  */

static struct attribute *
dw2_linkage_name_attr (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_linkage_name, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_MIPS_linkage_name, cu);

  return attr;
}

/* Return the DIE's linkage name as a string, either DW_AT_linkage_name
   or DW_AT_MIPS_linkage_name.  Returns NULL if the attribute is not
   defined for the given DIE.  */

static const char *
dw2_linkage_name (struct die_info *die, struct dwarf2_cu *cu)
{
  const char *linkage_name;

  linkage_name = dwarf2_string_attr (die, DW_AT_linkage_name, cu);
  if (linkage_name == NULL)
    linkage_name = dwarf2_string_attr (die, DW_AT_MIPS_linkage_name, cu);

  /* rustc emits invalid values for DW_AT_linkage_name.  Ignore these.
     See https://github.com/rust-lang/rust/issues/32925.  */
  if (cu->lang () == language_rust && linkage_name != NULL
      && strchr (linkage_name, '{') != NULL)
    linkage_name = NULL;

  return linkage_name;
}

/* Compute the fully qualified name of DIE in CU.  If PHYSNAME is nonzero,
   compute the physname for the object, which include a method's:
   - formal parameters (C++),
   - receiver type (Go),

   The term "physname" is a bit confusing.
   For C++, for example, it is the demangled name.
   For Go, for example, it's the mangled name.

   For Ada, return the DIE's linkage name rather than the fully qualified
   name.  PHYSNAME is ignored..

   The result is allocated on the objfile->per_bfd's obstack and
   canonicalized.  */

static const char *
dwarf2_compute_name (const char *name,
		     struct die_info *die, struct dwarf2_cu *cu,
		     int physname)
{
  struct objfile *objfile = cu->per_objfile->objfile;

  if (name == NULL)
    name = dwarf2_name (die, cu);

  enum language lang = cu->lang ();

  /* For Fortran GDB prefers DW_AT_*linkage_name for the physname if present
     but otherwise compute it by typename_concat inside GDB.
     FIXME: Actually this is not really true, or at least not always true.
     It's all very confusing.  compute_and_set_names doesn't try to demangle
     Fortran names because there is no mangling standard.  So new_symbol
     will set the demangled name to the result of dwarf2_full_name, and it is
     the demangled name that GDB uses if it exists.  */
  if ((lang == language_ada || lang == language_fortran) && physname)
    {
      /* For Ada unit, we prefer the linkage name over the name, as
	 the former contains the exported name, which the user expects
	 to be able to reference.  Ideally, we want the user to be able
	 to reference this entity using either natural or linkage name,
	 but we haven't started looking at this enhancement yet.  */
      const char *linkage_name = dw2_linkage_name (die, cu);

      if (linkage_name != NULL)
	return linkage_name;
    }

  /* Some versions of GNAT emit fully-qualified names already.  These
     have "__" separating the components -- something ordinary names
     will never have.  */
  if (lang == language_ada
      && name != nullptr
      && strstr (name, "__") != nullptr)
    return name;

  /* These are the only languages we know how to qualify names in.  */
  if (name != NULL
      && (lang == language_cplus
	  || lang == language_fortran
	  || lang == language_d
	  || lang == language_rust
	  || lang == language_ada))
    {
      if (die_needs_namespace (die, cu))
	{
	  const char *prefix;

	  string_file buf;

	  prefix = determine_prefix (die, cu);
	  if (*prefix != '\0')
	    {
	      gdb::unique_xmalloc_ptr<char> prefixed_name
		= typename_concat (prefix, name, physname, cu);

	      buf.puts (prefixed_name.get ());
	    }
	  else
	    buf.puts (name);

	  /* Template parameters may be specified in the DIE's DW_AT_name, or
	     as children with DW_TAG_template_type_param or
	     DW_TAG_value_type_param.  If the latter, add them to the name
	     here.  If the name already has template parameters, then
	     skip this step; some versions of GCC emit both, and
	     it is more efficient to use the pre-computed name.

	     Something to keep in mind about this process: it is very
	     unlikely, or in some cases downright impossible, to produce
	     something that will match the mangled name of a function.
	     If the definition of the function has the same debug info,
	     we should be able to match up with it anyway.  But fallbacks
	     using the minimal symbol, for instance to find a method
	     implemented in a stripped copy of libstdc++, will not work.
	     If we do not have debug info for the definition, we will have to
	     match them up some other way.

	     When we do name matching there is a related problem with function
	     templates; two instantiated function templates are allowed to
	     differ only by their return types, which we do not add here.  */

	  if (lang == language_cplus && strchr (name, '<') == NULL)
	    {
	      struct attribute *attr;
	      int first = 1;

	      die->building_fullname = 1;

	      for (die_info *child : die->children ())
		{
		  struct type *type;
		  LONGEST value;
		  const gdb_byte *bytes;
		  struct dwarf2_locexpr_baton *baton;
		  struct value *v;

		  if (child->tag != DW_TAG_template_type_param
		      && child->tag != DW_TAG_template_value_param)
		    continue;

		  if (first)
		    {
		      buf.puts ("<");
		      first = 0;
		    }
		  else
		    buf.puts (", ");

		  attr = dwarf2_attr (child, DW_AT_type, cu);
		  if (attr == NULL)
		    {
		      complaint (_("template parameter missing DW_AT_type"));
		      buf.puts ("UNKNOWN_TYPE");
		      continue;
		    }
		  type = die_type (child, cu);

		  if (child->tag == DW_TAG_template_type_param)
		    {
		      cu->language_defn->print_type (type, "", &buf, -1, 0,
						     &type_print_raw_options);
		      continue;
		    }

		  attr = dwarf2_attr (child, DW_AT_const_value, cu);
		  if (attr == NULL)
		    {
		      complaint (_("template parameter missing "
				   "DW_AT_const_value"));
		      buf.puts ("UNKNOWN_VALUE");
		      continue;
		    }

		  dwarf2_const_value_attr (attr, type, name,
					   &cu->comp_unit_obstack, cu,
					   &value, &bytes, &baton);

		  if (type->has_no_signedness ())
		    /* GDB prints characters as NUMBER 'CHAR'.  If that's
		       changed, this can use value_print instead.  */
		    cu->language_defn->printchar (value, type, &buf);
		  else
		    {
		      struct value_print_options opts;

		      if (baton != NULL)
			v = dwarf2_evaluate_loc_desc (type, NULL,
						      baton->data,
						      baton->size,
						      baton->per_cu,
						      baton->per_objfile);
		      else if (bytes != NULL)
			{
			  v = value::allocate (type);
			  memcpy (v->contents_writeable ().data (), bytes,
				  type->length ());
			}
		      else
			v = value_from_longest (type, value);

		      /* Specify decimal so that we do not depend on
			 the radix.  */
		      get_formatted_print_options (&opts, 'd');
		      opts.raw = true;
		      value_print (v, &buf, &opts);
		      release_value (v);
		    }
		}

	      die->building_fullname = 0;

	      if (!first)
		{
		  /* Close the argument list, with a space if necessary
		     (nested templates).  */
		  if (!buf.empty () && buf.string ().back () == '>')
		    buf.puts (" >");
		  else
		    buf.puts (">");
		}
	    }

	  /* For C++ methods, append formal parameter type
	     information, if PHYSNAME.  */

	  if (physname && die->tag == DW_TAG_subprogram
	      && lang == language_cplus)
	    {
	      struct type *type = read_type_die (die, cu);

	      c_type_print_args (type, &buf, 1, lang,
				 &type_print_raw_options);

	      if (lang == language_cplus)
		{
		  /* Assume that an artificial first parameter is
		     "this", but do not crash if it is not.  RealView
		     marks unnamed (and thus unused) parameters as
		     artificial; there is no way to differentiate
		     the two cases.  */
		  if (type->num_fields () > 0
		      && type->field (0).is_artificial ()
		      && type->field (0).type ()->code () == TYPE_CODE_PTR
		      && TYPE_CONST (type->field (0).type ()->target_type ()))
		    buf.puts (" const");
		}
	    }

	  const std::string &intermediate_name = buf.string ();

	  const char *canonical_name
	    = dwarf2_canonicalize_name (intermediate_name.c_str (), cu,
					objfile);

	  /* If we only computed INTERMEDIATE_NAME, or if
	     INTERMEDIATE_NAME is already canonical, then we need to
	     intern it.  */
	  if (canonical_name == NULL || canonical_name == intermediate_name.c_str ())
	    name = objfile->intern (intermediate_name);
	  else
	    name = canonical_name;
	}
    }

  return name;
}

/* Return the fully qualified name of DIE, based on its DW_AT_name.
   If scope qualifiers are appropriate they will be added.  The result
   will be allocated on the storage_obstack, or NULL if the DIE does
   not have a name.  NAME may either be from a previous call to
   dwarf2_name or NULL.

   The output string will be canonicalized (if C++).  */

static const char *
dwarf2_full_name (const char *name, struct die_info *die, struct dwarf2_cu *cu)
{
  return dwarf2_compute_name (name, die, cu, 0);
}

/* Construct a physname for the given DIE in CU.  NAME may either be
   from a previous call to dwarf2_name or NULL.  The result will be
   allocated on the objfile_obstack or NULL if the DIE does not have a
   name.

   The output string will be canonicalized (if C++).  */

static const char *
dwarf2_physname (const char *name, struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  const char *retval, *mangled = NULL, *canon = NULL;
  int need_copy = 1;

  /* In this case dwarf2_compute_name is just a shortcut not building anything
     on its own.  */
  if (!die_needs_namespace (die, cu))
    return dwarf2_compute_name (name, die, cu, 1);

  if (cu->lang () != language_rust)
    mangled = dw2_linkage_name (die, cu);

  /* DW_AT_linkage_name is missing in some cases - depend on what GDB
     has computed.  */
  gdb::unique_xmalloc_ptr<char> demangled;
  if (mangled != NULL)
    {
      if (cu->language_defn->store_sym_names_in_linkage_form_p ())
	{
	  /* Do nothing (do not demangle the symbol name).  */
	}
      else
	{
	  /* Use DMGL_RET_DROP for C++ template functions to suppress
	     their return type.  It is easier for GDB users to search
	     for such functions as `name(params)' than `long name(params)'.
	     In such case the minimal symbol names do not match the full
	     symbol names but for template functions there is never a need
	     to look up their definition from their declaration so
	     the only disadvantage remains the minimal symbol variant
	     `long name(params)' does not have the proper inferior type.  */
	  demangled = gdb_demangle (mangled, (DMGL_PARAMS | DMGL_ANSI
					      | DMGL_RET_DROP));
	}
      if (demangled)
	canon = demangled.get ();
      else
	{
	  canon = mangled;
	  need_copy = 0;
	}
    }

  if (canon == NULL || check_physname)
    {
      const char *physname = dwarf2_compute_name (name, die, cu, 1);

      if (canon != NULL && strcmp (physname, canon) != 0)
	{
	  /* It may not mean a bug in GDB.  The compiler could also
	     compute DW_AT_linkage_name incorrectly.  But in such case
	     GDB would need to be bug-to-bug compatible.  */

	  complaint (_("Computed physname <%s> does not match demangled <%s> "
		       "(from linkage <%s>) - DIE at %s [in module %s]"),
		     physname, canon, mangled, sect_offset_str (die->sect_off),
		     objfile_name (objfile));

	  /* Prefer DW_AT_linkage_name (in the CANON form) - when it
	     is available here - over computed PHYSNAME.  It is safer
	     against both buggy GDB and buggy compilers.  */

	  retval = canon;
	}
      else
	{
	  retval = physname;
	  need_copy = 0;
	}
    }
  else
    retval = canon;

  if (need_copy)
    retval = objfile->intern (retval);

  return retval;
}

/* Inspect DIE in CU for a namespace alias or a variable with alias
   attribute.  If one exists, record a new symbol for it.

   Returns true if an alias was recorded, false otherwise.  */

static bool
read_alias (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  /* If the die does not have a name, this is neither a namespace
     alias nor a variable alias.  */
  attr = dwarf2_attr (die, DW_AT_name, cu);
  if (attr != NULL)
    {
      int num;
      struct die_info *d = die;
      struct dwarf2_cu *imported_cu = cu;

      /* If the compiler has nested DW_AT_imported_declaration DIEs,
	 keep inspecting DIEs until we hit the underlying import.  */
#define MAX_NESTED_IMPORTED_DECLARATIONS 100
      for (num = 0; num  < MAX_NESTED_IMPORTED_DECLARATIONS; ++num)
	{
	  attr = dwarf2_attr (d, DW_AT_import, cu);
	  if (attr == NULL)
	    break;

	  d = follow_die_ref (d, attr, &imported_cu);
	  if (d->tag != DW_TAG_imported_declaration)
	    break;
	}

      if (num == MAX_NESTED_IMPORTED_DECLARATIONS)
	{
	  complaint (_("DIE at %s has too many recursively imported "
		       "declarations"), sect_offset_str (d->sect_off));
	  return false;
	}

      if (attr != NULL)
	{
	  struct type *type;
	  if (d->tag == DW_TAG_variable)
	    {
	      /* This declaration is a C/C++ global variable alias.
		 Add a symbol for it whose type is the same as the
		 aliased variable's.  */
	      type = die_type (d, imported_cu);
	      struct symbol *sym = new_symbol (die, type, cu);
	      attr = dwarf2_attr (d, DW_AT_location, imported_cu);
	      sym->set_loc_class_index (LOC_UNRESOLVED);
	      if (attr != nullptr)
		var_decode_location (attr, sym, cu);
	      return true;
	    }
	  else
	    {
	      sect_offset sect_off = attr->get_ref_die_offset ();
	      type = get_die_type_at_offset (sect_off, cu->per_cu,
					     cu->per_objfile);
	      if (type != nullptr && type->code () == TYPE_CODE_NAMESPACE)
		{
		  /* This declaration is a global namespace alias. Add
		     a symbol for it whose type is the aliased
		     namespace.  */
		  new_symbol (die, type, cu);
		  return true;
		}
	    }
	}
    }
  return false;
}

/* Return the using directives repository (global or local?) to use in the
   current context for CU.

   For Ada, imported declarations can materialize renamings, which *may* be
   global.  However it is impossible (for now?) in DWARF to distinguish
   "external" imported declarations and "static" ones.  As all imported
   declarations seem to be static in all other languages, make them all CU-wide
   global only in Ada.  */

static struct using_direct **
using_directives (struct dwarf2_cu *cu)
{
  if (cu->lang () == language_ada
      && cu->get_builder ()->outermost_context_p ())
    return cu->get_builder ()->get_global_using_directives ();
  else
    return cu->get_builder ()->get_local_using_directives ();
}

/* Read the DW_ATTR_decl_line attribute for the given DIE in the
   given CU.  If the format is not recognized or the attribute is
   not present, set it to 0.  */

static unsigned int
read_decl_line (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *decl_line = dwarf2_attr (die, DW_AT_decl_line, cu);
  if (decl_line == nullptr)
    return 0;

  std::optional<ULONGEST> val = decl_line->unsigned_constant ();
  if (val.has_value ())
    {
      if (*val <= UINT_MAX)
	return (unsigned int) *val;
      complaint (_("Declared line for using directive is too large"));
    }
  return 0;
}

/* Read the import statement specified by the given die and record it.  */

static void
read_import_statement (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct attribute *import_attr;
  struct die_info *imported_die;
  struct dwarf2_cu *imported_cu;
  const char *imported_name;
  const char *imported_name_prefix;
  const char *canonical_name;
  const char *import_alias;
  const char *imported_declaration = NULL;
  const char *import_prefix;
  std::vector<const char *> excludes;

  import_attr = dwarf2_attr (die, DW_AT_import, cu);
  if (import_attr == NULL)
    {
      complaint (_("Tag '%s' has no DW_AT_import"),
		 dwarf_tag_name (die->tag));
      return;
    }

  imported_cu = cu;
  imported_die = follow_die_ref_or_sig (die, import_attr, &imported_cu);
  imported_name = dwarf2_name (imported_die, imported_cu);
  if (imported_name == NULL)
    {
      /* GCC bug: https://bugzilla.redhat.com/show_bug.cgi?id=506524

	The import in the following code:
	namespace A
	  {
	    typedef int B;
	  }

	int main ()
	  {
	    using A::B;
	    B b;
	    return b;
	  }

	...
	 <2><51>: Abbrev Number: 3 (DW_TAG_imported_declaration)
	    <52>   DW_AT_decl_file   : 1
	    <53>   DW_AT_decl_line   : 6
	    <54>   DW_AT_import      : <0x75>
	 <2><58>: Abbrev Number: 4 (DW_TAG_typedef)
	    <59>   DW_AT_name        : B
	    <5b>   DW_AT_decl_file   : 1
	    <5c>   DW_AT_decl_line   : 2
	    <5d>   DW_AT_type        : <0x6e>
	...
	 <1><75>: Abbrev Number: 7 (DW_TAG_base_type)
	    <76>   DW_AT_byte_size   : 4
	    <77>   DW_AT_encoding    : 5        (signed)

	imports the wrong die ( 0x75 instead of 0x58 ).
	This case will be ignored until the gcc bug is fixed.  */
      return;
    }

  /* Figure out the local name after import.  */
  import_alias = dwarf2_name (die, cu);

  /* Figure out where the statement is being imported to.  */
  import_prefix = determine_prefix (die, cu);

  /* Figure out what the scope of the imported die is and prepend it
     to the name of the imported die.  */
  imported_name_prefix = determine_prefix (imported_die, imported_cu);

  if (imported_die->tag != DW_TAG_namespace
      && imported_die->tag != DW_TAG_module)
    {
      imported_declaration = imported_name;
      canonical_name = imported_name_prefix;
    }
  else if (strlen (imported_name_prefix) > 0)
    {
      gdb::unique_xmalloc_ptr<char> temp;
      temp = typename_concat (imported_name_prefix, imported_name, 0, cu);
      canonical_name = obstack_strdup (&objfile->objfile_obstack, temp.get ());
    }
  else
    canonical_name = imported_name;

  if (die->tag == DW_TAG_imported_module && cu->lang () == language_fortran)
    for (die_info *child_die : die->children ())
      {
	/* DWARF-4: A Fortran use statement with a “rename list” may be
	   represented by an imported module entry with an import attribute
	   referring to the module and owned entries corresponding to those
	   entities that are renamed as part of being imported.  */

	if (child_die->tag != DW_TAG_imported_declaration)
	  {
	    complaint (_("child DW_TAG_imported_declaration expected "
			 "- DIE at %s [in module %s]"),
		       sect_offset_str (child_die->sect_off),
		       objfile_name (objfile));
	    continue;
	  }

	import_attr = dwarf2_attr (child_die, DW_AT_import, cu);
	if (import_attr == NULL)
	  {
	    complaint (_("Tag '%s' has no DW_AT_import"),
		       dwarf_tag_name (child_die->tag));
	    continue;
	  }

	imported_cu = cu;
	imported_die = follow_die_ref_or_sig (child_die, import_attr,
					      &imported_cu);
	imported_name = dwarf2_name (imported_die, imported_cu);
	if (imported_name == NULL)
	  {
	    complaint (_("child DW_TAG_imported_declaration has unknown "
			 "imported name - DIE at %s [in module %s]"),
		       sect_offset_str (child_die->sect_off),
		       objfile_name (objfile));
	    continue;
	  }

	excludes.push_back (imported_name);

	process_die (child_die, cu);
      }

  add_using_directive (using_directives (cu),
		       import_prefix,
		       canonical_name,
		       import_alias,
		       imported_declaration,
		       excludes,
		       read_decl_line (die, cu),
		       &objfile->objfile_obstack);
}

file_and_directory &
find_file_and_directory (struct die_info *die, struct dwarf2_cu *cu)
{
  if (cu->per_cu->fnd != nullptr)
    return *cu->per_cu->fnd;

  /* Find the filename.  Do not use dwarf2_name here, since the filename
     is not a source language identifier.  */
  file_and_directory res (dwarf2_string_attr (die, DW_AT_name, cu),
			  dwarf2_string_attr (die, DW_AT_comp_dir, cu));

  if (res.get_comp_dir () == nullptr
      && cu->producer_is_gcc_lt_4_3 ()
      && res.get_name () != nullptr
      && IS_ABSOLUTE_PATH (res.get_name ()))
    {
      res.set_comp_dir (gdb_ldirname (res.get_name ()));
      res.set_name (make_unique_xstrdup (lbasename (res.get_name ())));
    }

  cu->per_cu->fnd = std::make_unique<file_and_directory> (std::move (res));
  return *cu->per_cu->fnd;
}

/* Handle DW_AT_stmt_list for a compilation unit.
   DIE is the DW_TAG_compile_unit die for CU.
   COMP_DIR is the compilation directory.  LOWPC is passed to
   dwarf_decode_lines.  See dwarf_decode_lines comments about it.  */

static void
handle_DW_AT_stmt_list (struct die_info *die, struct dwarf2_cu *cu,
			const file_and_directory &fnd, unrelocated_addr lowpc,
			bool have_code) /* ARI: editCase function */
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct attribute *attr;
  hashval_t line_header_local_hash;
  void **slot;
  int decode_mapping;

  gdb_assert (! cu->per_cu->is_debug_types);

  attr = dwarf2_attr (die, DW_AT_stmt_list, cu);
  if (attr == NULL || !attr->form_is_unsigned ())
    return;

  sect_offset line_offset = (sect_offset) attr->as_unsigned ();

  /* The line header hash table is only created if needed (it exists to
     prevent redundant reading of the line table for partial_units).
     If we're given a partial_unit, we'll need it.  If we're given a
     compile_unit, then use the line header hash table if it's already
     created, but don't create one just yet.  */

  if (per_objfile->line_header_hash == NULL
      && die->tag == DW_TAG_partial_unit)
    {
      per_objfile->line_header_hash
	.reset (htab_create_alloc (127, line_header_hash_voidp,
				   line_header_eq_voidp,
				   htab_delete_entry<line_header>,
				   xcalloc, xfree));
    }

  line_header line_header_local (line_offset, cu->per_cu->is_dwz);
  line_header_local_hash = line_header_hash (&line_header_local);
  if (per_objfile->line_header_hash != NULL)
    {
      slot = htab_find_slot_with_hash (per_objfile->line_header_hash.get (),
				       &line_header_local,
				       line_header_local_hash, NO_INSERT);

      /* For DW_TAG_compile_unit we need info like symtab::linetable which
	 is not present in *SLOT (since if there is something in *SLOT then
	 it will be for a partial_unit).  */
      if (die->tag == DW_TAG_partial_unit && slot != NULL)
	{
	  gdb_assert (*slot != NULL);
	  cu->line_header = (struct line_header *) *slot;
	  return;
	}
    }

  /* dwarf_decode_line_header does not yet provide sufficient information.
     We always have to call also dwarf_decode_lines for it.  */
  line_header_up lh = dwarf_decode_line_header (line_offset, cu,
						fnd.get_comp_dir ());
  if (lh == NULL)
    return;

  cu->line_header = lh.release ();
  cu->line_header_die_owner = die;

  if (per_objfile->line_header_hash == NULL)
    slot = NULL;
  else
    {
      slot = htab_find_slot_with_hash (per_objfile->line_header_hash.get (),
				       &line_header_local,
				       line_header_local_hash, INSERT);
      gdb_assert (slot != NULL);
    }
  if (slot != NULL && *slot == NULL)
    {
      /* This newly decoded line number information unit will be owned
	 by line_header_hash hash table.  */
      *slot = cu->line_header;
      cu->line_header_die_owner = NULL;
    }
  else
    {
      /* We cannot free any current entry in (*slot) as that struct line_header
	 may be already used by multiple CUs.  Create only temporary decoded
	 line_header for this CU - it may happen at most once for each line
	 number information unit.  And if we're not using line_header_hash
	 then this is what we want as well.  */
      gdb_assert (die->tag != DW_TAG_partial_unit);
    }
  decode_mapping = (die->tag != DW_TAG_partial_unit);
  /* The have_code check is here because, if LOWPC and HIGHPC are both 0x0,
     then there won't be any interesting code in the CU, but a check later on
     (in lnp_state_machine::check_line_address) will fail to properly exclude
     an entry that was removed via --gc-sections.  */
  dwarf_decode_lines (cu->line_header, cu, lowpc, decode_mapping && have_code);
}

/* Process DW_TAG_compile_unit or DW_TAG_partial_unit.  */

static void
read_file_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  CORE_ADDR lowpc;
  struct attribute *attr;

  unrelocated_addr unrel_low, unrel_high;
  get_scope_pc_bounds (die, &unrel_low, &unrel_high, cu);

  /* If we didn't find a lowpc, set it to highpc to avoid complaints
     from finish_block.  */
  if (unrel_low == ((unrelocated_addr) -1))
    unrel_low = unrel_high;
  lowpc = per_objfile->relocate (unrel_low);

  file_and_directory &fnd = find_file_and_directory (die, cu);

  /* GAS supports generating dwarf-5 info starting version 2.35.  Versions
     2.35-2.37 generate an incorrect CU name attribute: it's relative,
     implicitly prefixing it with the compilation dir.  Work around this by
     prefixing it with the source dir instead.  */
  if (cu->header.version == 5 && !IS_ABSOLUTE_PATH (fnd.get_name ())
      && cu->producer_is_gas_lt_2_38 ())
    {
      attr = dwarf2_attr (die, DW_AT_stmt_list, cu);
      if (attr != nullptr && attr->form_is_unsigned ())
	{
	  sect_offset line_offset = (sect_offset) attr->as_unsigned ();
	  line_header_up lh = dwarf_decode_line_header (line_offset, cu,
							fnd.get_comp_dir ());
	  if (lh->version == 5 && lh->include_dir_at (1) != nullptr)
	    {
	      std::string dir = lh->include_dir_at (1);
	      fnd.set_comp_dir (std::move (dir));
	    }
	}
    }

  cu->start_compunit_symtab (fnd.get_name (), fnd.intern_comp_dir (objfile),
			     lowpc);

  gdb_assert (per_objfile->sym_cu == nullptr);
  scoped_restore restore_sym_cu
    = make_scoped_restore (&per_objfile->sym_cu, cu);

  /* Decode line number information if present.  We do this before
     processing child DIEs, so that the line header table is available
     for DW_AT_decl_file.  */
  handle_DW_AT_stmt_list (die, cu, fnd, unrel_low, unrel_low != unrel_high);

  /* Process all dies in compilation unit.  */
  for (die_info *child_die : die->children ())
    process_die (child_die, cu);

  per_objfile->sym_cu = nullptr;

  /* Decode macro information, if present.  Dwarf 2 macro information
     refers to information in the line number info statement program
     header, so we can only read it if we've read the header
     successfully.  */
  attr = dwarf2_attr (die, DW_AT_macros, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_GNU_macros, cu);
  if (attr != nullptr && attr->form_is_unsigned () && cu->line_header)
    {
      if (dwarf2_attr (die, DW_AT_macro_info, cu))
	complaint (_("CU refers to both DW_AT_macros and DW_AT_macro_info"));

      dwarf_decode_macros (cu, attr->as_unsigned (), 1);
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_macro_info, cu);
      if (attr != nullptr && attr->form_is_unsigned () && cu->line_header)
	{
	  unsigned int macro_offset = attr->as_unsigned ();

	  dwarf_decode_macros (cu, macro_offset, 0);
	}
    }
}

/* See cu.h.

   This function is defined in this file (instead of cu.c) because it needs
   to see the definition of struct dwo_unit.  */

const dwarf2_section_info &
dwarf2_cu::section () const
{
  if (this->dwo_unit != nullptr)
    return *this->dwo_unit->section;
  else
    return *this->per_cu->section;
}

void
dwarf2_cu::setup_type_unit_groups (struct die_info *die)
{
  int first_time;
  struct attribute *attr;
  unsigned int i;
  struct signatured_type *sig_type;

  gdb_assert (per_cu->is_debug_types);
  sig_type = (struct signatured_type *) per_cu;

  attr = dwarf2_attr (die, DW_AT_stmt_list, this);

  /* If we're using .gdb_index (includes -readnow) then
     per_cu->type_unit_group may not have been set up yet.  */
  if (!sig_type->type_unit_group_key.has_value ())
    sig_type->type_unit_group_key = get_type_unit_group_key (this, attr);

  /* If we've already processed this stmt_list there's no real need to
     do it again, we could fake it and just recreate the part we need
     (file name,index -> symtab mapping).  If data shows this optimization
     is useful we can do it then.  */
  type_unit_group_unshareable *tug_unshare
    = per_objfile->get_type_unit_group_unshareable (*sig_type->type_unit_group_key);
  first_time = tug_unshare->compunit_symtab == NULL;

  /* We have to handle the case of both a missing DW_AT_stmt_list or bad
     debug info.  */
  line_header_up lh;
  if (attr != NULL && attr->form_is_unsigned ())
    {
      sect_offset line_offset = (sect_offset) attr->as_unsigned ();
      lh = dwarf_decode_line_header (line_offset, this, nullptr);
    }
  if (lh == NULL)
    {
      if (first_time)
	start_compunit_symtab ("", NULL, 0);
      else
	{
	  gdb_assert (tug_unshare->symtabs == NULL);
	  gdb_assert (m_builder == nullptr);
	  struct compunit_symtab *cust = tug_unshare->compunit_symtab;
	  m_builder = std::make_unique<buildsym_compunit>
			   (cust->objfile (), "",
			    cust->dirname (),
			    cust->language (),
			    0, cust);
	  list_in_scope = get_builder ()->get_file_symbols ();
	}
      return;
    }

  line_header = lh.release ();
  line_header_die_owner = die;

  if (first_time)
    {
      struct compunit_symtab *cust = start_compunit_symtab ("", NULL, 0);

      /* Note: We don't assign tu_group->compunit_symtab yet because we're
	 still initializing it, and our caller (a few levels up)
	 process_full_type_unit still needs to know if this is the first
	 time.  */

      tug_unshare->symtabs
	= XOBNEWVEC (&cust->objfile ()->objfile_obstack,
		     struct symtab *, line_header->file_names_size ());

      auto &file_names = line_header->file_names ();
      for (i = 0; i < file_names.size (); ++i)
	{
	  file_entry &fe = file_names[i];
	  dwarf2_start_subfile (this, fe, *line_header);
	  buildsym_compunit *b = get_builder ();
	  subfile *sf = b->get_current_subfile ();

	  if (sf->symtab == nullptr)
	    {
	      /* NOTE: start_subfile will recognize when it's been
		 passed a file it has already seen.  So we can't
		 assume there's a simple mapping from
		 cu->line_header->file_names to subfiles, plus
		 cu->line_header->file_names may contain dups.  */
	      const char *name = sf->name.c_str ();
	      const char *name_for_id = sf->name_for_id.c_str ();
	      sf->symtab = allocate_symtab (cust, name, name_for_id);
	    }

	  fe.symtab = b->get_current_subfile ()->symtab;
	  tug_unshare->symtabs[i] = fe.symtab;
	}
    }
  else
    {
      gdb_assert (m_builder == nullptr);
      struct compunit_symtab *cust = tug_unshare->compunit_symtab;
      m_builder = std::make_unique<buildsym_compunit>
		       (cust->objfile (), "",
			cust->dirname (),
			cust->language (),
			0, cust);
      list_in_scope = get_builder ()->get_file_symbols ();

      auto &file_names = line_header->file_names ();
      for (i = 0; i < file_names.size (); ++i)
	{
	  file_entry &fe = file_names[i];
	  fe.symtab = tug_unshare->symtabs[i];
	}
    }

  /* The main symtab is allocated last.  Type units don't have DW_AT_name
     so they don't have a "real" (so to speak) symtab anyway.
     There is later code that will assign the main symtab to all symbols
     that don't have one.  We need to handle the case of a symbol with a
     missing symtab (DW_AT_decl_file) anyway.  */
}

/* Process DW_TAG_type_unit.
   For TUs we want to skip the first top level sibling if it's not the
   actual type being defined by this TU.  In this case the first top
   level sibling is there to provide context only.  */

static void
read_type_unit_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  /* Initialize (or reinitialize) the machinery for building symtabs.
     We do this before processing child DIEs, so that the line header table
     is available for DW_AT_decl_file.  */
  cu->setup_type_unit_groups (die);

  for (die_info *child_die : die->children ())
    process_die (child_die, cu);
}

/* DWO/DWP files.

   http://gcc.gnu.org/wiki/DebugFission
   http://gcc.gnu.org/wiki/DebugFissionDWP

   To simplify handling of both DWO files ("object" files with the DWARF info)
   and DWP files (a file with the DWOs packaged up into one file), we treat
   DWP files as having a collection of virtual DWO files.  */

/* Lookup DWO file DWO_NAME.  */

static dwo_file *
lookup_dwo_file (dwarf2_per_bfd *per_bfd, const char *dwo_name,
		 const char *comp_dir)
{
#if CXX_STD_THREAD
  std::lock_guard<std::mutex> guard (per_bfd->dwo_files_lock);
#endif

  auto it = per_bfd->dwo_files.find (dwo_file_search {dwo_name, comp_dir});

  return it != per_bfd->dwo_files.end () ? it->get() : nullptr;
}

/* Add DWO_FILE to the per-BFD DWO file hash table.

   Return the dwo_file actually kept in the hash table.

   If another thread raced with this one, opening the exact same DWO file and
   inserting it first in the hash table, then keep that other thread's copy
   and DWO_FILE gets freed.  */

static dwo_file *
add_dwo_file (dwarf2_per_bfd *per_bfd, dwo_file_up dwo_file)
{
#if CXX_STD_THREAD
  std::lock_guard<std::mutex> lock (per_bfd->dwo_files_lock);
#endif

  return per_bfd->dwo_files.emplace (std::move (dwo_file)).first->get ();
}

void
cutu_reader::create_dwo_unit_hash_tables (dwo_file &dwo_file,
					  dwarf2_cu &skeleton_cu,
					  dwarf2_section_info &section,
					  ruh_kind section_kind)
{
  dwarf2_per_objfile &per_objfile = *skeleton_cu.per_objfile;
  dwarf2_per_bfd &per_bfd = *per_objfile.per_bfd;

  const gdb_byte *info_ptr = section.buffer;

  if (info_ptr == NULL)
    return;

  dwarf_read_debug_printf ("Reading %s for %s:", section.get_name (),
			   section.get_file_name ());

  const gdb_byte *end_ptr = info_ptr + section.size;

  while (info_ptr < end_ptr)
    {
      sect_offset sect_off = (sect_offset) (info_ptr - section.buffer);
      unit_head header;
      dwarf2_section_info *abbrev_section = &dwo_file.sections.abbrev;
      const gdb_byte *info_ptr_post_header
	= read_and_check_unit_head (&header, &section, abbrev_section,
				    info_ptr, section_kind);

      unsigned int length = header.get_length_with_initial ();
      info_ptr += length;

      /* Skip dummy units.  */
      if (info_ptr_post_header >= info_ptr
	  || peek_abbrev_code (section.get_bfd_owner (),
			       info_ptr_post_header) == 0)
	continue;

      if (header.unit_type != DW_UT_compile
	  && header.unit_type != DW_UT_split_compile
	  && header.unit_type != DW_UT_type
	  && header.unit_type != DW_UT_split_type)
	continue;

      ULONGEST signature;

      /* For type units (all DWARF versions) and DWARF 5 compile units, the
	 signature/DWO ID is already available in the header.  For compile
	 units in DWARF < 5, we need to read the DW_AT_GNU_dwo_id attribute
	 from the top-level DIE.

	 For DWARF < 5 compile units, the unit type will be set to DW_UT_compile
	 by read_and_check_comp_unit_head.  */
      if (header.version < 5 && header.unit_type == DW_UT_compile)
	{
	  /* The length of the CU is not necessary.  */
	  dwarf2_per_cu per_cu (&per_bfd, &section, sect_off, length,
				false /* is_dwz */);
	  cutu_reader reader (per_cu, per_objfile, language_minimal,
			      skeleton_cu, dwo_file);

	  std::optional<ULONGEST> opt_signature
	    = lookup_dwo_id (reader.cu (), reader.top_level_die ());

	  if (!opt_signature.has_value ())
	    {
	      complaint (_ (DWARF_ERROR_PREFIX
			    "debug entry at offset %s is missing its dwo_id"
			    " [in module %s]"),
			 sect_offset_str (sect_off),
			 dwo_file.dwo_name.c_str ());
	      continue;
	    }

	  signature = *opt_signature;
	}
      else
	signature = header.signature;

      auto dwo_unit = std::make_unique<struct dwo_unit> ();

      /* Set the fields common to compile and type units.  */
      dwo_unit->dwo_file = &dwo_file;
      dwo_unit->signature = signature;
      dwo_unit->section = &section;
      dwo_unit->sect_off = sect_off;
      dwo_unit->length = length;

      switch (header.unit_type)
	{
	case DW_UT_compile:
	case DW_UT_split_compile:
	{
	  dwarf_read_debug_printf ("  compile unit at offset %s, dwo_id %s",
				   sect_offset_str (sect_off),
				   hex_string (dwo_unit->signature));

	  auto [it, inserted] = dwo_file.cus.emplace (std::move (dwo_unit));
	  if (!inserted)
	    complaint (_("debug cu entry at offset %s is duplicate to"
			 " the entry at offset %s, signature %s"),
		       sect_offset_str (sect_off),
		       sect_offset_str ((*it)->sect_off),
		       hex_string (dwo_unit->signature));
	  break;
	}

	case DW_UT_type:
	case DW_UT_split_type:
	{
	  dwo_unit->type_offset_in_tu = header.type_offset_in_tu;

	  dwarf_read_debug_printf ("  type unit at offset %s, signature %s",
				   sect_offset_str (sect_off),
				   hex_string (dwo_unit->signature));

	  auto [it, inserted] = dwo_file.tus.emplace (std::move (dwo_unit));
	  if (!inserted)
	    complaint (_("debug type entry at offset %s is duplicate to"
			 " the entry at offset %s, signature %s"),
		       sect_offset_str (sect_off),
		       sect_offset_str ((*it)->sect_off),
		       hex_string (header.signature));
	  break;
	}
	}
    }
}

/* DWP file .debug_{cu,tu}_index section format:
   [ref: http://gcc.gnu.org/wiki/DebugFissionDWP]
   [ref: http://dwarfstd.org/doc/DWARF5.pdf, sect 7.3.5 "DWARF Package Files"]

   DWP Versions 1 & 2 are older, pre-standard format versions.  The first
   officially standard DWP format was published with DWARF v5 and is called
   Version 5.  There are no versions 3 or 4.

   DWP Version 1:

   Both index sections have the same format, and serve to map a 64-bit
   signature to a set of section numbers.  Each section begins with a header,
   followed by a hash table of 64-bit signatures, a parallel table of 32-bit
   indexes, and a pool of 32-bit section numbers.  The index sections will be
   aligned at 8-byte boundaries in the file.

   The index section header consists of:

    V, 32 bit version number
    -, 32 bits unused
    N, 32 bit number of compilation units or type units in the index
    M, 32 bit number of slots in the hash table

   Numbers are recorded using the byte order of the application binary.

   The hash table begins at offset 16 in the section, and consists of an array
   of M 64-bit slots.  Each slot contains a 64-bit signature (using the byte
   order of the application binary).  Unused slots in the hash table are 0.
   (We rely on the extreme unlikeliness of a signature being exactly 0.)

   The parallel table begins immediately after the hash table
   (at offset 16 + 8 * M from the beginning of the section), and consists of an
   array of 32-bit indexes (using the byte order of the application binary),
   corresponding 1-1 with slots in the hash table.  Each entry in the parallel
   table contains a 32-bit index into the pool of section numbers.  For unused
   hash table slots, the corresponding entry in the parallel table will be 0.

   The pool of section numbers begins immediately following the hash table
   (at offset 16 + 12 * M from the beginning of the section).  The pool of
   section numbers consists of an array of 32-bit words (using the byte order
   of the application binary).  Each item in the array is indexed starting
   from 0.  The hash table entry provides the index of the first section
   number in the set.  Additional section numbers in the set follow, and the
   set is terminated by a 0 entry (section number 0 is not used in ELF).

   In each set of section numbers, the .debug_info.dwo or .debug_types.dwo
   section must be the first entry in the set, and the .debug_abbrev.dwo must
   be the second entry. Other members of the set may follow in any order.

   ---

   DWP Versions 2 and 5:

   DWP Versions 2 and 5 combine all the .debug_info, etc. sections into one,
   and the entries in the index tables are now offsets into these sections.
   CU offsets begin at 0.  TU offsets begin at the size of the .debug_info
   section.

   Index Section Contents:
    Header
    Hash Table of Signatures   dwp_hash_table.hash_table
    Parallel Table of Indices  dwp_hash_table.unit_table
    Table of Section Offsets   dwp_hash_table.{v2|v5}.{section_ids,offsets}
    Table of Section Sizes     dwp_hash_table.{v2|v5}.sizes

   The index section header consists of:

    V, 32 bit version number
    L, 32 bit number of columns in the table of section offsets
    N, 32 bit number of compilation units or type units in the index
    M, 32 bit number of slots in the hash table

   Numbers are recorded using the byte order of the application binary.

   The hash table has the same format as version 1.
   The parallel table of indices has the same format as version 1,
   except that the entries are origin-1 indices into the table of sections
   offsets and the table of section sizes.

   The table of offsets begins immediately following the parallel table
   (at offset 16 + 12 * M from the beginning of the section).  The table is
   a two-dimensional array of 32-bit words (using the byte order of the
   application binary), with L columns and N+1 rows, in row-major order.
   Each row in the array is indexed starting from 0.  The first row provides
   a key to the remaining rows: each column in this row provides an identifier
   for a debug section, and the offsets in the same column of subsequent rows
   refer to that section.  The section identifiers for Version 2 are:

    DW_SECT_INFO         1  .debug_info.dwo
    DW_SECT_TYPES        2  .debug_types.dwo
    DW_SECT_ABBREV       3  .debug_abbrev.dwo
    DW_SECT_LINE         4  .debug_line.dwo
    DW_SECT_LOC          5  .debug_loc.dwo
    DW_SECT_STR_OFFSETS  6  .debug_str_offsets.dwo
    DW_SECT_MACINFO      7  .debug_macinfo.dwo
    DW_SECT_MACRO        8  .debug_macro.dwo

   The section identifiers for Version 5 are:

    DW_SECT_INFO_V5         1  .debug_info.dwo
    DW_SECT_RESERVED_V5     2  --
    DW_SECT_ABBREV_V5       3  .debug_abbrev.dwo
    DW_SECT_LINE_V5         4  .debug_line.dwo
    DW_SECT_LOCLISTS_V5     5  .debug_loclists.dwo
    DW_SECT_STR_OFFSETS_V5  6  .debug_str_offsets.dwo
    DW_SECT_MACRO_V5        7  .debug_macro.dwo
    DW_SECT_RNGLISTS_V5     8  .debug_rnglists.dwo

   The offsets provided by the CU and TU index sections are the base offsets
   for the contributions made by each CU or TU to the corresponding section
   in the package file.  Each CU and TU header contains an abbrev_offset
   field, used to find the abbreviations table for that CU or TU within the
   contribution to the .debug_abbrev.dwo section for that CU or TU, and should
   be interpreted as relative to the base offset given in the index section.
   Likewise, offsets into .debug_line.dwo from DW_AT_stmt_list attributes
   should be interpreted as relative to the base offset for .debug_line.dwo,
   and offsets into other debug sections obtained from DWARF attributes should
   also be interpreted as relative to the corresponding base offset.

   The table of sizes begins immediately following the table of offsets.
   Like the table of offsets, it is a two-dimensional array of 32-bit words,
   with L columns and N rows, in row-major order.  Each row in the array is
   indexed starting from 1 (row 0 is shared by the two tables).

   ---

   Hash table lookup is handled the same in version 1 and 2:

   We assume that N and M will not exceed 2^32 - 1.
   The size of the hash table, M, must be 2^k such that 2^k > 3*N/2.

   Given a 64-bit compilation unit signature or a type signature S, an entry
   in the hash table is located as follows:

   1) Calculate a primary hash H = S & MASK(k), where MASK(k) is a mask with
      the low-order k bits all set to 1.

   2) Calculate a secondary hash H' = (((S >> 32) & MASK(k)) | 1).

   3) If the hash table entry at index H matches the signature, use that
      entry.  If the hash table entry at index H is unused (all zeroes),
      terminate the search: the signature is not present in the table.

   4) Let H = (H + H') modulo M. Repeat at Step 3.

   Because M > N and H' and M are relatively prime, the search is guaranteed
   to stop at an unused slot or find the match.  */

/* Create a hash table to map DWO IDs to their CU/TU entry in
   .debug_{info,types}.dwo in DWP_FILE.
   Returns NULL if there isn't one.
   Note: This function processes DWP files only, not DWO files.  */

static struct dwp_hash_table *
create_dwp_hash_table (dwarf2_per_bfd *per_bfd, struct dwp_file *dwp_file,
		       dwarf2_section_info &index)
{
  bfd *dbfd = dwp_file->dbfd.get ();
  uint32_t version, nr_columns, nr_units, nr_slots;
  struct dwp_hash_table *htab;

  if (index.empty ())
    return NULL;

  const gdb_byte *index_ptr = index.buffer;
  const gdb_byte *index_end = index_ptr + index.size;

  /* For Version 5, the version is really 2 bytes of data & 2 bytes of padding.
     For now it's safe to just read 4 bytes (particularly as it's difficult to
     tell if you're dealing with Version 5 before you've read the version).   */
  version = read_4_bytes (dbfd, index_ptr);
  index_ptr += 4;
  if (version == 2 || version == 5)
    nr_columns = read_4_bytes (dbfd, index_ptr);
  else
    nr_columns = 0;
  index_ptr += 4;
  nr_units = read_4_bytes (dbfd, index_ptr);
  index_ptr += 4;
  nr_slots = read_4_bytes (dbfd, index_ptr);
  index_ptr += 4;

  if (version != 1 && version != 2 && version != 5)
    {
      error (_(DWARF_ERROR_PREFIX
	       "unsupported DWP file version (%s) [in module %s]"),
	     pulongest (version), dwp_file->name);
    }
  if (nr_slots != (nr_slots & -nr_slots))
    {
      error (_(DWARF_ERROR_PREFIX
	       "number of slots in DWP hash table (%s) is not power of 2"
	       " [in module %s]"),
	     pulongest (nr_slots), dwp_file->name);
    }

  htab = OBSTACK_ZALLOC (&per_bfd->obstack, struct dwp_hash_table);
  htab->version = version;
  htab->nr_columns = nr_columns;
  htab->nr_units = nr_units;
  htab->nr_slots = nr_slots;
  htab->hash_table = index_ptr;
  htab->unit_table = htab->hash_table + sizeof (uint64_t) * nr_slots;

  /* Exit early if the table is empty.  */
  if (nr_slots == 0 || nr_units == 0
      || (version == 2 && nr_columns == 0)
      || (version == 5 && nr_columns == 0))
    {
      /* All must be zero.  */
      if (nr_slots != 0 || nr_units != 0
	  || (version == 2 && nr_columns != 0)
	  || (version == 5 && nr_columns != 0))
	{
	  complaint (_("Empty DWP but nr_slots,nr_units,nr_columns not"
		       " all zero [in modules %s]"),
		     dwp_file->name);
	}
      return htab;
    }

  if (version == 1)
    {
      htab->section_pool.v1.indices =
	htab->unit_table + sizeof (uint32_t) * nr_slots;
      /* It's harder to decide whether the section is too small in v1.
	 V1 is deprecated anyway so we punt.  */
    }
  else if (version == 2)
    {
      const gdb_byte *ids_ptr = htab->unit_table + sizeof (uint32_t) * nr_slots;
      int *ids = htab->section_pool.v2.section_ids;
      size_t sizeof_ids = sizeof (htab->section_pool.v2.section_ids);
      /* Reverse map for error checking.  */
      int ids_seen[DW_SECT_MAX + 1];
      int i;

      if (nr_columns < 2)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, too few columns in section table"
		   " [in module %s]"),
		 dwp_file->name);
	}
      if (nr_columns > MAX_NR_V2_DWO_SECTIONS)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, too many columns in section table"
		   " [in module %s]"),
		 dwp_file->name);
	}
      memset (ids, 255, sizeof_ids);
      memset (ids_seen, 255, sizeof (ids_seen));
      for (i = 0; i < nr_columns; ++i)
	{
	  int id = read_4_bytes (dbfd, ids_ptr + i * sizeof (uint32_t));

	  if (id < DW_SECT_MIN || id > DW_SECT_MAX)
	    {
	      error (_(DWARF_ERROR_PREFIX
		       "bad DWP hash table, bad section id %d in section table"
		       " [in module %s]"),
		     id, dwp_file->name);
	    }
	  if (ids_seen[id] != -1)
	    {
	      error (_(DWARF_ERROR_PREFIX
		       "bad DWP hash table, duplicate section"
		       " id %d in section table [in module %s]"),
		     id, dwp_file->name);
	    }
	  ids_seen[id] = i;
	  ids[i] = id;
	}
      /* Must have exactly one info or types section.  */
      if (((ids_seen[DW_SECT_INFO] != -1)
	   + (ids_seen[DW_SECT_TYPES] != -1))
	  != 1)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, missing/duplicate"
		   " DWO info/types section [in module %s]"),
		 dwp_file->name);
	}
      /* Must have an abbrev section.  */
      if (ids_seen[DW_SECT_ABBREV] == -1)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, missing DWO abbrev section"
		   " [in module %s]"),
		 dwp_file->name);
	}
      htab->section_pool.v2.offsets = ids_ptr + sizeof (uint32_t) * nr_columns;
      htab->section_pool.v2.sizes =
	htab->section_pool.v2.offsets + (sizeof (uint32_t)
					 * nr_units * nr_columns);
      if ((htab->section_pool.v2.sizes + (sizeof (uint32_t)
					  * nr_units * nr_columns))
	  > index_end)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "DWP index section is corrupt (too small) [in module %s]"),
		 dwp_file->name);
	}
    }
  else /* version == 5  */
    {
      const gdb_byte *ids_ptr = htab->unit_table + sizeof (uint32_t) * nr_slots;
      int *ids = htab->section_pool.v5.section_ids;
      size_t sizeof_ids = sizeof (htab->section_pool.v5.section_ids);
      /* Reverse map for error checking.  */
      int ids_seen[DW_SECT_MAX_V5 + 1];

      if (nr_columns < 2)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, too few columns in section table"
		   " [in module %s]"),
		 dwp_file->name);
	}
      if (nr_columns > MAX_NR_V5_DWO_SECTIONS)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, too many columns in section table"
		   " [in module %s]"),
		 dwp_file->name);
	}
      memset (ids, 255, sizeof_ids);
      memset (ids_seen, 255, sizeof (ids_seen));
      for (int i = 0; i < nr_columns; ++i)
	{
	  int id = read_4_bytes (dbfd, ids_ptr + i * sizeof (uint32_t));

	  if (id < DW_SECT_MIN || id > DW_SECT_MAX_V5)
	    {
	      error (_(DWARF_ERROR_PREFIX
		       "bad DWP hash table, bad section id %d in section table"
		       " [in module %s]"),
		     id, dwp_file->name);
	    }
	  if (ids_seen[id] != -1)
	    {
	      error (_(DWARF_ERROR_PREFIX
		       "bad DWP hash table, duplicate section"
		       " id %d in section table [in module %s]"),
		     id, dwp_file->name);
	    }
	  ids_seen[id] = i;
	  ids[i] = id;
	}
      /* Must have seen an info section.  */
      if (ids_seen[DW_SECT_INFO_V5] == -1)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, missing/duplicate"
		   " DWO info/types section [in module %s]"),
		 dwp_file->name);
	}
      /* Must have an abbrev section.  */
      if (ids_seen[DW_SECT_ABBREV_V5] == -1)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, missing DWO abbrev section"
		   " [in module %s]"),
		 dwp_file->name);
	}
      htab->section_pool.v5.offsets = ids_ptr + sizeof (uint32_t) * nr_columns;
      htab->section_pool.v5.sizes
	= htab->section_pool.v5.offsets + (sizeof (uint32_t)
					 * nr_units * nr_columns);
      if ((htab->section_pool.v5.sizes + (sizeof (uint32_t)
					  * nr_units * nr_columns))
	  > index_end)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "DWP index section is corrupt (too small) [in module %s]"),
		 dwp_file->name);
	}
    }

  return htab;
}

/* Update SECTIONS with the data from SECTP.

   This function is like the other "locate" section routines, but in
   this context the sections to read comes from the DWP V1 hash table,
   not the full ELF section table.

   The result is non-zero for success, or zero if an error was found.  */

static int
locate_v1_virtual_dwo_sections (asection *sectp,
				struct virtual_v1_dwo_sections *sections)
{
  const struct dwop_section_names *names = &dwop_section_names;

  if (names->abbrev_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->abbrev.s.section != NULL)
	return 0;
      sections->abbrev.s.section = sectp;
      sections->abbrev.size = bfd_section_size (sectp);
    }
  else if (names->info_dwo.matches (sectp->name)
	   || names->types_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->info_or_types.s.section != NULL)
	return 0;
      sections->info_or_types.s.section = sectp;
      sections->info_or_types.size = bfd_section_size (sectp);
    }
  else if (names->line_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->line.s.section != NULL)
	return 0;
      sections->line.s.section = sectp;
      sections->line.size = bfd_section_size (sectp);
    }
  else if (names->loc_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->loc.s.section != NULL)
	return 0;
      sections->loc.s.section = sectp;
      sections->loc.size = bfd_section_size (sectp);
    }
  else if (names->macinfo_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->macinfo.s.section != NULL)
	return 0;
      sections->macinfo.s.section = sectp;
      sections->macinfo.size = bfd_section_size (sectp);
    }
  else if (names->macro_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->macro.s.section != NULL)
	return 0;
      sections->macro.s.section = sectp;
      sections->macro.size = bfd_section_size (sectp);
    }
  else if (names->str_offsets_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->str_offsets.s.section != NULL)
	return 0;
      sections->str_offsets.s.section = sectp;
      sections->str_offsets.size = bfd_section_size (sectp);
    }
  else
    {
      /* No other kind of section is valid.  */
      return 0;
    }

  return 1;
}

/* Create a dwo_unit object for the DWO unit with signature SIGNATURE.
   UNIT_INDEX is the index of the DWO unit in the DWP hash table.
   COMP_DIR is the DW_AT_comp_dir attribute of the referencing CU.
   This is for DWP version 1 files.  */

static dwo_unit_up
create_dwo_unit_in_dwp_v1 (dwarf2_per_bfd *per_bfd,
			   struct dwp_file *dwp_file,
			   uint32_t unit_index,
			   const char *comp_dir,
			   ULONGEST signature, int is_debug_types)
{
  const struct dwp_hash_table *dwp_htab =
    is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd.get ();
  const char *kind = is_debug_types ? "TU" : "CU";
  struct virtual_v1_dwo_sections sections;
  int i;

  gdb_assert (dwp_file->version == 1);

  dwarf_read_debug_printf ("Reading %s %s/%s in DWP V1 file: %s",
			   kind, pulongest (unit_index), hex_string (signature),
			   dwp_file->name);

  /* Fetch the sections of this DWO unit.
     Put a limit on the number of sections we look for so that bad data
     doesn't cause us to loop forever.  */

#define MAX_NR_V1_DWO_SECTIONS \
  (1 /* .debug_info or .debug_types */ \
   + 1 /* .debug_abbrev */ \
   + 1 /* .debug_line */ \
   + 1 /* .debug_loc */ \
   + 1 /* .debug_str_offsets */ \
   + 1 /* .debug_macro or .debug_macinfo */ \
   + 1 /* trailing zero */)

  memset (&sections, 0, sizeof (sections));

  for (i = 0; i < MAX_NR_V1_DWO_SECTIONS; ++i)
    {
      asection *sectp;
      uint32_t section_nr =
	read_4_bytes (dbfd,
		      dwp_htab->section_pool.v1.indices
		      + (unit_index + i) * sizeof (uint32_t));

      if (section_nr == 0)
	break;
      if (section_nr >= dwp_file->num_sections)
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, section number too large"
		   " [in module %s]"),
		 dwp_file->name);
	}

      sectp = dwp_file->elf_sections[section_nr];
      if (! locate_v1_virtual_dwo_sections (sectp, &sections))
	{
	  error (_(DWARF_ERROR_PREFIX
		   "bad DWP hash table, invalid section found [in module %s]"),
		 dwp_file->name);
	}
    }

  if (i < 2
      || sections.info_or_types.empty ()
      || sections.abbrev.empty ())
    {
      error (_(DWARF_ERROR_PREFIX
	       "bad DWP hash table, missing DWO sections [in module %s]"),
	     dwp_file->name);
    }
  if (i == MAX_NR_V1_DWO_SECTIONS)
    {
      error (_(DWARF_ERROR_PREFIX
	       "bad DWP hash table, too many DWO sections [in module %s]"),
	     dwp_file->name);
    }

  /* It's easier for the rest of the code if we fake a struct dwo_file and
     have dwo_unit "live" in that.  At least for now.

     The DWP file can be made up of a random collection of CUs and TUs.
     However, for each CU + set of TUs that came from the same original DWO
     file, we can combine them back into a virtual DWO file to save space
     (fewer struct dwo_file objects to allocate).  Remember that for really
     large apps there can be on the order of 8K CUs and 200K TUs, or more.  */

  std::string virtual_dwo_name =
    string_printf ("virtual-dwo/%d-%d-%d-%d",
		   sections.abbrev.get_id (),
		   sections.line.get_id (),
		   sections.loc.get_id (),
		   sections.str_offsets.get_id ());

  /* Can we use an existing virtual DWO file?  */
  dwo_file *dwo_file
    = lookup_dwo_file (per_bfd, virtual_dwo_name.c_str (), comp_dir);

  /* Create one if necessary.  */
  if (dwo_file == nullptr)
    {
      dwarf_read_debug_printf ("Creating virtual DWO: %s",
			       virtual_dwo_name.c_str ());

      dwo_file_up new_dwo_file = std::make_unique<struct dwo_file> ();
      new_dwo_file->dwo_name = std::move (virtual_dwo_name);
      new_dwo_file->comp_dir = comp_dir;
      new_dwo_file->sections.abbrev = sections.abbrev;
      new_dwo_file->sections.line = sections.line;
      new_dwo_file->sections.loc = sections.loc;
      new_dwo_file->sections.macinfo = sections.macinfo;
      new_dwo_file->sections.macro = sections.macro;
      new_dwo_file->sections.str_offsets = sections.str_offsets;

      /* The "str" section is global to the entire DWP file.  */
      new_dwo_file->sections.str = dwp_file->sections.str;

      /* The info or types section is assigned below to dwo_unit,
	 there's no need to record it in dwo_file.
	 Also, we can't simply record type sections in dwo_file because
	 we record a pointer into the vector in dwo_unit.  As we collect more
	 types we'll grow the vector and eventually have to reallocate space
	 for it, invalidating all copies of pointers into the previous
	 contents.  */
      dwo_file = add_dwo_file (per_bfd, std::move (new_dwo_file));
    }
  else
    dwarf_read_debug_printf ("Using existing virtual DWO: %s",
			     virtual_dwo_name.c_str ());

  auto dwo_unit = std::make_unique<struct dwo_unit> ();
  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = signature;
  dwo_unit->section_holder = std::make_unique<dwarf2_section_info> ();
  dwo_unit->section = dwo_unit->section_holder.get ();
  *dwo_unit->section = sections.info_or_types;
  /* dwo_unit->{offset,length,type_offset_in_tu} are set later.  */

  return dwo_unit;
}

/* Subroutine of create_dwo_unit_in_dwp_v2 and create_dwo_unit_in_dwp_v5 to
   simplify them.  Given a pointer to the containing section SECTION, and
   OFFSET,SIZE of the piece within that section used by a TU/CU, return a
   virtual section of just that piece.  */

static struct dwarf2_section_info
create_dwp_v2_or_v5_section (dwarf2_per_bfd *per_bfd,
			     struct dwarf2_section_info *section,
			     bfd_size_type offset, bfd_size_type size)
{
  struct dwarf2_section_info result;
  asection *sectp;

  gdb_assert (section != NULL);
  gdb_assert (!section->is_virtual);

  memset (&result, 0, sizeof (result));
  result.s.containing_section = section;
  result.is_virtual = true;

  if (size == 0)
    return result;

  sectp = section->get_bfd_section ();

  /* Flag an error if the piece denoted by OFFSET,SIZE is outside the
     bounds of the real section.  This is a pretty-rare event, so just
     flag an error (easier) instead of a warning and trying to cope.  */
  if (sectp == NULL
      || offset + size > bfd_section_size (sectp))
    {
      error (_(DWARF_ERROR_PREFIX
	       "Bad DWP V2 or V5 section info, doesn't fit in section %s"
	       " [in module %s]"),
	     sectp ? bfd_section_name (sectp) : "<unknown>",
	     per_bfd->filename ());
    }

  result.virtual_offset = offset;
  result.size = size;
  gdb_assert (section->readin);
  result.readin = true;
  result.buffer = section->buffer + offset;
  return result;
}

/* Create a dwo_unit object for the DWO unit with signature SIGNATURE.
   UNIT_INDEX is the index of the DWO unit in the DWP hash table.
   COMP_DIR is the DW_AT_comp_dir attribute of the referencing CU.
   This is for DWP version 2 files.  */

static dwo_unit_up
create_dwo_unit_in_dwp_v2 (dwarf2_per_bfd *per_bfd,
			   struct dwp_file *dwp_file,
			   uint32_t unit_index,
			   const char *comp_dir,
			   ULONGEST signature, int is_debug_types)
{
  const struct dwp_hash_table *dwp_htab =
    is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd.get ();
  const char *kind = is_debug_types ? "TU" : "CU";
  struct virtual_v2_or_v5_dwo_sections sections;
  int i;

  gdb_assert (dwp_file->version == 2);

  dwarf_read_debug_printf ("Reading %s %s/%s in DWP V2 file: %s",
			   kind, pulongest (unit_index), hex_string (signature),
			   dwp_file->name);

  /* Fetch the section offsets of this DWO unit.  */

  memset (&sections, 0, sizeof (sections));

  for (i = 0; i < dwp_htab->nr_columns; ++i)
    {
      uint32_t offset = read_4_bytes (dbfd,
				      dwp_htab->section_pool.v2.offsets
				      + (((unit_index - 1) * dwp_htab->nr_columns
					  + i)
					 * sizeof (uint32_t)));
      uint32_t size = read_4_bytes (dbfd,
				    dwp_htab->section_pool.v2.sizes
				    + (((unit_index - 1) * dwp_htab->nr_columns
					+ i)
				       * sizeof (uint32_t)));

      switch (dwp_htab->section_pool.v2.section_ids[i])
	{
	case DW_SECT_INFO:
	case DW_SECT_TYPES:
	  sections.info_or_types_offset = offset;
	  sections.info_or_types_size = size;
	  break;
	case DW_SECT_ABBREV:
	  sections.abbrev_offset = offset;
	  sections.abbrev_size = size;
	  break;
	case DW_SECT_LINE:
	  sections.line_offset = offset;
	  sections.line_size = size;
	  break;
	case DW_SECT_LOC:
	  sections.loc_offset = offset;
	  sections.loc_size = size;
	  break;
	case DW_SECT_STR_OFFSETS:
	  sections.str_offsets_offset = offset;
	  sections.str_offsets_size = size;
	  break;
	case DW_SECT_MACINFO:
	  sections.macinfo_offset = offset;
	  sections.macinfo_size = size;
	  break;
	case DW_SECT_MACRO:
	  sections.macro_offset = offset;
	  sections.macro_size = size;
	  break;
	}
    }

  /* It's easier for the rest of the code if we fake a struct dwo_file and
     have dwo_unit "live" in that.  At least for now.

     The DWP file can be made up of a random collection of CUs and TUs.
     However, for each CU + set of TUs that came from the same original DWO
     file, we can combine them back into a virtual DWO file to save space
     (fewer struct dwo_file objects to allocate).  Remember that for really
     large apps there can be on the order of 8K CUs and 200K TUs, or more.  */

  std::string virtual_dwo_name =
    string_printf ("virtual-dwo/%ld-%ld-%ld-%ld",
		   (long) (sections.abbrev_size ? sections.abbrev_offset : 0),
		   (long) (sections.line_size ? sections.line_offset : 0),
		   (long) (sections.loc_size ? sections.loc_offset : 0),
		   (long) (sections.str_offsets_size
			   ? sections.str_offsets_offset : 0));

  /* Can we use an existing virtual DWO file?  */
  dwo_file *dwo_file
    = lookup_dwo_file (per_bfd, virtual_dwo_name.c_str (), comp_dir);

  /* Create one if necessary.  */
  if (dwo_file == nullptr)
    {
      dwarf_read_debug_printf ("Creating virtual DWO: %s",
			       virtual_dwo_name.c_str ());

      dwo_file_up new_dwo_file = std::make_unique<struct dwo_file> ();
      new_dwo_file->dwo_name = std::move (virtual_dwo_name);
      new_dwo_file->comp_dir = comp_dir;
      new_dwo_file->sections.abbrev
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.abbrev,
				       sections.abbrev_offset,
				       sections.abbrev_size);
      new_dwo_file->sections.line
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.line,
				       sections.line_offset,
				       sections.line_size);
      new_dwo_file->sections.loc
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.loc,
				       sections.loc_offset, sections.loc_size);
      new_dwo_file->sections.macinfo
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.macinfo,
				       sections.macinfo_offset,
				       sections.macinfo_size);
      new_dwo_file->sections.macro
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.macro,
				       sections.macro_offset,
				       sections.macro_size);
      new_dwo_file->sections.str_offsets
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.str_offsets,
				       sections.str_offsets_offset,
				       sections.str_offsets_size);

      /* The "str" section is global to the entire DWP file.  */
      new_dwo_file->sections.str = dwp_file->sections.str;

      /* The info or types section is assigned below to dwo_unit,
	 there's no need to record it in dwo_file.
	 Also, we can't simply record type sections in dwo_file because
	 we record a pointer into the vector in dwo_unit.  As we collect more
	 types we'll grow the vector and eventually have to reallocate space
	 for it, invalidating all copies of pointers into the previous
	 contents.  */
      dwo_file = add_dwo_file (per_bfd, std::move (new_dwo_file));
    }
  else
    dwarf_read_debug_printf ("Using existing virtual DWO: %s",
			     virtual_dwo_name.c_str ());

  auto dwo_unit = std::make_unique<struct dwo_unit> ();
  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = signature;
  dwo_unit->section_holder = std::make_unique<dwarf2_section_info> ();
  dwo_unit->section = dwo_unit->section_holder.get ();
  *dwo_unit->section = create_dwp_v2_or_v5_section
			 (per_bfd,
			  is_debug_types
			  ? &dwp_file->sections.types
			  : &dwp_file->sections.info,
			  sections.info_or_types_offset,
			  sections.info_or_types_size);
  /* dwo_unit->{offset,length,type_offset_in_tu} are set later.  */

  return dwo_unit;
}

/* Create a dwo_unit object for the DWO unit with signature SIGNATURE.
   UNIT_INDEX is the index of the DWO unit in the DWP hash table.
   COMP_DIR is the DW_AT_comp_dir attribute of the referencing CU.
   This is for DWP version 5 files.  */

static dwo_unit_up
create_dwo_unit_in_dwp_v5 (dwarf2_per_bfd *per_bfd,
			   struct dwp_file *dwp_file,
			   uint32_t unit_index,
			   const char *comp_dir,
			   ULONGEST signature, int is_debug_types)
{
  const struct dwp_hash_table *dwp_htab
    = is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd.get ();
  const char *kind = is_debug_types ? "TU" : "CU";
  struct virtual_v2_or_v5_dwo_sections sections {};

  gdb_assert (dwp_file->version == 5);

  dwarf_read_debug_printf ("Reading %s %s/%s in DWP V5 file: %s",
			   kind, pulongest (unit_index), hex_string (signature),
			   dwp_file->name);

  /* Fetch the section offsets of this DWO unit.  */

  /*  memset (&sections, 0, sizeof (sections)); */

  for (int i = 0; i < dwp_htab->nr_columns; ++i)
    {
      uint32_t offset = read_4_bytes (dbfd,
				      dwp_htab->section_pool.v5.offsets
				      + (((unit_index - 1)
					  * dwp_htab->nr_columns
					  + i)
					 * sizeof (uint32_t)));
      uint32_t size = read_4_bytes (dbfd,
				    dwp_htab->section_pool.v5.sizes
				    + (((unit_index - 1) * dwp_htab->nr_columns
					+ i)
				       * sizeof (uint32_t)));

      switch (dwp_htab->section_pool.v5.section_ids[i])
	{
	  case DW_SECT_ABBREV_V5:
	    sections.abbrev_offset = offset;
	    sections.abbrev_size = size;
	    break;
	  case DW_SECT_INFO_V5:
	    sections.info_or_types_offset = offset;
	    sections.info_or_types_size = size;
	    break;
	  case DW_SECT_LINE_V5:
	    sections.line_offset = offset;
	    sections.line_size = size;
	    break;
	  case DW_SECT_LOCLISTS_V5:
	    sections.loclists_offset = offset;
	    sections.loclists_size = size;
	    break;
	  case DW_SECT_MACRO_V5:
	    sections.macro_offset = offset;
	    sections.macro_size = size;
	    break;
	  case DW_SECT_RNGLISTS_V5:
	    sections.rnglists_offset = offset;
	    sections.rnglists_size = size;
	    break;
	  case DW_SECT_STR_OFFSETS_V5:
	    sections.str_offsets_offset = offset;
	    sections.str_offsets_size = size;
	    break;
	  case DW_SECT_RESERVED_V5:
	  default:
	    break;
	}
    }

  /* It's easier for the rest of the code if we fake a struct dwo_file and
     have dwo_unit "live" in that.  At least for now.

     The DWP file can be made up of a random collection of CUs and TUs.
     However, for each CU + set of TUs that came from the same original DWO
     file, we can combine them back into a virtual DWO file to save space
     (fewer struct dwo_file objects to allocate).  Remember that for really
     large apps there can be on the order of 8K CUs and 200K TUs, or more.  */

  std::string virtual_dwo_name =
    string_printf ("virtual-dwo/%ld-%ld-%ld-%ld-%ld-%ld",
		 (long) (sections.abbrev_size ? sections.abbrev_offset : 0),
		 (long) (sections.line_size ? sections.line_offset : 0),
		 (long) (sections.loclists_size ? sections.loclists_offset : 0),
		 (long) (sections.str_offsets_size
			    ? sections.str_offsets_offset : 0),
		 (long) (sections.macro_size ? sections.macro_offset : 0),
		 (long) (sections.rnglists_size ? sections.rnglists_offset: 0));

  /* Can we use an existing virtual DWO file?  */
  dwo_file *dwo_file
    = lookup_dwo_file (per_bfd, virtual_dwo_name.c_str (), comp_dir);

  /* Create one if necessary.  */
  if (dwo_file == nullptr)
    {
      dwarf_read_debug_printf ("Creating virtual DWO: %s",
			       virtual_dwo_name.c_str ());

      dwo_file_up new_dwo_file = std::make_unique<struct dwo_file> ();
      new_dwo_file->dwo_name = std::move (virtual_dwo_name);
      new_dwo_file->comp_dir = comp_dir;
      new_dwo_file->sections.abbrev
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.abbrev,
				       sections.abbrev_offset,
				       sections.abbrev_size);
      new_dwo_file->sections.line
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.line,
				       sections.line_offset,
				       sections.line_size);
      new_dwo_file->sections.macro
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.macro,
				       sections.macro_offset,
				       sections.macro_size);
      new_dwo_file->sections.loclists
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.loclists,
				       sections.loclists_offset,
				       sections.loclists_size);
      new_dwo_file->sections.rnglists
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.rnglists,
				       sections.rnglists_offset,
				       sections.rnglists_size);
      new_dwo_file->sections.str_offsets
	= create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.str_offsets,
				       sections.str_offsets_offset,
				       sections.str_offsets_size);

      /* The "str" section is global to the entire DWP file.  */
      new_dwo_file->sections.str = dwp_file->sections.str;

      /* The info or types section is assigned below to dwo_unit,
	 there's no need to record it in dwo_file.
	 Also, we can't simply record type sections in dwo_file because
	 we record a pointer into the vector in dwo_unit.  As we collect more
	 types we'll grow the vector and eventually have to reallocate space
	 for it, invalidating all copies of pointers into the previous
	 contents.  */
      dwo_file = add_dwo_file (per_bfd, std::move (new_dwo_file));
    }
  else
    dwarf_read_debug_printf ("Using existing virtual DWO: %s",
			     virtual_dwo_name.c_str ());

  auto dwo_unit = std::make_unique<struct dwo_unit> ();
  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = signature;
  dwo_unit->section
    = XOBNEW (&per_bfd->obstack, struct dwarf2_section_info);
  *dwo_unit->section
    = create_dwp_v2_or_v5_section (per_bfd, &dwp_file->sections.info,
				   sections.info_or_types_offset,
				   sections.info_or_types_size);
  /* dwo_unit->{offset,length,type_offset_in_tu} are set later.  */

  return dwo_unit;
}

/* Lookup the DWO unit with SIGNATURE in DWP_FILE.
   Returns NULL if the signature isn't found.  */

static struct dwo_unit *
lookup_dwo_unit_in_dwp (dwarf2_per_bfd *per_bfd,
			struct dwp_file *dwp_file, const char *comp_dir,
			ULONGEST signature, int is_debug_types)
{
  const struct dwp_hash_table *dwp_htab =
    is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd.get ();
  uint32_t mask = dwp_htab->nr_slots - 1;
  uint32_t hash = signature & mask;
  uint32_t hash2 = ((signature >> 32) & mask) | 1;
  auto &dwo_unit_set
    = is_debug_types ? dwp_file->loaded_tus : dwp_file->loaded_cus;

  {
#if CXX_STD_THREAD
    std::lock_guard<std::mutex> guard (dwp_file->loaded_cutus_lock);
#endif

    if (auto it = dwo_unit_set.find (signature);
	it != dwo_unit_set.end ())
      return it->get ();
  }

  /* Use a for loop so that we don't loop forever on bad debug info.  */
  for (unsigned int i = 0; i < dwp_htab->nr_slots; ++i)
    {
      ULONGEST signature_in_table;

      signature_in_table =
	read_8_bytes (dbfd, dwp_htab->hash_table + hash * sizeof (uint64_t));
      if (signature_in_table == signature)
	{
	  uint32_t unit_index =
	    read_4_bytes (dbfd,
			  dwp_htab->unit_table + hash * sizeof (uint32_t));
	  dwo_unit_up dwo_unit;

	  if (dwp_file->version == 1)
	    dwo_unit
	      = create_dwo_unit_in_dwp_v1 (per_bfd, dwp_file, unit_index,
					   comp_dir, signature, is_debug_types);
	  else if (dwp_file->version == 2)
	    dwo_unit
	      = create_dwo_unit_in_dwp_v2 (per_bfd, dwp_file, unit_index,
					   comp_dir, signature, is_debug_types);
	  else /* version == 5  */
	    dwo_unit
	      = create_dwo_unit_in_dwp_v5 (per_bfd, dwp_file, unit_index,
					   comp_dir, signature, is_debug_types);

	  /* If another thread raced with this one, opening the exact same
	     DWO unit, then we'll keep that other thread's copy.  */
#if CXX_STD_THREAD
	  std::lock_guard<std::mutex> guard (dwp_file->loaded_cutus_lock);
#endif

	  auto it = dwo_unit_set.emplace (std::move (dwo_unit)).first;
	  return it->get ();
	}

      if (signature_in_table == 0)
	return NULL;

      hash = (hash + hash2) & mask;
    }

  error (_(DWARF_ERROR_PREFIX
	   "bad DWP hash table, lookup didn't terminate [in module %s]"),
	 dwp_file->name);
}

/* Subroutine of open_dwo_file,open_dwp_file to simplify them.
   Open the file specified by FILE_NAME and hand it off to BFD for
   preliminary analysis.  Return a newly initialized bfd *, which
   includes a canonicalized copy of FILE_NAME.
   If IS_DWP is TRUE, we're opening a DWP file, otherwise a DWO file.
   SEARCH_CWD is true if the current directory is to be searched.
   It will be searched before debug-file-directory.
   If successful, the file is added to the bfd include table of the
   objfile's bfd (see gdb_bfd_record_inclusion).
   If unable to find/open the file, return NULL.
   NOTE: This function is derived from symfile_bfd_open.  */

static gdb_bfd_ref_ptr
try_open_dwop_file (dwarf2_per_bfd *per_bfd, const char *file_name, int is_dwp,
		    int search_cwd)
{
  int desc;
  /* Blech.  OPF_TRY_CWD_FIRST also disables searching the path list if
     FILE_NAME contains a '/'.  So we can't use it.  Instead prepend "."
     to debug_file_directory.  */
  const char *search_path;
  static const char dirname_separator_string[] = { DIRNAME_SEPARATOR, '\0' };

  gdb::unique_xmalloc_ptr<char> search_path_holder;
  if (search_cwd)
    {
      const std::string &debug_dir = per_bfd->captured_debug_dir;

      if (!debug_dir.empty ())
	{
	  search_path_holder.reset (concat (".", dirname_separator_string,
					    debug_dir.c_str (),
					    (char *) NULL));
	  search_path = search_path_holder.get ();
	}
      else
	search_path = ".";
    }
  else
    search_path = per_bfd->captured_debug_dir.c_str ();

  /* Add the path for the executable binary to the list of search paths.  */
  std::string objfile_dir = gdb_ldirname (per_bfd->filename ());
  search_path_holder.reset (concat (objfile_dir.c_str (),
				    dirname_separator_string,
				    search_path, nullptr));
  search_path = search_path_holder.get ();

  openp_flags flags = OPF_RETURN_REALPATH;
  if (is_dwp)
    flags |= OPF_SEARCH_IN_PATH;

  gdb::unique_xmalloc_ptr<char> absolute_name;
  desc = openp (search_path, flags, file_name, O_RDONLY | O_BINARY,
		&absolute_name, per_bfd->captured_cwd.c_str ());
  if (desc < 0)
    return NULL;

  gdb_bfd_ref_ptr sym_bfd (gdb_bfd_open (absolute_name.get (),
					 gnutarget, desc));
  if (sym_bfd == NULL)
    return NULL;

  {
#if CXX_STD_THREAD
    /* The operations below are not thread-safe, use a lock to synchronize
       concurrent accesses.  */
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock (mutex);
#endif

    if (!bfd_check_format (sym_bfd.get (), bfd_object))
      return NULL;

    /* Success.  Record the bfd as having been included by the objfile's bfd.
     This is important because things like demangled_names_hash lives in the
     objfile's per_bfd space and may have references to things like symbol
     names that live in the DWO/DWP file's per_bfd space.  PR 16426.  */
    gdb_bfd_record_inclusion (per_bfd->obfd, sym_bfd.get ());
  }

  return sym_bfd;
}

/* Try to open DWO file FILE_NAME.
   COMP_DIR is the DW_AT_comp_dir attribute.
   The result is the bfd handle of the file.
   If there is a problem finding or opening the file, return NULL.
   Upon success, the canonicalized path of the file is stored in the bfd,
   same as symfile_bfd_open.  */

gdb_bfd_ref_ptr
cutu_reader::open_dwo_file (dwarf2_per_bfd *per_bfd, const char *file_name,
			    const char *comp_dir)
{
  if (IS_ABSOLUTE_PATH (file_name))
    return try_open_dwop_file (per_bfd, file_name,
			       0 /*is_dwp*/, 0 /*search_cwd*/);

  /* Before trying the search path, try DWO_NAME in COMP_DIR.  */

  if (comp_dir != NULL)
    {
      std::string path_to_try = path_join (comp_dir, file_name);

      /* NOTE: If comp_dir is a relative path, this will also try the
	 search path, which seems useful.  */
      gdb_bfd_ref_ptr abfd
	= try_open_dwop_file (per_bfd, path_to_try.c_str (), 0 /* is_dwp */,
			      1 /* search_cwd */);

      if (abfd != NULL)
	return abfd;
    }

  /* That didn't work, try debug-file-directory, which, despite its name,
     is a list of paths.  */

  if (per_bfd->captured_debug_dir.empty ())
    return NULL;

  return try_open_dwop_file (per_bfd, file_name, 0 /* is_dwp */,
			     1 /* search_cwd */);
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWO debugging sections we are interested in.  */

void
cutu_reader::locate_dwo_sections (objfile *objfile, dwo_file &dwo_file)
{
  const struct dwop_section_names *names = &dwop_section_names;
  dwo_sections &dwo_sections = dwo_file.sections;
  bool complained_about_macro_already = false;

  for (asection *sec : gdb_bfd_sections (dwo_file.dbfd))
    {
      struct dwarf2_section_info *dw_sect = nullptr;

      if (names->abbrev_dwo.matches (sec->name))
	dw_sect = &dwo_sections.abbrev;
      else if (names->info_dwo.matches (sec->name))
	dw_sect = &dwo_sections.infos.emplace_back (dwarf2_section_info {});
      else if (names->line_dwo.matches (sec->name))
	dw_sect = &dwo_sections.line;
      else if (names->loc_dwo.matches (sec->name))
	dw_sect = &dwo_sections.loc;
      else if (names->loclists_dwo.matches (sec->name))
	dw_sect = &dwo_sections.loclists;
      else if (names->macinfo_dwo.matches (sec->name))
	dw_sect = &dwo_sections.macinfo;
      else if (names->macro_dwo.matches (sec->name))
	{
	  /* gcc versions <= 13 generate multiple .debug_macro.dwo sections with
	     some unresolved links between them.  It's not usable, so do as if
	     there were not there.  */
	  if (!complained_about_macro_already)
	    {
	      if (dwo_sections.macro.s.section == nullptr)
		dw_sect = &dwo_sections.macro;
	      else
		{
		  warning (_("Multiple .debug_macro.dwo sections found in "
			     "%s, ignoring them."), dwo_file.dbfd->filename);

		  dwo_sections.macro = dwarf2_section_info {};
		  complained_about_macro_already = true;
		}
	    }
	}
      else if (names->rnglists_dwo.matches (sec->name))
	dw_sect = &dwo_sections.rnglists;
      else if (names->str_dwo.matches (sec->name))
	dw_sect = &dwo_sections.str;
      else if (names->str_offsets_dwo.matches (sec->name))
	dw_sect = &dwo_sections.str_offsets;
      else if (names->types_dwo.matches (sec->name))
	dw_sect = &dwo_sections.types.emplace_back (dwarf2_section_info {});

      if (dw_sect != nullptr)
	{
	  /* Make sure we don't overwrite a section info that has been filled in
	 already.  */
	  gdb_assert (!dw_sect->readin);

	  dw_sect->s.section = sec;
	  dw_sect->size = bfd_section_size (sec);
	  dw_sect->read (objfile);
	}
    }
}

/* Initialize the use of the DWO file specified by DWO_NAME and referenced
   by PER_CU.  This is for the non-DWP case.
   The result is NULL if DWO_NAME can't be found.  */

dwo_file_up
cutu_reader::open_and_init_dwo_file (dwarf2_cu *cu, const char *dwo_name,
				     const char *comp_dir)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  gdb_bfd_ref_ptr dbfd
    = open_dwo_file (per_objfile->per_bfd, dwo_name, comp_dir);
  if (dbfd == NULL)
    {
      dwarf_read_debug_printf ("DWO file not found: %s", dwo_name);

      return NULL;
    }

  dwo_file_up dwo_file = std::make_unique<struct dwo_file> ();
  dwo_file->dwo_name = dwo_name;
  dwo_file->comp_dir = comp_dir;
  dwo_file->dbfd = std::move (dbfd);

  this->locate_dwo_sections (per_objfile->objfile, *dwo_file);

  for (dwarf2_section_info &section : dwo_file->sections.infos)
    create_dwo_unit_hash_tables (*dwo_file, *cu, section, ruh_kind::COMPILE);

  for (dwarf2_section_info &section : dwo_file->sections.types)
    create_dwo_unit_hash_tables (*dwo_file, *cu, section, ruh_kind::TYPE);

  dwarf_read_debug_printf ("DWO file found: %s", dwo_name);

  bfd_cache_close (dwo_file->dbfd.get ());

  return dwo_file;
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWP debugging sections common to version 1 and 2 that
   we are interested in.  */

static void
dwarf2_locate_common_dwp_sections (struct objfile *objfile, bfd *abfd,
				   asection *sectp, dwp_file *dwp_file)
{
  const struct dwop_section_names *names = &dwop_section_names;
  unsigned int elf_section_nr = elf_section_data (sectp)->this_idx;

  /* Record the ELF section number for later lookup: this is what the
     .debug_cu_index,.debug_tu_index tables use in DWP V1.  */
  gdb_assert (elf_section_nr < dwp_file->num_sections);
  dwp_file->elf_sections[elf_section_nr] = sectp;

  /* Look for specific sections that we need.  */
  struct dwarf2_section_info *dw_sect = nullptr;
  if (names->str_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.str;
  else if (names->cu_index.matches (sectp->name))
    dw_sect = &dwp_file->sections.cu_index;
  else if (names->tu_index.matches (sectp->name))
    dw_sect = &dwp_file->sections.tu_index;

  if (dw_sect != nullptr)
    {
      /* Make sure we don't overwrite a section info that has been filled in
	 already.  */
      gdb_assert (!dw_sect->readin);

      dw_sect->s.section = sectp;
      dw_sect->size = bfd_section_size (sectp);
      dw_sect->read (objfile);
    }
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWP version 2 debugging sections that we are interested
   in.  This is split into a separate function because we don't know if we
   have version 1 or 2 or 5 until we parse the cu_index/tu_index sections.  */

static void
dwarf2_locate_v2_dwp_sections (struct objfile *objfile, bfd *abfd,
			       asection *sectp, void *dwp_file_ptr)
{
  struct dwp_file *dwp_file = (struct dwp_file *) dwp_file_ptr;
  const struct dwop_section_names *names = &dwop_section_names;
  unsigned int elf_section_nr = elf_section_data (sectp)->this_idx;

  /* Record the ELF section number for later lookup: this is what the
     .debug_cu_index,.debug_tu_index tables use in DWP V1.  */
  gdb_assert (elf_section_nr < dwp_file->num_sections);
  dwp_file->elf_sections[elf_section_nr] = sectp;

  /* Look for specific sections that we need.  */
  struct dwarf2_section_info *dw_sect = nullptr;
  if (names->abbrev_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.abbrev;
  else if (names->info_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.info;
  else if (names->line_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.line;
  else if (names->loc_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.loc;
  else if (names->macinfo_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.macinfo;
  else if (names->macro_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.macro;
  else if (names->str_offsets_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.str_offsets;
  else if (names->types_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.types;

  if (dw_sect != nullptr)
    {
      /* Make sure we don't overwrite a section info that has been filled in
	 already.  */
      gdb_assert (!dw_sect->readin);

      dw_sect->s.section = sectp;
      dw_sect->size = bfd_section_size (sectp);
      dw_sect->read (objfile);
    }
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWP version 5 debugging sections that we are interested
   in.  This is split into a separate function because we don't know if we
   have version 1 or 2 or 5 until we parse the cu_index/tu_index sections.  */

static void
dwarf2_locate_v5_dwp_sections (struct objfile *objfile, bfd *abfd,
			       asection *sectp, void *dwp_file_ptr)
{
  struct dwp_file *dwp_file = (struct dwp_file *) dwp_file_ptr;
  const struct dwop_section_names *names = &dwop_section_names;
  unsigned int elf_section_nr = elf_section_data (sectp)->this_idx;

  /* Record the ELF section number for later lookup: this is what the
     .debug_cu_index,.debug_tu_index tables use in DWP V1.  */
  gdb_assert (elf_section_nr < dwp_file->num_sections);
  dwp_file->elf_sections[elf_section_nr] = sectp;

  /* Look for specific sections that we need.  */
  struct dwarf2_section_info *dw_sect = nullptr;
  if (names->abbrev_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.abbrev;
  else if (names->info_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.info;
  else if (names->line_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.line;
  else if (names->loclists_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.loclists;
  else if (names->macro_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.macro;
  else if (names->rnglists_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.rnglists;
  else if (names->str_offsets_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.str_offsets;

  if (dw_sect != nullptr)
    {
      /* Make sure we don't overwrite a section info that has been filled in
	 already.  */
      gdb_assert (!dw_sect->readin);

      dw_sect->s.section = sectp;
      dw_sect->size = bfd_section_size (sectp);
      dw_sect->read (objfile);
    }
}

/* Try to open DWP file FILE_NAME.
   The result is the bfd handle of the file.
   If there is a problem finding or opening the file, return NULL.
   Upon success, the canonicalized path of the file is stored in the bfd,
   same as symfile_bfd_open.  */

static gdb_bfd_ref_ptr
open_dwp_file (dwarf2_per_bfd *per_bfd, const char *file_name)
{
  gdb_bfd_ref_ptr abfd
    = try_open_dwop_file (per_bfd, file_name, 1 /* is_dwp */,
			  1 /* search_cwd */);
  if (abfd != NULL)
    return abfd;

  /* Work around upstream bug 15652.
     http://sourceware.org/bugzilla/show_bug.cgi?id=15652
     [Whether that's a "bug" is debatable, but it is getting in our way.]
     We have no real idea where the dwp file is, because gdb's realpath-ing
     of the executable's path may have discarded the needed info.
     [IWBN if the dwp file name was recorded in the executable, akin to
     .gnu_debuglink, but that doesn't exist yet.]
     Strip the directory from FILE_NAME and search again.  */
  if (!per_bfd->captured_debug_dir.empty ())
    {
      /* Don't implicitly search the current directory here.
	 If the user wants to search "." to handle this case,
	 it must be added to debug-file-directory.  */
      return try_open_dwop_file (per_bfd, lbasename (file_name),
				 1 /* is_dwp */, 0 /* search_cwd */);
    }

  return NULL;
}

/* Initialize the use of the DWP file for the current objfile.
   By convention the name of the DWP file is ${objfile}.dwp.  */

static void
open_and_init_dwp_file (dwarf2_per_objfile *per_objfile)
{
  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  /* Try to find first .dwp for the binary file before any symbolic links
     resolving.  */

  /* If the objfile is a debug file, find the name of the real binary
     file and get the name of dwp file from there.  */
  std::string dwp_name;
  if (objfile->separate_debug_objfile_backlink != NULL)
    {
      struct objfile *backlink = objfile->separate_debug_objfile_backlink;
      const char *backlink_basename = lbasename (backlink->original_name);

      dwp_name = gdb_ldirname (objfile->original_name) + SLASH_STRING + backlink_basename;
    }
  else
    dwp_name = objfile->original_name;

  dwp_name += ".dwp";

  gdb_bfd_ref_ptr dbfd (open_dwp_file (per_bfd, dwp_name.c_str ()));
  if (dbfd == NULL
      && strcmp (objfile->original_name, objfile_name (objfile)) != 0)
    {
      /* Try to find .dwp for the binary file after gdb_realpath resolving.  */
      dwp_name = objfile_name (objfile);
      dwp_name += ".dwp";
      dbfd = open_dwp_file (per_bfd, dwp_name.c_str ());
    }

  if (dbfd == NULL)
    {
      dwarf_read_debug_printf ("DWP file not found: %s", dwp_name.c_str ());

      return;
    }

  const char *name = bfd_get_filename (dbfd.get ());
  auto dwp_file = std::make_unique<struct dwp_file> (name, std::move (dbfd));

  dwp_file->num_sections = elf_numsections (dwp_file->dbfd);
  dwp_file->elf_sections
    = OBSTACK_CALLOC (&per_bfd->obstack, dwp_file->num_sections, asection *);

  for (asection *sec : gdb_bfd_sections (dwp_file->dbfd))
    dwarf2_locate_common_dwp_sections (objfile, dwp_file->dbfd.get (), sec,
				       dwp_file.get ());

  dwp_file->cus = create_dwp_hash_table (per_bfd, dwp_file.get (),
					 dwp_file->sections.cu_index);
  dwp_file->tus = create_dwp_hash_table (per_bfd, dwp_file.get (),
					 dwp_file->sections.tu_index);

  /* The DWP file version is stored in the hash table.  Oh well.  */
  if (dwp_file->cus && dwp_file->tus
      && dwp_file->cus->version != dwp_file->tus->version)
    {
      /* Technically speaking, we should try to limp along, but this is
	 pretty bizarre.  We use pulongest here because that's the established
	 portability solution (e.g, we cannot use %u for uint32_t).  */
      error (_(DWARF_ERROR_PREFIX
	       "DWP file CU version %s doesn't match TU version %s"
	       " [in DWP file %s]"),
	     pulongest (dwp_file->cus->version),
	     pulongest (dwp_file->tus->version), dwp_name.c_str ());
    }

  if (dwp_file->cus)
    dwp_file->version = dwp_file->cus->version;
  else if (dwp_file->tus)
    dwp_file->version = dwp_file->tus->version;
  else
    dwp_file->version = 2;

  for (asection *sec : gdb_bfd_sections (dwp_file->dbfd))
    {
      if (dwp_file->version == 2)
	dwarf2_locate_v2_dwp_sections (objfile, dwp_file->dbfd.get (), sec,
				       dwp_file.get ());
      else
	dwarf2_locate_v5_dwp_sections (objfile, dwp_file->dbfd.get (), sec,
				       dwp_file.get ());
    }

  dwarf_read_debug_printf ("DWP file found: %s", dwp_file->name);
  dwarf_read_debug_printf ("    %s CUs, %s TUs",
			   pulongest (dwp_file->cus ? dwp_file->cus->nr_units : 0),
			   pulongest (dwp_file->tus ? dwp_file->tus->nr_units : 0));

  bfd_cache_close (dwp_file->dbfd.get ());

  /* Everything worked, install this dwp_file in the per_bfd.  */
  per_objfile->per_bfd->dwp_file = std::move (dwp_file);
}

/* Subroutine of lookup_dwo_comp_unit, lookup_dwo_type_unit.
   Look up the CU/TU with signature SIGNATURE, either in DWO file DWO_NAME
   or in the DWP file for the objfile, referenced by THIS_UNIT.
   If non-NULL, comp_dir is the DW_AT_comp_dir attribute.
   IS_DEBUG_TYPES is non-zero if reading a TU, otherwise read a CU.

   This is called, for example, when wanting to read a variable with a
   complex location.  Therefore we don't want to do file i/o for every call.
   Therefore we don't want to look for a DWO file on every call.
   Therefore we first see if we've already seen SIGNATURE in a DWP file,
   then we check if we've already seen DWO_NAME, and only THEN do we check
   for a DWO file.

   The result is a pointer to the dwo_unit object or NULL if we didn't find it
   (dwo_id mismatch or couldn't find the DWO/DWP file).  */

dwo_unit *
cutu_reader::lookup_dwo_cutu (dwarf2_cu *cu, const char *dwo_name,
			      const char *comp_dir, ULONGEST signature,
			      int is_debug_types)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  struct objfile *objfile = per_objfile->objfile;
  const char *kind = is_debug_types ? "TU" : "CU";

  /* First see if there's a DWP file.
     If we have a DWP file but didn't find the DWO inside it, don't
     look for the original DWO file.  It makes gdb behave differently
     depending on whether one is debugging in the build tree.  */

  dwp_file *dwp_file = per_objfile->per_bfd->dwp_file.get ();

  if (dwp_file != NULL)
    {
      const struct dwp_hash_table *dwp_htab =
	is_debug_types ? dwp_file->tus : dwp_file->cus;

      if (dwp_htab != NULL)
	{
	  struct dwo_unit *dwo_cutu =
	    lookup_dwo_unit_in_dwp (per_bfd, dwp_file, comp_dir, signature,
				    is_debug_types);

	  if (dwo_cutu != NULL)
	    {
	      dwarf_read_debug_printf ("Virtual DWO %s %s found: @%s",
				       kind, hex_string (signature),
				       host_address_to_string (dwo_cutu));

	      return dwo_cutu;
	    }
	}
    }
  else
    {
      /* No DWP file, look for the DWO file.  */
      dwo_file *dwo_file = lookup_dwo_file (per_bfd, dwo_name, comp_dir);

      if (dwo_file == nullptr)
	{
	  /* Read in the file and build a table of the CUs/TUs it contains.

	     NOTE: This will be nullptr if unable to open the file.  */
	  dwo_file_up new_dwo_file
	    = open_and_init_dwo_file (cu, dwo_name, comp_dir);

	  if (new_dwo_file != nullptr)
	    dwo_file = add_dwo_file (per_bfd, std::move (new_dwo_file));
	}

      if (dwo_file != NULL)
	{
	  struct dwo_unit *dwo_cutu = NULL;

	  if (is_debug_types && !dwo_file->tus.empty ())
	    {
	      if (auto it = dwo_file->tus.find (signature);
		  it != dwo_file->tus.end ())
		dwo_cutu = it->get ();
	    }
	  else if (!is_debug_types && !dwo_file->cus.empty ())
	    {
	      if (auto it = dwo_file->cus.find (signature);
		  it != dwo_file->cus.end ())
		dwo_cutu = it->get ();
	    }

	  if (dwo_cutu != NULL)
	    {
	      dwarf_read_debug_printf ("DWO %s %s(%s) found: @%s",
				       kind, dwo_name, hex_string (signature),
				       host_address_to_string (dwo_cutu));

	      return dwo_cutu;
	    }
	}
    }

  /* We didn't find it.  This could mean a dwo_id mismatch, or
     someone deleted the DWO/DWP file, or the search path isn't set up
     correctly to find the file.  */

  dwarf_read_debug_printf ("DWO %s %s(%s) not found",
			   kind, dwo_name, hex_string (signature));

  /* This is a warning and not a complaint because it can be caused by
     pilot error (e.g., user accidentally deleting the DWO).  */
  {
    /* Print the name of the DWP file if we looked there, helps the user
       better diagnose the problem.  */
    std::string dwp_text;

    if (dwp_file != NULL)
      dwp_text = string_printf (" [in DWP file %s]",
				lbasename (dwp_file->name));

    warning (_("Could not find DWO %s %s(%s)%s referenced by %s at offset %s"
	       " [in module %s]"),
	     kind, dwo_name, hex_string (signature), dwp_text.c_str (), kind,
	     sect_offset_str (cu->per_cu->sect_off), objfile_name (objfile));
  }
  return NULL;
}

/* Lookup the DWO CU DWO_NAME/SIGNATURE referenced from THIS_CU.
   See lookup_dwo_cutu_unit for details.  */

dwo_unit *
cutu_reader::lookup_dwo_comp_unit (dwarf2_cu *cu, const char *dwo_name,
				   const char *comp_dir, ULONGEST signature)
{
  gdb_assert (!cu->per_cu->is_debug_types);

  return lookup_dwo_cutu (cu, dwo_name, comp_dir, signature, 0);
}

/* Lookup the DWO TU DWO_NAME/SIGNATURE referenced from THIS_TU.
   See lookup_dwo_cutu_unit for details.  */

dwo_unit *
cutu_reader::lookup_dwo_type_unit (dwarf2_cu *cu, const char *dwo_name,
				   const char *comp_dir)
{
  gdb_assert (cu->per_cu->is_debug_types);

  signatured_type *sig_type = (signatured_type *) cu->per_cu;

  return lookup_dwo_cutu (cu, dwo_name, comp_dir, sig_type->signature, 1);
}

/* Traversal function for queue_and_load_all_dwo_tus.  */

static int
queue_and_load_dwo_tu (dwo_unit *dwo_unit, dwarf2_cu *cu)
{
  ULONGEST signature = dwo_unit->signature;
  signatured_type *sig_type = lookup_dwo_signatured_type (cu, signature);

  if (sig_type != NULL)
    {
      /* We pass NULL for DEPENDENT_CU because we don't yet know if there's
	 a real dependency of PER_CU on SIG_TYPE.  That is detected later
	 while processing PER_CU.  */
      if (maybe_queue_comp_unit (NULL, sig_type, cu->per_objfile))
	load_full_type_unit (sig_type, cu->per_objfile);
      cu->per_cu->imported_symtabs.push_back (sig_type);
    }

  return 1;
}

/* Queue all TUs contained in the DWO of CU to be read in.
   The DWO may have the only definition of the type, though it may not be
   referenced anywhere in PER_CU.  Thus we have to load *all* its TUs.
   http://sourceware.org/bugzilla/show_bug.cgi?id=15021  */

static void
queue_and_load_all_dwo_tus (dwarf2_cu *cu)
{
  struct dwo_unit *dwo_unit;
  struct dwo_file *dwo_file;

  gdb_assert (cu != nullptr);
  gdb_assert (!cu->per_cu->is_debug_types);
  gdb_assert (cu->per_objfile->per_bfd->dwp_file == nullptr);

  dwo_unit = cu->dwo_unit;
  gdb_assert (dwo_unit != NULL);

  dwo_file = dwo_unit->dwo_file;

  for (const dwo_unit_up &unit : dwo_file->tus)
    queue_and_load_dwo_tu (unit.get (), cu);
}

/* Read in various DIEs.  */

/* DW_AT_abstract_origin inherits whole DIEs (not just their attributes).
   Inherit only the children of the DW_AT_abstract_origin DIE not being
   already referenced by DW_AT_abstract_origin from the children of the
   current DIE.  */

static void
inherit_abstract_dies (struct die_info *die, struct dwarf2_cu *cu)
{
  attribute *attr = dwarf2_attr (die, DW_AT_abstract_origin, cu);
  if (attr == nullptr)
    return;

  /* Note that following die references may follow to a die in a
     different CU.  */
  dwarf2_cu *origin_cu = cu;

  /* Parent of DIE - referenced by DW_AT_abstract_origin.  */
  die_info *origin_die = follow_die_ref (die, attr, &origin_cu);

  /* We're inheriting ORIGIN's children into the scope we'd put DIE's
     symbols in.  */
  struct pending **origin_previous_list_in_scope = origin_cu->list_in_scope;
  origin_cu->list_in_scope = cu->list_in_scope;

  if (die->tag != origin_die->tag
      && !(die->tag == DW_TAG_inlined_subroutine
	   && origin_die->tag == DW_TAG_subprogram))
    complaint (_("DIE %s and its abstract origin %s have different tags"),
	       sect_offset_str (die->sect_off),
	       sect_offset_str (origin_die->sect_off));

  /* Find if the concrete and abstract trees are structurally the
     same.  This is a shallow traversal and it is not bullet-proof;
     the compiler can trick the debugger into believing that the trees
     are isomorphic, whereas they actually are not.  However, the
     likelihood of this happening is pretty low, and a full-fledged
     check would be an overkill.  */
  bool are_isomorphic = true;
  die_info *concrete_child = die->child;
  die_info *abstract_child = origin_die->child;
  while (concrete_child != nullptr || abstract_child != nullptr)
    {
      if (concrete_child == nullptr
	  || abstract_child == nullptr
	  || concrete_child->tag != abstract_child->tag)
	{
	  are_isomorphic = false;
	  break;
	}

      concrete_child = concrete_child->next;
      abstract_child = abstract_child->next;
    }

  /* Walk the origin's children in parallel to the concrete children.
     This helps match an origin child in case the debug info misses
     DW_AT_abstract_origin attributes.  Keep in mind that the abstract
     origin tree may not have the same tree structure as the concrete
     DIE, though.  */
  die_info *corresponding_abstract_child
    = are_isomorphic ? origin_die->child : nullptr;

  std::vector<sect_offset> offsets;

  for (die_info *child_die : die->children ())
    {
      /* We are trying to process concrete instance entries:
	 DW_TAG_call_site DIEs indeed have a DW_AT_abstract_origin tag, but
	 it's not relevant to our analysis here. i.e. detecting DIEs that are
	 present in the abstract instance but not referenced in the concrete
	 one.  */
      if (child_die->tag == DW_TAG_call_site
	  || child_die->tag == DW_TAG_GNU_call_site)
	{
	  if (are_isomorphic)
	    corresponding_abstract_child
	      = corresponding_abstract_child->next;
	  continue;
	}

      /* For each CHILD_DIE, find the corresponding child of
	 ORIGIN_DIE.  If there is more than one layer of
	 DW_AT_abstract_origin, follow them all; there shouldn't be,
	 but GCC versions at least through 4.4 generate this (GCC PR
	 40573).  */
      die_info *child_origin_die = child_die;
      dwarf2_cu *child_origin_cu = cu;
      while (true)
	{
	  attr = dwarf2_attr (child_origin_die, DW_AT_abstract_origin,
			      child_origin_cu);
	  if (attr == nullptr)
	    break;

	  die_info *prev_child_origin_die = child_origin_die;
	  child_origin_die = follow_die_ref (child_origin_die, attr,
					     &child_origin_cu);

	  if (prev_child_origin_die == child_origin_die)
	    {
	      /* Handle DIE with self-reference.  */
	      break;
	    }
	}

      /* If missing DW_AT_abstract_origin, try the corresponding child
	 of the origin.  Clang emits such lexical scopes.  */
      if (child_origin_die == child_die
	  && dwarf2_attr (child_die, DW_AT_abstract_origin, cu) == nullptr
	  && are_isomorphic
	  && child_die->tag == DW_TAG_lexical_block)
	child_origin_die = corresponding_abstract_child;

      /* According to DWARF3 3.3.8.2 #3 new entries without their abstract
	 counterpart may exist.  */
      if (child_origin_die != child_die)
	{
	  if (child_die->tag != child_origin_die->tag
	      && !(child_die->tag == DW_TAG_inlined_subroutine
		   && child_origin_die->tag == DW_TAG_subprogram))
	    complaint (_("Child DIE %s and its abstract origin %s have "
			 "different tags"),
		       sect_offset_str (child_die->sect_off),
		       sect_offset_str (child_origin_die->sect_off));
	  if (child_origin_die->parent != origin_die)
	    complaint (_("Child DIE %s and its abstract origin %s have "
			 "different parents"),
		       sect_offset_str (child_die->sect_off),
		       sect_offset_str (child_origin_die->sect_off));
	  else
	    offsets.push_back (child_origin_die->sect_off);
	}

      if (are_isomorphic)
	corresponding_abstract_child = corresponding_abstract_child->next;
    }

  if (!offsets.empty ())
    {
      std::sort (offsets.begin (), offsets.end ());

      for (auto offsets_it = offsets.begin () + 1;
	   offsets_it < offsets.end ();
	   ++offsets_it)
	if (*(offsets_it - 1) == *offsets_it)
	  complaint (_("Multiple children of DIE %s refer "
		       "to DIE %s as their abstract origin"),
		     sect_offset_str (die->sect_off),
		     sect_offset_str (*offsets_it));
    }

  auto offsets_it = offsets.begin ();
  for (die_info *origin_child_die : origin_die->children ())
    {
      /* Is ORIGIN_CHILD_DIE referenced by any of the DIE children?  */
      while (offsets_it < offsets.end ()
	     && *offsets_it < origin_child_die->sect_off)
	++offsets_it;

      if (offsets_it == offsets.end ()
	  || *offsets_it > origin_child_die->sect_off)
	{
	  /* Found that ORIGIN_CHILD_DIE is really not referenced.
	     Check whether we're already processing ORIGIN_CHILD_DIE.
	     This can happen with mutually referenced abstract_origins.
	     PR 16581.  */
	  if (!origin_child_die->in_process)
	    process_die (origin_child_die, origin_cu);
	}
    }

  origin_cu->list_in_scope = origin_previous_list_in_scope;

  if (cu != origin_cu)
    compute_delayed_physnames (origin_cu);
}

/* Return TRUE if the given DIE is the program's "main".  DWARF 4 has
   defined a dedicated DW_AT_main_subprogram attribute to indicate the
   starting function of the program, however with older versions the
   DW_CC_program value of the DW_AT_calling_convention attribute was
   used instead as the only means available.  We handle both variants.  */

static bool
dwarf2_func_is_main_p (struct die_info *die, struct dwarf2_cu *cu)
{
  if (dwarf2_flag_true_p (die, DW_AT_main_subprogram, cu))
    return true;
  struct attribute *attr = dwarf2_attr (die, DW_AT_calling_convention, cu);
  if (attr == nullptr)
    return false;
  std::optional<ULONGEST> value = attr->unsigned_constant ();
  return value.has_value () && *value == DW_CC_program;
}

/* A helper to handle Ada's "Pragma Import" feature when it is applied
   to a function.  */

static bool
check_ada_pragma_import (struct die_info *die, struct dwarf2_cu *cu)
{
  if (cu->lang () != language_ada)
    return false;

  /* A Pragma Import will have both a name and a linkage name.  With a
     newer version of GNAT, we have to examine the full name, because
     the compiler might decide to emit a linkage name matching the
     full name in some scenario.  */
  const char *name = dwarf2_full_name (nullptr, die, cu);
  if (name == nullptr)
    return false;

  const char *linkage_name = dw2_linkage_name (die, cu);
  /* Disallow the special Ada symbols.  */
  if (!is_ada_import_or_export (cu, name, linkage_name))
    return false;

  /* A Pragma Import will be a declaration, while a Pragma Export will
     not be.  */
  if (!die_is_declaration (die, cu))
    return false;

  new_symbol (die, read_type_die (die, cu), cu);
  return true;
}

/* Apply fixups to LOW_PC and HIGH_PC due to an incorrect DIE in CU.  */

static void
fixup_low_high_pc (struct dwarf2_cu *cu, struct die_info *die, CORE_ADDR *low_pc,
		   CORE_ADDR *high_pc)
{
  if (die->tag != DW_TAG_subprogram)
    return;

  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();

  if (gdbarch_bfd_arch_info (gdbarch)->arch == bfd_arch_arm
      && cu->producer_is_gas_ge_2_39 ())
    {
      /* Gas version 2.39 produces DWARF for a Thumb subprogram with a low_pc
	 attribute with the thumb bit set (PR gas/31115).  Work around this.  */
      *low_pc = gdbarch_addr_bits_remove (gdbarch, *low_pc);
      if (high_pc != nullptr)
	*high_pc = gdbarch_addr_bits_remove (gdbarch, *high_pc);
    }
}

static void
read_func_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  struct context_stack *newobj;
  CORE_ADDR lowpc;
  CORE_ADDR highpc;
  struct attribute *attr, *call_line, *call_file;
  const char *name;
  struct block *block;
  int inlined_func = (die->tag == DW_TAG_inlined_subroutine);
  std::vector<struct symbol *> template_args;
  struct template_symbol *templ_func = NULL;

  if (inlined_func)
    {
      /* If we do not have call site information, we can't show the
	 caller of this inlined function.  That's too confusing, so
	 only use the scope for local variables.  */
      call_line = dwarf2_attr (die, DW_AT_call_line, cu);
      call_file = dwarf2_attr (die, DW_AT_call_file, cu);
      if (call_line == NULL || call_file == NULL)
	{
	  read_lexical_block_scope (die, cu);
	  return;
	}
    }

  name = dwarf2_name (die, cu);
  if (name == nullptr)
    name = dw2_linkage_name (die, cu);

  /* Ignore functions with missing or empty names.  These are actually
     illegal according to the DWARF standard.  */
  if (name == NULL)
    {
      complaint (_("missing name for subprogram DIE at %s"),
		 sect_offset_str (die->sect_off));
      return;
    }

  if (check_ada_pragma_import (die, cu))
    {
      /* We already made the symbol for the Pragma Import, and because
	 it is a declaration, we know it won't have any other
	 important information, so we can simply return.  */
      return;
    }

  /* Ignore functions with missing or invalid low and high pc attributes.  */
  unrelocated_addr unrel_low, unrel_high;
  if (dwarf2_get_pc_bounds (die, &unrel_low, &unrel_high, cu, nullptr, nullptr)
      <= PC_BOUNDS_INVALID)
    {
      if (have_complaint ())
	{
	  attr = dwarf2_attr (die, DW_AT_external, cu);
	  bool external_p = attr != nullptr && attr->as_boolean ();
	  attr = dwarf2_attr (die, DW_AT_inline, cu);
	  bool inlined_p = false;
	  if (attr != nullptr)
	    {
	      std::optional<ULONGEST> value = attr->unsigned_constant ();
	      inlined_p = (value.has_value ()
			   && (*value == DW_INL_inlined
			       || *value == DW_INL_declared_inlined));
	    }
	  attr = dwarf2_attr (die, DW_AT_declaration, cu);
	  bool decl_p = attr != nullptr && attr->as_boolean ();
	  if (!external_p && !inlined_p && !decl_p)
	    complaint (_("cannot get low and high bounds "
			 "for subprogram DIE at %s"),
		       sect_offset_str (die->sect_off));
	}
      return;
    }

  lowpc = per_objfile->relocate (unrel_low);
  highpc = per_objfile->relocate (unrel_high);
  fixup_low_high_pc (cu, die, &lowpc, &highpc);

  /* If we have any template arguments, then we must allocate a
     different sort of symbol.  */
  for (die_info *child_die : die->children ())
    {
      if (child_die->tag == DW_TAG_template_type_param
	  || child_die->tag == DW_TAG_template_value_param)
	{
	  templ_func = new (&objfile->objfile_obstack) template_symbol;
	  templ_func->subclass = SYMBOL_TEMPLATE;
	  break;
	}
    }

  gdb_assert (cu->get_builder () != nullptr);
  newobj = cu->get_builder ()->push_context (0, lowpc);
  newobj->name = new_symbol (die, read_type_die (die, cu), cu, templ_func);

  if (dwarf2_func_is_main_p (die, cu))
    set_objfile_main_name (objfile, newobj->name->linkage_name (),
			   cu->lang ());

  /* If there is a location expression for DW_AT_frame_base, record
     it.  */
  attr = dwarf2_attr (die, DW_AT_frame_base, cu);
  if (attr != nullptr)
    dwarf2_symbol_mark_computed (attr, newobj->name, cu, 1);

  /* If there is a location for the static link, record it.  */
  newobj->static_link = NULL;
  attr = dwarf2_attr (die, DW_AT_static_link, cu);
  if (attr != nullptr)
    {
      newobj->static_link
	= XOBNEW (&objfile->objfile_obstack, struct dynamic_prop);
      attr_to_dynamic_prop (attr, die, cu, newobj->static_link,
			    cu->addr_type ());
    }

  cu->list_in_scope = cu->get_builder ()->get_local_symbols ();

  for (die_info *child_die : die->children ())
    {
      if (child_die->tag == DW_TAG_template_type_param
	  || child_die->tag == DW_TAG_template_value_param)
	{
	  struct symbol *arg = new_symbol (child_die, NULL, cu);

	  if (arg != NULL)
	    template_args.push_back (arg);
	}
      else
	process_die (child_die, cu);
    }

  inherit_abstract_dies (die, cu);

  /* If we have a DW_AT_specification, we might need to import using
     directives from the context of the specification DIE.  See the
     comment in determine_prefix.  */
  if (cu->lang () == language_cplus
      && dwarf2_attr (die, DW_AT_specification, cu))
    {
      struct dwarf2_cu *spec_cu = cu;
      struct die_info *spec_die = die_specification (die, &spec_cu);

      while (spec_die)
	{
	  for (die_info *child_die : spec_die->children ())
	    if (child_die->tag == DW_TAG_imported_module)
	      process_die (child_die, spec_cu);

	  /* In some cases, GCC generates specification DIEs that
	     themselves contain DW_AT_specification attributes.  */
	  spec_die = die_specification (spec_die, &spec_cu);
	}
    }

  struct context_stack cstk = cu->get_builder ()->pop_context ();
  /* Make a block for the local symbols within.  */
  block = cu->get_builder ()->finish_block (cstk.name, cstk.old_blocks,
				     cstk.static_link, lowpc, highpc);

  /* For C++, set the block's scope.  */
  if ((cu->lang () == language_cplus
       || cu->lang () == language_fortran
       || cu->lang () == language_d
       || cu->lang () == language_rust)
      && cu->processing_has_namespace_info)
    block->set_scope (determine_prefix (die, cu),
		      &objfile->objfile_obstack);

  /* If we have address ranges, record them.  */
  dwarf2_record_block_ranges (die, block, cu);

  gdbarch_make_symbol_special (gdbarch, cstk.name, objfile);

  /* Attach template arguments to function.  */
  if (!template_args.empty ())
    {
      gdb_assert (templ_func != NULL);

      templ_func->n_template_arguments = template_args.size ();
      templ_func->template_arguments
	= XOBNEWVEC (&objfile->objfile_obstack, struct symbol *,
		     templ_func->n_template_arguments);
      memcpy (templ_func->template_arguments,
	      template_args.data (),
	      (templ_func->n_template_arguments * sizeof (struct symbol *)));

      /* Make sure that the symtab is set on the new symbols.  Even
	 though they don't appear in this symtab directly, other parts
	 of gdb assume that symbols do, and this is reasonably
	 true.  */
      for (symbol *sym : template_args)
	sym->set_symtab (templ_func->symtab ());
    }

  /* In C++, we can have functions nested inside functions (e.g., when
     a function declares a class that has methods).  This means that
     when we finish processing a function scope, we may need to go
     back to building a containing block's symbol lists.  */
  *cu->get_builder ()->get_local_symbols () = cstk.locals;
  cu->get_builder ()->set_local_using_directives (cstk.local_using_directives);

  /* If we've finished processing a top-level function, subsequent
     symbols go in the file symbol list.  */
  if (cu->get_builder ()->outermost_context_p ())
    cu->list_in_scope = cu->get_builder ()->get_file_symbols ();
}

/* Process all the DIES contained within a lexical block scope.  Start
   a new scope, process the dies, and then close the scope.  */

static void
read_lexical_block_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  CORE_ADDR lowpc, highpc;

  /* Ignore blocks with missing or invalid low and high pc attributes.  */
  /* ??? Perhaps consider discontiguous blocks defined by DW_AT_ranges
     as multiple lexical blocks?  Handling children in a sane way would
     be nasty.  Might be easier to properly extend generic blocks to
     describe ranges.  */
  unrelocated_addr unrel_low, unrel_high;
  switch (dwarf2_get_pc_bounds (die, &unrel_low, &unrel_high, cu,
				nullptr, nullptr))
    {
    case PC_BOUNDS_NOT_PRESENT:
      /* DW_TAG_lexical_block has no attributes, process its children as if
	 there was no wrapping by that DW_TAG_lexical_block.
	 GCC does no longer produces such DWARF since GCC r224161.  */
      for (die_info *child_die : die->children ())
	{
	  /* We might already be processing this DIE.  This can happen
	     in an unusual circumstance -- where a subroutine A
	     appears lexically in another subroutine B, but A actually
	     inlines B.  The recursion is broken here, rather than in
	     inherit_abstract_dies, because it seems better to simply
	     drop concrete children here.  */
	  if (!child_die->in_process)
	    process_die (child_die, cu);
	}
      return;
    case PC_BOUNDS_INVALID:
      return;
    }
  lowpc = per_objfile->relocate (unrel_low);
  highpc = per_objfile->relocate (unrel_high);

  cu->get_builder ()->push_context (0, lowpc);
  for (die_info *child_die : die->children ())
    process_die (child_die, cu);

  inherit_abstract_dies (die, cu);
  struct context_stack cstk = cu->get_builder ()->pop_context ();

  if (*cu->get_builder ()->get_local_symbols () != NULL
      || (*cu->get_builder ()->get_local_using_directives ()) != NULL)
    {
      struct block *block
	= cu->get_builder ()->finish_block (0, cstk.old_blocks, NULL,
				     cstk.start_addr, highpc);

      /* Note that recording ranges after traversing children, as we
	 do here, means that recording a parent's ranges entails
	 walking across all its children's ranges as they appear in
	 the address map, which is quadratic behavior.

	 It would be nicer to record the parent's ranges before
	 traversing its children, simply overriding whatever you find
	 there.  But since we don't even decide whether to create a
	 block until after we've traversed its children, that's hard
	 to do.  */
      dwarf2_record_block_ranges (die, block, cu);
    }
  *cu->get_builder ()->get_local_symbols () = cstk.locals;
  cu->get_builder ()->set_local_using_directives (cstk.local_using_directives);
}

static void dwarf2_ranges_read_low_addrs
     (unsigned offset,
      struct dwarf2_cu *cu,
      dwarf_tag tag,
      std::vector<unrelocated_addr> &result);

/* Read in DW_TAG_call_site and insert it to CU->call_site_htab.  */

static void
read_call_site_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  struct attribute *attr;
  int nparams;

  attr = dwarf2_attr (die, DW_AT_call_return_pc, cu);
  if (attr == NULL)
    {
      /* This was a pre-DWARF-5 GNU extension alias
	 for DW_AT_call_return_pc.  */
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
    }
  if (!attr)
    {
      complaint (_("missing DW_AT_call_return_pc for DW_TAG_call_site "
		   "DIE %s [in module %s]"),
		 sect_offset_str (die->sect_off), objfile_name (objfile));
      return;
    }
  unrelocated_addr pc = attr->as_address ();

  /* Count parameters at the caller.  */

  nparams = 0;
  for (die_info *child_die : die->children ())
    {
      if (child_die->tag != DW_TAG_call_site_parameter
	  && child_die->tag != DW_TAG_GNU_call_site_parameter)
	{
	  complaint (_("Tag %d is not DW_TAG_call_site_parameter in "
		       "DW_TAG_call_site child DIE %s [in module %s]"),
		     child_die->tag, sect_offset_str (child_die->sect_off),
		     objfile_name (objfile));
	  continue;
	}

      nparams++;
    }

  struct call_site *call_site
    = new (XOBNEWVAR (&objfile->objfile_obstack,
		      struct call_site,
		      sizeof (*call_site) + sizeof (call_site->parameter[0]) * nparams))
    struct call_site (pc, cu->per_cu, per_objfile);
  
  if (!cu->call_site_htab.emplace (call_site).second)
    {
      complaint (_("Duplicate PC %s for DW_TAG_call_site "
		   "DIE %s [in module %s]"),
		 paddress (gdbarch, (CORE_ADDR) pc), sect_offset_str (die->sect_off),
		 objfile_name (objfile));
      return;
    }

  /* We never call the destructor of call_site, so we must ensure it is
     trivially destructible.  */
  static_assert(std::is_trivially_destructible<struct call_site>::value);

  if (dwarf2_flag_true_p (die, DW_AT_call_tail_call, cu)
      || dwarf2_flag_true_p (die, DW_AT_GNU_tail_call, cu))
    {
      struct die_info *func_die;

      /* Skip also over DW_TAG_inlined_subroutine.  */
      for (func_die = die->parent;
	   func_die && func_die->tag != DW_TAG_subprogram
	   && func_die->tag != DW_TAG_subroutine_type;
	   func_die = func_die->parent);

      /* DW_AT_call_all_calls is a superset
	 of DW_AT_call_all_tail_calls.  */
      if (func_die
	  && !dwarf2_flag_true_p (func_die, DW_AT_call_all_calls, cu)
	  && !dwarf2_flag_true_p (func_die, DW_AT_GNU_all_call_sites, cu)
	  && !dwarf2_flag_true_p (func_die, DW_AT_call_all_tail_calls, cu)
	  && !dwarf2_flag_true_p (func_die, DW_AT_GNU_all_tail_call_sites, cu))
	{
	  /* TYPE_TAIL_CALL_LIST is not interesting in functions where it is
	     not complete.  But keep CALL_SITE for look ups via call_site_htab,
	     both the initial caller containing the real return address PC and
	     the final callee containing the current PC of a chain of tail
	     calls do not need to have the tail call list complete.  But any
	     function candidate for a virtual tail call frame searched via
	     TYPE_TAIL_CALL_LIST must have the tail call list complete to be
	     determined unambiguously.  */
	}
      else
	{
	  struct type *func_type = NULL;

	  if (func_die)
	    func_type = get_die_type (func_die, cu);
	  if (func_type != NULL)
	    {
	      gdb_assert (func_type->code () == TYPE_CODE_FUNC);

	      /* Enlist this call site to the function.  */
	      call_site->tail_call_next = TYPE_TAIL_CALL_LIST (func_type);
	      TYPE_TAIL_CALL_LIST (func_type) = call_site;
	    }
	  else
	    complaint (_("Cannot find function owning DW_TAG_call_site "
			 "DIE %s [in module %s]"),
		       sect_offset_str (die->sect_off), objfile_name (objfile));
	}
    }

  attr = dwarf2_attr (die, DW_AT_call_target, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_GNU_call_site_target, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_call_origin, cu);
  if (attr == NULL)
    {
      /* This was a pre-DWARF-5 GNU extension alias for DW_AT_call_origin.  */
      attr = dwarf2_attr (die, DW_AT_abstract_origin, cu);
    }

  call_site->target.set_loc_dwarf_block (nullptr);
  if (!attr || (attr->form_is_block () && attr->as_block ()->size == 0))
    /* Keep NULL DWARF_BLOCK.  */;
  else if (attr->form_is_block ())
    {
      struct dwarf2_locexpr_baton *dlbaton;
      struct dwarf_block *block = attr->as_block ();

      dlbaton = OBSTACK_ZALLOC (&objfile->objfile_obstack,
				struct dwarf2_locexpr_baton);
      dlbaton->data = block->data;
      dlbaton->size = block->size;
      dlbaton->per_objfile = per_objfile;
      dlbaton->per_cu = cu->per_cu;

      call_site->target.set_loc_dwarf_block (dlbaton);
    }
  else if (attr->form_is_ref ())
    {
      struct dwarf2_cu *target_cu = cu;
      struct die_info *target_die;

      target_die = follow_die_ref (die, attr, &target_cu);
      gdb_assert (target_cu->per_objfile->objfile == objfile);

      struct attribute *ranges_attr
	= dwarf2_attr (target_die, DW_AT_ranges, target_cu);

      if (die_is_declaration (target_die, target_cu))
	{
	  const char *target_physname;

	  /* Prefer the mangled name; otherwise compute the demangled one.  */
	  target_physname = dw2_linkage_name (target_die, target_cu);
	  if (target_physname == NULL)
	    target_physname = dwarf2_physname (NULL, target_die, target_cu);
	  if (target_physname == NULL)
	    complaint (_("DW_AT_call_target target DIE has invalid "
			 "physname, for referencing DIE %s [in module %s]"),
		       sect_offset_str (die->sect_off), objfile_name (objfile));
	  else
	    call_site->target.set_loc_physname (target_physname);
	}
      else if (ranges_attr != nullptr && ranges_attr->form_is_unsigned ())
	{
	  ULONGEST ranges_offset = (ranges_attr->as_unsigned ()
				    + target_cu->gnu_ranges_base);
	  std::vector<unrelocated_addr> addresses;
	  dwarf2_ranges_read_low_addrs (ranges_offset, target_cu,
					target_die->tag, addresses);
	  unrelocated_addr *saved = XOBNEWVEC (&objfile->objfile_obstack,
					       unrelocated_addr,
					       addresses.size ());
	  std::copy (addresses.begin (), addresses.end (), saved);
	  call_site->target.set_loc_array (addresses.size (), saved);
	}
      else
	{
	  unrelocated_addr lowpc;

	  /* DW_AT_entry_pc should be preferred.  */
	  if (dwarf2_get_pc_bounds (target_die, &lowpc, NULL, target_cu,
				    nullptr, nullptr)
	      <= PC_BOUNDS_INVALID)
	    complaint (_("DW_AT_call_target target DIE has invalid "
			 "low pc, for referencing DIE %s [in module %s]"),
		       sect_offset_str (die->sect_off), objfile_name (objfile));
	  else
	    call_site->target.set_loc_physaddr (lowpc);
	}
    }
  else
    complaint (_("DW_TAG_call_site DW_AT_call_target is neither "
		 "block nor reference, for DIE %s [in module %s]"),
	       sect_offset_str (die->sect_off), objfile_name (objfile));

  for (die_info *child_die : die->children ())
    {
      struct call_site_parameter *parameter;
      struct attribute *loc, *origin;

      if (child_die->tag != DW_TAG_call_site_parameter
	  && child_die->tag != DW_TAG_GNU_call_site_parameter)
	{
	  /* Already printed the complaint above.  */
	  continue;
	}

      gdb_assert (call_site->parameter_count < nparams);
      parameter = &call_site->parameter[call_site->parameter_count];

      /* DW_AT_location specifies the register number or DW_AT_abstract_origin
	 specifies DW_TAG_formal_parameter.  Value of the data assumed for the
	 register is contained in DW_AT_call_value.  */

      loc = dwarf2_attr (child_die, DW_AT_location, cu);
      origin = dwarf2_attr (child_die, DW_AT_call_parameter, cu);
      if (origin == NULL)
	{
	  /* This was a pre-DWARF-5 GNU extension alias
	     for DW_AT_call_parameter.  */
	  origin = dwarf2_attr (child_die, DW_AT_abstract_origin, cu);
	}
      if (loc == NULL && origin != NULL && origin->form_is_ref ())
	{
	  parameter->kind = CALL_SITE_PARAMETER_PARAM_OFFSET;

	  sect_offset sect_off = origin->get_ref_die_offset ();
	  if (!cu->header.offset_in_unit_p (sect_off))
	    {
	      /* As DW_OP_GNU_parameter_ref uses CU-relative offset this
		 binding can be done only inside one CU.  Such referenced DIE
		 therefore cannot be even moved to DW_TAG_partial_unit.  */
	      complaint (_("DW_AT_call_parameter offset is not in CU for "
			   "DW_TAG_call_site child DIE %s [in module %s]"),
			 sect_offset_str (child_die->sect_off),
			 objfile_name (objfile));
	      continue;
	    }
	  parameter->u.param_cu_off
	    = (cu_offset) (sect_off - cu->header.sect_off);
	}
      else if (loc == NULL || origin != NULL || !loc->form_is_block ())
	{
	  complaint (_("No DW_FORM_block* DW_AT_location for "
		       "DW_TAG_call_site child DIE %s [in module %s]"),
		     sect_offset_str (child_die->sect_off), objfile_name (objfile));
	  continue;
	}
      else
	{
	  struct dwarf_block *block = loc->as_block ();

	  parameter->u.dwarf_reg = dwarf_block_to_dwarf_reg
	    (block->data, &block->data[block->size]);
	  if (parameter->u.dwarf_reg != -1)
	    parameter->kind = CALL_SITE_PARAMETER_DWARF_REG;
	  else if (dwarf_block_to_sp_offset (gdbarch, block->data,
				    &block->data[block->size],
					     &parameter->u.fb_offset))
	    parameter->kind = CALL_SITE_PARAMETER_FB_OFFSET;
	  else
	    {
	      complaint (_("Only single DW_OP_reg or DW_OP_fbreg is supported "
			   "for DW_FORM_block* DW_AT_location is supported for "
			   "DW_TAG_call_site child DIE %s "
			   "[in module %s]"),
			 sect_offset_str (child_die->sect_off),
			 objfile_name (objfile));
	      continue;
	    }
	}

      attr = dwarf2_attr (child_die, DW_AT_call_value, cu);
      if (attr == NULL)
	attr = dwarf2_attr (child_die, DW_AT_GNU_call_site_value, cu);
      if (attr == NULL || !attr->form_is_block ())
	{
	  complaint (_("No DW_FORM_block* DW_AT_call_value for "
		       "DW_TAG_call_site child DIE %s [in module %s]"),
		     sect_offset_str (child_die->sect_off),
		     objfile_name (objfile));
	  continue;
	}

      struct dwarf_block *block = attr->as_block ();
      parameter->value = block->data;
      parameter->value_size = block->size;

      /* Parameters are not pre-cleared by memset above.  */
      parameter->data_value = NULL;
      parameter->data_value_size = 0;
      call_site->parameter_count++;

      attr = dwarf2_attr (child_die, DW_AT_call_data_value, cu);
      if (attr == NULL)
	attr = dwarf2_attr (child_die, DW_AT_GNU_call_site_data_value, cu);
      if (attr != nullptr)
	{
	  if (!attr->form_is_block ())
	    complaint (_("No DW_FORM_block* DW_AT_call_data_value for "
			 "DW_TAG_call_site child DIE %s [in module %s]"),
		       sect_offset_str (child_die->sect_off),
		       objfile_name (objfile));
	  else
	    {
	      block = attr->as_block ();
	      parameter->data_value = block->data;
	      parameter->data_value_size = block->size;
	    }
	}
    }
}

/* Helper function for read_variable.  If DIE represents a virtual
   table, then return the type of the concrete object that is
   associated with the virtual table.  Otherwise, return NULL.  */

static struct type *
rust_containing_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr = dwarf2_attr (die, DW_AT_type, cu);
  if (attr == NULL)
    return NULL;

  /* Find the type DIE.  */
  struct die_info *type_die = NULL;
  struct dwarf2_cu *type_cu = cu;

  if (attr->form_is_ref ())
    type_die = follow_die_ref (die, attr, &type_cu);
  if (type_die == NULL)
    return NULL;

  if (dwarf2_attr (type_die, DW_AT_containing_type, type_cu) == NULL)
    return NULL;
  return die_containing_type (type_die, type_cu);
}

/* Read a variable (DW_TAG_variable) DIE and create a new symbol.  */

static void
read_variable (struct die_info *die, struct dwarf2_cu *cu)
{
  struct rust_vtable_symbol *storage = NULL;

  if (cu->lang () == language_rust)
    {
      struct type *containing_type = rust_containing_type (die, cu);

      if (containing_type != NULL)
	{
	  struct objfile *objfile = cu->per_objfile->objfile;

	  storage = new (&objfile->objfile_obstack) rust_vtable_symbol;
	  storage->concrete_type = containing_type;
	  storage->subclass = SYMBOL_RUST_VTABLE;
	}
    }

  struct symbol *res = new_symbol (die, NULL, cu, storage);
  struct attribute *abstract_origin
    = dwarf2_attr (die, DW_AT_abstract_origin, cu);
  struct attribute *loc = dwarf2_attr (die, DW_AT_location, cu);
  if (res == NULL && loc && abstract_origin)
    {
      /* We have a variable without a name, but with a location and an abstract
	 origin.  This may be a concrete instance of an abstract variable
	 referenced from an DW_OP_GNU_variable_value, so save it to find it back
	 later.  */
      struct dwarf2_cu *origin_cu = cu;
      struct die_info *origin_die
	= follow_die_ref (die, abstract_origin, &origin_cu);
      dwarf2_per_objfile *per_objfile = cu->per_objfile;
      per_objfile->per_bfd->abstract_to_concrete
	[origin_die->sect_off].push_back (die->sect_off);
    }
}

/* Call CALLBACK from DW_AT_ranges attribute value OFFSET
   reading .debug_rnglists.
   Callback's type should be:
    void (CORE_ADDR range_beginning, CORE_ADDR range_end)
   Return true if the attributes are present and valid, otherwise,
   return false.  */

template <typename Callback>
static bool
dwarf2_rnglists_process (unsigned offset, struct dwarf2_cu *cu,
			 dwarf_tag tag, Callback &&callback)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  bfd *obfd = objfile->obfd.get ();
  /* Base address selection entry.  */
  std::optional<unrelocated_addr> base;
  const gdb_byte *buffer;
  bool overflow = false;
  ULONGEST addr_index;
  struct dwarf2_section_info *rnglists_section;

  base = cu->base_address;
  rnglists_section = cu_debug_rnglists_section (cu, tag);
  rnglists_section->read (objfile);

  if (offset >= rnglists_section->size)
    {
      complaint (_("Offset %d out of bounds for DW_AT_ranges attribute"),
		 offset);
      return false;
    }
  buffer = rnglists_section->buffer + offset;

  while (1)
    {
      /* Initialize it due to a false compiler warning.  */
      unrelocated_addr range_beginning = {}, range_end = {};
      const gdb_byte *buf_end = (rnglists_section->buffer
				 + rnglists_section->size);
      unsigned int bytes_read;

      if (buffer == buf_end)
	{
	  overflow = true;
	  break;
	}
      const auto rlet = static_cast<enum dwarf_range_list_entry>(*buffer++);
      switch (rlet)
	{
	case DW_RLE_end_of_list:
	  break;
	case DW_RLE_base_address:
	  if (buffer + cu->header.addr_size > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  base = cu->header.read_address (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  break;
	case DW_RLE_base_addressx:
	  addr_index = read_unsigned_leb128 (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  base = read_addr_index (cu, addr_index);
	  break;
	case DW_RLE_start_length:
	  if (buffer + cu->header.addr_size > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_beginning = cu->header.read_address (obfd, buffer,
						     &bytes_read);
	  buffer += bytes_read;
	  range_end
	    = (unrelocated_addr) ((CORE_ADDR) range_beginning
				  + read_unsigned_leb128 (obfd, buffer,
							  &bytes_read));
	  buffer += bytes_read;
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  break;
	case DW_RLE_startx_length:
	  addr_index = read_unsigned_leb128 (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  range_beginning = read_addr_index (cu, addr_index);
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_end
	    = (unrelocated_addr) ((CORE_ADDR) range_beginning
				  + read_unsigned_leb128 (obfd, buffer,
							  &bytes_read));
	  buffer += bytes_read;
	  break;
	case DW_RLE_offset_pair:
	  range_beginning = (unrelocated_addr) read_unsigned_leb128 (obfd, buffer,
								     &bytes_read);
	  buffer += bytes_read;
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_end = (unrelocated_addr) read_unsigned_leb128 (obfd, buffer,
							       &bytes_read);
	  buffer += bytes_read;
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  break;
	case DW_RLE_start_end:
	  if (buffer + 2 * cu->header.addr_size > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_beginning = cu->header.read_address (obfd, buffer,
						     &bytes_read);
	  buffer += bytes_read;
	  range_end = cu->header.read_address (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  break;
	case DW_RLE_startx_endx:
	  addr_index = read_unsigned_leb128 (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  range_beginning = read_addr_index (cu, addr_index);
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  addr_index = read_unsigned_leb128 (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  range_end = read_addr_index (cu, addr_index);
	  break;
	default:
	  complaint (_("Invalid .debug_rnglists data (no base address)"));
	  return false;
	}
      if (rlet == DW_RLE_end_of_list || overflow)
	break;
      if (rlet == DW_RLE_base_address)
	continue;

      if (range_beginning > range_end)
	{
	  /* Inverted range entries are invalid.  */
	  complaint (_("Invalid .debug_rnglists data (inverted range)"));
	  return false;
	}

      /* Empty range entries have no effect.  */
      if (range_beginning == range_end)
	continue;

      /* Only DW_RLE_offset_pair needs the base address added.  */
      if (rlet == DW_RLE_offset_pair)
	{
	  if (!base.has_value ())
	    {
	      /* We have no valid base address for the DW_RLE_offset_pair.  */
	      complaint (_("Invalid .debug_rnglists data (no base address for "
			   "DW_RLE_offset_pair)"));
	      return false;
	    }

	  range_beginning = (unrelocated_addr) ((CORE_ADDR) range_beginning
						+ (CORE_ADDR) *base);
	  range_end = (unrelocated_addr) ((CORE_ADDR) range_end
					  + (CORE_ADDR) *base);
	}

      /* A not-uncommon case of bad debug info.
	 Don't pollute the addrmap with bad data.  */
      if (range_beginning == (unrelocated_addr) 0
	  && !per_objfile->per_bfd->has_section_at_zero)
	{
	  complaint (_(".debug_rnglists entry has start address of zero"
		       " [in module %s]"), objfile_name (objfile));
	  continue;
	}

      callback (range_beginning, range_end);
    }

  if (overflow)
    {
      complaint (_("Offset %d is not terminated "
		   "for DW_AT_ranges attribute"),
		 offset);
      return false;
    }

  return true;
}

/* Call CALLBACK from DW_AT_ranges attribute value OFFSET reading .debug_ranges.
   Callback's type should be:
    void (unrelocated_addr range_beginning, unrelocated_addr range_end)
   Return 1 if the attributes are present and valid, otherwise, return 0.  */

template <typename Callback>
static int
dwarf2_ranges_process (unsigned offset, struct dwarf2_cu *cu, dwarf_tag tag,
		       Callback &&callback)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  unit_head *cu_header = &cu->header;
  bfd *obfd = objfile->obfd.get ();
  unsigned int addr_size = cu_header->addr_size;
  CORE_ADDR mask = ~(~(CORE_ADDR)1 << (addr_size * 8 - 1));
  /* Base address selection entry.  */
  std::optional<unrelocated_addr> base;
  unsigned int dummy;
  const gdb_byte *buffer;

  if (cu_header->version >= 5)
    return dwarf2_rnglists_process (offset, cu, tag, callback);

  base = cu->base_address;

  per_objfile->per_bfd->ranges.read (objfile);
  if (offset >= per_objfile->per_bfd->ranges.size)
    {
      complaint (_("Offset %d out of bounds for DW_AT_ranges attribute"),
		 offset);
      return 0;
    }
  buffer = per_objfile->per_bfd->ranges.buffer + offset;

  while (1)
    {
      unrelocated_addr range_beginning, range_end;

      range_beginning = cu->header.read_address (obfd, buffer, &dummy);
      buffer += addr_size;
      range_end = cu->header.read_address (obfd, buffer, &dummy);
      buffer += addr_size;
      offset += 2 * addr_size;

      /* An end of list marker is a pair of zero addresses.  */
      if (range_beginning == (unrelocated_addr) 0
	  && range_end == (unrelocated_addr) 0)
	/* Found the end of list entry.  */
	break;

      /* Each base address selection entry is a pair of 2 values.
	 The first is the largest possible address, the second is
	 the base address.  Check for a base address here.  */
      if (((CORE_ADDR) range_beginning & mask) == mask)
	{
	  /* If we found the largest possible address, then we already
	     have the base address in range_end.  */
	  base = range_end;
	  continue;
	}

      if (!base.has_value ())
	{
	  /* We have no valid base address for the ranges
	     data.  */
	  complaint (_("Invalid .debug_ranges data (no base address)"));
	  return 0;
	}

      if (range_beginning > range_end)
	{
	  /* Inverted range entries are invalid.  */
	  complaint (_("Invalid .debug_ranges data (inverted range)"));
	  return 0;
	}

      /* Empty range entries have no effect.  */
      if (range_beginning == range_end)
	continue;

      range_beginning = (unrelocated_addr) ((CORE_ADDR) range_beginning
					    + (CORE_ADDR) *base);
      range_end = (unrelocated_addr) ((CORE_ADDR) range_end
				      + (CORE_ADDR) *base);

      /* A not-uncommon case of bad debug info.
	 Don't pollute the addrmap with bad data.  */
      if (range_beginning == (unrelocated_addr) 0
	  && !per_objfile->per_bfd->has_section_at_zero)
	{
	  complaint (_(".debug_ranges entry has start address of zero"
		       " [in module %s]"), objfile_name (objfile));
	  continue;
	}

      callback (range_beginning, range_end);
    }

  return 1;
}

/* See read.h.  */

int
dwarf2_ranges_read (unsigned offset, unrelocated_addr *low_return,
		    unrelocated_addr *high_return, struct dwarf2_cu *cu,
		    addrmap_mutable *map, void *datum, dwarf_tag tag)
{
  int low_set = 0;
  unrelocated_addr low = {};
  unrelocated_addr high = {};
  int retval;

  retval = dwarf2_ranges_process (offset, cu, tag,
    [&] (unrelocated_addr range_beginning, unrelocated_addr range_end)
    {
      if (map != nullptr)
	{
	  /* addrmap only accepts CORE_ADDR, so we must cast here.  */
	  map->set_empty ((CORE_ADDR) range_beginning,
			  (CORE_ADDR) range_end - 1,
			  datum);
	}

      /* FIXME: This is recording everything as a low-high
	 segment of consecutive addresses.  We should have a
	 data structure for discontiguous block ranges
	 instead.  */
      if (! low_set)
	{
	  low = range_beginning;
	  high = range_end;
	  low_set = 1;
	}
      else
	{
	  if (range_beginning < low)
	    low = range_beginning;
	  if (range_end > high)
	    high = range_end;
	}
    });
  if (!retval)
    return 0;

  if (! low_set)
    /* If the first entry is an end-of-list marker, the range
       describes an empty scope, i.e. no instructions.  */
    return 0;

  if (low_return)
    *low_return = low;
  if (high_return)
    *high_return = high;
  return 1;
}

/* Process ranges and fill in a vector of the low PC values only.  */

static void
dwarf2_ranges_read_low_addrs (unsigned offset, struct dwarf2_cu *cu,
			      dwarf_tag tag,
			      std::vector<unrelocated_addr> &result)
{
  dwarf2_ranges_process (offset, cu, tag,
			 [&] (unrelocated_addr start, unrelocated_addr end)
    {
      result.push_back (start);
    });
}

/* Determine the low and high pc of a DW_TAG_entry_point.  */

static pc_bounds_kind
dwarf2_get_pc_bounds_entry_point (die_info *die, unrelocated_addr *low,
				  unrelocated_addr *high, dwarf2_cu *cu)
{
  gdb_assert (low != nullptr);
  gdb_assert (high != nullptr);

  if (die->parent->tag != DW_TAG_subprogram)
    {
      complaint (_("DW_TAG_entry_point not embedded in DW_TAG_subprogram"));
      return PC_BOUNDS_INVALID;
    }

  /* A DW_TAG_entry_point is embedded in an subprogram.  Therefore, we can use
     the highpc from its enveloping subprogram and get the lowpc from
     DWARF.  */
  const enum pc_bounds_kind bounds_kind = dwarf2_get_pc_bounds (die->parent,
								low, high,
								cu, nullptr,
								nullptr);
  if (bounds_kind == PC_BOUNDS_INVALID || bounds_kind == PC_BOUNDS_NOT_PRESENT)
    return bounds_kind;

  attribute *attr_low = dwarf2_attr (die, DW_AT_low_pc, cu);
  if (!attr_low)
    {
      complaint (_("DW_TAG_entry_point is missing DW_AT_low_pc"));
      return PC_BOUNDS_INVALID;
    }
  *low = attr_low->as_address ();
  return bounds_kind;
}

/* Determine the low and high pc using the DW_AT_low_pc and DW_AT_high_pc or
   DW_AT_ranges attributes of a DIE.  */

static pc_bounds_kind
dwarf_get_pc_bounds_ranges_or_highlow_pc (die_info *die, unrelocated_addr *low,
					  unrelocated_addr *high, dwarf2_cu *cu,
					  addrmap_mutable *map, void *datum)
{
  gdb_assert (low != nullptr);
  gdb_assert (high != nullptr);

  struct attribute *attr;
  struct attribute *attr_high;
  enum pc_bounds_kind ret;

  attr_high = dwarf2_attr (die, DW_AT_high_pc, cu);
  if (attr_high)
    {
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
      if (attr != nullptr)
	{
	  *low = attr->as_address ();
	  *high = attr_high->as_address ();
	  if (cu->header.version >= 4 && attr_high->form_is_constant ())
	    *high = (unrelocated_addr) ((ULONGEST) *high + (ULONGEST) *low);

	  /* Found consecutive range of addresses.  */
	  ret = PC_BOUNDS_HIGH_LOW;
	}
      else
	{
	  /* Found high w/o low attribute.  */
	  ret = PC_BOUNDS_INVALID;
	}
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_ranges, cu);
      if (attr != nullptr && attr->form_is_unsigned ())
	{
	  /* Offset in the .debug_ranges or .debug_rnglist section (depending
	     on DWARF version).  */
	  ULONGEST ranges_offset = attr->as_unsigned ();

	  /* See dwarf2_cu::gnu_ranges_base's doc for why we might want to add
	     this value.  */
	  if (die->tag != DW_TAG_compile_unit)
	    ranges_offset += cu->gnu_ranges_base;

	  /* Value of the DW_AT_ranges attribute is the offset in the
	     .debug_ranges section.  */
	  if (!dwarf2_ranges_read (ranges_offset, low, high, cu,
				   map, datum, die->tag))
	    return PC_BOUNDS_INVALID;
	  /* Found discontinuous range of addresses.  */
	  ret = PC_BOUNDS_RANGES;
	}
      else
	{
	  /* Could not find high_pc or ranges attributed and thus no bounds
	     pair.  */
	  ret = PC_BOUNDS_NOT_PRESENT;
	}
    }

    return ret;
}

/* See read.h.  */

pc_bounds_kind
dwarf2_get_pc_bounds (struct die_info *die, unrelocated_addr *lowpc,
		      unrelocated_addr *highpc, struct dwarf2_cu *cu,
		      addrmap_mutable *map, void *datum)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  unrelocated_addr low = {};
  unrelocated_addr high = {};
  enum pc_bounds_kind ret;

  if (die->tag == DW_TAG_entry_point)
    ret = dwarf2_get_pc_bounds_entry_point (die, &low, &high, cu);
  else
    ret = dwarf_get_pc_bounds_ranges_or_highlow_pc (die, &low, &high, cu, map,
						    datum);

  if (ret == PC_BOUNDS_NOT_PRESENT || ret == PC_BOUNDS_INVALID)
    return ret;

  /* partial_die_info::read has also the strict LOW < HIGH requirement.  */
  if (high <= low)
    return PC_BOUNDS_INVALID;

  /* When using the GNU linker, .gnu.linkonce. sections are used to
     eliminate duplicate copies of functions and vtables and such.
     The linker will arbitrarily choose one and discard the others.
     The AT_*_pc values for such functions refer to local labels in
     these sections.  If the section from that file was discarded, the
     labels are not in the output, so the relocs get a value of 0.
     If this is a discarded function, mark the pc bounds as invalid,
     so that GDB will ignore it.  */
  if (low == (unrelocated_addr) 0
      && !per_objfile->per_bfd->has_section_at_zero)
    return PC_BOUNDS_INVALID;

  gdb_assert (lowpc != nullptr);
  *lowpc = low;
  if (highpc != nullptr)
    *highpc = high;
  return ret;
}

/* Assuming that DIE represents a subprogram DIE or a lexical block, get
   its low and high PC addresses.  Do nothing if these addresses could not
   be determined.  Otherwise, set LOWPC to the low address if it is smaller,
   and HIGHPC to the high address if greater than HIGHPC.  */

static void
dwarf2_get_subprogram_pc_bounds (struct die_info *die,
				 unrelocated_addr *lowpc,
				 unrelocated_addr *highpc,
				 struct dwarf2_cu *cu)
{
  unrelocated_addr low, high;

  if (dwarf2_get_pc_bounds (die, &low, &high, cu, nullptr, nullptr)
      >= PC_BOUNDS_RANGES)
    {
      *lowpc = std::min (*lowpc, low);
      *highpc = std::max (*highpc, high);
    }

  /* If the language does not allow nested subprograms (either inside
     subprograms or lexical blocks), we're done.  */
  if (cu->lang () != language_ada)
    return;

  /* Check all the children of the given DIE.  If it contains nested
     subprograms, then check their pc bounds.  Likewise, we need to
     check lexical blocks as well, as they may also contain subprogram
     definitions.  */
  for (die_info *child : die->children ())
    {
      if (child->tag == DW_TAG_subprogram
	  || child->tag == DW_TAG_lexical_block)
	dwarf2_get_subprogram_pc_bounds (child, lowpc, highpc, cu);
    }
}

/* Get the low and high pc's represented by the scope DIE, and store
   them in *LOWPC and *HIGHPC.  If the correct values can't be
   determined, set *LOWPC to -1 and *HIGHPC to 0.  */

static void
get_scope_pc_bounds (struct die_info *die,
		     unrelocated_addr *lowpc, unrelocated_addr *highpc,
		     struct dwarf2_cu *cu)
{
  unrelocated_addr best_low = (unrelocated_addr) -1;
  unrelocated_addr best_high = {};
  unrelocated_addr current_low, current_high;

  if (dwarf2_get_pc_bounds (die, &current_low, &current_high, cu,
			    nullptr, nullptr)
      >= PC_BOUNDS_RANGES)
    {
      best_low = current_low;
      best_high = current_high;
    }
  else
    {
      for (die_info *child : die->children ())
	{
	  switch (child->tag) {
	  case DW_TAG_subprogram:
	    dwarf2_get_subprogram_pc_bounds (child, &best_low, &best_high, cu);
	    break;
	  case DW_TAG_namespace:
	  case DW_TAG_module:
	    /* FIXME: carlton/2004-01-16: Should we do this for
	       DW_TAG_class_type/DW_TAG_structure_type, too?  I think
	       that current GCC's always emit the DIEs corresponding
	       to definitions of methods of classes as children of a
	       DW_TAG_compile_unit or DW_TAG_namespace (as opposed to
	       the DIEs giving the declarations, which could be
	       anywhere).  But I don't see any reason why the
	       standards says that they have to be there.  */
	    get_scope_pc_bounds (child, &current_low, &current_high, cu);

	    if (current_low != ((unrelocated_addr) -1))
	      {
		best_low = std::min (best_low, current_low);
		best_high = std::max (best_high, current_high);
	      }
	    break;
	  default:
	    /* Ignore.  */
	    break;
	  }
	}
    }

  *lowpc = best_low;
  *highpc = best_high;
}

/* Return the base address for DIE (which is represented by BLOCK) within
   CU.  The base address is the DW_AT_low_pc, or if that is not present,
   the first address in the first range defined by DW_AT_ranges.

   The DWARF standard actually says that if DIE has neither DW_AT_low_pc or
   DW_AT_ranges then we should search in the parent of DIE for those
   properties, and so on up the hierarchy, until we find a die with one of
   those attributes, and use that as the base address.  We don't implement
   that yet simply because we've never encountered a need for it.  */

static std::optional<CORE_ADDR>
dwarf2_die_base_address (struct die_info *die, struct block *block,
			 struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  struct attribute *attr = dwarf2_attr (die, DW_AT_low_pc, cu);
  if (attr != nullptr)
    {
      CORE_ADDR res = per_objfile->relocate (attr->as_address ());
      fixup_low_high_pc (cu, die, &res, nullptr);
      return res;
    }
  else if (block->ranges ().size () > 0)
    return block->ranges ()[0].start ();

  return {};
}

/* Return true if ADDR is within any of the ranges covered by BLOCK.  If
   there are no sub-ranges then just check against the block's start and
   end addresses, otherwise, check each sub-range covered by the block.  */

static bool
dwarf2_addr_in_block_ranges (CORE_ADDR addr, struct block *block)
{
  if (block->ranges ().size () == 0)
    return addr >= block->start () && addr < block->end ();

  /* Check if ADDR is within any of the block's sub-ranges.  */
  for (const blockrange &br : block->ranges ())
    {
      if (addr >= br.start () && addr < br.end ())
	return true;
    }

  /* ADDR is not within any of the block's sub-ranges.  */
  return false;
}


/* Set the entry PC for BLOCK which represents DIE from CU.  Relies on the
   range information (if present) already having been read from DIE and
   stored into BLOCK.  */

static void
dwarf2_record_block_entry_pc (struct die_info *die, struct block *block,
			      struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  /* Filled with the entry-pc if we can find it.  */
  std::optional<CORE_ADDR> entry;

  /* Set the block's entry PC where possible.  */
  struct attribute *attr = dwarf2_attr (die, DW_AT_entry_pc, cu);
  if (attr != nullptr)
    {
      /* DWARF-5 allows for the DW_AT_entry_pc to be an unsigned constant
	 offset from the containing DIE's base address.  We don't limit the
	 constant handling to DWARF-5 though.  If a broken compiler emits
	 this for DWARF-4 then we handle it just as we would for DWARF-5.  */
      if (attr->form_is_constant ())
	{
	  if (attr->form_is_unsigned ())
	    {
	      CORE_ADDR offset = attr->as_unsigned ();

	      std::optional<CORE_ADDR> base
		= dwarf2_die_base_address (die, block, cu);

	      if (base.has_value ())
		entry.emplace (base.value () + offset);
	    }
	  else
	    {
	      /* We could possibly handle signed constants, but this is out
		 of spec, so for now, just complain and ignore it.  */
	      complaint (_("Invalid form for DW_AT_entry_pc: %s"),
			 dwarf_form_name (attr->form));
	    }
	}
      else
	entry.emplace (per_objfile->relocate (attr->as_address ()));
    }
  else
    entry = dwarf2_die_base_address (die, block, cu);

  if (entry.has_value ())
    {
      CORE_ADDR entry_pc = entry.value ();

      /* Some compilers (e.g. GCC) will have the DW_AT_entry_pc point at an
	 empty sub-range, which by a strict reading of the DWARF means that
	 the entry-pc is outside the blocks code range.  If we continue
	 using this address then GDB will confuse itself, breakpoints will
	 be placed at the entry-pc, but once stopped there, GDB will not
	 recognise that it is inside this block.

	 To avoid this, ignore entry-pc values that are outside the block's
	 range, GDB will then select a suitable default entry-pc.  */
      if (dwarf2_addr_in_block_ranges (entry_pc, block))
	block->set_entry_pc (entry_pc);
      else
	complaint (_("in %s, DIE %s, DW_AT_entry_pc (%s) outside "
		     "block range (%s -> %s)"),
		   objfile_name (per_objfile->objfile),
		   sect_offset_str (die->sect_off),
		   paddress (per_objfile->objfile->arch (), entry_pc),
		   paddress (per_objfile->objfile->arch (), block->start ()),
		   paddress (per_objfile->objfile->arch (), block->end ()));
    }
}

/* Record the address ranges for BLOCK, offset by BASEADDR, as given
   in DIE.  Also set the entry PC for BLOCK.  */

static void
dwarf2_record_block_ranges (struct die_info *die, struct block *block,
			    struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct attribute *attr;
  struct attribute *attr_high;

  /* Like dwarf_get_pc_bounds_ranges_or_highlow_pc, we read either the
     low/high pc attributes, OR the ranges attribute, but not both.  If we
     parse both here then we open up the possibility that, due to invalid
     DWARF, a block's start() and end() might not contain all of the ranges.

     We have seen this in the wild with older (pre v9) versions of GCC.  In
     this case a GCC bug meant that a DIE was linked via DW_AT_abstract_origin
     to the wrong DIE.  Instead of pointing at the abstract DIE, GCC was
     linking one instance DIE to an earlier instance DIE.  The first instance
     DIE had low/high pc attributes, while the second instance DIE had a
     ranges attribute.  When processing the incorrectly linked instance GDB
     would see a DIE with both a low/high pc and some ranges data.  However,
     the ranges data was all outside the low/high range, which would trigger
     asserts when setting the entry-pc.  */

  attr_high = dwarf2_attr (die, DW_AT_high_pc, cu);
  if (attr_high)
    {
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
      if (attr != nullptr)
	{
	  unrelocated_addr unrel_low = attr->as_address ();
	  unrelocated_addr unrel_high = attr_high->as_address ();

	  if (cu->header.version >= 4 && attr_high->form_is_constant ())
	    unrel_high = (unrelocated_addr) ((ULONGEST) unrel_high
					     + (ULONGEST) unrel_low);

	  CORE_ADDR low = per_objfile->relocate (unrel_low);
	  CORE_ADDR high = per_objfile->relocate (unrel_high);
	  fixup_low_high_pc (cu, die, &low, &high);
	  cu->get_builder ()->record_block_range (block, low, high - 1);
	}

      attr = dwarf2_attr (die, DW_AT_ranges, cu);
      if (attr != nullptr && attr->form_is_unsigned ())
	complaint (_("in %s, DIE %s, DW_AT_ranges ignored due to DW_AT_low_pc"),
		   objfile_name (per_objfile->objfile),
		   sect_offset_str (die->sect_off));
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_ranges, cu);
      if (attr != nullptr && attr->form_is_unsigned ())
	{
	  /* Offset in the .debug_ranges or .debug_rnglist section (depending
	     on DWARF version).  */
	  ULONGEST ranges_offset = attr->as_unsigned ();

	  /* See dwarf2_cu::gnu_ranges_base's doc for why we might want to add
	     this value.  */
	  if (die->tag != DW_TAG_compile_unit)
	    ranges_offset += cu->gnu_ranges_base;

	  std::vector<blockrange> blockvec;
	  dwarf2_ranges_process (ranges_offset, cu, die->tag,
				 [&] (unrelocated_addr start,
				      unrelocated_addr end)
	  {
	    CORE_ADDR abs_start = per_objfile->relocate (start);
	    CORE_ADDR abs_end = per_objfile->relocate (end);
	    cu->get_builder ()->record_block_range (block, abs_start,
						    abs_end - 1);
	    blockvec.emplace_back (abs_start, abs_end);
	  });

	  block->set_ranges (make_blockranges (objfile, blockvec));
	}
    }

  dwarf2_record_block_entry_pc (die, block, cu);
}

/* Return the accessibility of DIE, as given by DW_AT_accessibility.
   If that attribute is not available, return the appropriate
   default.  */

static enum dwarf_access_attribute
dwarf2_access_attribute (struct die_info *die, struct dwarf2_cu *cu)
{
  attribute *attr = dwarf2_attr (die, DW_AT_accessibility, cu);
  if (attr != nullptr)
    {
      std::optional<ULONGEST> value = attr->unsigned_constant ();
      if (value.has_value ())
	{
	  if (*value == DW_ACCESS_public
	      || *value == DW_ACCESS_protected
	      || *value == DW_ACCESS_private)
	    return (dwarf_access_attribute) *value;
	  complaint (_("Unhandled DW_AT_accessibility value (%s)"),
		     pulongest (*value));
	}
    }

  if (cu->header.version < 3 || cu->producer_is_gxx_lt_4_6 ())
    {
      /* The default DWARF 2 accessibility for members is public, the default
	 accessibility for inheritance is private.  */

      if (die->tag != DW_TAG_inheritance)
	return DW_ACCESS_public;
      else
	return DW_ACCESS_private;
    }
  else
    {
      /* DWARF 3+ defines the default accessibility a different way.  The same
	 rules apply now for DW_TAG_inheritance as for the members and it only
	 depends on the container kind.  */

      if (die->parent->tag == DW_TAG_class_type)
	return DW_ACCESS_private;
      else
	return DW_ACCESS_public;
    }
}

/* Look for DW_AT_data_member_location or DW_AT_data_bit_offset and
   store the results in FIELD.  */

static void
handle_member_location (struct die_info *die, struct dwarf2_cu *cu,
			struct field *field)
{
  const auto data_member_location_attr
    = dwarf2_attr (die, DW_AT_data_member_location, cu);

  if (data_member_location_attr != nullptr)
    {
      bool has_bit_offset = false;
      LONGEST bit_offset = 0;
      LONGEST anonymous_size = 0;
      const auto bit_offset_attr = dwarf2_attr (die, DW_AT_bit_offset, cu);

      if (bit_offset_attr != nullptr && bit_offset_attr->form_is_constant ())
	{
	  has_bit_offset = true;
	  bit_offset = bit_offset_attr->confused_constant ().value_or (0);

	  const auto byte_size_attr = dwarf2_attr (die, DW_AT_byte_size, cu);

	  if (byte_size_attr != nullptr && byte_size_attr->form_is_constant ())
	    {
	      /* The size of the anonymous object containing
		 the bit field is explicit, so use the
		 indicated size (in bytes).  */
	      anonymous_size
		= byte_size_attr->unsigned_constant ().value_or (0);
	    }
	}

      if (data_member_location_attr->form_is_constant ())
	{
	  LONGEST offset
	    = data_member_location_attr->confused_constant ().value_or (0);

	  /* Work around this GCC 11 bug, where it would erroneously use -1
	     data member locations, instead of 0:

	       Negative DW_AT_data_member_location
	       https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101378
	     */
	  if (offset == -1 && cu->producer_is_gcc_11 ())
	    {
	      complaint (_("DW_AT_data_member_location value of -1, assuming 0"));
	      offset = 0;
	    }

	  field->set_loc_bitpos (offset * bits_per_byte);
	  if (has_bit_offset)
	    apply_bit_offset_to_field (*field, bit_offset, anonymous_size);
	}
      else if (data_member_location_attr->form_is_block ())
	{
	  CORE_ADDR offset;
	  if (decode_locdesc (data_member_location_attr->as_block (), cu,
			      &offset))
	    {
	      field->set_loc_bitpos (offset * bits_per_byte);

	      if (has_bit_offset)
		apply_bit_offset_to_field (*field, bit_offset, anonymous_size);
	    }
	  else
	    {
	      dwarf2_per_objfile *per_objfile = cu->per_objfile;
	      struct objfile *objfile = per_objfile->objfile;
	      struct dwarf2_locexpr_baton *dlbaton;
	      if (has_bit_offset)
		{
		  dwarf2_field_location_baton *flbaton
		    = OBSTACK_ZALLOC (&objfile->objfile_obstack,
				      dwarf2_field_location_baton);
		  flbaton->is_field_location = true;
		  flbaton->bit_offset = bit_offset;
		  flbaton->explicit_byte_size = anonymous_size;
		  dlbaton = flbaton;
		}
	      else
		dlbaton = OBSTACK_ZALLOC (&objfile->objfile_obstack,
					  struct dwarf2_locexpr_baton);
	      dlbaton->data = data_member_location_attr->as_block ()->data;
	      dlbaton->size = data_member_location_attr->as_block ()->size;
	      /* When using this baton, we want to compute the address
		 of the field, not the value.  This is why
		 is_reference is set to false here.  */
	      dlbaton->is_reference = false;
	      dlbaton->per_objfile = per_objfile;
	      dlbaton->per_cu = cu->per_cu;

	      field->set_loc_dwarf_block_addr (dlbaton);
	    }
	}
      else
	complaint (_("Unsupported form %s for DW_AT_data_member_location"),
		   dwarf_form_name (data_member_location_attr->form));
    }
  else
    {
      const auto data_bit_offset_attr
	= dwarf2_attr (die, DW_AT_data_bit_offset, cu);

      if (data_bit_offset_attr != nullptr)
	{
	  if (data_bit_offset_attr->form_is_constant ())
	    field->set_loc_bitpos (data_bit_offset_attr->unsigned_constant ()
				   .value_or (0));
	  else if (data_bit_offset_attr->form_is_block ())
	    {
	      /* This is a DWARF extension.  See
		 https://dwarfstd.org/issues/250501.1.html.  */
	      dwarf2_per_objfile *per_objfile = cu->per_objfile;
	      dwarf2_locexpr_baton *dlbaton
		= OBSTACK_ZALLOC (&per_objfile->objfile->objfile_obstack,
				  dwarf2_locexpr_baton);
	      dlbaton->data = data_bit_offset_attr->as_block ()->data;
	      dlbaton->size = data_bit_offset_attr->as_block ()->size;
	      dlbaton->per_objfile = per_objfile;
	      dlbaton->per_cu = cu->per_cu;

	      field->set_loc_dwarf_block_bitpos (dlbaton);
	    }
	  else
	    complaint (_("Unsupported form %s for DW_AT_data_bit_offset"),
		       dwarf_form_name (data_bit_offset_attr->form));
	}
    }
}

/* A helper that computes the location of a field.  The CU and the
   DW_TAG_member DIE are passed in.  The results are stored in
   *FP.  */

static void
compute_field_location (dwarf2_cu *cu, die_info *die, field *fp)
{
  /* Get type of field.  */
  fp->set_type (die_type (die, cu));

  fp->set_loc_bitpos (0);

  /* Get bit size of field (zero if none).  */
  attribute *attr = dwarf2_attr (die, DW_AT_bit_size, cu);
  if (attr != nullptr)
    fp->set_bitsize (attr->unsigned_constant ().value_or (0));
  else
    fp->set_bitsize (0);

  /* Get bit offset of field.  */
  handle_member_location (die, cu, fp);
}

/* Add an aggregate field to the field list.  */

static void
dwarf2_add_field (struct field_info *fip, struct die_info *die,
		  struct dwarf2_cu *cu)
{
  struct nextfield *new_field;
  struct attribute *attr;
  struct field *fp;
  const char *fieldname = "";

  if (die->tag == DW_TAG_inheritance)
    new_field = &fip->baseclasses.emplace_back ();
  else
    new_field = &fip->fields.emplace_back ();

  new_field->offset = die->sect_off;

  switch (dwarf2_access_attribute (die, cu))
    {
    case DW_ACCESS_public:
      break;
    case DW_ACCESS_private:
      new_field->field.set_accessibility (accessibility::PRIVATE);
      break;
    case DW_ACCESS_protected:
      new_field->field.set_accessibility (accessibility::PROTECTED);
      break;
    default:
      gdb_assert_not_reached ("invalid accessibility");
    }

  attr = dwarf2_attr (die, DW_AT_virtuality, cu);
  if (attr != nullptr && attr->as_virtuality ())
    new_field->field.set_virtual ();

  fp = &new_field->field;

  if ((die->tag == DW_TAG_member || die->tag == DW_TAG_namelist_item)
      && !die_is_declaration (die, cu))
    {
      if (die->tag == DW_TAG_namelist_item)
	{
	  /* Typically, DW_TAG_namelist_item are references to namelist items.
	     If so, follow that reference.  */
	  struct attribute *attr1 = dwarf2_attr (die, DW_AT_namelist_item, cu);
	  struct die_info *item_die = nullptr;
	  struct dwarf2_cu *item_cu = cu;
	  if (attr1->form_is_ref ())
	    item_die = follow_die_ref (die, attr1, &item_cu);
	  if (item_die != nullptr)
	    die = item_die;
	}
      /* Data member other than a C++ static data member.  */

      compute_field_location (cu, die, fp);

      /* Get name of field.  */
      fieldname = dwarf2_name (die, cu);
      if (fieldname == NULL)
	fieldname = "";

      /* The name is already allocated along with this objfile, so we don't
	 need to duplicate it for the type.  */
      fp->set_name (fieldname);

      /* Change accessibility for artificial fields (e.g. virtual table
	 pointer or virtual base class pointer) to private.  */
      if (dwarf2_attr (die, DW_AT_artificial, cu))
	{
	  fp->set_is_artificial (true);
	  fp->set_accessibility (accessibility::PRIVATE);
	}
    }
  else if (die->tag == DW_TAG_member || die->tag == DW_TAG_variable)
    {
      /* C++ static member.  */

      /* NOTE: carlton/2002-11-05: It should be a DW_TAG_member that
	 is a declaration, but all versions of G++ as of this writing
	 (so through at least 3.2.1) incorrectly generate
	 DW_TAG_variable tags.  */

      const char *physname;

      /* Get name of field.  */
      fieldname = dwarf2_name (die, cu);
      if (fieldname == NULL)
	return;

      attr = dwarf2_attr (die, DW_AT_const_value, cu);
      if (attr
	  /* Only create a symbol if this is an external value.
	     new_symbol checks this and puts the value in the global symbol
	     table, which we want.  If it is not external, new_symbol
	     will try to put the value in cu->list_in_scope which is wrong.  */
	  && dwarf2_flag_true_p (die, DW_AT_external, cu))
	{
	  /* A static const member, not much different than an enum as far as
	     we're concerned, except that we can support more types.  */
	  new_symbol (die, NULL, cu);
	}

      /* Get physical name.  */
      physname = dwarf2_physname (fieldname, die, cu);

      /* The name is already allocated along with this objfile, so we don't
	 need to duplicate it for the type.  */
      fp->set_loc_physname (physname ? physname : "");
      fp->set_type (die_type (die, cu));
      fp->set_name (fieldname);
    }
  else if (die->tag == DW_TAG_inheritance)
    {
      /* C++ base class field.  */
      handle_member_location (die, cu, fp);
      fp->set_bitsize (0);
      fp->set_type (die_type (die, cu));
      fp->set_name (fp->type ()->name ());
    }
  else
    gdb_assert_not_reached ("missing case in dwarf2_add_field");
}

/* Can the type given by DIE define another type?  */

static bool
type_can_define_types (const struct die_info *die)
{
  switch (die->tag)
    {
    case DW_TAG_typedef:
    case DW_TAG_class_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_enumeration_type:
      return true;

    default:
      return false;
    }
}

/* Add a type definition defined in the scope of the FIP's class.  */

static void
dwarf2_add_type_defn (struct field_info *fip, struct die_info *die,
		      struct dwarf2_cu *cu)
{
  struct decl_field fp;
  memset (&fp, 0, sizeof (fp));

  gdb_assert (type_can_define_types (die));

  /* Get name of field.  NULL is okay here, meaning an anonymous type.  */
  fp.name = dwarf2_name (die, cu);
  fp.type = read_type_die (die, cu);

  /* Save accessibility.  */
  dwarf_access_attribute accessibility = dwarf2_access_attribute (die, cu);
  switch (accessibility)
    {
    case DW_ACCESS_public:
      /* The assumed value if neither private nor protected.  */
      break;
    case DW_ACCESS_private:
      fp.accessibility = accessibility::PRIVATE;
      break;
    case DW_ACCESS_protected:
      fp.accessibility = accessibility::PROTECTED;
      break;
    }

  if (die->tag == DW_TAG_typedef)
    fip->typedef_field_list.push_back (fp);
  else
    fip->nested_types_list.push_back (fp);
}

/* A convenience typedef that's used when finding the discriminant
   field for a variant part.  */
using offset_map_type = gdb::unordered_map<sect_offset, int>;

/* Compute the discriminant range for a given variant.  OBSTACK is
   where the results will be stored.  VARIANT is the variant to
   process.  IS_UNSIGNED indicates whether the discriminant is signed
   or unsigned.  */

static const gdb::array_view<discriminant_range>
convert_variant_range (struct obstack *obstack, const variant_field &variant,
		       bool is_unsigned)
{
  std::vector<discriminant_range> ranges;

  if (variant.is_default ())
    return {};

  if (variant.discr_list_data == nullptr)
    {
      ULONGEST value;

      if (is_unsigned)
	value = variant.discriminant_attr->unsigned_constant ().value_or (0);
      else
	value = variant.discriminant_attr->signed_constant ().value_or (0);

      discriminant_range r = { value, value };
      ranges.push_back (r);
    }
  else
    {
      gdb::array_view<const gdb_byte> data (variant.discr_list_data->data,
					    variant.discr_list_data->size);
      while (!data.empty ())
	{
	  if (data[0] != DW_DSC_range && data[0] != DW_DSC_label)
	    {
	      complaint (_("invalid discriminant marker: %d"), data[0]);
	      break;
	    }
	  bool is_range = data[0] == DW_DSC_range;
	  data = data.slice (1);

	  ULONGEST low, high;
	  unsigned int bytes_read;

	  if (data.empty ())
	    {
	      complaint (_("DW_AT_discr_list missing low value"));
	      break;
	    }
	  if (is_unsigned)
	    low = read_unsigned_leb128 (nullptr, data.data (), &bytes_read);
	  else
	    low = (ULONGEST) read_signed_leb128 (nullptr, data.data (),
						 &bytes_read);
	  data = data.slice (bytes_read);

	  if (is_range)
	    {
	      if (data.empty ())
		{
		  complaint (_("DW_AT_discr_list missing high value"));
		  break;
		}
	      if (is_unsigned)
		high = read_unsigned_leb128 (nullptr, data.data (),
					     &bytes_read);
	      else
		high = (LONGEST) read_signed_leb128 (nullptr, data.data (),
						     &bytes_read);
	      data = data.slice (bytes_read);
	    }
	  else
	    high = low;

	  ranges.push_back ({ low, high });
	}
    }

  discriminant_range *result = XOBNEWVEC (obstack, discriminant_range,
					  ranges.size ());
  std::copy (ranges.begin (), ranges.end (), result);
  return gdb::array_view<discriminant_range> (result, ranges.size ());
}

static const gdb::array_view<variant_part> create_variant_parts
  (struct obstack *obstack,
   const offset_map_type &offset_map,
   struct field_info *fi,
   const std::vector<variant_part_builder> &variant_parts);

/* Fill in a "struct variant" for a given variant field.  RESULT is
   the variant to fill in.  OBSTACK is where any needed allocations
   will be done.  OFFSET_MAP holds the mapping from section offsets to
   fields for the type.  FI describes the fields of the type we're
   processing.  FIELD is the variant field we're converting.  */

static void
create_one_variant (variant &result, struct obstack *obstack,
		    const offset_map_type &offset_map,
		    struct field_info *fi, const variant_field &field)
{
  result.discriminants = convert_variant_range (obstack, field, false);
  result.first_field = field.first_field + fi->baseclasses.size ();
  result.last_field = field.last_field + fi->baseclasses.size ();
  result.parts = create_variant_parts (obstack, offset_map, fi,
				       field.variant_parts);
}

/* Fill in a "struct variant_part" for a given variant part.  RESULT
   is the variant part to fill in.  OBSTACK is where any needed
   allocations will be done.  OFFSET_MAP holds the mapping from
   section offsets to fields for the type.  FI describes the fields of
   the type we're processing.  BUILDER is the variant part to be
   converted.  */

static void
create_one_variant_part (variant_part &result,
			 struct obstack *obstack,
			 const offset_map_type &offset_map,
			 struct field_info *fi,
			 const variant_part_builder &builder)
{
  auto iter = offset_map.find (builder.discriminant_offset);
  if (iter == offset_map.end ())
    {
      result.discriminant_index = -1;
      /* Doesn't matter.  */
      result.is_unsigned = false;
    }
  else
    {
      result.discriminant_index = iter->second;
      result.is_unsigned
	= fi->fields[result.discriminant_index].field.type ()->is_unsigned ();
    }

  size_t n = builder.variants.size ();
  variant *output = new (obstack) variant[n];
  for (size_t i = 0; i < n; ++i)
    create_one_variant (output[i], obstack, offset_map, fi,
			builder.variants[i]);

  result.variants = gdb::array_view<variant> (output, n);
}

/* Create a vector of variant parts that can be attached to a type.
   OBSTACK is where any needed allocations will be done.  OFFSET_MAP
   holds the mapping from section offsets to fields for the type.  FI
   describes the fields of the type we're processing.  VARIANT_PARTS
   is the vector to convert.  */

static const gdb::array_view<variant_part>
create_variant_parts (struct obstack *obstack,
		      const offset_map_type &offset_map,
		      struct field_info *fi,
		      const std::vector<variant_part_builder> &variant_parts)
{
  if (variant_parts.empty ())
    return {};

  size_t n = variant_parts.size ();
  variant_part *result = new (obstack) variant_part[n];
  for (size_t i = 0; i < n; ++i)
    create_one_variant_part (result[i], obstack, offset_map, fi,
			     variant_parts[i]);

  return gdb::array_view<variant_part> (result, n);
}

/* Compute the variant part vector for FIP, attaching it to TYPE when
   done.  */

static void
add_variant_property (struct field_info *fip, struct type *type,
		      struct dwarf2_cu *cu)
{
  /* Map section offsets of fields to their field index.  Note the
     field index here does not take the number of baseclasses into
     account.  */
  offset_map_type offset_map;
  for (int i = 0; i < fip->fields.size (); ++i)
    offset_map[fip->fields[i].offset] = i;

  struct objfile *objfile = cu->per_objfile->objfile;
  gdb::array_view<const variant_part> parts
    = create_variant_parts (&objfile->objfile_obstack, offset_map, fip,
			    fip->variant_parts);

  struct dynamic_prop prop;
  prop.set_variant_parts ((gdb::array_view<variant_part> *)
			  obstack_copy (&objfile->objfile_obstack, &parts,
					sizeof (parts)));

  type->add_dyn_prop (DYN_PROP_VARIANT_PARTS, prop);
}

/* Create the vector of fields, and attach it to the type.  */

static void
dwarf2_attach_fields_to_type (struct field_info *fip, struct type *type,
			      struct dwarf2_cu *cu)
{
  int nfields = fip->nfields ();

  /* Record the field count, allocate space for the array of fields,
     and create blank accessibility bitfields if necessary.  */
  type->alloc_fields (nfields);

  if (!fip->baseclasses.empty () && cu->lang () != language_ada)
    {
      ALLOCATE_CPLUS_STRUCT_TYPE (type);
      TYPE_N_BASECLASSES (type) = fip->baseclasses.size ();
    }

  if (!fip->variant_parts.empty ())
    add_variant_property (fip, type, cu);

  /* Copy the saved-up fields into the field vector.  */
  for (int i = 0; i < nfields; ++i)
    {
      struct nextfield &field
	= ((i < fip->baseclasses.size ()) ? fip->baseclasses[i]
	   : fip->fields[i - fip->baseclasses.size ()]);

      type->field (i) = field.field;
    }
}

/* Return true if this member function is a constructor, false
   otherwise.  */

static int
dwarf2_is_constructor (struct die_info *die, struct dwarf2_cu *cu)
{
  const char *fieldname;
  const char *type_name;
  int len;

  if (die->parent == NULL)
    return 0;

  if (die->parent->tag != DW_TAG_structure_type
      && die->parent->tag != DW_TAG_union_type
      && die->parent->tag != DW_TAG_class_type)
    return 0;

  fieldname = dwarf2_name (die, cu);
  type_name = dwarf2_name (die->parent, cu);
  if (fieldname == NULL || type_name == NULL)
    return 0;

  len = strlen (fieldname);
  return (strncmp (fieldname, type_name, len) == 0
	  && (type_name[len] == '\0' || type_name[len] == '<'));
}

/* Add a member function to the proper fieldlist.  */

static void
dwarf2_add_member_fn (struct field_info *fip, struct die_info *die,
		      struct type *type, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct attribute *attr;
  int i;
  struct fnfieldlist *flp = nullptr;
  struct fn_field *fnp;
  const char *fieldname;
  struct type *this_type;

  if (cu->lang () == language_ada)
    error (_("unexpected member function in Ada type"));

  /* Get name of member function.  */
  fieldname = dwarf2_name (die, cu);
  if (fieldname == NULL)
    return;

  /* Look up member function name in fieldlist.  */
  for (i = 0; i < fip->fnfieldlists.size (); i++)
    {
      if (strcmp (fip->fnfieldlists[i].name, fieldname) == 0)
	{
	  flp = &fip->fnfieldlists[i];
	  break;
	}
    }

  /* Create a new fnfieldlist if necessary.  */
  if (flp == nullptr)
    {
      flp = &fip->fnfieldlists.emplace_back ();
      flp->name = fieldname;
      i = fip->fnfieldlists.size () - 1;
    }

  /* Create a new member function field and add it to the vector of
     fnfieldlists.  */
  fnp = &flp->fnfields.emplace_back ();

  /* Delay processing of the physname until later.  */
  if (cu->lang () == language_cplus)
    add_to_method_list (type, i, flp->fnfields.size () - 1, fieldname,
			die, cu);
  else
    {
      const char *physname = dwarf2_physname (fieldname, die, cu);
      fnp->physname = physname ? physname : "";
    }

  fnp->type = type_allocator (objfile, cu->lang ()).new_type ();
  this_type = read_type_die (die, cu);
  if (this_type && this_type->code () == TYPE_CODE_FUNC)
    {
      int nparams = this_type->num_fields ();

      /* TYPE is the domain of this method, and THIS_TYPE is the type
	   of the method itself (TYPE_CODE_METHOD).  */
      smash_to_method_type (fnp->type, type,
			    this_type->target_type (),
			    this_type->fields (),
			    this_type->has_varargs ());

      /* Handle static member functions.
	 Dwarf2 has no clean way to discern C++ static and non-static
	 member functions.  G++ helps GDB by marking the first
	 parameter for non-static member functions (which is the this
	 pointer) as artificial.  We obtain this information from
	 read_subroutine_type via TYPE_FIELD_ARTIFICIAL.  */
      if (nparams == 0 || this_type->field (0).is_artificial () == 0)
	fnp->voffset = VOFFSET_STATIC;
    }
  else
    complaint (_("member function type missing for '%s'"),
	       dwarf2_full_name (fieldname, die, cu));

  /* Get fcontext from DW_AT_containing_type if present.  */
  if (dwarf2_attr (die, DW_AT_containing_type, cu) != NULL)
    fnp->fcontext = die_containing_type (die, cu);

  /* dwarf2 doesn't have stubbed physical names, so the setting of is_const and
     is_volatile is irrelevant, as it is needed by gdb_mangle_name only.  */

  /* Get accessibility.  */
  dwarf_access_attribute accessibility = dwarf2_access_attribute (die, cu);
  switch (accessibility)
    {
    case DW_ACCESS_private:
      fnp->accessibility = accessibility::PRIVATE;
      break;
    case DW_ACCESS_protected:
      fnp->accessibility = accessibility::PROTECTED;
      break;
    }

  /* Check for artificial methods.  */
  attr = dwarf2_attr (die, DW_AT_artificial, cu);
  if (attr && attr->as_boolean ())
    fnp->is_artificial = 1;

  /* Check for defaulted methods.  */
  attr = dwarf2_attr (die, DW_AT_defaulted, cu);
  if (attr != nullptr)
    fnp->defaulted = attr->defaulted ();

  /* Check for deleted methods.  */
  attr = dwarf2_attr (die, DW_AT_deleted, cu);
  if (attr != nullptr && attr->as_boolean ())
    fnp->is_deleted = 1;

  fnp->is_constructor = dwarf2_is_constructor (die, cu);

  /* Get index in virtual function table if it is a virtual member
     function.  For older versions of GCC, this is an offset in the
     appropriate virtual table, as specified by DW_AT_containing_type.
     For everyone else, it is an expression to be evaluated relative
     to the object address.  */

  attr = dwarf2_attr (die, DW_AT_vtable_elem_location, cu);
  if (attr != nullptr)
    {
      if (attr->form_is_block () && attr->as_block ()->size > 0)
	{
	  struct dwarf_block *block = attr->as_block ();
	  CORE_ADDR offset;

	  if (block->data[0] == DW_OP_constu
	      && decode_locdesc (block, cu, &offset))
	    {
	      /* "Old"-style GCC.  See
		 https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44126
		 for discussion.  This was known and a patch available
		 in 2010, but as of 2023, both GCC and clang still
		 emit this.  */
	      fnp->voffset = offset + 2;
	    }
	  else if ((block->data[0] == DW_OP_deref
		    || (block->size > 1
			&& block->data[0] == DW_OP_deref_size
			&& block->data[1] == cu->header.addr_size))
		   && decode_locdesc (block, cu, &offset))
	    {
	      fnp->voffset = offset;
	      if ((fnp->voffset % cu->header.addr_size) != 0)
		dwarf2_complex_location_expr_complaint ();
	      else
		fnp->voffset /= cu->header.addr_size;
	      fnp->voffset += 2;
	    }
	  else
	    dwarf2_complex_location_expr_complaint ();

	  if (!fnp->fcontext)
	    {
	      /* If there is no `this' field and no DW_AT_containing_type,
		 we cannot actually find a base class context for the
		 vtable!  */
	      if (this_type->num_fields () == 0
		  || !this_type->field (0).is_artificial ())
		{
		  complaint (_("cannot determine context for virtual member "
			       "function \"%s\" (offset %s)"),
			     fieldname, sect_offset_str (die->sect_off));
		}
	      else
		{
		  fnp->fcontext = this_type->field (0).type ()->target_type ();
		}
	    }
	}
      else if (attr->form_is_section_offset ())
	{
	  dwarf2_complex_location_expr_complaint ();
	}
      else
	{
	  dwarf2_invalid_attrib_class_complaint ("DW_AT_vtable_elem_location",
						 fieldname);
	}
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_virtuality, cu);
      if (attr != nullptr && attr->as_virtuality () != DW_VIRTUALITY_none)
	{
	  /* GCC does this, as of 2008-08-25; PR debug/37237.  */
	  complaint (_("Member function \"%s\" (offset %s) is virtual "
		       "but the vtable offset is not specified"),
		     fieldname, sect_offset_str (die->sect_off));
	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  TYPE_CPLUS_DYNAMIC (type) = 1;
	}
    }
}

/* Create the vector of member function fields, and attach it to the type.  */

static void
dwarf2_attach_fn_fields_to_type (struct field_info *fip, struct type *type,
				 struct dwarf2_cu *cu)
{
  if (cu->lang () == language_ada)
    error (_("unexpected member functions in Ada type"));

  ALLOCATE_CPLUS_STRUCT_TYPE (type);
  TYPE_FN_FIELDLISTS (type) = (struct fn_fieldlist *)
    TYPE_ZALLOC (type,
		 sizeof (struct fn_fieldlist) * fip->fnfieldlists.size ());

  for (int i = 0; i < fip->fnfieldlists.size (); i++)
    {
      struct fnfieldlist &nf = fip->fnfieldlists[i];
      struct fn_fieldlist *fn_flp = &TYPE_FN_FIELDLIST (type, i);

      TYPE_FN_FIELDLIST_NAME (type, i) = nf.name;
      TYPE_FN_FIELDLIST_LENGTH (type, i) = nf.fnfields.size ();
      /* No need to zero-initialize, initialization is done by the copy in
	 the loop below.  */
      fn_flp->fn_fields = (struct fn_field *)
	TYPE_ALLOC (type, sizeof (struct fn_field) * nf.fnfields.size ());

      for (int k = 0; k < nf.fnfields.size (); ++k)
	fn_flp->fn_fields[k] = nf.fnfields[k];
    }

  TYPE_NFN_FIELDS (type) = fip->fnfieldlists.size ();
}

/* Returns non-zero if NAME is the name of a vtable member in CU's
   language, zero otherwise.  */
static int
is_vtable_name (const char *name, struct dwarf2_cu *cu)
{
  static const char vptr[] = "_vptr";

  /* Look for the C++ form of the vtable.  */
  if (startswith (name, vptr) && is_cplus_marker (name[sizeof (vptr) - 1]))
    return 1;

  return 0;
}

/* GCC outputs unnamed structures that are really pointers to member
   functions, with the ABI-specified layout.  If TYPE describes
   such a structure, smash it into a member function type.

   GCC shouldn't do this; it should just output pointer to member DIEs.
   This is GCC PR debug/28767.  */

static void
quirk_gcc_member_function_pointer (struct type *type, struct objfile *objfile)
{
  struct type *pfn_type, *self_type, *new_type;

  /* Check for a structure with no name and two children.  */
  if (type->code () != TYPE_CODE_STRUCT || type->num_fields () != 2)
    return;

  /* Check for __pfn and __delta members.  */
  if (type->field (0).name () == NULL
      || strcmp (type->field (0).name (), "__pfn") != 0
      || type->field (1).name () == NULL
      || strcmp (type->field (1).name (), "__delta") != 0)
    return;

  /* Find the type of the method.  */
  pfn_type = type->field (0).type ();
  if (pfn_type == NULL
      || pfn_type->code () != TYPE_CODE_PTR
      || pfn_type->target_type ()->code () != TYPE_CODE_FUNC)
    return;

  /* Look for the "this" argument.  */
  pfn_type = pfn_type->target_type ();
  if (pfn_type->num_fields () == 0
      /* || pfn_type->field (0).type () == NULL */
      || pfn_type->field (0).type ()->code () != TYPE_CODE_PTR)
    return;

  self_type = pfn_type->field (0).type ()->target_type ();
  new_type = type_allocator (type).new_type ();
  smash_to_method_type (new_type, self_type, pfn_type->target_type (),
			pfn_type->fields (), pfn_type->has_varargs ());
  smash_to_methodptr_type (type, new_type);
}

/* Helper for quirk_ada_thick_pointer.  If TYPE is an array type that
   requires rewriting, then copy it and return the updated copy.
   Otherwise return nullptr.  */

static struct type *
rewrite_array_type (struct type *type)
{
  if (type->code () != TYPE_CODE_ARRAY)
    return nullptr;

  struct type *index_type = type->index_type ();
  range_bounds *current_bounds = index_type->bounds ();

  /* Handle multi-dimensional arrays.  */
  struct type *new_target = rewrite_array_type (type->target_type ());
  if (new_target == nullptr)
    {
      /* Maybe we don't need to rewrite this array.  */
      if (current_bounds->low.is_constant ()
	  && current_bounds->high.is_constant ())
	return nullptr;
    }

  /* Either the target type was rewritten, or the bounds have to be
     updated.  Either way we want to copy the type and update
     everything.  */
  struct type *copy = copy_type (type);
  copy->copy_fields (type);
  if (new_target != nullptr)
    copy->set_target_type (new_target);

  struct type *index_copy = copy_type (index_type);
  range_bounds *bounds
    = (struct range_bounds *) TYPE_ZALLOC (index_copy,
					   sizeof (range_bounds));
  *bounds = *current_bounds;
  bounds->low.set_const_val (1);
  bounds->high.set_const_val (0);
  index_copy->set_bounds (bounds);
  copy->set_index_type (index_copy);

  return copy;
}

/* While some versions of GCC will generate complicated DWARF for an
   array (see quirk_ada_thick_pointer), more recent versions were
   modified to emit an explicit thick pointer structure.  However, in
   this case, the array still has DWARF expressions for its ranges,
   and these must be ignored.  */

static void
quirk_ada_thick_pointer_struct (struct die_info *die, struct dwarf2_cu *cu,
				struct type *type)
{
  gdb_assert (cu->lang () == language_ada);

  /* Check for a structure with two children.  */
  if (type->code () != TYPE_CODE_STRUCT || type->num_fields () != 2)
    return;

  /* Check for P_ARRAY and P_BOUNDS members.  */
  if (type->field (0).name () == NULL
      || strcmp (type->field (0).name (), "P_ARRAY") != 0
      || type->field (1).name () == NULL
      || strcmp (type->field (1).name (), "P_BOUNDS") != 0)
    return;

  /* Make sure we're looking at a pointer to an array.  */
  if (type->field (0).type ()->code () != TYPE_CODE_PTR)
    return;

  /* The Ada code already knows how to handle these types, so all that
     we need to do is turn the bounds into static bounds.  However, we
     don't want to rewrite existing array or index types in-place,
     because those may be referenced in other contexts where this
     rewriting is undesirable.  */
  struct type *new_ary_type
    = rewrite_array_type (type->field (0).type ()->target_type ());
  if (new_ary_type != nullptr)
    type->field (0).set_type (lookup_pointer_type (new_ary_type));
}

/* If the DIE has a DW_AT_alignment attribute, return its value, doing
   appropriate error checking and issuing complaints if there is a
   problem.  */

static ULONGEST
get_alignment (struct dwarf2_cu *cu, struct die_info *die)
{
  struct attribute *attr = dwarf2_attr (die, DW_AT_alignment, cu);

  if (attr == nullptr)
    return 0;

  if (!attr->form_is_constant ())
    {
      complaint (_("DW_AT_alignment must have constant form"
		   " - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return 0;
    }

  std::optional<ULONGEST> val = attr->unsigned_constant ();
  if (!val.has_value ())
    return 0;

  ULONGEST align = *val;

  if (align == 0)
    {
      complaint (_("DW_AT_alignment value must not be zero"
		   " - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return 0;
    }
  if ((align & (align - 1)) != 0)
    {
      complaint (_("DW_AT_alignment value must be a power of 2"
		   " - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return 0;
    }

  return align;
}

/* If the DIE has a DW_AT_alignment attribute, use its value to set
   the alignment for TYPE.  */

static void
maybe_set_alignment (struct dwarf2_cu *cu, struct die_info *die,
		     struct type *type)
{
  if (!set_type_align (type, get_alignment (cu, die)))
    complaint (_("DW_AT_alignment value too large"
		 " - DIE at %s [in module %s]"),
	       sect_offset_str (die->sect_off),
	       objfile_name (cu->per_objfile->objfile));
}

/* Check if the given VALUE is a valid enum dwarf_calling_convention
   constant for a type, according to DWARF5 spec, Table 5.5.  */

static bool
is_valid_DW_AT_calling_convention_for_type (ULONGEST value)
{
  switch (value)
    {
    case DW_CC_normal:
    case DW_CC_pass_by_reference:
    case DW_CC_pass_by_value:
      return true;

    default:
      complaint (_("unrecognized DW_AT_calling_convention value "
		   "(%s) for a type"), pulongest (value));
      return false;
    }
}

/* Check if the given VALUE is a valid enum dwarf_calling_convention
   constant for a subroutine, according to DWARF5 spec, Table 3.3, and
   also according to GNU-specific values (see include/dwarf2.h).  */

static bool
is_valid_DW_AT_calling_convention_for_subroutine (ULONGEST value)
{
  switch (value)
    {
    case DW_CC_normal:
    case DW_CC_program:
    case DW_CC_nocall:
      return true;

    case DW_CC_GNU_renesas_sh:
    case DW_CC_GNU_borland_fastcall_i386:
    case DW_CC_GDB_IBM_OpenCL:
      return true;

    default:
      complaint (_("unrecognized DW_AT_calling_convention value "
		   "(%s) for a subroutine"), pulongest (value));
      return false;
    }
}

/* Called when we find the DIE that starts a structure or union scope
   (definition) to create a type for the structure or union.  Fill in
   the type's name and general properties; the members will not be
   processed until process_structure_scope.  A symbol table entry for
   the type will also not be done until process_structure_scope (assuming
   the type has a name).

   NOTE: we need to call these functions regardless of whether or not the
   DIE has a DW_AT_name attribute, since it might be an anonymous
   structure or union.  This gets the type entered into our set of
   user defined types.  */

static struct type *
read_structure_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;
  struct attribute *attr;
  const char *name;

  /* If the definition of this type lives in .debug_types, read that type.
     Don't follow DW_AT_specification though, that will take us back up
     the chain and we want to go down.  */
  attr = die->attr (DW_AT_signature);
  if (attr != nullptr)
    {
      type = get_DW_AT_signature_type (die, attr, cu);

      /* The type's CU may not be the same as CU.
	 Ensure TYPE is recorded with CU in die_type_hash.  */
      return set_die_type (die, type, cu);
    }

  type = type_allocator (objfile, cu->lang ()).new_type ();
  INIT_CPLUS_SPECIFIC (type);

  name = dwarf2_name (die, cu);
  if (name != NULL)
    {
      if (cu->lang  () == language_cplus
	  || cu->lang () == language_d
	  || cu->lang () == language_rust
	  || cu->lang () == language_ada)
	{
	  const char *full_name = dwarf2_full_name (name, die, cu);

	  /* dwarf2_full_name might have already finished building the DIE's
	     type.  If so, there is no need to continue.  */
	  if (get_die_type (die, cu) != NULL)
	    return get_die_type (die, cu);

	  type->set_name (full_name);
	}
      else
	{
	  /* The name is already allocated along with this objfile, so
	     we don't need to duplicate it for the type.  */
	  type->set_name (name);
	}
    }

  if (die->tag == DW_TAG_structure_type)
    {
      type->set_code (TYPE_CODE_STRUCT);
    }
  else if (die->tag == DW_TAG_union_type)
    {
      type->set_code (TYPE_CODE_UNION);
    }
  else if (die->tag == DW_TAG_namelist)
    {
      type->set_code (TYPE_CODE_NAMELIST);
    }
  else
    {
      type->set_code (TYPE_CODE_STRUCT);
    }

  if (cu->lang () == language_cplus && die->tag == DW_TAG_class_type)
    type->set_is_declared_class (true);

  /* Store the calling convention in the type if it's available in
     the die.  Otherwise the calling convention remains set to
     the default value DW_CC_normal.  */
  attr = dwarf2_attr (die, DW_AT_calling_convention, cu);
  if (attr != nullptr)
    {
      std::optional<ULONGEST> value = attr->unsigned_constant ();
      if (value.has_value ()
	  && is_valid_DW_AT_calling_convention_for_type (*value))
	{
	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  TYPE_CPLUS_CALLING_CONVENTION (type)
	    = (enum dwarf_calling_convention) *value;
	}
    }

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr)
    {
      if (attr->form_is_constant ())
	type->set_length (attr->unsigned_constant ().value_or (0));
      else
	{
	  struct dynamic_prop prop;
	  if (attr_to_dynamic_prop (attr, die, cu, &prop, cu->addr_type ()))
	    type->add_dyn_prop (DYN_PROP_BYTE_SIZE, prop);

	  type->set_length (0);
	}
    }
  else
    type->set_length (0);

  maybe_set_alignment (cu, die, type);

  if (cu->producer_is_icc_lt_14 () && type->length () == 0)
    {
      /* ICC<14 does not output the required DW_AT_declaration on
	 incomplete types, but gives them a size of zero.  */
      type->set_is_stub (true);
    }
  else
    type->set_stub_is_supported (true);

  if (die_is_declaration (die, cu))
    type->set_is_stub (true);
  else if (attr == NULL && die->child == NULL
	   && cu->producer_is_realview ())
    /* RealView does not output the required DW_AT_declaration
       on incomplete types.  */
    type->set_is_stub (true);

  /* We need to add the type field to the die immediately so we don't
     infinitely recurse when dealing with pointers to the structure
     type within the structure itself.  */
  set_die_type (die, type, cu);

  /* set_die_type should be already done.  */
  set_descriptive_type (type, die, cu);

  return type;
}

static void handle_struct_member_die
  (struct die_info *child_die,
   struct type *type,
   struct field_info *fi,
   std::vector<struct symbol *> *template_args,
   struct dwarf2_cu *cu);

/* A helper for handle_struct_member_die that handles
   DW_TAG_variant_part.  */

static void
handle_variant_part (struct die_info *die, struct type *type,
		     struct field_info *fi,
		     std::vector<struct symbol *> *template_args,
		     struct dwarf2_cu *cu)
{
  variant_part_builder *new_part;
  if (fi->current_variant_part == nullptr)
    new_part = &fi->variant_parts.emplace_back ();
  else if (!fi->current_variant_part->processing_variant)
    {
      complaint (_("nested DW_TAG_variant_part seen "
		   "- DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return;
    }
  else
    {
      variant_field &current = fi->current_variant_part->variants.back ();
      new_part = &current.variant_parts.emplace_back ();
    }

  /* When we recurse, we want callees to add to this new variant
     part.  */
  scoped_restore save_current_variant_part
    = make_scoped_restore (&fi->current_variant_part, new_part);

  struct attribute *discr = dwarf2_attr (die, DW_AT_discr, cu);
  if (discr == NULL)
    {
      /* It's a univariant form, an extension we support.  */
    }
  else if (discr->form_is_ref ())
    {
      struct dwarf2_cu *target_cu = cu;
      struct die_info *target_die = follow_die_ref (die, discr, &target_cu);

      new_part->discriminant_offset = target_die->sect_off;
    }
  else
    {
      complaint (_("DW_AT_discr does not have DIE reference form"
		   " - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
    }

  for (die_info *child_die : die->children ())
    handle_struct_member_die (child_die, type, fi, template_args, cu);
}

/* A helper for handle_struct_member_die that handles
   DW_TAG_variant.  */

static void
handle_variant (struct die_info *die, struct type *type,
		struct field_info *fi,
		std::vector<struct symbol *> *template_args,
		struct dwarf2_cu *cu)
{
  if (fi->current_variant_part == nullptr)
    {
      complaint (_("saw DW_TAG_variant outside DW_TAG_variant_part "
		   "- DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return;
    }
  if (fi->current_variant_part->processing_variant)
    {
      complaint (_("nested DW_TAG_variant seen "
		   "- DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return;
    }

  scoped_restore save_processing_variant
    = make_scoped_restore (&fi->current_variant_part->processing_variant,
			   true);

  variant_field &variant = fi->current_variant_part->variants.emplace_back ();
  variant.first_field = fi->fields.size ();

  /* In a variant we want to get the discriminant and also add a
     field for our sole member child.  */
  struct attribute *discr = dwarf2_attr (die, DW_AT_discr_value, cu);
  if (discr == nullptr || !discr->form_is_constant ())
    {
      discr = dwarf2_attr (die, DW_AT_discr_list, cu);
      if (discr == nullptr || discr->as_block ()->size == 0)
	{
	  /* Nothing to do here -- default branch.  */
	}
      else
	variant.discr_list_data = discr->as_block ();
    }
  else
    variant.discriminant_attr = discr;

  for (die_info *variant_child : die->children ())
    handle_struct_member_die (variant_child, type, fi, template_args, cu);

  variant.last_field = fi->fields.size ();
}

/* A helper for process_structure_scope that handles a single member
   DIE.  */

static void
handle_struct_member_die (struct die_info *child_die, struct type *type,
			  struct field_info *fi,
			  std::vector<struct symbol *> *template_args,
			  struct dwarf2_cu *cu)
{
  if (child_die->tag == DW_TAG_member
      || child_die->tag == DW_TAG_variable
      || child_die->tag == DW_TAG_namelist_item)
    {
      /* NOTE: carlton/2002-11-05: A C++ static data member
	 should be a DW_TAG_member that is a declaration, but
	 all versions of G++ as of this writing (so through at
	 least 3.2.1) incorrectly generate DW_TAG_variable
	 tags for them instead.  */
      dwarf2_add_field (fi, child_die, cu);
    }
  else if (child_die->tag == DW_TAG_subprogram)
    {
      /* Rust doesn't have member functions in the C++ sense.
	 However, it does emit ordinary functions as children
	 of a struct DIE.  */
      if (cu->lang () == language_rust)
	read_func_scope (child_die, cu);
      else
	{
	  /* C++ member function.  */
	  dwarf2_add_member_fn (fi, child_die, type, cu);
	}
    }
  else if (child_die->tag == DW_TAG_inheritance)
    {
      /* C++ base class field.  */
      dwarf2_add_field (fi, child_die, cu);
    }
  else if (type_can_define_types (child_die))
    dwarf2_add_type_defn (fi, child_die, cu);
  else if (child_die->tag == DW_TAG_template_type_param
	   || child_die->tag == DW_TAG_template_value_param)
    {
      struct symbol *arg = new_symbol (child_die, NULL, cu);

      if (arg != NULL)
	template_args->push_back (arg);
    }
  else if (child_die->tag == DW_TAG_variant_part)
    handle_variant_part (child_die, type, fi, template_args, cu);
  else if (child_die->tag == DW_TAG_variant)
    handle_variant (child_die, type, fi, template_args, cu);
}

/* Create a property baton for a field.  DIE is the field's DIE.  The
   baton's "field" member is filled in, but the other members of the
   baton are not.  The new property baton is returned.  */

static dwarf2_property_baton *
find_field_create_baton (dwarf2_cu *cu, die_info *die)
{
  dwarf2_property_baton *result
    = XOBNEW (&cu->per_objfile->objfile->objfile_obstack,
	      struct dwarf2_property_baton);
  memset (&result->field, 0, sizeof (result->field));
  compute_field_location (cu, die, &result->field);
  return result;
}

/* Finish creating a structure or union type, including filling in its
   members and creating a symbol for it. This function also handles Fortran
   namelist variables, their items or members and creating a symbol for
   them.  */

static void
process_structure_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;

  type = get_die_type (die, cu);
  if (type == NULL)
    type = read_structure_type (die, cu);

  bool has_template_parameters = false;
  if (die->child != NULL && ! die_is_declaration (die, cu))
    {
      struct field_info fi;
      std::vector<struct symbol *> template_args;

      for (die_info *child_die : die->children ())
	handle_struct_member_die (child_die, type, &fi, &template_args, cu);

      /* Attach template arguments to type.  */
      if (!template_args.empty ())
	{
	  has_template_parameters = true;
	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  TYPE_N_TEMPLATE_ARGUMENTS (type) = template_args.size ();
	  TYPE_TEMPLATE_ARGUMENTS (type)
	    = XOBNEWVEC (&objfile->objfile_obstack,
			 struct symbol *,
			 TYPE_N_TEMPLATE_ARGUMENTS (type));
	  memcpy (TYPE_TEMPLATE_ARGUMENTS (type),
		  template_args.data (),
		  (TYPE_N_TEMPLATE_ARGUMENTS (type)
		   * sizeof (struct symbol *)));
	}

      /* Attach fields and member functions to the type.  */
      if (fi.nfields () > 0)
	dwarf2_attach_fields_to_type (&fi, type, cu);
      if (!fi.fnfieldlists.empty ())
	{
	  dwarf2_attach_fn_fields_to_type (&fi, type, cu);

	  /* Get the type which refers to the base class (possibly this
	     class itself) which contains the vtable pointer for the current
	     class from the DW_AT_containing_type attribute.  This use of
	     DW_AT_containing_type is a GNU extension.  */

	  if (dwarf2_attr (die, DW_AT_containing_type, cu) != NULL)
	    {
	      struct type *t = die_containing_type (die, cu);

	      set_type_vptr_basetype (type, t);
	      if (type == t)
		{
		  int i;

		  /* Our own class provides vtbl ptr.  */
		  for (i = t->num_fields () - 1;
		       i >= TYPE_N_BASECLASSES (t);
		       --i)
		    {
		      const char *fieldname = t->field (i).name ();

		      if (is_vtable_name (fieldname, cu))
			{
			  set_type_vptr_fieldno (type, i);
			  break;
			}
		    }

		  /* Complain if virtual function table field not found.  */
		  if (i < TYPE_N_BASECLASSES (t))
		    complaint (_("virtual function table pointer "
				 "not found when defining class '%s'"),
			       type->name () ? type->name () : "");
		}
	      else
		{
		  set_type_vptr_fieldno (type, TYPE_VPTR_FIELDNO (t));
		}
	    }
	  else if (cu->producer_is_xlc ())
	    {
	      /* The IBM XLC compiler does not provide direct indication
		 of the containing type, but the vtable pointer is
		 always named __vfp.  */

	      int i;

	      for (i = type->num_fields () - 1;
		   i >= TYPE_N_BASECLASSES (type);
		   --i)
		{
		  if (strcmp (type->field (i).name (), "__vfp") == 0)
		    {
		      set_type_vptr_fieldno (type, i);
		      set_type_vptr_basetype (type, type);
		      break;
		    }
		}
	    }
	}

      /* Copy fi.typedef_field_list linked list elements content into the
	 allocated array TYPE_TYPEDEF_FIELD_ARRAY (type).  */
      if (!fi.typedef_field_list.empty ())
	{
	  int count = fi.typedef_field_list.size ();

	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  /* No zero-initialization is needed, the elements are initialized by
	     the copy in the loop below.  */
	  TYPE_TYPEDEF_FIELD_ARRAY (type)
	    = ((struct decl_field *)
	       TYPE_ALLOC (type,
			   sizeof (TYPE_TYPEDEF_FIELD (type, 0)) * count));
	  TYPE_TYPEDEF_FIELD_COUNT (type) = count;

	  for (int i = 0; i < fi.typedef_field_list.size (); ++i)
	    TYPE_TYPEDEF_FIELD (type, i) = fi.typedef_field_list[i];
	}

      /* Copy fi.nested_types_list linked list elements content into the
	 allocated array TYPE_NESTED_TYPES_ARRAY (type).  */
      if (!fi.nested_types_list.empty ()
	  && cu->lang () != language_ada)
	{
	  int count = fi.nested_types_list.size ();

	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  /* No zero-initialization is needed, the elements are initialized by
	     the copy in the loop below.  */
	  TYPE_NESTED_TYPES_ARRAY (type)
	    = ((struct decl_field *)
	       TYPE_ALLOC (type, sizeof (struct decl_field) * count));
	  TYPE_NESTED_TYPES_COUNT (type) = count;

	  for (int i = 0; i < fi.nested_types_list.size (); ++i)
	    TYPE_NESTED_TYPES_FIELD (type, i) = fi.nested_types_list[i];
	}
    }

  quirk_gcc_member_function_pointer (type, objfile);
  if (cu->lang () == language_rust && die->tag == DW_TAG_union_type)
    cu->rust_unions.push_back (type);
  else if (cu->lang () == language_ada)
    quirk_ada_thick_pointer_struct (die, cu, type);

  /* NOTE: carlton/2004-03-16: GCC 3.4 (or at least one of its
     snapshots) has been known to create a die giving a declaration
     for a class that has, as a child, a die giving a definition for a
     nested class.  So we have to process our children even if the
     current die is a declaration.  Normally, of course, a declaration
     won't have any children at all.  */

  for (die_info *child_die : die->children ())
    {
      if (child_die->tag == DW_TAG_member
	  || child_die->tag == DW_TAG_variable
	  || child_die->tag == DW_TAG_inheritance
	  || child_die->tag == DW_TAG_template_value_param
	  || child_die->tag == DW_TAG_template_type_param)
	{
	  /* Do nothing.  */
	}
      else
	process_die (child_die, cu);
    }

  /* Do not consider external references.  According to the DWARF standard,
     these DIEs are identified by the fact that they have no byte_size
     attribute, and a declaration attribute.  */
  if (dwarf2_attr (die, DW_AT_byte_size, cu) != NULL
      || !die_is_declaration (die, cu)
      || dwarf2_attr (die, DW_AT_signature, cu) != NULL)
    {
      struct symbol *sym = new_symbol (die, type, cu);

      if (has_template_parameters)
	{
	  struct symtab *symtab;
	  if (sym != nullptr)
	    symtab = sym->symtab ();
	  else if (cu->line_header != nullptr)
	    {
	      /* Any related symtab will do.  */
	      symtab
		= cu->line_header->file_names ()[0].symtab;
	    }
	  else
	    {
	      symtab = nullptr;
	      complaint (_("could not find suitable "
			   "symtab for template parameter"
			   " - DIE at %s [in module %s]"),
			 sect_offset_str (die->sect_off),
			 objfile_name (objfile));
	    }

	  if (symtab != nullptr)
	    {
	      /* Make sure that the symtab is set on the new symbols.
		 Even though they don't appear in this symtab directly,
		 other parts of gdb assume that symbols do, and this is
		 reasonably true.  */
	      for (int i = 0; i < TYPE_N_TEMPLATE_ARGUMENTS (type); ++i)
		TYPE_TEMPLATE_ARGUMENT (type, i)->set_symtab (symtab);
	    }
	}
    }
}

/* Read DW_AT_endianity from DIE and compute the byte order that
   should be used.  The CU's arch is used as the default.  The result
   is true if the returned arch differs from the default, and false if
   they are the same.  If provided, the out parameter BYTE_ORDER is
   also set.  */

static bool
die_byte_order (die_info *die, dwarf2_cu *cu, enum bfd_endian *byte_order)
{
  gdbarch *arch = cu->per_objfile->objfile->arch ();
  enum bfd_endian arch_order = gdbarch_byte_order (arch);
  enum bfd_endian new_order = arch_order;

  attribute *attr = dwarf2_attr (die, DW_AT_endianity, cu);
  if (attr != nullptr && attr->form_is_constant ())
    {
      std::optional<ULONGEST> endianity = attr->unsigned_constant ();

      if (endianity.has_value ())
	{
	  switch (*endianity)
	    {
	    case DW_END_default:
	      /* Nothing.  */
	      break;
	    case DW_END_big:
	      new_order = BFD_ENDIAN_BIG;
	      break;
	    case DW_END_little:
	      new_order = BFD_ENDIAN_LITTLE;
	      break;
	    default:
	      complaint (_("DW_AT_endianity has unrecognized value %s"),
			 pulongest (*endianity));
	      break;
	    }
	}
    }

  if (byte_order != nullptr)
    *byte_order = new_order;

  return new_order != arch_order;
}

/* Assuming DIE is an enumeration type, and TYPE is its associated
   type, update TYPE using some information only available in DIE's
   children.  In particular, the fields are computed.  If IS_UNSIGNED
   is set, the enumeration type's sign is already known (a true value
   means unsigned), and so examining the constants to determine the
   sign isn't needed; when this is unset, the enumerator constants are
   read as signed values.  */

static void
update_enumeration_type_from_children (struct die_info *die,
				       struct type *type,
				       struct dwarf2_cu *cu,
				       std::optional<bool> is_unsigned)
{
  /* This is used to check whether the enum is signed or unsigned; for
     simplicity, it is always correct regardless of whether
     IS_UNSIGNED is set.  */
  bool unsigned_enum = is_unsigned.value_or (true);
  bool flag_enum = true;

  std::vector<struct field> fields;

  for (die_info *child_die : die->children ())
    {
      struct attribute *attr;
      LONGEST value;
      const char *name;

      if (child_die->tag != DW_TAG_enumerator)
	continue;

      attr = dwarf2_attr (child_die, DW_AT_const_value, cu);
      if (attr == NULL)
	continue;

      name = dwarf2_name (child_die, cu);
      if (name == NULL)
	name = "<anonymous enumerator>";

      /* Can't check UNSIGNED_ENUM here because that is
	 optimistic.  */
      if (is_unsigned.has_value () && *is_unsigned)
	value = attr->unsigned_constant ().value_or (0);
      else
	{
	  /* Read as signed, either because we don't know the sign or
	     because we know it is definitely signed.  */
	  value = attr->signed_constant ().value_or (0);

	  if (value < 0)
	    {
	      unsigned_enum = false;
	      flag_enum = false;
	    }
	}

      if (flag_enum && count_one_bits_ll (value) >= 2)
	flag_enum = false;

      struct field &field = fields.emplace_back ();
      field.set_name (dwarf2_physname (name, child_die, cu));
      field.set_loc_enumval (value);
    }

  if (!fields.empty ())
    type->copy_fields (fields);
  else
    flag_enum = false;

  type->set_is_unsigned (unsigned_enum);
  type->set_is_flag_enum (flag_enum);
}

/* Given a DW_AT_enumeration_type die, set its type.  We do not
   complete the type's fields yet, or create any symbols.  */

static struct type *
read_enumeration_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;
  struct attribute *attr;
  const char *name;

  /* If the definition of this type lives in .debug_types, read that type.
     Don't follow DW_AT_specification though, that will take us back up
     the chain and we want to go down.  */
  attr = die->attr (DW_AT_signature);
  if (attr != nullptr)
    {
      type = get_DW_AT_signature_type (die, attr, cu);

      /* The type's CU may not be the same as CU.
	 Ensure TYPE is recorded with CU in die_type_hash.  */
      return set_die_type (die, type, cu);
    }

  type = type_allocator (objfile, cu->lang ()).new_type ();

  type->set_code (TYPE_CODE_ENUM);
  name = dwarf2_full_name (NULL, die, cu);
  if (name != NULL)
    type->set_name (name);

  attr = dwarf2_attr (die, DW_AT_type, cu);
  if (attr != NULL)
    {
      struct type *underlying_type = die_type (die, cu);

      type->set_target_type (underlying_type);
    }

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr)
    type->set_length (attr->unsigned_constant ().value_or (0));
  else
    type->set_length (0);

  maybe_set_alignment (cu, die, type);

  /* The enumeration DIE can be incomplete.  In Ada, any type can be
     declared as private in the package spec, and then defined only
     inside the package body.  Such types are known as Taft Amendment
     Types.  When another package uses such a type, an incomplete DIE
     may be generated by the compiler.  */
  if (die_is_declaration (die, cu))
    type->set_is_stub (true);

  /* If the underlying type is known, and is unsigned, then we'll
     assume the enumerator constants are unsigned.  Otherwise we have
     to assume they are signed.  */
  std::optional<bool> is_unsigned;

  /* If this type has an underlying type that is not a stub, then we
     may use its attributes.  We always use the "unsigned" attribute
     in this situation, because ordinarily we guess whether the type
     is unsigned -- but the guess can be wrong and the underlying type
     can tell us the reality.  However, we defer to a local size
     attribute if one exists, because this lets the compiler override
     the underlying type if needed.  */
  if (type->target_type () != NULL && !type->target_type ()->is_stub ())
    {
      struct type *underlying_type = type->target_type ();
      underlying_type = check_typedef (underlying_type);

      is_unsigned = underlying_type->is_unsigned ();
      type->set_is_unsigned (*is_unsigned);

      if (type->length () == 0)
	type->set_length (underlying_type->length ());

      if (TYPE_RAW_ALIGN (type) == 0
	  && TYPE_RAW_ALIGN (underlying_type) != 0)
	set_type_align (type, TYPE_RAW_ALIGN (underlying_type));
    }

  type->set_is_declared_class (dwarf2_flag_true_p (die, DW_AT_enum_class, cu));

  type->set_endianity_is_not_default (die_byte_order (die, cu, nullptr));

  set_die_type (die, type, cu);

  /* Finish the creation of this type by using the enum's children.
     Note that, as usual, this must come after set_die_type to avoid
     infinite recursion when trying to compute the names of the
     enumerators.  */
  update_enumeration_type_from_children (die, type, cu, is_unsigned);

  return type;
}

/* Given a pointer to a die which begins an enumeration, process all
   the dies that define the members of the enumeration, and create the
   symbol for the enumeration type.

   NOTE: We reverse the order of the element list.  */

static void
process_enumeration_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *this_type;

  this_type = get_die_type (die, cu);
  if (this_type == NULL)
    this_type = read_enumeration_type (die, cu);

  if (die->child != NULL)
    {
      for (die_info *child_die : die->children ())
	{
	  if (child_die->tag != DW_TAG_enumerator)
	    {
	      process_die (child_die, cu);
	    }
	  else
	    new_symbol (child_die, this_type, cu);
	}
    }

  /* If we are reading an enum from a .debug_types unit, and the enum
     is a declaration, and the enum is not the signatured type in the
     unit, then we do not want to add a symbol for it.  Adding a
     symbol would in some cases obscure the true definition of the
     enum, giving users an incomplete type when the definition is
     actually available.  Note that we do not want to do this for all
     enums which are just declarations, because C++0x allows forward
     enum declarations.  */
  if (cu->per_cu->is_debug_types
      && die_is_declaration (die, cu))
    {
      struct signatured_type *sig_type;

      sig_type = (struct signatured_type *) cu->per_cu;
      gdb_assert (to_underlying (sig_type->type_offset_in_section) != 0);
      if (sig_type->type_offset_in_section != die->sect_off)
	return;
    }

  new_symbol (die, this_type, cu);
}

/* Helper function for quirk_ada_thick_pointer that examines a bounds
   expression for an index type and finds the corresponding field
   offset in the hidden "P_BOUNDS" structure.  Returns true on success
   and updates *FIELD, false if it fails to recognize an
   expression.  */

static bool
recognize_bound_expression (struct die_info *die, enum dwarf_attribute name,
			    int *bounds_offset, struct field *field,
			    struct dwarf2_cu *cu)
{
  struct attribute *attr = dwarf2_attr (die, name, cu);
  if (attr == nullptr || !attr->form_is_block ())
    return false;

  const struct dwarf_block *block = attr->as_block ();
  const gdb_byte *start = block->data;
  const gdb_byte *end = block->data + block->size;

  /* The expression to recognize generally looks like:

     (DW_OP_push_object_address; DW_OP_plus_uconst: 8; DW_OP_deref;
     DW_OP_plus_uconst: 4; DW_OP_deref_size: 4)

     However, the second "plus_uconst" may be missing:

     (DW_OP_push_object_address; DW_OP_plus_uconst: 8; DW_OP_deref;
     DW_OP_deref_size: 4)

     This happens when the field is at the start of the structure.

     Also, the final deref may not be sized:

     (DW_OP_push_object_address; DW_OP_plus_uconst: 4; DW_OP_deref;
     DW_OP_deref)

     This happens when the size of the index type happens to be the
     same as the architecture's word size.  This can occur with or
     without the second plus_uconst.  */

  if (end - start < 2)
    return false;
  if (*start++ != DW_OP_push_object_address)
    return false;
  if (*start++ != DW_OP_plus_uconst)
    return false;

  uint64_t this_bound_off;
  start = gdb_read_uleb128 (start, end, &this_bound_off);
  if (start == nullptr || (int) this_bound_off != this_bound_off)
    return false;
  /* Update *BOUNDS_OFFSET if needed, or alternatively verify that it
     is consistent among all bounds.  */
  if (*bounds_offset == -1)
    *bounds_offset = this_bound_off;
  else if (*bounds_offset != this_bound_off)
    return false;

  if (start == end || *start++ != DW_OP_deref)
    return false;

  int offset = 0;
  if (start ==end)
    return false;
  else if (*start == DW_OP_deref_size || *start == DW_OP_deref)
    {
      /* This means an offset of 0.  */
    }
  else if (*start++ != DW_OP_plus_uconst)
    return false;
  else
    {
      /* The size is the parameter to DW_OP_plus_uconst.  */
      uint64_t val;
      start = gdb_read_uleb128 (start, end, &val);
      if (start == nullptr)
	return false;
      if ((int) val != val)
	return false;
      offset = val;
    }

  if (start == end)
    return false;

  uint64_t size;
  if (*start == DW_OP_deref_size)
    {
      start = gdb_read_uleb128 (start + 1, end, &size);
      if (start == nullptr)
	return false;
    }
  else if (*start == DW_OP_deref)
    {
      size = cu->header.addr_size;
      ++start;
    }
  else
    return false;

  field->set_loc_bitpos (8 * offset);
  if (size != field->type ()->length ())
    field->set_bitsize (8 * size);

  return true;
}

/* With -fgnat-encodings=minimal, gcc will emit some unusual DWARF for
   some kinds of Ada arrays:

   <1><11db>: Abbrev Number: 7 (DW_TAG_array_type)
      <11dc>   DW_AT_name        : (indirect string, offset: 0x1bb8): string
      <11e0>   DW_AT_data_location: 2 byte block: 97 6
	  (DW_OP_push_object_address; DW_OP_deref)
      <11e3>   DW_AT_type        : <0x1173>
      <11e7>   DW_AT_sibling     : <0x1201>
   <2><11eb>: Abbrev Number: 8 (DW_TAG_subrange_type)
      <11ec>   DW_AT_type        : <0x1206>
      <11f0>   DW_AT_lower_bound : 6 byte block: 97 23 8 6 94 4
	  (DW_OP_push_object_address; DW_OP_plus_uconst: 8; DW_OP_deref;
	   DW_OP_deref_size: 4)
      <11f7>   DW_AT_upper_bound : 8 byte block: 97 23 8 6 23 4 94 4
	  (DW_OP_push_object_address; DW_OP_plus_uconst: 8; DW_OP_deref;
	   DW_OP_plus_uconst: 4; DW_OP_deref_size: 4)

   This actually represents a "thick pointer", which is a structure
   with two elements: one that is a pointer to the array data, and one
   that is a pointer to another structure; this second structure holds
   the array bounds.

   This returns a new type on success, or nullptr if this didn't
   recognize the type.  */

static struct type *
quirk_ada_thick_pointer (struct die_info *die, struct dwarf2_cu *cu,
			 struct type *type)
{
  struct attribute *attr = dwarf2_attr (die, DW_AT_data_location, cu);
  /* So far we've only seen this with block form.  */
  if (attr == nullptr || !attr->form_is_block ())
    return nullptr;

  /* Note that this will fail if the structure layout is changed by
     the compiler.  However, we have no good way to recognize some
     other layout, because we don't know what expression the compiler
     might choose to emit should this happen.  */
  struct dwarf_block *blk = attr->as_block ();
  if (blk->size != 2
      || blk->data[0] != DW_OP_push_object_address
      || blk->data[1] != DW_OP_deref)
    return nullptr;

  int bounds_offset = -1;
  int max_align = -1;
  std::vector<struct field> range_fields;
  for (die_info *child_die : die->children ())
    {
      if (child_die->tag == DW_TAG_subrange_type)
	{
	  struct type *underlying = read_subrange_index_type (child_die, cu);

	  int this_align = type_align (underlying);
	  if (this_align > max_align)
	    max_align = this_align;

	  range_fields.emplace_back ();
	  range_fields.emplace_back ();

	  struct field &lower = range_fields[range_fields.size () - 2];
	  struct field &upper = range_fields[range_fields.size () - 1];

	  lower.set_type (underlying);
	  lower.set_is_artificial (true);

	  upper.set_type (underlying);
	  upper.set_is_artificial (true);

	  if (!recognize_bound_expression (child_die, DW_AT_lower_bound,
					   &bounds_offset, &lower, cu)
	      || !recognize_bound_expression (child_die, DW_AT_upper_bound,
					      &bounds_offset, &upper, cu))
	    return nullptr;
	}
    }

  /* This shouldn't really happen, but double-check that we found
     where the bounds are stored.  */
  if (bounds_offset == -1)
    return nullptr;

  struct objfile *objfile = cu->per_objfile->objfile;
  for (int i = 0; i < range_fields.size (); i += 2)
    {
      char name[20];

      /* Set the name of each field in the bounds.  */
      xsnprintf (name, sizeof (name), "LB%d", i / 2);
      range_fields[i].set_name (objfile->intern (name));
      xsnprintf (name, sizeof (name), "UB%d", i / 2);
      range_fields[i + 1].set_name (objfile->intern (name));
    }

  type_allocator alloc (objfile, cu->lang ());
  struct type *bounds = alloc.new_type ();
  bounds->set_code (TYPE_CODE_STRUCT);

  bounds->copy_fields (range_fields);

  int last_fieldno = range_fields.size () - 1;
  int bounds_size = (bounds->field (last_fieldno).loc_bitpos () / 8
		     + bounds->field (last_fieldno).type ()->length ());
  bounds->set_length (align_up (bounds_size, max_align));

  /* Rewrite the existing array type in place.  Specifically, we
     remove any dynamic properties we might have read, and we replace
     the index types.  */
  struct type *iter = type;
  for (int i = 0; i < range_fields.size (); i += 2)
    {
      gdb_assert (iter->code () == TYPE_CODE_ARRAY);
      iter->main_type->dyn_prop_list = nullptr;
      iter->set_index_type
	(create_static_range_type (alloc, bounds->field (i).type (), 1, 0));
      iter = iter->target_type ();
    }

  struct type *result = type_allocator (objfile, cu->lang ()).new_type ();
  result->set_code (TYPE_CODE_STRUCT);

  result->alloc_fields (2);

  /* The names are chosen to coincide with what the compiler does with
     -fgnat-encodings=all, which the Ada code in gdb already
     understands.  */
  result->field (0).set_name ("P_ARRAY");
  result->field (0).set_type (lookup_pointer_type (type));

  result->field (1).set_name ("P_BOUNDS");
  result->field (1).set_type (lookup_pointer_type (bounds));
  result->field (1).set_loc_bitpos (8 * bounds_offset);

  result->set_name (type->name ());
  result->set_length (result->field (0).type ()->length ()
		      + result->field (1).type ()->length ());

  return result;
}

/* Extract all information from a DW_TAG_array_type DIE and put it in
   the DIE's type field.  For now, this only handles one dimensional
   arrays.  */

static struct type *
read_array_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;
  struct type *element_type, *range_type, *index_type;
  struct attribute *attr;
  const char *name;
  struct dynamic_prop *byte_stride_prop = NULL;
  unsigned int bit_stride = 0;

  element_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  attr = dwarf2_attr (die, DW_AT_byte_stride, cu);
  if (attr != NULL)
    {
      int stride_ok;
      struct type *prop_type = cu->addr_sized_int_type (false);

      byte_stride_prop
	= (struct dynamic_prop *) alloca (sizeof (struct dynamic_prop));
      stride_ok = attr_to_dynamic_prop (attr, die, cu, byte_stride_prop,
					prop_type);
      if (!stride_ok)
	{
	  complaint (_("unable to read array DW_AT_byte_stride "
		       " - DIE at %s [in module %s]"),
		     sect_offset_str (die->sect_off),
		     objfile_name (cu->per_objfile->objfile));
	  /* Ignore this attribute.  We will likely not be able to print
	     arrays of this type correctly, but there is little we can do
	     to help if we cannot read the attribute's value.  */
	  byte_stride_prop = NULL;
	}
    }

  attr = dwarf2_attr (die, DW_AT_bit_stride, cu);
  if (attr != NULL)
    bit_stride = attr->unsigned_constant ().value_or (0);

  /* Irix 6.2 native cc creates array types without children for
     arrays with unspecified length.  */
  if (die->child == NULL)
    {
      index_type = builtin_type (objfile)->builtin_int;
      type_allocator alloc (objfile, cu->lang ());
      range_type = create_static_range_type (alloc, index_type, 0, -1);
      type = create_array_type_with_stride (alloc, element_type, range_type,
					    byte_stride_prop, bit_stride);
      return set_die_type (die, type, cu);
    }

  std::vector<struct type *> range_types;
  for (die_info *child_die : die->children ())
    {
      if (child_die->tag == DW_TAG_subrange_type
	  || child_die->tag == DW_TAG_generic_subrange)
	{
	  struct type *child_type = read_type_die (child_die, cu);

	  if (child_type != NULL)
	    {
	      /* The range type was successfully read.  Save it for the
		 array type creation.  */
	      range_types.push_back (child_type);
	    }
	}
    }

  if (range_types.empty ())
    {
      complaint (_("unable to find array range - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return NULL;
    }

  /* Dwarf2 dimensions are output from left to right, create the
     necessary array types in backwards order.  */

  type = element_type;

  type_allocator alloc (cu->per_objfile->objfile, cu->lang ());
  if (read_array_order (die, cu) == DW_ORD_col_major)
    {
      int i = 0;

      while (i < range_types.size ())
	{
	  type = create_array_type_with_stride (alloc, type, range_types[i++],
						byte_stride_prop, bit_stride);
	  type->set_is_multi_dimensional (true);
	  bit_stride = 0;
	  byte_stride_prop = nullptr;
	}
    }
  else
    {
      size_t ndim = range_types.size ();
      while (ndim-- > 0)
	{
	  type = create_array_type_with_stride (alloc, type, range_types[ndim],
						byte_stride_prop, bit_stride);
	  type->set_is_multi_dimensional (true);
	  bit_stride = 0;
	  byte_stride_prop = nullptr;
	}
    }

  /* Clear the flag on the outermost array type.  */
  type->set_is_multi_dimensional (false);
  gdb_assert (type != element_type);

  /* Understand Dwarf2 support for vector types (like they occur on
     the PowerPC w/ AltiVec).  Gcc just adds another attribute to the
     array type.  This is not part of the Dwarf2/3 standard yet, but a
     custom vendor extension.  The main difference between a regular
     array and the vector variant is that vectors are passed by value
     to functions.  */
  attr = dwarf2_attr (die, DW_AT_GNU_vector, cu);
  if (attr != nullptr)
    make_vector_type (type);

  /* The DIE may have DW_AT_byte_size set.  For example an OpenCL
     implementation may choose to implement triple vectors using this
     attribute.  */
  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr && attr->form_is_unsigned ())
    {
      if (attr->as_unsigned () >= type->length ())
	type->set_length (attr->as_unsigned ());
      else
	complaint (_("DW_AT_byte_size for array type smaller "
		     "than the total size of elements"));
    }

  name = dwarf2_full_name (nullptr, die, cu);
  if (name)
    type->set_name (name);

  maybe_set_alignment (cu, die, type);

  struct type *replacement_type = nullptr;
  if (cu->lang () == language_ada)
    {
      replacement_type = quirk_ada_thick_pointer (die, cu, type);
      if (replacement_type != nullptr)
	type = replacement_type;
    }

  /* Install the type in the die.  */
  set_die_type (die, type, cu, replacement_type != nullptr);

  /* set_die_type should be already done.  */
  set_descriptive_type (type, die, cu);

  return type;
}

static enum dwarf_array_dim_ordering
read_array_order (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_ordering, cu);

  if (attr != nullptr)
    {
      std::optional<ULONGEST> val = attr->unsigned_constant ();
      if (val.has_value () &&
	  (*val == DW_ORD_row_major || *val == DW_ORD_col_major))
	return (enum dwarf_array_dim_ordering) *val;
    }

  /* GNU F77 is a special case, as at 08/2004 array type info is the
     opposite order to the dwarf2 specification, but data is still
     laid out as per normal fortran.

     FIXME: dsl/2004-8-20: If G77 is ever fixed, this will also need
     version checking.  */

  if (cu->lang () == language_fortran && cu->producer_is_gf77 ())
    {
      return DW_ORD_row_major;
    }

  switch (cu->language_defn->array_ordering ())
    {
    case array_column_major:
      return DW_ORD_col_major;
    case array_row_major:
    default:
      return DW_ORD_row_major;
    };
}

/* Extract all information from a DW_TAG_set_type DIE and put it in
   the DIE's type field.  */

static struct type *
read_set_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *domain_type, *set_type;
  struct attribute *attr;

  domain_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  set_type = get_die_type (die, cu);
  if (set_type)
    return set_type;

  type_allocator alloc (cu->per_objfile->objfile, cu->lang ());
  set_type = create_set_type (alloc, domain_type);

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr && attr->form_is_unsigned ())
    set_type->set_length (attr->as_unsigned ());

  maybe_set_alignment (cu, die, set_type);

  return set_die_type (die, set_type, cu);
}

/* A helper for read_common_block that creates a locexpr baton.
   SYM is the symbol which we are marking as computed.
   COMMON_DIE is the DIE for the common block.
   COMMON_LOC is the location expression attribute for the common
   block itself.
   MEMBER_LOC is the location expression attribute for the particular
   member of the common block that we are processing.
   CU is the CU from which the above come.  */

static void
mark_common_block_symbol_computed (struct symbol *sym,
				   struct die_info *common_die,
				   struct attribute *common_loc,
				   struct attribute *member_loc,
				   struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct dwarf2_locexpr_baton *baton;
  gdb_byte *ptr;
  unsigned int cu_off;
  enum bfd_endian byte_order = gdbarch_byte_order (objfile->arch ());
  LONGEST offset = 0;

  gdb_assert (common_loc && member_loc);
  gdb_assert (common_loc->form_is_block ());
  gdb_assert (member_loc->form_is_block ()
	      || member_loc->form_is_constant ());

  baton = OBSTACK_ZALLOC (&objfile->objfile_obstack,
			  struct dwarf2_locexpr_baton);
  baton->per_objfile = per_objfile;
  baton->per_cu = cu->per_cu;
  gdb_assert (baton->per_cu);

  baton->size = 5 /* DW_OP_call4 */ + 1 /* DW_OP_plus */;

  if (member_loc->form_is_constant ())
    {
      offset = member_loc->unsigned_constant ().value_or (0);
      baton->size += 1 /* DW_OP_addr */ + cu->header.addr_size;
    }
  else
    baton->size += member_loc->as_block ()->size;

  ptr = (gdb_byte *) obstack_alloc (&objfile->objfile_obstack, baton->size);
  baton->data = ptr;

  *ptr++ = DW_OP_call4;
  cu_off = common_die->sect_off - cu->per_cu->sect_off;
  store_unsigned_integer (ptr, 4, byte_order, cu_off);
  ptr += 4;

  if (member_loc->form_is_constant ())
    {
      *ptr++ = DW_OP_addr;
      store_unsigned_integer (ptr, cu->header.addr_size, byte_order, offset);
      ptr += cu->header.addr_size;
    }
  else
    {
      /* We have to copy the data here, because DW_OP_call4 will only
	 use a DW_AT_location attribute.  */
      struct dwarf_block *block = member_loc->as_block ();
      memcpy (ptr, block->data, block->size);
      ptr += block->size;
    }

  *ptr++ = DW_OP_plus;
  gdb_assert (ptr - baton->data == baton->size);

  SYMBOL_LOCATION_BATON (sym) = baton;
  sym->set_loc_class_index (dwarf2_locexpr_index);
}

/* Create appropriate locally-scoped variables for all the
   DW_TAG_common_block entries.  Also create a struct common_block
   listing all such variables for `info common'.  COMMON_BLOCK_DOMAIN
   is used to separate the common blocks name namespace from regular
   variable names.  */

static void
read_common_block (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_location, cu);
  if (attr != nullptr)
    {
      /* Support the .debug_loc offsets.  */
      if (attr->form_is_block ())
	{
	  /* Ok.  */
	}
      else if (attr->form_is_section_offset ())
	{
	  dwarf2_complex_location_expr_complaint ();
	  attr = NULL;
	}
      else
	{
	  dwarf2_invalid_attrib_class_complaint ("DW_AT_location",
						 "common block member");
	  attr = NULL;
	}
    }

  if (die->child != NULL)
    {
      struct objfile *objfile = cu->per_objfile->objfile;
      size_t size;
      struct common_block *common_block;
      struct symbol *sym;

      auto range = die->children ();
      size_t n_entries = std::distance (range.begin (), range.end ());

      size = (sizeof (struct common_block)
	      + (n_entries - 1) * sizeof (struct symbol *));
      common_block
	= (struct common_block *) obstack_alloc (&objfile->objfile_obstack,
						 size);
      memset (common_block->contents, 0, n_entries * sizeof (struct symbol *));
      common_block->n_entries = 0;

      for (die_info *child_die : die->children ())
	{
	  /* Create the symbol in the DW_TAG_common_block block in the current
	     symbol scope.  */
	  sym = new_symbol (child_die, NULL, cu);
	  if (sym != NULL)
	    {
	      struct attribute *member_loc;

	      common_block->contents[common_block->n_entries++] = sym;

	      member_loc = dwarf2_attr (child_die, DW_AT_data_member_location,
					cu);
	      if (member_loc)
		{
		  /* GDB has handled this for a long time, but it is
		     not specified by DWARF.  It seems to have been
		     emitted by gfortran at least as recently as:
		     http://gcc.gnu.org/bugzilla/show_bug.cgi?id=23057.  */
		  complaint (_("Variable in common block has "
			       "DW_AT_data_member_location "
			       "- DIE at %s [in module %s]"),
			       sect_offset_str (child_die->sect_off),
			     objfile_name (objfile));

		  if (member_loc->form_is_section_offset ())
		    dwarf2_complex_location_expr_complaint ();
		  else if (member_loc->form_is_constant ()
			   || member_loc->form_is_block ())
		    {
		      if (attr != nullptr)
			mark_common_block_symbol_computed (sym, die, attr,
							   member_loc, cu);
		    }
		  else
		    dwarf2_complex_location_expr_complaint ();
		}
	    }
	}

      sym = new_symbol (die, builtin_type (objfile)->builtin_void, cu);
      sym->set_value_common_block (common_block);
    }
}

/* Create a type for a C++ namespace.  */

static struct type *
read_namespace_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  const char *previous_prefix, *name;
  int is_anonymous;
  struct type *type;

  /* For extensions, reuse the type of the original namespace.  */
  if (dwarf2_attr (die, DW_AT_extension, cu) != NULL)
    {
      struct die_info *ext_die;
      struct dwarf2_cu *ext_cu = cu;

      ext_die = dwarf2_extension (die, &ext_cu);
      type = read_type_die (ext_die, ext_cu);

      /* EXT_CU may not be the same as CU.
	 Ensure TYPE is recorded with CU in die_type_hash.  */
      return set_die_type (die, type, cu);
    }

  name = namespace_name (die, &is_anonymous, cu);

  /* Now build the name of the current namespace.  */

  previous_prefix = determine_prefix (die, cu);
  gdb::unique_xmalloc_ptr<char> name_storage;
  if (previous_prefix[0] != '\0')
    {
      name_storage = typename_concat (previous_prefix, name, 0, cu);
      name = name_storage.get ();
    }

  /* Create the type.  */
  type = type_allocator (objfile, cu->lang ()).new_type (TYPE_CODE_NAMESPACE,
							 0, name);

  return set_die_type (die, type, cu);
}

/* Read a namespace scope.  */

static void
read_namespace (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  int is_anonymous;

  /* Add a symbol associated to this if we haven't seen the namespace
     before.  Also, add a using directive if it's an anonymous
     namespace.  */

  if (dwarf2_attr (die, DW_AT_extension, cu) == NULL)
    {
      struct type *type;

      type = read_type_die (die, cu);
      new_symbol (die, type, cu);

      namespace_name (die, &is_anonymous, cu);
      if (is_anonymous)
	{
	  const char *previous_prefix = determine_prefix (die, cu);

	  std::vector<const char *> excludes;
	  add_using_directive (using_directives (cu),
			       previous_prefix, type->name (), NULL,
			       NULL, excludes,
			       read_decl_line (die, cu),
			       &objfile->objfile_obstack);
	}
    }

  if (die->child != NULL)
    {
      for (die_info *child_die : die->children ())
	process_die (child_die, cu);
    }
}

/* Read a Fortran module or Ada package as type.  For Fortran, This
   DIE can be only a declaration used for imported module.  Still we
   need that type as local Fortran "use ... only" declaration imports
   depend on the created type in determine_prefix.  */

static struct type *
read_module_type (struct die_info *die, struct dwarf2_cu *cu)
{
  enum language lang = cu->lang ();
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;

  if (lang == language_ada)
    {
      const char *pkg_name = dwarf2_full_name (nullptr, die, cu);
      type = type_allocator (objfile, lang).new_type (TYPE_CODE_NAMESPACE,
						      0, pkg_name);
    }
  else
    {
      const char *module_name = dwarf2_name (die, cu);
      type = type_allocator (objfile, lang).new_type (TYPE_CODE_MODULE,
						      0, module_name);
    }

  return set_die_type (die, type, cu);
}

/* Read a module.  This tag is used by Fortran (for modules), but also
   by Ada (for packages).  */

static void
read_module (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *type;

  type = read_type_die (die, cu);
  new_symbol (die, type, cu);

  for (die_info *child_die : die->children ())
    process_die (child_die, cu);
}

/* Return the name of the namespace represented by DIE.  Set
   *IS_ANONYMOUS to tell whether or not the namespace is an anonymous
   namespace.  */

static const char *
namespace_name (struct die_info *die, int *is_anonymous, struct dwarf2_cu *cu)
{
  struct die_info *current_die;
  const char *name = NULL;

  /* Loop through the extensions until we find a name.  */

  for (current_die = die;
       current_die != NULL;
       current_die = dwarf2_extension (die, &cu))
    {
      /* We don't use dwarf2_name here so that we can detect the absence
	 of a name -> anonymous namespace.  */
      name = dwarf2_string_attr (die, DW_AT_name, cu);

      if (name != NULL)
	break;
    }

  /* Is it an anonymous namespace?  */

  *is_anonymous = (name == NULL);
  if (*is_anonymous)
    name = CP_ANONYMOUS_NAMESPACE_STR;

  return name;
}

/* Extract all information from a DW_TAG_pointer_type DIE and add to
   the user defined type vector.  */

static struct type *
read_tag_pointer_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct gdbarch *gdbarch = cu->per_objfile->objfile->arch ();
  unit_head *cu_header = &cu->header;
  struct type *type;
  struct attribute *attr_byte_size;
  struct attribute *attr_address_class;
  int byte_size;
  struct type *target_type;

  target_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  type = lookup_pointer_type (target_type);

  attr_byte_size = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr_byte_size)
    byte_size = (attr_byte_size->unsigned_constant ()
		 .value_or (cu_header->addr_size));
  else
    byte_size = cu_header->addr_size;

  attr_address_class = dwarf2_attr (die, DW_AT_address_class, cu);
  ULONGEST addr_class;
  if (attr_address_class)
    addr_class = (attr_address_class->unsigned_constant ()
		  .value_or (DW_ADDR_none));
  else
    addr_class = DW_ADDR_none;

  ULONGEST alignment = get_alignment (cu, die);

  /* If the pointer size, alignment, or address class is different
     than the default, create a type variant marked as such and set
     the length accordingly.  */
  if (type->length () != byte_size
      || (alignment != 0 && TYPE_RAW_ALIGN (type) != 0
	  && alignment != TYPE_RAW_ALIGN (type))
      || addr_class != DW_ADDR_none)
    {
      if (gdbarch_address_class_type_flags_p (gdbarch))
	{
	  type_instance_flags type_flags
	    = gdbarch_address_class_type_flags (gdbarch, byte_size,
						addr_class);
	  gdb_assert ((type_flags & ~TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL)
		      == 0);
	  type = make_type_with_address_space (type, type_flags);
	}
      else if (type->length () != byte_size)
	{
	  complaint (_("invalid pointer size %d"), byte_size);
	}
      else if (TYPE_RAW_ALIGN (type) != alignment)
	{
	  complaint (_("Invalid DW_AT_alignment"
		       " - DIE at %s [in module %s]"),
		     sect_offset_str (die->sect_off),
		     objfile_name (cu->per_objfile->objfile));
	}
      else
	{
	  /* Should we also complain about unhandled address classes?  */
	}
    }

  type->set_length (byte_size);
  set_type_align (type, alignment);
  return set_die_type (die, type, cu);
}

/* Extract all information from a DW_TAG_ptr_to_member_type DIE and add to
   the user defined type vector.  */

static struct type *
read_tag_ptr_to_member_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *type;
  struct type *to_type;
  struct type *domain;

  to_type = die_type (die, cu);
  domain = die_containing_type (die, cu);

  /* The calls above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  if (check_typedef (to_type)->code () == TYPE_CODE_METHOD)
    type = lookup_methodptr_type (to_type);
  else if (check_typedef (to_type)->code () == TYPE_CODE_FUNC)
    {
      struct type *new_type
	= type_allocator (cu->per_objfile->objfile, cu->lang ()).new_type ();

      smash_to_method_type (new_type, domain, to_type->target_type (),
			    to_type->fields (), to_type->has_varargs ());
      type = lookup_methodptr_type (new_type);
    }
  else
    type = lookup_memberptr_type (to_type, domain);

  return set_die_type (die, type, cu);
}

/* Extract all information from a DW_TAG_{rvalue_,}reference_type DIE and add to
   the user defined type vector.  */

static struct type *
read_tag_reference_type (struct die_info *die, struct dwarf2_cu *cu,
			  enum type_code refcode)
{
  unit_head *cu_header = &cu->header;
  struct type *type, *target_type;
  struct attribute *attr;

  gdb_assert (refcode == TYPE_CODE_REF || refcode == TYPE_CODE_RVALUE_REF);

  target_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  type = lookup_reference_type (target_type, refcode);
  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr)
    type->set_length (attr->unsigned_constant ()
		      .value_or (cu_header->addr_size));
  else
    type->set_length (cu_header->addr_size);

  maybe_set_alignment (cu, die, type);
  return set_die_type (die, type, cu);
}

/* Add the given cv-qualifiers to the element type of the array.  GCC
   outputs DWARF type qualifiers that apply to an array, not the
   element type.  But GDB relies on the array element type to carry
   the cv-qualifiers.  This mimics section 6.7.3 of the C99
   specification.  */

static struct type *
add_array_cv_type (struct die_info *die, struct dwarf2_cu *cu,
		   struct type *base_type, int cnst, int voltl)
{
  struct type *el_type, *inner_array;

  base_type = copy_type (base_type);
  inner_array = base_type;

  while (inner_array->target_type ()->code () == TYPE_CODE_ARRAY)
    {
      inner_array->set_target_type (copy_type (inner_array->target_type ()));
      inner_array = inner_array->target_type ();
    }

  el_type = inner_array->target_type ();
  cnst |= TYPE_CONST (el_type);
  voltl |= TYPE_VOLATILE (el_type);
  inner_array->set_target_type (make_cv_type (cnst, voltl, el_type, NULL));

  return set_die_type (die, base_type, cu);
}

static struct type *
read_tag_const_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *cv_type;

  base_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  cv_type = get_die_type (die, cu);
  if (cv_type)
    return cv_type;

  /* In case the const qualifier is applied to an array type, the element type
     is so qualified, not the array type (section 6.7.3 of C99).  */
  if (base_type->code () == TYPE_CODE_ARRAY)
    return add_array_cv_type (die, cu, base_type, 1, 0);

  cv_type = make_cv_type (1, TYPE_VOLATILE (base_type), base_type, 0);
  return set_die_type (die, cv_type, cu);
}

static struct type *
read_tag_volatile_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *cv_type;

  base_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  cv_type = get_die_type (die, cu);
  if (cv_type)
    return cv_type;

  /* In case the volatile qualifier is applied to an array type, the
     element type is so qualified, not the array type (section 6.7.3
     of C99).  */
  if (base_type->code () == TYPE_CODE_ARRAY)
    return add_array_cv_type (die, cu, base_type, 0, 1);

  cv_type = make_cv_type (TYPE_CONST (base_type), 1, base_type, 0);
  return set_die_type (die, cv_type, cu);
}

/* Handle DW_TAG_restrict_type.  */

static struct type *
read_tag_restrict_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *cv_type;

  base_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  cv_type = get_die_type (die, cu);
  if (cv_type)
    return cv_type;

  cv_type = make_restrict_type (base_type);
  return set_die_type (die, cv_type, cu);
}

/* Handle DW_TAG_atomic_type.  */

static struct type *
read_tag_atomic_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *cv_type;

  base_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  cv_type = get_die_type (die, cu);
  if (cv_type)
    return cv_type;

  cv_type = make_atomic_type (base_type);
  return set_die_type (die, cv_type, cu);
}

/* Extract all information from a DW_TAG_string_type DIE and add to
   the user defined type vector.  It isn't really a user defined type,
   but it behaves like one, with other DIE's using an AT_user_def_type
   attribute to reference it.  */

static struct type *
read_tag_string_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  struct type *type, *range_type, *index_type, *char_type;
  struct attribute *attr;
  struct dynamic_prop prop;
  bool length_is_constant = true;
  LONGEST length;

  /* There are a couple of places where bit sizes might be made use of
     when parsing a DW_TAG_string_type, however, no producer that we know
     of make use of these.  Handling bit sizes that are a multiple of the
     byte size is easy enough, but what about other bit sizes?  Lets deal
     with that problem when we have to.  Warn about these attributes being
     unsupported, then parse the type and ignore them like we always
     have.  */
  if (dwarf2_attr (die, DW_AT_bit_size, cu) != nullptr
      || dwarf2_attr (die, DW_AT_string_length_bit_size, cu) != nullptr)
    {
      static bool warning_printed = false;
      if (!warning_printed)
	{
	  warning (_("DW_AT_bit_size and DW_AT_string_length_bit_size not "
		     "currently supported on DW_TAG_string_type."));
	  warning_printed = true;
	}
    }

  attr = dwarf2_attr (die, DW_AT_string_length, cu);
  if (attr != nullptr && !attr->form_is_constant ())
    {
      /* The string length describes the location at which the length of
	 the string can be found.  The size of the length field can be
	 specified with one of the attributes below.  */
      struct type *prop_type;
      struct attribute *len
	= dwarf2_attr (die, DW_AT_string_length_byte_size, cu);
      if (len == nullptr)
	len = dwarf2_attr (die, DW_AT_byte_size, cu);
      if (len != nullptr && len->form_is_constant ())
	{
	  LONGEST sz = len->unsigned_constant ().value_or (0);
	  prop_type = objfile_int_type (objfile, sz, true);
	}
      else
	{
	  /* If the size is not specified then we assume it is the size of
	     an address on this target.  */
	  prop_type = cu->addr_sized_int_type (true);
	}

      /* Convert the attribute into a dynamic property.  */
      if (!attr_to_dynamic_prop (attr, die, cu, &prop, prop_type))
	length = 1;
      else
	length_is_constant = false;
    }
  else if (attr != nullptr)
    {
      /* This DW_AT_string_length just contains the length with no
	 indirection.  There's no need to create a dynamic property in
	 this case.  */
      length = attr->unsigned_constant ().value_or (0);
    }
  else if ((attr = dwarf2_attr (die, DW_AT_byte_size, cu)) != nullptr)
    {
      /* We don't currently support non-constant byte sizes for strings.  */
      length = attr->unsigned_constant ().value_or (1);
    }
  else
    {
      /* Use 1 as a fallback length if we have nothing else.  */
      length = 1;
    }

  index_type = builtin_type (objfile)->builtin_int;
  type_allocator alloc (objfile, cu->lang ());
  if (length_is_constant)
    range_type = create_static_range_type (alloc, index_type, 1, length);
  else
    {
      struct dynamic_prop low_bound;

      low_bound.set_const_val (1);
      range_type = create_range_type (alloc, index_type, &low_bound, &prop, 0);
    }
  char_type = language_string_char_type (cu->language_defn, gdbarch);
  type = create_string_type (alloc, char_type, range_type);

  return set_die_type (die, type, cu);
}

/* Assuming that DIE corresponds to a function, returns nonzero
   if the function is prototyped.  */

static int
prototyped_function_p (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_prototyped, cu);
  if (attr && attr->as_boolean ())
    return 1;

  /* The DWARF standard implies that the DW_AT_prototyped attribute
     is only meaningful for C, but the concept also extends to other
     languages that allow unprototyped functions (Eg: Objective C).
     For all other languages, assume that functions are always
     prototyped.  */
  if (cu->lang () != language_c
      && cu->lang () != language_objc
      && cu->lang () != language_opencl)
    return 1;

  /* RealView does not emit DW_AT_prototyped.  We can not distinguish
     prototyped and unprototyped functions; default to prototyped,
     since that is more common in modern code (and RealView warns
     about unprototyped functions).  */
  if (cu->producer_is_realview ())
    return 1;

  return 0;
}

/* Handle DIES due to C code like:

   struct foo
   {
   int (*funcp)(int a, long l);
   int b;
   };

   ('funcp' generates a DW_TAG_subroutine_type DIE).  */

static struct type *
read_subroutine_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;		/* Type that this function returns.  */
  struct type *ftype;		/* Function that returns above type.  */
  struct attribute *attr;

  type = die_type (die, cu);

  /* PR gas/29517 occurs in 2.39, and is fixed in 2.40, but it's only fixed
     for dwarf version >= 3 which supports DW_TAG_unspecified_type.  */
  if (type->code () == TYPE_CODE_VOID
      && !type->is_stub ()
      && die->child == nullptr
      && (cu->header.version == 2 || cu->producer_is_gas_2_39 ()))
    {
      /* Work around PR gas/29517, pretend we have an DW_TAG_unspecified_type
	 return type.  */
      type = (type_allocator (cu->per_objfile->objfile, cu->lang ())
	      .new_type (TYPE_CODE_VOID, 0, nullptr));
      type->set_is_stub (true);
    }

  /* The die_type call above may have already set the type for this DIE.  */
  ftype = get_die_type (die, cu);
  if (ftype)
    return ftype;

  ftype = lookup_function_type (type);

  if (prototyped_function_p (die, cu))
    ftype->set_is_prototyped (true);

  /* Store the calling convention in the type if it's available in
     the subroutine die.  Otherwise set the calling convention to
     the default value DW_CC_normal.  */
  attr = dwarf2_attr (die, DW_AT_calling_convention, cu);
  if (attr != nullptr)
    {
      std::optional<ULONGEST> value = attr->unsigned_constant ();
      if (value.has_value ()
	  && is_valid_DW_AT_calling_convention_for_subroutine (*value))
	TYPE_CALLING_CONVENTION (ftype)
	  = (enum dwarf_calling_convention) *value;
    }
  else if (cu->producer_is_xlc_opencl ())
    TYPE_CALLING_CONVENTION (ftype) = DW_CC_GDB_IBM_OpenCL;
  else
    TYPE_CALLING_CONVENTION (ftype) = DW_CC_normal;

  /* Record whether the function returns normally to its caller or not
     if the DWARF producer set that information.  */
  attr = dwarf2_attr (die, DW_AT_noreturn, cu);
  if (attr && attr->as_boolean ())
    TYPE_NO_RETURN (ftype) = 1;

  /* We need to add the subroutine type to the die immediately so
     we don't infinitely recurse when dealing with parameters
     declared as the same subroutine type.  */
  set_die_type (die, ftype, cu);

  if (die->child != NULL)
    {
      struct type *void_type = builtin_type (objfile)->builtin_void;
      int nparams, iparams;

      /* Count the number of parameters.
	 FIXME: GDB currently ignores vararg functions, but knows about
	 vararg member functions.  */
      nparams = 0;
      for (die_info *child_die : die->children ())
	{
	  if (child_die->tag == DW_TAG_formal_parameter)
	    nparams++;
	  else if (child_die->tag == DW_TAG_unspecified_parameters)
	    ftype->set_has_varargs (true);
	}

      /* Allocate storage for parameters and fill them in.  */
      ftype->alloc_fields (nparams);

      /* TYPE_FIELD_TYPE must never be NULL.  Pre-fill the array to ensure it
	 even if we error out during the parameters reading below.  */
      for (iparams = 0; iparams < nparams; iparams++)
	ftype->field (iparams).set_type (void_type);

      iparams = 0;
      for (die_info *child_die : die->children ())
	{
	  if (child_die->tag == DW_TAG_formal_parameter)
	    {
	      struct type *arg_type;

	      /* DWARF version 2 has no clean way to discern C++
		 static and non-static member functions.  G++ helps
		 GDB by marking the first parameter for non-static
		 member functions (which is the this pointer) as
		 artificial.  We pass this information to
		 dwarf2_add_member_fn via TYPE_FIELD_ARTIFICIAL.

		 DWARF version 3 added DW_AT_object_pointer, which GCC
		 4.5 does not yet generate.  */
	      attr = dwarf2_attr (child_die, DW_AT_artificial, cu);
	      if (attr != nullptr)
		ftype->field (iparams).set_is_artificial (attr->as_boolean ());
	      else
		ftype->field (iparams).set_is_artificial (false);
	      arg_type = die_type (child_die, cu);

	      /* RealView does not mark THIS as const, which the testsuite
		 expects.  GCC marks THIS as const in method definitions,
		 but not in the class specifications (GCC PR 43053).  */
	      if (cu->lang () == language_cplus
		  && !TYPE_CONST (arg_type)
		  && ftype->field (iparams).is_artificial ())
		{
		  int is_this = 0;
		  struct dwarf2_cu *arg_cu = cu;
		  const char *name = dwarf2_name (child_die, cu);

		  attr = dwarf2_attr (die, DW_AT_object_pointer, cu);
		  if (attr != nullptr)
		    {
		      /* If the compiler emits this, use it.  */
		      if (follow_die_ref (die, attr, &arg_cu) == child_die)
			is_this = 1;
		    }
		  else if (name && strcmp (name, "this") == 0)
		    /* Function definitions will have the argument names.  */
		    is_this = 1;
		  else if (name == NULL && iparams == 0)
		    /* Declarations may not have the names, so like
		       elsewhere in GDB, assume an artificial first
		       argument is "this".  */
		    is_this = 1;

		  if (is_this)
		    arg_type = make_cv_type (1, TYPE_VOLATILE (arg_type),
					     arg_type, 0);
		}

	      ftype->field (iparams).set_type (arg_type);
	      iparams++;
	    }
	}
    }

  return ftype;
}

static struct type *
read_typedef (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  const char *name = dwarf2_full_name (NULL, die, cu);
  struct type *this_type;
  struct gdbarch *gdbarch = objfile->arch ();
  struct type *target_type = die_type (die, cu);

  if (gdbarch_dwarf2_omit_typedef_p (gdbarch, target_type,
				     cu->get_producer (), name))
    {
      /* The long double is defined as a base type in C.  GCC creates a long
	 double typedef with target-type _Float128 for the long double to
	 identify it as the IEEE Float128 value.  This is a GCC hack since the
	 DWARF doesn't distinguish between the IBM long double and IEEE
	 128-bit float.	 Replace the GCC workaround for the long double
	 typedef with the actual type information copied from the target-type
	 with the correct long double base type name.  */
      this_type = copy_type (target_type);
      this_type->set_name (name);
      set_die_type (die, this_type, cu);
      return this_type;
    }

  type_allocator alloc (objfile, cu->lang ());
  this_type = alloc.new_type (TYPE_CODE_TYPEDEF, 0, name);
  this_type->set_target_is_stub (true);
  set_die_type (die, this_type, cu);
  if (target_type != this_type)
    this_type->set_target_type (target_type);
  else
    {
      /* Self-referential typedefs are, it seems, not allowed by the DWARF
	 spec and cause infinite loops in GDB.  */
      complaint (_("Self-referential DW_TAG_typedef "
		   "- DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off), objfile_name (objfile));
      this_type->set_target_type (nullptr);
    }
  if (name == NULL)
    {
      /* Gcc-7 and before supports -feliminate-dwarf2-dups, which generates
	 anonymous typedefs, which is, strictly speaking, invalid DWARF.
	 Handle these by just returning the target type, rather than
	 constructing an anonymous typedef type and trying to handle this
	 elsewhere.  */
      set_die_type (die, target_type, cu);
      return target_type;
    }
  return this_type;
}

/* Helper for get_dwarf2_rational_constant that computes the value of
   a given gmp_mpz given an attribute.  */

static void
get_mpz_for_rational (dwarf2_cu *cu, gdb_mpz *value, attribute *attr)
{
  /* GCC will sometimes emit a 16-byte constant value as a DWARF
     location expression that pushes an implicit value.  */
  if (attr->form == DW_FORM_exprloc)
    {
      dwarf_block *blk = attr->as_block ();
      if (blk->size > 0 && blk->data[0] == DW_OP_implicit_value)
	{
	  uint64_t len;
	  const gdb_byte *ptr = safe_read_uleb128 (blk->data + 1,
						   blk->data + blk->size,
						   &len);
	  if (ptr - blk->data + len <= blk->size)
	    {
	      value->read (gdb::make_array_view (ptr, len),
			   bfd_big_endian (cu->per_objfile->objfile->obfd.get ())
			   ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE,
			   true);
	      return;
	    }
	}

      /* On failure set it to 1.  */
      *value = gdb_mpz (1);
    }
  else if (attr->form_is_block ())
    {
      dwarf_block *blk = attr->as_block ();
      value->read (gdb::make_array_view (blk->data, blk->size),
		   bfd_big_endian (cu->per_objfile->objfile->obfd.get ())
		   ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE,
		   true);
    }
  else
    {
      /* Rational constants for Ada are always unsigned.  */
      *value = gdb_mpz (attr->unsigned_constant ().value_or (1));
    }
}

/* Assuming DIE is a rational DW_TAG_constant, read the DIE's
   numerator and denominator into NUMERATOR and DENOMINATOR (resp).

   If the numerator and/or numerator attribute is missing,
   a complaint is filed, and NUMERATOR and DENOMINATOR are left
   untouched.  */

static void
get_dwarf2_rational_constant (struct die_info *die, struct dwarf2_cu *cu,
			      gdb_mpz *numerator, gdb_mpz *denominator)
{
  struct attribute *num_attr, *denom_attr;

  num_attr = dwarf2_attr (die, DW_AT_GNU_numerator, cu);
  if (num_attr == nullptr)
    complaint (_("DW_AT_GNU_numerator missing in %s DIE at %s"),
	       dwarf_tag_name (die->tag), sect_offset_str (die->sect_off));

  denom_attr = dwarf2_attr (die, DW_AT_GNU_denominator, cu);
  if (denom_attr == nullptr)
    complaint (_("DW_AT_GNU_denominator missing in %s DIE at %s"),
	       dwarf_tag_name (die->tag), sect_offset_str (die->sect_off));

  if (num_attr == nullptr || denom_attr == nullptr)
    return;

  get_mpz_for_rational (cu, numerator, num_attr);
  get_mpz_for_rational (cu, denominator, denom_attr);
}

/* Same as get_dwarf2_rational_constant, but extracting an unsigned
   rational constant, rather than a signed one.

   If the rational constant has a negative value, a complaint
   is filed, and NUMERATOR and DENOMINATOR are left untouched.  */

static void
get_dwarf2_unsigned_rational_constant (struct die_info *die,
				       struct dwarf2_cu *cu,
				       gdb_mpz *numerator,
				       gdb_mpz *denominator)
{
  gdb_mpz num (1);
  gdb_mpz denom (1);

  get_dwarf2_rational_constant (die, cu, &num, &denom);
  if (num < 0 && denom < 0)
    {
      num.negate ();
      denom.negate ();
    }
  else if (num < 0)
    {
      complaint (_("unexpected negative value for DW_AT_GNU_numerator"
		   " in DIE at %s"),
		 sect_offset_str (die->sect_off));
      return;
    }
  else if (denom < 0)
    {
      complaint (_("unexpected negative value for DW_AT_GNU_denominator"
		   " in DIE at %s"),
		 sect_offset_str (die->sect_off));
      return;
    }

  *numerator = std::move (num);
  *denominator = std::move (denom);
}

/* Assuming that ENCODING is a string whose contents starting at the
   K'th character is "_nn" where "nn" is a decimal number, scan that
   number and set RESULT to the value. K is updated to point to the
   character immediately following the number.

   If the string does not conform to the format described above, false
   is returned, and K may or may not be changed.  */

static bool
ada_get_gnat_encoded_number (const char *encoding, int &k, gdb_mpz *result)
{
  /* The next character should be an underscore ('_') followed
     by a digit.  */
  if (encoding[k] != '_' || !isdigit (encoding[k + 1]))
    return false;

  /* Skip the underscore.  */
  k++;
  int start = k;

  /* Determine the number of digits for our number.  */
  while (isdigit (encoding[k]))
    k++;
  if (k == start)
    return false;

  std::string copy (&encoding[start], k - start);
  return result->set (copy.c_str (), 10);
}

/* Scan two numbers from ENCODING at OFFSET, assuming the string is of
   the form _NN_DD, where NN and DD are decimal numbers.  Set NUM and
   DENOM, update OFFSET, and return true on success.  Return false on
   failure.  */

static bool
ada_get_gnat_encoded_ratio (const char *encoding, int &offset,
			    gdb_mpz *num, gdb_mpz *denom)
{
  if (!ada_get_gnat_encoded_number (encoding, offset, num))
    return false;
  return ada_get_gnat_encoded_number (encoding, offset, denom);
}

/* Assuming DIE corresponds to a fixed point type, finish the creation
   of the corresponding TYPE by setting its type-specific data.  CU is
   the DIE's CU.  SUFFIX is the "XF" type name suffix coming from GNAT
   encodings.  It is nullptr if the GNAT encoding should be
   ignored.  */

static void
finish_fixed_point_type (struct type *type, const char *suffix,
			 struct die_info *die, struct dwarf2_cu *cu)
{
  gdb_assert (type->code () == TYPE_CODE_FIXED_POINT
	      && TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_FIXED_POINT);

  /* If GNAT encodings are preferred, don't examine the
     attributes.  */
  struct attribute *attr = nullptr;
  if (suffix == nullptr)
    {
      attr = dwarf2_attr (die, DW_AT_binary_scale, cu);
      if (attr == nullptr)
	attr = dwarf2_attr (die, DW_AT_decimal_scale, cu);
      if (attr == nullptr)
	attr = dwarf2_attr (die, DW_AT_small, cu);
    }

  /* Numerator and denominator of our fixed-point type's scaling factor.
     The default is a scaling factor of 1, which we use as a fallback
     when we are not able to decode it (problem with the debugging info,
     unsupported forms, bug in GDB, etc...).  Using that as the default
     allows us to at least print the unscaled value, which might still
     be useful to a user.  */
  gdb_mpz scale_num (1);
  gdb_mpz scale_denom (1);

  if (attr == nullptr)
    {
      int offset = 0;
      if (suffix != nullptr
	  && ada_get_gnat_encoded_ratio (suffix, offset, &scale_num,
					 &scale_denom)
	  /* The number might be encoded as _nn_dd_nn_dd, where the
	     second ratio is the 'small value.  In this situation, we
	     want the second value.  */
	  && (suffix[offset] != '_'
	      || ada_get_gnat_encoded_ratio (suffix, offset, &scale_num,
					     &scale_denom)))
	{
	  /* Found it.  */
	}
      else
	{
	  /* Scaling factor not found.  Assume a scaling factor of 1,
	     and hope for the best.  At least the user will be able to
	     see the encoded value.  */
	  scale_num = 1;
	  scale_denom = 1;
	  complaint (_("no scale found for fixed-point type (DIE at %s)"),
		     sect_offset_str (die->sect_off));
	}
    }
  else if (attr->name == DW_AT_binary_scale)
    {
      LONGEST scale_exp = attr->signed_constant ().value_or (0);
      gdb_mpz &num_or_denom = scale_exp > 0 ? scale_num : scale_denom;

      num_or_denom <<= std::abs (scale_exp);
    }
  else if (attr->name == DW_AT_decimal_scale)
    {
      LONGEST scale_exp = attr->signed_constant ().value_or (0);
      gdb_mpz &num_or_denom = scale_exp > 0 ? scale_num : scale_denom;

      num_or_denom = gdb_mpz::pow (10, std::abs (scale_exp));
    }
  else if (attr->name == DW_AT_small)
    {
      struct die_info *scale_die;
      struct dwarf2_cu *scale_cu = cu;

      scale_die = follow_die_ref (die, attr, &scale_cu);
      if (scale_die->tag == DW_TAG_constant)
	get_dwarf2_unsigned_rational_constant (scale_die, scale_cu,
					       &scale_num, &scale_denom);
      else
	complaint (_("%s DIE not supported as target of DW_AT_small attribute"
		     " (DIE at %s)"),
		   dwarf_tag_name (die->tag), sect_offset_str (die->sect_off));
    }
  else
    {
      complaint (_("unsupported scale attribute %s for fixed-point type"
		   " (DIE at %s)"),
		 dwarf_attr_name (attr->name),
		 sect_offset_str (die->sect_off));
    }

  type->fixed_point_info ().scaling_factor = gdb_mpq (scale_num, scale_denom);
}

/* The gnat-encoding suffix for fixed point.  */

#define GNAT_FIXED_POINT_SUFFIX "___XF_"

/* If NAME encodes an Ada fixed-point type, return a pointer to the
   "XF" suffix of the name.  The text after this is what encodes the
   'small and 'delta information.  Otherwise, return nullptr.  */

static const char *
gnat_encoded_fixed_point_type_info (const char *name)
{
  return strstr (name, GNAT_FIXED_POINT_SUFFIX);
}

/* Allocate a floating-point type of size BITS and name NAME.  Pass NAME_HINT
   (which may be different from NAME) to the architecture back-end to allow
   it to guess the correct format if necessary.  */

static struct type *
dwarf2_init_float_type (struct dwarf2_cu *cu, int bits, const char *name,
			const char *name_hint, enum bfd_endian byte_order)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  const struct floatformat **format;
  struct type *type;

  type_allocator alloc (objfile, cu->lang ());
  format = gdbarch_floatformat_for_type (gdbarch, name_hint, bits);
  if (format)
    type = init_float_type (alloc, bits, name, format, byte_order);
  else
    type = alloc.new_type (TYPE_CODE_ERROR, bits, name);

  return type;
}

/* Allocate an integer type of size BITS and name NAME.  */

static struct type *
dwarf2_init_integer_type (struct dwarf2_cu *cu, int bits, int unsigned_p,
			  const char *name)
{
  struct type *type;
  struct objfile *objfile = cu->per_objfile->objfile;

  /* Versions of Intel's C Compiler generate an integer type called "void"
     instead of using DW_TAG_unspecified_type.  This has been seen on
     at least versions 14, 17, and 18.  */
  if (bits == 0 && cu->producer_is_icc () && name != nullptr
      && strcmp (name, "void") == 0)
    type = builtin_type (objfile)->builtin_void;
  else
    {
      type_allocator alloc (objfile, cu->lang ());
      type = init_integer_type (alloc, bits, unsigned_p, name);
    }

  return type;
}

/* Return true if DIE has a DW_AT_small attribute whose value is
   a constant rational, where both the numerator and denominator
   are equal to zero.

   CU is the DIE's Compilation Unit.  */

static bool
has_zero_over_zero_small_attribute (struct die_info *die,
				    struct dwarf2_cu *cu)
{
  struct attribute *attr = dwarf2_attr (die, DW_AT_small, cu);
  if (attr == nullptr)
    return false;

  struct dwarf2_cu *scale_cu = cu;
  struct die_info *scale_die
    = follow_die_ref (die, attr, &scale_cu);

  if (scale_die->tag != DW_TAG_constant)
    return false;

  gdb_mpz num (1), denom (1);
  get_dwarf2_rational_constant (scale_die, cu, &num, &denom);
  return num == 0 && denom == 0;
}

/* Initialise and return a floating point type of size BITS suitable for
   use as a component of a complex number.  The NAME_HINT is passed through
   when initialising the floating point type and is the name of the complex
   type.

   As DWARF doesn't currently provide an explicit name for the components
   of a complex number, but it can be helpful to have these components
   named, we try to select a suitable name based on the size of the
   component.  */
static struct type *
dwarf2_init_complex_target_type (struct dwarf2_cu *cu,
				 int bits, const char *name_hint,
				 enum bfd_endian byte_order)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  gdbarch *gdbarch = objfile->arch ();
  struct type *tt = nullptr;

  /* Try to find a suitable floating point builtin type of size BITS.
     We're going to use the name of this type as the name for the complex
     target type that we are about to create.  */
  switch (cu->lang ())
    {
    case language_fortran:
      switch (bits)
	{
	case 32:
	  tt = builtin_f_type (gdbarch)->builtin_real;
	  break;
	case 64:
	  tt = builtin_f_type (gdbarch)->builtin_real_s8;
	  break;
	case 96:	/* The x86-32 ABI specifies 96-bit long double.  */
	case 128:
	  tt = builtin_f_type (gdbarch)->builtin_real_s16;
	  break;
	}
      break;
    default:
      switch (bits)
	{
	case 32:
	  tt = builtin_type (gdbarch)->builtin_float;
	  break;
	case 64:
	  if (builtin_type (gdbarch)->builtin_long_double->length () == 8
	      && name_hint != nullptr
	      && strstr (name_hint, "long") != nullptr)
	    {
	      /* Use "long double" for "complex long double".  */
	      tt = builtin_type (gdbarch)->builtin_long_double;
	    }
	  else
	    tt = builtin_type (gdbarch)->builtin_double;
	  break;
	case 96:	/* The x86-32 ABI specifies 96-bit long double.  */
	case 128:
	  tt = builtin_type (gdbarch)->builtin_long_double;
	  break;
	}
      break;
    }

  /* If the type we found doesn't match the size we were looking for, then
     pretend we didn't find a type at all, the complex target type we
     create will then be nameless.  */
  if (tt != nullptr && tt->length () * TARGET_CHAR_BIT != bits)
    tt = nullptr;

  const char *name = (tt == nullptr) ? nullptr : tt->name ();
  return dwarf2_init_float_type (cu, bits, name, name_hint, byte_order);
}

/* Find a representation of a given base type and install
   it in the TYPE field of the die.  */

static struct type *
read_base_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;
  struct attribute *attr;
  ULONGEST encoding = 0;
  const char *name;

  attr = dwarf2_attr (die, DW_AT_encoding, cu);
  if (attr != nullptr)
    {
      std::optional<ULONGEST> value = attr->unsigned_constant ();
      if (value.has_value ())
	encoding = *value;
    }

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  std::optional<ULONGEST> byte_size;
  if (attr != nullptr)
    byte_size = attr->unsigned_constant ();
  attr = dwarf2_attr (die, DW_AT_bit_size, cu);
  std::optional<ULONGEST> bit_size;
  if (attr != nullptr)
    bit_size = attr->unsigned_constant ();

  attr = dwarf2_attr (die, DW_AT_data_bit_offset, cu);
  std::optional<ULONGEST> bit_offset;
  if (attr != nullptr)
    bit_offset = attr->unsigned_constant ();

  int bits = 0;
  if (byte_size.has_value ())
    bits = TARGET_CHAR_BIT * *byte_size;
  else if (bit_size.has_value ())
    bits = align_up (*bit_size, 8);
  else
    {
      /* No size, so arrange for an error type.  */
      complaint (_("DW_TAG_base_type has neither bit- nor byte-size"));
      encoding = (ULONGEST) -1;
    }

  name = dwarf2_full_name (nullptr, die, cu);
  if (!name)
    complaint (_("DW_AT_name missing from DW_TAG_base_type"));

  enum bfd_endian byte_order;
  bool not_default = die_byte_order (die, cu, &byte_order);

  if ((encoding == DW_ATE_signed_fixed || encoding == DW_ATE_unsigned_fixed)
      && cu->lang () == language_ada
      && has_zero_over_zero_small_attribute (die, cu))
    {
      /* brobecker/2018-02-24: This is a fixed point type for which
	 the scaling factor is represented as fraction whose value
	 does not make sense (zero divided by zero), so we should
	 normally never see these.  However, there is a small category
	 of fixed point types for which GNAT is unable to provide
	 the scaling factor via the standard DWARF mechanisms, and
	 for which the info is provided via the GNAT encodings instead.
	 This is likely what this DIE is about.  */
      encoding = (encoding == DW_ATE_signed_fixed
		  ? DW_ATE_signed
		  : DW_ATE_unsigned);
    }

  /* With GNAT encodings, fixed-point information will be encoded in
     the type name.  Note that this can also occur with the above
     zero-over-zero case, which is why this is a separate "if" rather
     than an "else if".  */
  const char *gnat_encoding_suffix = nullptr;
  if ((encoding == DW_ATE_signed || encoding == DW_ATE_unsigned)
      && cu->lang () == language_ada
      && name != nullptr)
    {
      gnat_encoding_suffix = gnat_encoded_fixed_point_type_info (name);
      if (gnat_encoding_suffix != nullptr)
	{
	  gdb_assert (startswith (gnat_encoding_suffix,
				  GNAT_FIXED_POINT_SUFFIX));
	  name = obstack_strndup (&cu->per_objfile->objfile->objfile_obstack,
				  name, gnat_encoding_suffix - name);
	  /* Use -1 here so that SUFFIX points at the "_" after the
	     "XF".  */
	  gnat_encoding_suffix += strlen (GNAT_FIXED_POINT_SUFFIX) - 1;

	  encoding = (encoding == DW_ATE_signed
		      ? DW_ATE_signed_fixed
		      : DW_ATE_unsigned_fixed);
	}
    }

  type_allocator alloc (objfile, cu->lang ());
  switch (encoding)
    {
      case DW_ATE_address:
	/* Turn DW_ATE_address into a void * pointer.  */
	type = alloc.new_type (TYPE_CODE_VOID, TARGET_CHAR_BIT, NULL);
	type = init_pointer_type (alloc, bits, name, type);
	break;
      case DW_ATE_boolean:
	type = init_boolean_type (alloc, bits, 1, name);
	break;
      case DW_ATE_complex_float:
	type = dwarf2_init_complex_target_type (cu, bits / 2, name,
						byte_order);
	if (type->code () == TYPE_CODE_ERROR)
	  {
	    if (name == nullptr)
	      {
		struct obstack *obstack
		  = &cu->per_objfile->objfile->objfile_obstack;
		name = obconcat (obstack, "_Complex ", type->name (),
				 nullptr);
	      }
	    type = alloc.new_type (TYPE_CODE_ERROR, bits, name);
	  }
	else
	  type = init_complex_type (name, type);
	break;
      case DW_ATE_decimal_float:
	type = init_decfloat_type (alloc, bits, name);
	break;
      case DW_ATE_float:
	type = dwarf2_init_float_type (cu, bits, name, name, byte_order);
	break;
      case DW_ATE_signed:
	type = dwarf2_init_integer_type (cu, bits, 0, name);
	break;
      case DW_ATE_unsigned:
	if (cu->lang () == language_fortran
	    && name
	    && startswith (name, "character("))
	  type = init_character_type (alloc, bits, 1, name);
	else
	  type = dwarf2_init_integer_type (cu, bits, 1, name);
	break;
      case DW_ATE_signed_char:
	if (cu->lang () == language_ada
	    || cu->lang () == language_m2
	    || cu->lang () == language_pascal
	    || cu->lang () == language_fortran)
	  type = init_character_type (alloc, bits, 0, name);
	else
	  type = dwarf2_init_integer_type (cu, bits, 0, name);
	break;
      case DW_ATE_unsigned_char:
	if (cu->lang () == language_ada
	    || cu->lang () == language_m2
	    || cu->lang () == language_pascal
	    || cu->lang () == language_fortran
	    || cu->lang () == language_rust)
	  type = init_character_type (alloc, bits, 1, name);
	else
	  type = dwarf2_init_integer_type (cu, bits, 1, name);
	break;
      case DW_ATE_UTF:
	{
	  type = init_character_type (alloc, bits, 1, name);
	  return set_die_type (die, type, cu);
	}
	break;
      case DW_ATE_signed_fixed:
	type = init_fixed_point_type (alloc, bits, 0, name);
	finish_fixed_point_type (type, gnat_encoding_suffix, die, cu);
	break;
      case DW_ATE_unsigned_fixed:
	type = init_fixed_point_type (alloc, bits, 1, name);
	finish_fixed_point_type (type, gnat_encoding_suffix, die, cu);
	break;

      default:
	complaint (_("unsupported DW_AT_encoding: '%s'"),
		   dwarf_type_encoding_name (encoding));
	type = alloc.new_type (TYPE_CODE_ERROR, bits, name);
	break;
    }

  if (type->code () == TYPE_CODE_INT
      && name != nullptr
      && strcmp (name, "char") == 0)
    type->set_has_no_signedness (true);

  maybe_set_alignment (cu, die, type);

  type->set_endianity_is_not_default (not_default);

  /* If both a byte size and bit size were provided, then that means
     that not every bit in the object contributes to the value.  */
  if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_INT
      && byte_size.has_value ()
      && bit_size.has_value ())
    {
      /* DWARF says: If this attribute is omitted a default data bit
	 offset of zero is assumed.  */
      ULONGEST offset = bit_offset.value_or (0);

      /* Only use the attributes if they make sense together.  */
      if (*bit_size + offset <= 8 * type->length ())
	{
	  TYPE_MAIN_TYPE (type)->type_specific.int_stuff.bit_size = *bit_size;
	  TYPE_MAIN_TYPE (type)->type_specific.int_stuff.bit_offset = offset;
	}
    }

  return set_die_type (die, type, cu);
}

/* A helper function that returns the name of DIE, if it refers to a
   variable declaration.  */

static const char *
var_decl_name (struct die_info *die, struct dwarf2_cu *cu)
{
  if (die->tag != DW_TAG_variable)
    return nullptr;

  attribute *attr = dwarf2_attr (die, DW_AT_declaration, cu);
  if (attr == nullptr || !attr->as_boolean ())
    return nullptr;

  attr = dwarf2_attr (die, DW_AT_name, cu);
  if (attr == nullptr)
    return nullptr;
  return attr->as_string ();
}

/* Parse dwarf attribute if it's a block, reference or constant and put the
   resulting value of the attribute into struct bound_prop.
   Returns 1 if ATTR could be resolved into PROP, 0 otherwise.  */

static int
attr_to_dynamic_prop (const struct attribute *attr, struct die_info *die,
		      struct dwarf2_cu *cu, struct dynamic_prop *prop,
		      struct type *default_type)
{
  struct dwarf2_property_baton *baton;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct obstack *obstack = &objfile->objfile_obstack;

  gdb_assert (default_type != NULL);

  if (attr == NULL || prop == NULL)
    return 0;

  if (attr->form_is_block ())
    {
      baton = XOBNEW (obstack, struct dwarf2_property_baton);
      baton->property_type = default_type;
      baton->locexpr.per_cu = cu->per_cu;
      baton->locexpr.per_objfile = per_objfile;

      struct dwarf_block block;
      if (attr->form == DW_FORM_data16)
	{
	  size_t data_size = 16;
	  block.size = (data_size
			+ 2 /* Extra bytes for DW_OP and arg.  */);
	  gdb_byte *data = XOBNEWVEC (obstack, gdb_byte, block.size);
	  data[0] = DW_OP_implicit_value;
	  data[1] = data_size;
	  memcpy (&data[2], attr->as_block ()->data, data_size);
	  block.data = data;
	}
      else
	block = *attr->as_block ();

      baton->locexpr.size = block.size;
      baton->locexpr.data = block.data;
      switch (attr->name)
	{
	case DW_AT_string_length:
	  baton->locexpr.is_reference = true;
	  break;
	default:
	  baton->locexpr.is_reference = false;
	  break;
	}

      prop->set_locexpr (baton);
      gdb_assert (prop->baton () != NULL);
    }
  else if (attr->form_is_ref ())
    {
      struct dwarf2_cu *target_cu = cu;
      struct die_info *target_die;
      struct attribute *target_attr;

      target_die = follow_die_ref (die, attr, &target_cu);
      target_attr = dwarf2_attr (target_die, DW_AT_location, target_cu);
      if (target_attr == NULL)
	target_attr = dwarf2_attr (target_die, DW_AT_data_member_location,
				   target_cu);
      if (target_attr == nullptr)
	target_attr = dwarf2_attr (target_die, DW_AT_data_bit_offset,
				   target_cu);
      if (target_attr == NULL)
	{
	  const char *name = var_decl_name (target_die, target_cu);
	  if (name != nullptr)
	    {
	      prop->set_variable_name (name);
	      return 1;
	    }
	  return 0;
	}

      switch (target_attr->name)
	{
	  case DW_AT_location:
	    if (target_attr->form_is_section_offset ())
	      {
		baton = XOBNEW (obstack, struct dwarf2_property_baton);
		baton->property_type = die_type (target_die, target_cu);
		fill_in_loclist_baton (cu, &baton->loclist, target_attr);
		prop->set_loclist (baton);
		gdb_assert (prop->baton () != NULL);
	      }
	    else if (target_attr->form_is_block ())
	      {
		baton = XOBNEW (obstack, struct dwarf2_property_baton);
		baton->property_type = die_type (target_die, target_cu);
		baton->locexpr.per_cu = cu->per_cu;
		baton->locexpr.per_objfile = per_objfile;
		struct dwarf_block *block = target_attr->as_block ();
		baton->locexpr.size = block->size;
		baton->locexpr.data = block->data;
		baton->locexpr.is_reference = true;
		prop->set_locexpr (baton);
		gdb_assert (prop->baton () != NULL);
	      }
	    else
	      {
		dwarf2_invalid_attrib_class_complaint ("DW_AT_location",
						       "dynamic property");
		return 0;
	      }
	    break;
	  case DW_AT_data_member_location:
	  case DW_AT_data_bit_offset:
	    {
	      baton = find_field_create_baton (cu, target_die);
	      if (baton == nullptr)
		return 0;

	      baton->property_type = read_type_die (target_die->parent,
						    target_cu);
	      prop->set_field (baton);
	      break;
	    }
	}
    }
  else if (attr->form_is_constant ())
    prop->set_const_val (attr->constant_value (0));
  else if (attr->form_is_section_offset ())
    {
      switch (attr->name)
	{
	case DW_AT_string_length:
	  baton = XOBNEW (obstack, struct dwarf2_property_baton);
	  baton->property_type = default_type;
	  fill_in_loclist_baton (cu, &baton->loclist, attr);
	  prop->set_loclist (baton);
	  gdb_assert (prop->baton () != NULL);
	  break;
	default:
	  goto invalid;
	}
    }
  else
    goto invalid;

  return 1;

 invalid:
  dwarf2_invalid_attrib_class_complaint (dwarf_form_name (attr->form),
					 dwarf2_name (die, cu));
  return 0;
}

/* See read.h.  */

/* Read the DW_AT_type attribute for a sub-range.  If this attribute is not
   present (which is valid) then compute the default type based on the
   compilation units address size.  */

static struct type *
read_subrange_index_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *index_type = die_type (die, cu);

  /* Dwarf-2 specifications explicitly allows to create subrange types
     without specifying a base type.
     In that case, the base type must be set to the type of
     the lower bound, upper bound or count, in that order, if any of these
     three attributes references an object that has a type.
     If no base type is found, the Dwarf-2 specifications say that
     a signed integer type of size equal to the size of an address should
     be used.
     For the following C code: `extern char gdb_int [];'
     GCC produces an empty range DIE.
     FIXME: muller/2010-05-28: Possible references to object for low bound,
     high bound or count are not yet handled by this code.  */
  if (index_type->code () == TYPE_CODE_VOID)
    index_type = cu->addr_sized_int_type (false);

  return index_type;
}

/* Read the given DW_AT_subrange DIE.  */

static struct type *
read_subrange_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *orig_base_type;
  struct type *range_type;
  struct attribute *attr;
  struct dynamic_prop low, high;
  int low_default_is_valid;
  int high_bound_is_count = 0;
  const char *name;

  orig_base_type = read_subrange_index_type (die, cu);

  /* If ORIG_BASE_TYPE is a typedef, it will not be TYPE_UNSIGNED,
     whereas the real type might be.  So, we use ORIG_BASE_TYPE when
     creating the range type, but we use the result of check_typedef
     when examining properties of the type.  */
  base_type = check_typedef (orig_base_type);

  /* The die_type call above may have already set the type for this DIE.  */
  range_type = get_die_type (die, cu);
  if (range_type)
    return range_type;

  high.set_const_val (0);

  /* Set LOW_DEFAULT_IS_VALID if current language and DWARF version allow
     omitting DW_AT_lower_bound.  */
  switch (cu->lang ())
    {
    case language_c:
    case language_cplus:
      low.set_const_val (0);
      low_default_is_valid = 1;
      break;
    case language_fortran:
      low.set_const_val (1);
      low_default_is_valid = 1;
      break;
    case language_d:
    case language_objc:
    case language_rust:
      low.set_const_val (0);
      low_default_is_valid = (cu->header.version >= 4);
      break;
    case language_ada:
    case language_m2:
    case language_pascal:
      low.set_const_val (1);
      low_default_is_valid = (cu->header.version >= 4);
      break;
    default:
      low.set_const_val (0);
      low_default_is_valid = 0;
      break;
    }

  attr = dwarf2_attr (die, DW_AT_lower_bound, cu);
  if (attr != nullptr)
    attr_to_dynamic_prop (attr, die, cu, &low, base_type);
  else if (!low_default_is_valid)
    complaint (_("Missing DW_AT_lower_bound "
				      "- DIE at %s [in module %s]"),
	       sect_offset_str (die->sect_off),
	       objfile_name (cu->per_objfile->objfile));

  struct attribute *attr_ub, *attr_count;
  attr = attr_ub = dwarf2_attr (die, DW_AT_upper_bound, cu);
  if (!attr_to_dynamic_prop (attr, die, cu, &high, base_type))
    {
      attr = attr_count = dwarf2_attr (die, DW_AT_count, cu);
      if (attr_to_dynamic_prop (attr, die, cu, &high, base_type))
	{
	  /* If bounds are constant do the final calculation here.  */
	  if (low.is_constant () && high.is_constant ())
	    high.set_const_val (low.const_val () + high.const_val () - 1);
	  else
	    high_bound_is_count = 1;
	}
      else
	{
	  if (attr_ub != NULL)
	    complaint (_("Unresolved DW_AT_upper_bound "
			 "- DIE at %s [in module %s]"),
		       sect_offset_str (die->sect_off),
		       objfile_name (cu->per_objfile->objfile));
	  if (attr_count != NULL)
	    complaint (_("Unresolved DW_AT_count "
			 "- DIE at %s [in module %s]"),
		       sect_offset_str (die->sect_off),
		       objfile_name (cu->per_objfile->objfile));
	}
    }

  LONGEST bias = 0;
  struct attribute *bias_attr = dwarf2_attr (die, DW_AT_GNU_bias, cu);
  if (bias_attr != nullptr)
    {
      if (base_type->is_unsigned ())
	bias = (LONGEST) bias_attr->unsigned_constant ().value_or (0);
      else
	bias = bias_attr->signed_constant ().value_or (0);
    }

  /* Normally, the DWARF producers are expected to use a signed
     constant form (Eg. DW_FORM_sdata) to express negative bounds.
     But this is unfortunately not always the case, as witnessed
     with GCC, for instance, where the ambiguous DW_FORM_dataN form
     is used instead.  To work around that ambiguity, we treat
     the bounds as signed, and thus sign-extend their values, when
     the base type is signed.

     Skip it if the base type's length is larger than ULONGEST, to avoid
     the undefined behavior of a too large left shift.  We don't really handle
     constants larger than 8 bytes anyway, at the moment.  */

  if (base_type->length () <= sizeof (ULONGEST))
    {
      ULONGEST negative_mask
	= -((ULONGEST) 1 << (base_type->length () * TARGET_CHAR_BIT - 1));

      if (low.is_constant ()
	  && !base_type->is_unsigned () && (low.const_val () & negative_mask))
	low.set_const_val (low.const_val () | negative_mask);

      if (high.is_constant ()
	  && !base_type->is_unsigned () && (high.const_val () & negative_mask))
	high.set_const_val (high.const_val () | negative_mask);
    }

  /* Check for bit and byte strides.  */
  struct dynamic_prop byte_stride_prop;
  attribute *attr_byte_stride = dwarf2_attr (die, DW_AT_byte_stride, cu);
  if (attr_byte_stride != nullptr)
    {
      struct type *prop_type = cu->addr_sized_int_type (false);
      attr_to_dynamic_prop (attr_byte_stride, die, cu, &byte_stride_prop,
			    prop_type);
    }

  struct dynamic_prop bit_stride_prop;
  attribute *attr_bit_stride = dwarf2_attr (die, DW_AT_bit_stride, cu);
  if (attr_bit_stride != nullptr)
    {
      /* It only makes sense to have either a bit or byte stride.  */
      if (attr_byte_stride != nullptr)
	{
	  complaint (_("Found DW_AT_bit_stride and DW_AT_byte_stride "
		       "- DIE at %s [in module %s]"),
		     sect_offset_str (die->sect_off),
		     objfile_name (cu->per_objfile->objfile));
	  attr_bit_stride = nullptr;
	}
      else
	{
	  struct type *prop_type = cu->addr_sized_int_type (false);
	  attr_to_dynamic_prop (attr_bit_stride, die, cu, &bit_stride_prop,
				prop_type);
	}
    }

  type_allocator alloc (cu->per_objfile->objfile, cu->lang ());
  if (attr_byte_stride != nullptr
      || attr_bit_stride != nullptr)
    {
      bool byte_stride_p = (attr_byte_stride != nullptr);
      struct dynamic_prop *stride
	= byte_stride_p ? &byte_stride_prop : &bit_stride_prop;

      range_type
	= create_range_type_with_stride (alloc, orig_base_type, &low,
					 &high, bias, stride, byte_stride_p);
    }
  else
    range_type = create_range_type (alloc, orig_base_type, &low, &high, bias);

  if (high_bound_is_count)
    range_type->bounds ()->flag_upper_bound_is_count = 1;

  /* Ada expects an empty array on no boundary attributes.  */
  if (attr == NULL && cu->lang () != language_ada)
    range_type->bounds ()->high.set_undefined ();

  name = dwarf2_full_name (nullptr, die, cu);
  if (name)
    range_type->set_name (name);

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr)
    range_type->set_length (attr->unsigned_constant ().value_or (0));

  maybe_set_alignment (cu, die, range_type);

  set_die_type (die, range_type, cu);

  /* set_die_type should be already done.  */
  set_descriptive_type (range_type, die, cu);

  return range_type;
}

static struct type *
read_unspecified_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *type;

  type = (type_allocator (cu->per_objfile->objfile, cu->lang ())
	  .new_type (TYPE_CODE_VOID, 0, nullptr));
  type->set_name (dwarf2_name (die, cu));

  /* In Ada, an unspecified type is typically used when the description
     of the type is deferred to a different unit.  When encountering
     such a type, we treat it as a stub, and try to resolve it later on,
     when needed.
     Mark this as a stub type for all languages though.  */
  type->set_is_stub (true);

  return set_die_type (die, type, cu);
}

/* Read a single die and all its descendents.  Set the die's next
   field to NULL; set other fields in the die correctly, and set all
   of the descendents' fields correctly.  PARENT is the parent of the
   die in question.  */

die_info *
cutu_reader::read_die_and_children (die_info *parent)
{
  die_info *die = this->read_full_die (0, true);

  if (die == nullptr)
    return nullptr;

  bool inserted = m_cu->die_hash.emplace (die).second;
  gdb_assert (inserted);

  if (die->has_children)
    die->child = this->read_die_and_siblings (die);
  else
    die->child = nullptr;

  die->next = nullptr;
  die->parent = parent;
  return die;
}

/* Read a die, all of its descendents, and all of its siblings; set
   all of the fields of all of the dies correctly.  Arguments are as
   in read_die_and_children.  */

die_info *
cutu_reader::read_die_and_siblings (die_info *parent)
{
  die_info *first_die = nullptr;
  die_info *last_sibling = nullptr;

  while (true)
    {
      die_info *die = this->read_die_and_children (parent);

      if (die == nullptr)
	return first_die;

      if (first_die == nullptr)
	first_die = die;
      else
	last_sibling->next = die;

      last_sibling = die;
    }
}

/* See read.h.  */

void
cutu_reader::read_all_dies ()
{
  const gdb_byte *begin_info_ptr = m_info_ptr;

  if (m_top_level_die->has_children)
    {
      gdb_assert (m_cu->die_hash.empty ());
      m_cu->die_hash.reserve (m_cu->header.get_length_without_initial () / 12);
      m_top_level_die->child = this->read_die_and_siblings (m_top_level_die);
    }

  m_cu->dies = m_top_level_die;

  if (dwarf_die_debug)
    {
      gdb_printf (gdb_stdlog, "Read die from %s@0x%tx of %s:\n",
		  m_die_section->get_name (),
		  begin_info_ptr - m_die_section->buffer,
		  bfd_get_filename (m_abfd));
      m_top_level_die->child->dump (dwarf_die_debug);
    }
}

/* Read a die and all its attributes, leave space for NUM_EXTRA_ATTRS
   attributes.

   The caller is responsible for filling in the extra attributes
   and updating die_info::num_attrs.

   Return a newly allocated die with its information, except for its
   child, next, and parent fields.  */

die_info *
cutu_reader::read_full_die (int num_extra_attrs, bool allow_reprocess)
{
  unsigned int bytes_read, i;
  const struct abbrev_info *abbrev;

  sect_offset sect_off = static_cast<sect_offset> (m_info_ptr - m_buffer);
  unsigned int abbrev_number
    = read_unsigned_leb128 (m_abfd, m_info_ptr, &bytes_read);
  m_info_ptr += bytes_read;

  if (abbrev_number == 0)
    return nullptr;

  abbrev = m_abbrev_table->lookup_abbrev (abbrev_number);
  if (!abbrev)
    error (_(DWARF_ERROR_PREFIX
	     "could not find abbrev number %d [in module %s]"),
	   abbrev_number, bfd_get_filename (m_abfd));

  die_info *die = die_info::allocate (&m_cu->comp_unit_obstack,
				      abbrev->num_attrs + num_extra_attrs);
  die->sect_off = sect_off;
  die->tag = abbrev->tag;
  die->abbrev = abbrev_number;
  die->has_children = abbrev->has_children;

  /* Make the result usable.
     The caller needs to update num_attrs after adding the extra
     attributes.  */
  die->num_attrs = abbrev->num_attrs;

  for (i = 0; i < abbrev->num_attrs; ++i)
    m_info_ptr = this->read_attribute (&die->attrs[i], &abbrev->attrs[i],
				       m_info_ptr, allow_reprocess);

  return die;
}

/* Read a die and all its attributes.

   Return a newly allocated die with its information, except for its
   child, next, and parent fields.  */

die_info *
cutu_reader::read_toplevel_die (gdb::array_view<attribute *> extra_attrs)
{
  const gdb_byte *begin_info_ptr = m_info_ptr;
  die_info *die = this->read_full_die (extra_attrs.size (), false);

  /* Copy in the extra attributes, if any.  */
  attribute *next = &die->attrs[die->num_attrs];
  for (attribute *extra : extra_attrs)
    *next++ = *extra;

  struct attribute *attr = die->attr (DW_AT_str_offsets_base);
  if (attr != nullptr && attr->form_is_unsigned ())
    m_cu->str_offsets_base = attr->as_unsigned ();

  attr = die->attr (DW_AT_loclists_base);
  if (attr != nullptr)
    m_cu->loclist_base = attr->as_unsigned ();

  auto maybe_addr_base = die->addr_base ();
  if (maybe_addr_base.has_value ())
    m_cu->addr_base = *maybe_addr_base;

  attr = die->attr (DW_AT_rnglists_base);
  if (attr != nullptr)
    m_cu->rnglists_base = attr->as_unsigned ();

  for (int i = 0; i < die->num_attrs; ++i)
    {
      if (die->attrs[i].form_requires_reprocessing ())
	this->read_attribute_reprocess (&die->attrs[i], die->tag);
    }

  die->num_attrs += extra_attrs.size ();

  if (dwarf_die_debug)
    {
      gdb_printf (gdb_stdlog, "Read die from %s@0x%tx of %s:\n",
		  m_die_section->get_name (),
		  (begin_info_ptr - m_die_section->buffer),
		  bfd_get_filename (m_abfd));
      die->dump (dwarf_die_debug);
    }

  return die;
}

struct compunit_symtab *
cooked_index_functions::find_compunit_symtab_by_address
     (struct objfile *objfile, CORE_ADDR address)
{
  if (objfile->sect_index_data == -1)
    return nullptr;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  cooked_index *table = wait (objfile, true);

  CORE_ADDR baseaddr = objfile->data_section_offset ();
  dwarf2_per_cu *per_cu
    = table->lookup ((unrelocated_addr) (address - baseaddr));
  if (per_cu == nullptr)
    return nullptr;

  return dw2_instantiate_symtab (per_cu, per_objfile, false);
}

bool
cooked_index_functions::expand_symtabs_matching
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

  cooked_index *table = wait (objfile, true);

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

  lookup_name_info lookup_name_without_params
    = lookup_name->make_ignore_params ();
  bool completing = lookup_name->completion_mode ();

  /* Unique styles of language splitting.  */
  static const enum language unique_styles[] =
  {
    /* No splitting is also a style.  */
    language_c,
    /* This includes Rust.  */
    language_cplus,
    /* This includes Go.  */
    language_d,
    language_ada
  };

  symbol_name_match_type match_type
    = lookup_name_without_params.match_type ();

  std::bitset<nr_languages> unique_styles_used;
  if (lang_matcher != nullptr)
    for (unsigned iter = 0; iter < nr_languages; ++iter)
      {
	enum language lang = (enum language) iter;
	if (!lang_matcher (lang))
	  continue;

	switch (lang)
	  {
	  case language_cplus:
	  case language_rust:
	    unique_styles_used[language_cplus] = true;
	    break;
	  case language_d:
	  case language_go:
	    unique_styles_used[language_d] = true;
	    break;
	  case language_ada:
	    unique_styles_used[language_ada] = true;
	    break;
	  default:
	    unique_styles_used[language_c] = true;
	  }

	if (unique_styles_used.count ()
	    == sizeof (unique_styles) / sizeof (unique_styles[0]))
	  break;
      }

  for (enum language lang : unique_styles)
    {
      if (lang_matcher != nullptr
	  && !unique_styles_used.test (lang))
	continue;

      std::vector<std::string_view> name_vec
	= lookup_name_without_params.split_name (lang);
      std::vector<std::string> name_str_vec (name_vec.begin (), name_vec.end ());
      std::vector<lookup_name_info> segment_lookup_names;
      segment_lookup_names.reserve (name_vec.size ());
      for (auto &segment_name : name_str_vec)
	segment_lookup_names.emplace_back (segment_name, match_type,
					   completing, true);

      for (const cooked_index_entry *entry : table->find (name_str_vec.back (),
							  completing))
	{
	  QUIT;

	  /* No need to consider symbols from expanded CUs.  */
	  if (per_objfile->symtab_set_p (entry->per_cu))
	    continue;

	  /* If file-matching was done, we don't need to consider
	     symbols from unmarked CUs.  */
	  if (file_matcher != nullptr && !entry->per_cu->mark)
	    continue;

	  /* See if the symbol matches the type filter.  */
	  if (!entry->matches (search_flags)
	      || !entry->matches (domain))
	    continue;

	  if (lang_matcher != nullptr)
	    {
	      /* Try to skip CUs with non-matching language.  */
	      entry->per_cu->ensure_lang (per_objfile);
	      if (!entry->per_cu->maybe_multi_language ()
		  && !lang_matcher (entry->per_cu->lang ()))
		continue;
	    }

	  /* We've found the base name of the symbol; now walk its
	     parentage chain, ensuring that each component
	     matches.  */
	  bool found = true;

	  const cooked_index_entry *parent = entry->get_parent ();
	  const language_defn *lang_def = language_def (entry->lang);
	  for (int i = name_vec.size () - 1; i > 0; --i)
	    {
	      /* If we ran out of entries, or if this segment doesn't
		 match, this did not match.  */
	      if (parent == nullptr)
		{
		  found = false;
		  break;
		}
	      if (parent->lang != language_unknown)
		{
		  symbol_name_matcher_ftype *name_matcher
		    = lang_def->get_symbol_name_matcher
		      (segment_lookup_names[i-1]);
		  if (!name_matcher (parent->canonical,
				     segment_lookup_names[i-1], nullptr))
		    {
		      found = false;
		      break;
		    }
		}

	      parent = parent->get_parent ();
	    }

	  if (!found)
	    continue;

	  /* Might have been looking for "a::b" and found
	     "x::a::b".  */
	  if (((match_type == symbol_name_match_type::FULL
		|| (lang != language_ada
		    && match_type == symbol_name_match_type::EXPRESSION)))
	      && parent != nullptr)
	    continue;

	  /* Check that the full name matches -- either by matching
	     the lookup name ourselves, or by passing the full name to
	     the symbol matcher.  The former is a bit of a hack: it
	     seems like the loop above could just examine every
	     element of the name, avoiding the need to check here; but
	     this is hard.  See PR symtab/32733.  */
	  if (symbol_matcher != nullptr || entry->lang != language_unknown)
	    {
	      auto_obstack temp_storage;
	      const char *full_name = entry->full_name (&temp_storage,
							FOR_ADA_LINKAGE_NAME);
	      if (symbol_matcher == nullptr)
		{
		  symbol_name_matcher_ftype *name_matcher
		    = (lang_def->get_symbol_name_matcher
		       (lookup_name_without_params));
		  if (!name_matcher (full_name, lookup_name_without_params,
				     nullptr))
		    continue;
		}
	      else if (!symbol_matcher (full_name))
		continue;
	    }

	  if (!dw2_expand_symtabs_matching_one (entry->per_cu, per_objfile,
						file_matcher,
						expansion_notify, nullptr))
	    return false;
	}
    }

  return true;
}

/* Start reading .debug_info using the indexer.  */

static void
start_debug_info_reader (dwarf2_per_objfile *per_objfile)
{
  /* Set the index table early so that sharing works even while
     scanning; and then start the scanning.  */
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  auto worker = std::make_unique<cooked_index_worker_debug_info> (per_objfile);
  per_bfd->start_reading (std::make_unique<cooked_index> (std::move (worker)));
}



/* Read the .debug_loclists or .debug_rnglists header (they are the same format)
   contents from the given SECTION in the HEADER.

   HEADER_OFFSET is the offset of the header in the section.  */
static void
read_loclists_rnglists_header (struct loclists_rnglists_header *header,
			       struct dwarf2_section_info *section,
			       sect_offset header_offset)
{
  unsigned int bytes_read;
  bfd *abfd = section->get_bfd_owner ();
  const gdb_byte *info_ptr = section->buffer + to_underlying (header_offset);

  header->length = read_initial_length (abfd, info_ptr, &bytes_read);
  info_ptr += bytes_read;

  header->version = read_2_bytes (abfd, info_ptr);
  info_ptr += 2;

  header->addr_size = read_1_byte (abfd, info_ptr);
  info_ptr += 1;

  header->segment_collector_size = read_1_byte (abfd, info_ptr);
  info_ptr += 1;

  header->offset_entry_count = read_4_bytes (abfd, info_ptr);
}

/* Return the DW_AT_loclists_base value for the CU.  */
static ULONGEST
lookup_loclist_base (struct dwarf2_cu *cu)
{
  /* For the .dwo unit, the loclist_base points to the first offset following
     the header. The header consists of the following entities-
     1. Unit Length (4 bytes for 32 bit DWARF format, and 12 bytes for the 64
	 bit format)
     2. version (2 bytes)
     3. address size (1 byte)
     4. segment selector size (1 byte)
     5. offset entry count (4 bytes)
     These sizes are derived as per the DWARFv5 standard.  */
  if (cu->dwo_unit != nullptr)
    {
      if (cu->header.initial_length_size == 4)
	 return LOCLIST_HEADER_SIZE32;
      return LOCLIST_HEADER_SIZE64;
    }
  return cu->loclist_base;
}

/* Given a DW_FORM_loclistx value LOCLIST_INDEX, fetch the offset from the
   array of offsets in the .debug_loclists section.  */

static sect_offset
read_loclist_index (struct dwarf2_cu *cu, ULONGEST loclist_index)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  bfd *abfd = objfile->obfd.get ();
  ULONGEST loclist_header_size =
    (cu->header.initial_length_size == 4 ? LOCLIST_HEADER_SIZE32
     : LOCLIST_HEADER_SIZE64);
  ULONGEST loclist_base = lookup_loclist_base (cu);

  /* Offset in .debug_loclists of the offset for LOCLIST_INDEX.  */
  ULONGEST start_offset =
    loclist_base + loclist_index * cu->header.offset_size;

  /* Get loclists section.  */
  struct dwarf2_section_info *section = cu_debug_loc_section (cu);

  /* Read the loclists section content.  */
  section->read (objfile);
  if (section->buffer == NULL)
    error (_("DW_FORM_loclistx used without .debug_loclists "
	     "section [in module %s]"), objfile_name (objfile));

  /* DW_AT_loclists_base points after the .debug_loclists contribution header,
     so if loclist_base is smaller than the header size, we have a problem.  */
  if (loclist_base < loclist_header_size)
    error (_("DW_AT_loclists_base is smaller than header size [in module %s]"),
	   objfile_name (objfile));

  /* Read the header of the loclists contribution.  */
  struct loclists_rnglists_header header;
  read_loclists_rnglists_header (&header, section,
				 (sect_offset) (loclist_base - loclist_header_size));

  /* Verify the loclist index is valid.  */
  if (loclist_index >= header.offset_entry_count)
    error (_("DW_FORM_loclistx pointing outside of "
	     ".debug_loclists offset array [in module %s]"),
	   objfile_name (objfile));

  /* Validate that reading won't go beyond the end of the section.  */
  if (start_offset + cu->header.offset_size > section->size)
    error (_("Reading DW_FORM_loclistx index beyond end of"
	     ".debug_loclists section [in module %s]"),
	   objfile_name (objfile));

  const gdb_byte *info_ptr = section->buffer + start_offset;

  if (cu->header.offset_size == 4)
    return (sect_offset) (bfd_get_32 (abfd, info_ptr) + loclist_base);
  else
    return (sect_offset) (bfd_get_64 (abfd, info_ptr) + loclist_base);
}

/* Given a DW_FORM_rnglistx value RNGLIST_INDEX, fetch the offset from the
   array of offsets in the .debug_rnglists section.  */

static sect_offset
read_rnglist_index (struct dwarf2_cu *cu, ULONGEST rnglist_index,
		    dwarf_tag tag)
{
  struct dwarf2_per_objfile *dwarf2_per_objfile = cu->per_objfile;
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  bfd *abfd = objfile->obfd.get ();
  ULONGEST rnglist_header_size =
    (cu->header.initial_length_size == 4 ? RNGLIST_HEADER_SIZE32
     : RNGLIST_HEADER_SIZE64);

  /* When reading a DW_FORM_rnglistx from a DWO, we read from the DWO's
     .debug_rnglists.dwo section.  The rnglists base given in the skeleton
     doesn't apply.  */
  ULONGEST rnglist_base =
      (cu->dwo_unit != nullptr) ? rnglist_header_size : cu->rnglists_base;

  /* Offset in .debug_rnglists of the offset for RNGLIST_INDEX.  */
  ULONGEST start_offset =
    rnglist_base + rnglist_index * cu->header.offset_size;

  /* Get rnglists section.  */
  struct dwarf2_section_info *section = cu_debug_rnglists_section (cu, tag);

  /* Read the rnglists section content.  */
  section->read (objfile);
  if (section->buffer == nullptr)
    error (_("DW_FORM_rnglistx used without .debug_rnglists section "
	     "[in module %s]"),
	   objfile_name (objfile));

  /* DW_AT_rnglists_base points after the .debug_rnglists contribution header,
     so if rnglist_base is smaller than the header size, we have a problem.  */
  if (rnglist_base < rnglist_header_size)
    error (_("DW_AT_rnglists_base is smaller than header size [in module %s]"),
	   objfile_name (objfile));

  /* Read the header of the rnglists contribution.  */
  struct loclists_rnglists_header header;
  read_loclists_rnglists_header (&header, section,
				 (sect_offset) (rnglist_base - rnglist_header_size));

  /* Verify the rnglist index is valid.  */
  if (rnglist_index >= header.offset_entry_count)
    error (_("DW_FORM_rnglistx index pointing outside of "
	     ".debug_rnglists offset array [in module %s]"),
	   objfile_name (objfile));

  /* Validate that reading won't go beyond the end of the section.  */
  if (start_offset + cu->header.offset_size > section->size)
    error (_("Reading DW_FORM_rnglistx index beyond end of"
	     ".debug_rnglists section [in module %s]"),
	   objfile_name (objfile));

  const gdb_byte *info_ptr = section->buffer + start_offset;

  if (cu->header.offset_size == 4)
    return (sect_offset) (read_4_bytes (abfd, info_ptr) + rnglist_base);
  else
    return (sect_offset) (read_8_bytes (abfd, info_ptr) + rnglist_base);
}

/* Process the attributes that had to be skipped in the first round. These
   attributes are the ones that need str_offsets_base or addr_base attributes.
   They could not have been processed in the first round, because at the time
   the values of str_offsets_base or addr_base may not have been known.  */

void
cutu_reader::read_attribute_reprocess (attribute *attr, dwarf_tag tag)
{
  switch (attr->form)
    {
      case DW_FORM_addrx:
      case DW_FORM_GNU_addr_index:
	attr->set_address (read_addr_index (m_cu,
					    attr->as_unsigned_reprocess ()));
	break;
      case DW_FORM_loclistx:
	{
	  sect_offset loclists_sect_off
	    = read_loclist_index (m_cu, attr->as_unsigned_reprocess ());

	  attr->set_unsigned (to_underlying (loclists_sect_off));
	}
	break;
      case DW_FORM_rnglistx:
	{
	  sect_offset rnglists_sect_off
	    = read_rnglist_index (m_cu, attr->as_unsigned_reprocess (), tag);

	  attr->set_unsigned (to_underlying (rnglists_sect_off));
	}
	break;
      case DW_FORM_strx:
      case DW_FORM_strx1:
      case DW_FORM_strx2:
      case DW_FORM_strx3:
      case DW_FORM_strx4:
      case DW_FORM_GNU_str_index:
	{
	  unsigned int str_index = attr->as_unsigned_reprocess ();
	  gdb_assert (!attr->canonical_string_p ());
	  if (m_dwo_file != NULL)
	    attr->set_string_noncanonical
	      (this->read_dwo_str_index (str_index));
	  else
	    attr->set_string_noncanonical (read_stub_str_index (m_cu,
								str_index));
	  break;
	}
      default:
	gdb_assert_not_reached ("Unexpected DWARF form.");
    }
}

/* Read an attribute value described by an attribute form.  */

const gdb_byte *
cutu_reader::read_attribute_value (attribute *attr, unsigned form,
				   LONGEST implicit_const,
				   const gdb_byte *info_ptr,
				   bool allow_reprocess)
{
  dwarf2_per_objfile *per_objfile = m_cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  unit_head *cu_header = &m_cu->header;
  unsigned int bytes_read;
  struct dwarf_block *blk;

  attr->form = (enum dwarf_form) form;
  switch (form)
    {
    case DW_FORM_ref_addr:
      if (cu_header->version == 2)
	attr->set_unsigned ((ULONGEST) cu_header->read_address (m_abfd,
								info_ptr,
								&bytes_read));
      else
	attr->set_unsigned (cu_header->read_offset (m_abfd, info_ptr,
						    &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_GNU_ref_alt:
      attr->set_unsigned (cu_header->read_offset (m_abfd, info_ptr,
						  &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_addr:
      {
	unrelocated_addr addr
	  = cu_header->read_address (m_abfd, info_ptr, &bytes_read);
	attr->set_address (addr);
	info_ptr += bytes_read;
      }
      break;
    case DW_FORM_block2:
      blk = dwarf_alloc_block (m_cu);
      blk->size = read_2_bytes (m_abfd, info_ptr);
      info_ptr += 2;
      blk->data = read_n_bytes (m_abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->set_block (blk);
      break;
    case DW_FORM_block4:
      blk = dwarf_alloc_block (m_cu);
      blk->size = read_4_bytes (m_abfd, info_ptr);
      info_ptr += 4;
      blk->data = read_n_bytes (m_abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->set_block (blk);
      break;
    case DW_FORM_data2:
      attr->set_unsigned (read_2_bytes (m_abfd, info_ptr));
      info_ptr += 2;
      break;
    case DW_FORM_data4:
    case DW_FORM_ref_sup4:
      attr->set_unsigned (read_4_bytes (m_abfd, info_ptr));
      info_ptr += 4;
      break;
    case DW_FORM_data8:
    case DW_FORM_ref_sup8:
      attr->set_unsigned (read_8_bytes (m_abfd, info_ptr));
      info_ptr += 8;
      break;
    case DW_FORM_data16:
      blk = dwarf_alloc_block (m_cu);
      blk->size = 16;
      blk->data = read_n_bytes (m_abfd, info_ptr, 16);
      info_ptr += 16;
      attr->set_block (blk);
      break;
    case DW_FORM_sec_offset:
      attr->set_unsigned (cu_header->read_offset (m_abfd, info_ptr,
						  &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_loclistx:
      {
	attr->set_unsigned_reprocess (read_unsigned_leb128 (m_abfd, info_ptr,
							    &bytes_read));
	info_ptr += bytes_read;
	if (allow_reprocess)
	  this->read_attribute_reprocess (attr);
      }
      break;
    case DW_FORM_string:
      attr->set_string_noncanonical (read_direct_string (m_abfd, info_ptr,
							 &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_strp:
      if (!m_cu->per_cu->is_dwz)
	{
	  attr->set_string_noncanonical (read_indirect_string (per_objfile,
							       m_abfd, info_ptr,
							       cu_header,
							       &bytes_read));
	  info_ptr += bytes_read;
	  break;
	}
      [[fallthrough]];
    case DW_FORM_line_strp:
      if (!m_cu->per_cu->is_dwz)
	{
	  attr->set_string_noncanonical
	    (per_objfile->read_line_string (info_ptr, cu_header,
					    &bytes_read));
	  info_ptr += bytes_read;
	  break;
	}
      [[fallthrough]];
    case DW_FORM_GNU_strp_alt:
    case DW_FORM_strp_sup:
      {
	dwz_file *dwz = per_objfile->per_bfd->get_dwz_file (true);
	LONGEST str_offset
	  = cu_header->read_offset (m_abfd, info_ptr, &bytes_read);

	attr->set_string_noncanonical
	  (dwz->read_string (objfile, str_offset));
	info_ptr += bytes_read;
      }
      break;
    case DW_FORM_exprloc:
    case DW_FORM_block:
      blk = dwarf_alloc_block (m_cu);
      blk->size = read_unsigned_leb128 (m_abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      blk->data = read_n_bytes (m_abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->set_block (blk);
      break;
    case DW_FORM_block1:
      blk = dwarf_alloc_block (m_cu);
      blk->size = read_1_byte (m_abfd, info_ptr);
      info_ptr += 1;
      blk->data = read_n_bytes (m_abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->set_block (blk);
      break;
    case DW_FORM_data1:
    case DW_FORM_flag:
      attr->set_unsigned (read_1_byte (m_abfd, info_ptr));
      info_ptr += 1;
      break;
    case DW_FORM_flag_present:
      attr->set_unsigned (1);
      break;
    case DW_FORM_sdata:
      attr->set_signed (read_signed_leb128 (m_abfd, info_ptr, &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_rnglistx:
      {
	attr->set_unsigned_reprocess (read_unsigned_leb128 (m_abfd, info_ptr,
							    &bytes_read));
	info_ptr += bytes_read;
	if (allow_reprocess)
	  this->read_attribute_reprocess (attr);
      }
      break;
    case DW_FORM_udata:
      attr->set_unsigned (read_unsigned_leb128 (m_abfd, info_ptr, &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_ref1:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_1_byte (m_abfd, info_ptr)));
      info_ptr += 1;
      break;
    case DW_FORM_ref2:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_2_bytes (m_abfd, info_ptr)));
      info_ptr += 2;
      break;
    case DW_FORM_ref4:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_4_bytes (m_abfd, info_ptr)));
      info_ptr += 4;
      break;
    case DW_FORM_ref8:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_8_bytes (m_abfd, info_ptr)));
      info_ptr += 8;
      break;
    case DW_FORM_ref_sig8:
      attr->set_signature (read_8_bytes (m_abfd, info_ptr));
      info_ptr += 8;
      break;
    case DW_FORM_ref_udata:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_unsigned_leb128 (m_abfd, info_ptr,
						   &bytes_read)));
      info_ptr += bytes_read;
      break;
    case DW_FORM_indirect:
      form = read_unsigned_leb128 (m_abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      if (form == DW_FORM_implicit_const)
	{
	  implicit_const = read_signed_leb128 (m_abfd, info_ptr, &bytes_read);
	  info_ptr += bytes_read;
	}
      info_ptr = this->read_attribute_value (attr, form, implicit_const,
					     info_ptr, allow_reprocess);
      break;
    case DW_FORM_implicit_const:
      attr->set_signed (implicit_const);
      break;
    case DW_FORM_addrx:
    case DW_FORM_GNU_addr_index:
      attr->set_unsigned_reprocess (read_unsigned_leb128 (m_abfd, info_ptr,
							  &bytes_read));
      info_ptr += bytes_read;
      if (allow_reprocess)
	this->read_attribute_reprocess (attr);
      break;
    case DW_FORM_strx:
    case DW_FORM_strx1:
    case DW_FORM_strx2:
    case DW_FORM_strx3:
    case DW_FORM_strx4:
    case DW_FORM_GNU_str_index:
      {
	ULONGEST str_index;
	if (form == DW_FORM_strx1)
	  {
	    str_index = read_1_byte (m_abfd, info_ptr);
	    info_ptr += 1;
	  }
	else if (form == DW_FORM_strx2)
	  {
	    str_index = read_2_bytes (m_abfd, info_ptr);
	    info_ptr += 2;
	  }
	else if (form == DW_FORM_strx3)
	  {
	    str_index = read_3_bytes (m_abfd, info_ptr);
	    info_ptr += 3;
	  }
	else if (form == DW_FORM_strx4)
	  {
	    str_index = read_4_bytes (m_abfd, info_ptr);
	    info_ptr += 4;
	  }
	else
	  {
	    str_index = read_unsigned_leb128 (m_abfd, info_ptr, &bytes_read);
	    info_ptr += bytes_read;
	  }
	attr->set_unsigned_reprocess (str_index);
	if (allow_reprocess)
	  this->read_attribute_reprocess (attr);
      }
      break;
    default:
      error (_(DWARF_ERROR_PREFIX
	       "Cannot handle %s in DWARF reader [in module %s]"),
	     dwarf_form_name (form), bfd_get_filename (m_abfd));
    }

  /* Super hack.  */
  if (m_cu->per_cu->is_dwz && attr->form_is_ref ())
    attr->form = DW_FORM_GNU_ref_alt;

  /* We have seen instances where the compiler tried to emit a byte
     size attribute of -1 which ended up being encoded as an unsigned
     0xffffffff.  Although 0xffffffff is technically a valid size value,
     an object of this size seems pretty unlikely so we can relatively
     safely treat these cases as if the size attribute was invalid and
     treat them as zero by default.  */
  if (attr->name == DW_AT_byte_size
      && form == DW_FORM_data4
      && attr->as_unsigned () >= 0xffffffff)
    {
      complaint
	(_("Suspicious DW_AT_byte_size value treated as zero instead of %s"),
	 hex_string (attr->as_unsigned ()));
      attr->set_unsigned (0);
    }

  return info_ptr;
}

/* Read an attribute described by an abbreviated attribute.  */

const gdb_byte *
cutu_reader::read_attribute (attribute *attr, const attr_abbrev *abbrev,
			     const gdb_byte *info_ptr, bool allow_reprocess)
{
  attr->name = abbrev->name;
  attr->string_is_canonical = 0;
  return this->read_attribute_value (attr, abbrev->form, abbrev->implicit_const,
				     info_ptr, allow_reprocess);
}

/* See read.h.  */

const char *
read_indirect_string_at_offset (dwarf2_per_objfile *per_objfile,
				LONGEST str_offset)
{
  return per_objfile->per_bfd->str.read_string (per_objfile->objfile,
						str_offset, "DW_FORM_strp");
}

/* Return pointer to string at .debug_str offset as read from BUF.
   BUF is assumed to be in a compilation unit described by CU_HEADER.
   Return *BYTES_READ_PTR count of bytes read from BUF.  */

static const char *
read_indirect_string (dwarf2_per_objfile *per_objfile, bfd *abfd,
		      const gdb_byte *buf, const unit_head *cu_header,
		      unsigned int *bytes_read_ptr)
{
  LONGEST str_offset = cu_header->read_offset (abfd, buf, bytes_read_ptr);

  return read_indirect_string_at_offset (per_objfile, str_offset);
}

/* See read.h.  */

const char *
dwarf2_per_objfile::read_line_string (const gdb_byte *buf,
				      unsigned int offset_size)
{
  bfd *abfd = objfile->obfd.get ();
  ULONGEST str_offset = read_offset (abfd, buf, offset_size);

  return per_bfd->line_str.read_string (objfile, str_offset, "DW_FORM_line_strp");
}

/* See read.h.  */

const char *
dwarf2_per_objfile::read_line_string (const gdb_byte *buf,
				      const unit_head *cu_header,
				      unsigned int *bytes_read_ptr)
{
  bfd *abfd = objfile->obfd.get ();
  LONGEST str_offset = cu_header->read_offset (abfd, buf, bytes_read_ptr);

  return per_bfd->line_str.read_string (objfile, str_offset, "DW_FORM_line_strp");
}

/* Given index ADDR_INDEX in .debug_addr, fetch the value.
   ADDR_BASE is the DW_AT_addr_base (DW_AT_GNU_addr_base) attribute or zero.
   ADDR_SIZE is the size of addresses from the CU header.  */

static unrelocated_addr
read_addr_index_1 (dwarf2_per_objfile *per_objfile, unsigned int addr_index,
		   std::optional<ULONGEST> addr_base, int addr_size)
{
  struct objfile *objfile = per_objfile->objfile;
  bfd *abfd = objfile->obfd.get ();
  const gdb_byte *info_ptr;
  ULONGEST addr_base_or_zero = addr_base.has_value () ? *addr_base : 0;

  per_objfile->per_bfd->addr.read (objfile);
  if (per_objfile->per_bfd->addr.buffer == NULL)
    error (_("DW_FORM_addr_index used without .debug_addr section [in module %s]"),
	   objfile_name (objfile));
  if (addr_base_or_zero + addr_index * addr_size
      >= per_objfile->per_bfd->addr.size)
    error (_("DW_FORM_addr_index pointing outside of "
	     ".debug_addr section [in module %s]"),
	   objfile_name (objfile));
  info_ptr = (per_objfile->per_bfd->addr.buffer + addr_base_or_zero
	      + addr_index * addr_size);
  if (addr_size == 4)
    return (unrelocated_addr) bfd_get_32 (abfd, info_ptr);
  else
    return (unrelocated_addr) bfd_get_64 (abfd, info_ptr);
}

/* Given index ADDR_INDEX in .debug_addr, fetch the value.  */

static unrelocated_addr
read_addr_index (struct dwarf2_cu *cu, unsigned int addr_index)
{
  return read_addr_index_1 (cu->per_objfile, addr_index,
			    cu->addr_base, cu->header.addr_size);
}

/* Given a pointer to an leb128 value, fetch the value from .debug_addr.  */

static unrelocated_addr
read_addr_index_from_leb128 (struct dwarf2_cu *cu, const gdb_byte *info_ptr,
			     unsigned int *bytes_read)
{
  bfd *abfd = cu->per_objfile->objfile->obfd.get ();
  unsigned int addr_index = read_unsigned_leb128 (abfd, info_ptr, bytes_read);

  return read_addr_index (cu, addr_index);
}

/* See read.h.  */

unrelocated_addr
dwarf2_read_addr_index (dwarf2_per_cu *per_cu, dwarf2_per_objfile *per_objfile,
			unsigned int addr_index)
{
  struct dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  std::optional<ULONGEST> addr_base;
  int addr_size;

  /* We need addr_base and addr_size.
     If we don't have PER_CU->cu, we have to get it.
     Nasty, but the alternative is storing the needed info in PER_CU,
     which at this point doesn't seem justified: it's not clear how frequently
     it would get used and it would increase the size of every PER_CU.
     Entry points like dwarf2_per_cu_addr_size do a similar thing
     so we're not in uncharted territory here.
     Alas we need to be a bit more complicated as addr_base is contained
     in the DIE.

     We don't need to read the entire CU(/TU).
     We just need the header and top level die.

     IWBN to use the aging mechanism to let us lazily later discard the CU.
     For now we skip this optimization.  */

  if (cu != NULL)
    {
      addr_base = cu->addr_base;
      addr_size = cu->header.addr_size;
    }
  else
    {
      cutu_reader reader (*per_cu, *per_objfile, nullptr, nullptr, false,
			  language_minimal);
      addr_base = reader.cu ()->addr_base;
      addr_size = reader.cu ()->header.addr_size;
    }

  return read_addr_index_1 (per_objfile, addr_index, addr_base, addr_size);
}

/* Given a DW_FORM_GNU_str_index value STR_INDEX, fetch the string.
   STR_SECTION, STR_OFFSETS_SECTION can be from a Fission stub or a
   DWO file.  */

static const char *
read_str_index (struct dwarf2_cu *cu,
		struct dwarf2_section_info *str_section,
		struct dwarf2_section_info *str_offsets_section,
		ULONGEST str_offsets_base, ULONGEST str_index,
		unsigned offset_size)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  const char *objf_name = objfile_name (objfile);
  bfd *abfd = objfile->obfd.get ();
  const gdb_byte *info_ptr;
  ULONGEST str_offset;
  static const char form_name[] = "DW_FORM_GNU_str_index or DW_FORM_strx";

  str_section->read (objfile);
  str_offsets_section->read (objfile);
  if (str_section->buffer == NULL)
    error (_("%s used without %s section"
	     " in CU at offset %s [in module %s]"),
	   form_name, str_section->get_name (),
	   sect_offset_str (cu->header.sect_off), objf_name);
  if (str_offsets_section->buffer == NULL)
    error (_("%s used without %s section"
	     " in CU at offset %s [in module %s]"),
	   form_name, str_section->get_name (),
	   sect_offset_str (cu->header.sect_off), objf_name);

  ULONGEST str_offsets_offset = str_offsets_base + str_index * offset_size;
  if (str_offsets_offset >= str_offsets_section->size)
    error (_(DWARF_ERROR_PREFIX
	     "Offset from %s pointing outside of %s section in CU at offset %s"
	     " [in module %s]"),
	   form_name, str_offsets_section->get_name (),
	   sect_offset_str (cu->header.sect_off), objf_name);
  info_ptr = str_offsets_section->buffer + str_offsets_offset;

  if (offset_size == 4)
    str_offset = bfd_get_32 (abfd, info_ptr);
  else
    str_offset = bfd_get_64 (abfd, info_ptr);
  if (str_offset >= str_section->size)
    error (_("Offset from %s pointing outside of"
	     " %s section in CU at offset %s [in module %s]"),
	   form_name, str_section->get_name (),
	   sect_offset_str (cu->header.sect_off), objf_name);
  return (const char *) (str_section->buffer + str_offset);
}

/* Given a DW_FORM_GNU_str_index from a DWO file, fetch the string.  */

const char *
cutu_reader::read_dwo_str_index (ULONGEST str_index)
{
  unsigned offset_size;
  ULONGEST str_offsets_base;
  if (m_cu->header.version >= 5)
    {
      /* We have a DWARF5 CU with a reference to a .debug_str_offsets section,
	 so assume the .debug_str_offsets section is DWARF5 as well, and
	 parse the header.  FIXME: Parse the header only once.  */
      unsigned int bytes_read = 0;
      bfd *abfd = m_dwo_file->sections.str_offsets.get_bfd_owner ();
      const gdb_byte *p = m_dwo_file->sections.str_offsets.buffer;

      /* Header: Initial length.  */
      read_initial_length (abfd, p + bytes_read, &bytes_read);

      /* Determine offset_size based on the .debug_str_offsets header.  */
      const bool dwarf5_is_dwarf64 = bytes_read != 4;
      offset_size = dwarf5_is_dwarf64 ? 8 : 4;

      /* Header: Version.  */
      unsigned version = read_2_bytes (abfd, p + bytes_read);
      bytes_read += 2;

      if (version <= 4)
	{
	  /* We'd like one warning here about ignoring the section, but
	     because we parse the header more than once (see FIXME above)
	     we'd have many warnings, so use a complaint instead, which at
	     least has a limit. */
	  complaint (_("Section .debug_str_offsets in %s has unsupported"
		       " version %d, use empty string."),
		       m_dwo_file->dwo_name.c_str (), version);
	  return "";
	}

      /* Header: Padding.  */
      bytes_read += 2;

      str_offsets_base = bytes_read;
    }
  else
    {
      /* We have a pre-DWARF5 CU with a reference to a .debug_str_offsets
	 section, assume the .debug_str_offsets section is pre-DWARF5 as
	 well, which doesn't have a header.  */
      str_offsets_base = 0;

      /* Determine offset_size based on the .debug_info header.  */
      offset_size = m_cu->header.offset_size;
  }

  return read_str_index (m_cu, &m_dwo_file->sections.str,
			 &m_dwo_file->sections.str_offsets, str_offsets_base,
			 str_index, offset_size);
}

/* Given a DW_FORM_GNU_str_index from a Fission stub, fetch the string.  */

static const char *
read_stub_str_index (struct dwarf2_cu *cu, ULONGEST str_index)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  const char *objf_name = objfile_name (objfile);
  static const char form_name[] = "DW_FORM_GNU_str_index";
  static const char str_offsets_attr_name[] = "DW_AT_str_offsets";

  if (!cu->str_offsets_base.has_value ())
    error (_("%s used in Fission stub without %s"
	     " in CU at offset 0x%lx [in module %s]"),
	   form_name, str_offsets_attr_name,
	   (long) cu->header.offset_size, objf_name);

  return read_str_index (cu,
			 &cu->per_objfile->per_bfd->str,
			 &cu->per_objfile->per_bfd->str_offsets,
			 *cu->str_offsets_base, str_index,
			 cu->header.offset_size);
}

/* Return the length of an LEB128 number in BUF.  */

static int
leb128_size (const gdb_byte *buf)
{
  const gdb_byte *begin = buf;
  gdb_byte byte;

  while (1)
    {
      byte = *buf++;
      if ((byte & 128) == 0)
	return buf - begin;
    }
}

/* Converts DWARF language names to GDB language names.  */

enum language
dwarf_lang_to_enum_language (ULONGEST lang)
{
  enum language language;

  switch (lang)
    {
    case DW_LANG_C89:
    case DW_LANG_C99:
    case DW_LANG_C11:
    case DW_LANG_C17:
    case DW_LANG_C23:
    case DW_LANG_C:
    case DW_LANG_UPC:
      language = language_c;
      break;
    case DW_LANG_Java:
    case DW_LANG_C_plus_plus:
    case DW_LANG_C_plus_plus_11:
    case DW_LANG_C_plus_plus_14:
    case DW_LANG_C_plus_plus_17:
    case DW_LANG_C_plus_plus_20:
    case DW_LANG_C_plus_plus_23:
      language = language_cplus;
      break;
    case DW_LANG_D:
      language = language_d;
      break;
    case DW_LANG_Fortran77:
    case DW_LANG_Fortran90:
    case DW_LANG_Fortran95:
    case DW_LANG_Fortran03:
    case DW_LANG_Fortran08:
    case DW_LANG_Fortran18:
    case DW_LANG_Fortran23:
      language = language_fortran;
      break;
    case DW_LANG_Go:
      language = language_go;
      break;
    case DW_LANG_Assembly:
    case DW_LANG_Mips_Assembler:
      language = language_asm;
      break;
    case DW_LANG_Ada83:
    case DW_LANG_Ada95:
    case DW_LANG_Ada2005:
    case DW_LANG_Ada2012:
      language = language_ada;
      break;
    case DW_LANG_Modula2:
      language = language_m2;
      break;
    case DW_LANG_Pascal83:
      language = language_pascal;
      break;
    case DW_LANG_ObjC:
      language = language_objc;
      break;
    case DW_LANG_Rust:
    case DW_LANG_Rust_old:
      language = language_rust;
      break;
    case DW_LANG_OpenCL:
      language = language_opencl;
      break;
    case DW_LANG_Cobol74:
    case DW_LANG_Cobol85:
    default:
      language = language_minimal;
      break;
    }

  return language;
}

/* Return the NAME attribute of DIE in *CU, or return NULL if not there.  Also
   return in *CU the cu in which the attribute was actually found.  */

static struct attribute *
dwarf2_attr (struct die_info *die, unsigned int name, struct dwarf2_cu **cu)
{
  for (;;)
    {
      unsigned int i;
      struct attribute *spec = NULL;

      for (i = 0; i < die->num_attrs; ++i)
	{
	  if (die->attrs[i].name == name)
	    return &die->attrs[i];
	  if (die->attrs[i].name == DW_AT_specification
	      || die->attrs[i].name == DW_AT_abstract_origin)
	    spec = &die->attrs[i];
	}

      if (!spec)
	break;

      struct die_info *prev_die = die;
      die = follow_die_ref (die, spec, cu);
      if (die == prev_die)
	/* Self-reference, we're done.  */
	break;
    }

  return NULL;
}

/* Return the NAME attribute of DIE in CU, or return NULL if not there.  */

static struct attribute *
dwarf2_attr (struct die_info *die, unsigned int name, struct dwarf2_cu *cu)
{
  return dwarf2_attr (die, name, &cu);
}

/* Return the string associated with a string-typed attribute, or NULL if it
   is either not found or is of an incorrect type.  */

static const char *
dwarf2_string_attr (struct die_info *die, unsigned int name, struct dwarf2_cu *cu)
{
  struct attribute *attr;
  const char *str = NULL;

  attr = dwarf2_attr (die, name, cu);

  if (attr != NULL)
    {
      str = attr->as_string ();
      if (str == nullptr)
	complaint (_("string type expected for attribute %s for "
		     "DIE at %s [in module %s]"),
		   dwarf_attr_name (name), sect_offset_str (die->sect_off),
		   objfile_name (cu->per_objfile->objfile));
    }

  return str;
}

/* Return the dwo name or NULL if not present. If present, it is in either
   DW_AT_GNU_dwo_name or DW_AT_dwo_name attribute.  */
static const char *
dwarf2_dwo_name (struct die_info *die, struct dwarf2_cu *cu)
{
  const char *dwo_name = dwarf2_string_attr (die, DW_AT_GNU_dwo_name, cu);
  if (dwo_name == nullptr)
    dwo_name = dwarf2_string_attr (die, DW_AT_dwo_name, cu);
  return dwo_name;
}

/* Return non-zero iff the attribute NAME is defined for the given DIE,
   and holds a non-zero value.  This function should only be used for
   DW_FORM_flag or DW_FORM_flag_present attributes.  */

static int
dwarf2_flag_true_p (struct die_info *die, unsigned name, struct dwarf2_cu *cu)
{
  struct attribute *attr = dwarf2_attr (die, name, cu);

  return attr != nullptr && attr->as_boolean ();
}

static int
die_is_declaration (struct die_info *die, struct dwarf2_cu *cu)
{
  /* A DIE is a declaration if it has a DW_AT_declaration attribute
     which value is non-zero.  However, we have to be careful with
     DIEs having a DW_AT_specification attribute, because dwarf2_attr()
     (via dwarf2_flag_true_p) follows this attribute.  So we may
     end up accidentally finding a declaration attribute that belongs
     to a different DIE referenced by the specification attribute,
     even though the given DIE does not have a declaration attribute.  */
  return (dwarf2_flag_true_p (die, DW_AT_declaration, cu)
	  && dwarf2_attr (die, DW_AT_specification, cu) == NULL);
}

/* Return the die giving the specification for DIE, if there is
   one.  *SPEC_CU is the CU containing DIE on input, and the CU
   containing the return value on output.  If there is no
   specification, but there is an abstract origin, that is
   returned.  */

static struct die_info *
die_specification (struct die_info *die, struct dwarf2_cu **spec_cu)
{
  struct attribute *spec_attr = dwarf2_attr (die, DW_AT_specification,
					     *spec_cu);

  if (spec_attr == NULL)
    spec_attr = dwarf2_attr (die, DW_AT_abstract_origin, *spec_cu);

  if (spec_attr == NULL)
    return NULL;
  else
    return follow_die_ref (die, spec_attr, spec_cu);
}

/* A convenience function to find the proper .debug_line section for a CU.  */

static struct dwarf2_section_info *
get_debug_line_section (struct dwarf2_cu *cu)
{
  struct dwarf2_section_info *section;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  /* For TUs in DWO files, the DW_AT_stmt_list attribute lives in the
     DWO file.  */
  if (cu->dwo_unit && cu->per_cu->is_debug_types)
    section = &cu->dwo_unit->dwo_file->sections.line;
  else if (cu->per_cu->is_dwz)
    section = &per_objfile->per_bfd->get_dwz_file (true)->line;
  else
    section = &per_objfile->per_bfd->line;

  return section;
}

/* Read the statement program header starting at OFFSET in
   .debug_line, or .debug_line.dwo.  Return a pointer
   to a struct line_header, allocated using xmalloc.
   Returns NULL if there is a problem reading the header, e.g., if it
   has a version we don't understand.

   NOTE: the strings in the include directory and file name tables of
   the returned object point into the dwarf line section buffer,
   and must not be freed.  */

static line_header_up
dwarf_decode_line_header (sect_offset sect_off, struct dwarf2_cu *cu,
			  const char *comp_dir)
{
  struct dwarf2_section_info *section;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  section = get_debug_line_section (cu);
  section->read (per_objfile->objfile);
  if (section->buffer == NULL)
    {
      if (cu->dwo_unit && cu->per_cu->is_debug_types)
	complaint (_("missing .debug_line.dwo section"));
      else
	complaint (_("missing .debug_line section"));
      return 0;
    }

  return dwarf_decode_line_header (sect_off, cu->per_cu->is_dwz,
				   per_objfile, section, &cu->header,
				   comp_dir);
}

/* Subroutine of dwarf_decode_lines to simplify it.
   Return the file name for the given file_entry.
   CU_INFO describes the CU's DW_AT_name and DW_AT_comp_dir.
   If space for the result is malloc'd, *NAME_HOLDER will be set.
   Returns NULL if FILE_INDEX should be ignored, i.e., it is
   equivalent to CU_INFO.  */

static const char *
compute_include_file_name (const struct line_header *lh, const file_entry &fe,
			   const file_and_directory &cu_info,
			   std::string &name_holder)
{
  const char *include_name = fe.name;
  const char *include_name_to_compare = include_name;

  const char *dir_name = fe.include_dir (lh);

  std::string hold_compare;
  if (!IS_ABSOLUTE_PATH (include_name)
      && (dir_name != nullptr || cu_info.get_comp_dir () != nullptr))
    {
      /* Avoid creating a duplicate name for CU_INFO.
	 We do this by comparing INCLUDE_NAME and CU_INFO.
	 Before we do the comparison, however, we need to account
	 for DIR_NAME and COMP_DIR.
	 First prepend dir_name (if non-NULL).  If we still don't
	 have an absolute path prepend comp_dir (if non-NULL).
	 However, the directory we record in the include-file's
	 psymtab does not contain COMP_DIR (to match the
	 corresponding symtab(s)).

	 Example:

	 bash$ cd /tmp
	 bash$ gcc -g ./hello.c
	 include_name = "hello.c"
	 dir_name = "."
	 DW_AT_comp_dir = comp_dir = "/tmp"
	 DW_AT_name = "./hello.c"

      */

      if (dir_name != NULL)
	{
	  name_holder = path_join (dir_name, include_name);
	  include_name = name_holder.c_str ();
	  include_name_to_compare = include_name;
	}
      if (!IS_ABSOLUTE_PATH (include_name)
	  && cu_info.get_comp_dir () != nullptr)
	{
	  hold_compare = path_join (cu_info.get_comp_dir (), include_name);
	  include_name_to_compare = hold_compare.c_str ();
	}
    }

  std::string copied_name;
  const char *cu_filename = cu_info.get_name ();
  if (!IS_ABSOLUTE_PATH (cu_filename) && cu_info.get_comp_dir () != nullptr)
    {
      copied_name = path_join (cu_info.get_comp_dir (), cu_filename);
      cu_filename = copied_name.c_str ();
    }

  if (FILENAME_CMP (include_name_to_compare, cu_filename) == 0)
    return nullptr;
  return include_name;
}

/* State machine to track the state of the line number program.  */

class lnp_state_machine
{
public:
  /* Initialize a machine state for the start of a line number
     program.  */
  lnp_state_machine (struct dwarf2_cu *cu, gdbarch *arch, line_header *lh);

  file_entry *current_file ()
  {
    /* lh->file_names is 0-based, but the file name numbers in the
       statement program are 1-based.  */
    return m_line_header->file_name_at (m_file);
  }

  /* Record the line in the state machine.  END_SEQUENCE is true if
     we're processing the end of a sequence.  */
  void record_line (bool end_sequence);

  /* Check ADDRESS is -1, -2, or zero and less than UNRELOCATED_LOWPC, and if
     true nop-out rest of the lines in this sequence.  */
  void check_line_address (struct dwarf2_cu *cu,
			   const gdb_byte *line_ptr,
			   unrelocated_addr unrelocated_lowpc,
			   unrelocated_addr address);

  void handle_set_discriminator (unsigned int discriminator)
  {
    m_discriminator = discriminator;
    m_line_has_non_zero_discriminator |= discriminator != 0;
  }

  /* Handle DW_LNE_set_address.  */
  void handle_set_address (unrelocated_addr address)
  {
    m_op_index = 0;
    m_address
      = (unrelocated_addr) gdbarch_adjust_dwarf2_line (m_gdbarch,
						       (CORE_ADDR) address,
						       false);
  }

  /* Handle DW_LNS_advance_pc.  */
  void handle_advance_pc (CORE_ADDR adjust);

  /* Handle a special opcode.  */
  void handle_special_opcode (unsigned char op_code);

  /* Handle DW_LNS_advance_line.  */
  void handle_advance_line (int line_delta)
  {
    advance_line (line_delta);
  }

  /* Handle DW_LNS_set_file.  */
  void handle_set_file (file_name_index file);

  /* Handle DW_LNS_negate_stmt.  */
  void handle_negate_stmt ()
  {
    m_flags ^= LEF_IS_STMT;
  }

  /* Handle DW_LNS_const_add_pc.  */
  void handle_const_add_pc ();

  /* Handle DW_LNS_fixed_advance_pc.  */
  void handle_fixed_advance_pc (CORE_ADDR addr_adj)
  {
    addr_adj = gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
    m_address = (unrelocated_addr) ((CORE_ADDR) m_address + addr_adj);
    m_op_index = 0;
  }

  /* Handle DW_LNS_copy.  */
  void handle_copy ()
  {
    record_line (false);
    m_discriminator = 0;
    m_flags &= ~LEF_PROLOGUE_END;
    m_flags &= ~LEF_EPILOGUE_BEGIN;
  }

  /* Handle DW_LNE_end_sequence.  */
  void handle_end_sequence ()
  {
    m_currently_recording_lines = true;
  }

  /* Handle DW_LNS_set_prologue_end.  */
  void handle_set_prologue_end ()
  {
    m_flags |= LEF_PROLOGUE_END;
  }

  void handle_set_epilogue_begin ()
  {
    m_flags |= LEF_EPILOGUE_BEGIN;
  }

private:
  /* Advance the line by LINE_DELTA.  */
  void advance_line (int line_delta)
  {
    m_line += line_delta;

    if (line_delta != 0)
      m_line_has_non_zero_discriminator = m_discriminator != 0;
  }

  struct dwarf2_cu *m_cu;

  gdbarch *m_gdbarch;

  /* The line number header.  */
  line_header *m_line_header;

  /* These are part of the standard DWARF line number state machine,
     and initialized according to the DWARF spec.  */

  unsigned char m_op_index = 0;
  /* The line table index of the current file.  */
  file_name_index m_file = 1;
  unsigned int m_line = 1;

  /* These are initialized in the constructor.  */

  unrelocated_addr m_address;
  linetable_entry_flags m_flags;
  unsigned int m_discriminator = 0;

  /* Additional bits of state we need to track.  */

  /* The last file a line number was recorded for.  */
  struct subfile *m_last_subfile = NULL;

  /* The address of the last line entry.  */
  unrelocated_addr m_last_address;

  /* Set to true when a previous line at the same address (using
     m_last_address) had LEF_IS_STMT set in m_flags.  This is reset to false
     when a line entry at a new address (m_address different to
     m_last_address) is processed.  */
  bool m_stmt_at_address = false;

  /* When true, record the lines we decode.  */
  bool m_currently_recording_lines = true;

  /* The last line number that was recorded, used to coalesce
     consecutive entries for the same line.  This can happen, for
     example, when discriminators are present.  PR 17276.  */
  unsigned int m_last_line = 0;
  bool m_line_has_non_zero_discriminator = false;
};

void
lnp_state_machine::handle_advance_pc (CORE_ADDR adjust)
{
  CORE_ADDR addr_adj = (((m_op_index + adjust)
			 / m_line_header->maximum_ops_per_instruction)
			* m_line_header->minimum_instruction_length);
  addr_adj = gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
  m_address = (unrelocated_addr) ((CORE_ADDR) m_address + addr_adj);
  m_op_index = ((m_op_index + adjust)
		% m_line_header->maximum_ops_per_instruction);
}

void
lnp_state_machine::handle_special_opcode (unsigned char op_code)
{
  unsigned char adj_opcode = op_code - m_line_header->opcode_base;
  unsigned char adj_opcode_d = adj_opcode / m_line_header->line_range;
  unsigned char adj_opcode_r = adj_opcode % m_line_header->line_range;
  CORE_ADDR addr_adj = (((m_op_index + adj_opcode_d)
			 / m_line_header->maximum_ops_per_instruction)
			* m_line_header->minimum_instruction_length);
  addr_adj = gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
  m_address = (unrelocated_addr) ((CORE_ADDR) m_address + addr_adj);
  m_op_index = ((m_op_index + adj_opcode_d)
		% m_line_header->maximum_ops_per_instruction);

  int line_delta = m_line_header->line_base + adj_opcode_r;
  advance_line (line_delta);
  record_line (false);
  m_discriminator = 0;
  m_flags &= ~LEF_PROLOGUE_END;
  m_flags &= ~LEF_EPILOGUE_BEGIN;
}

void
lnp_state_machine::handle_set_file (file_name_index file)
{
  m_file = file;

  const file_entry *fe = current_file ();
  if (fe == NULL)
    dwarf2_debug_line_missing_file_complaint ();
  else
    {
      m_line_has_non_zero_discriminator = m_discriminator != 0;
      dwarf2_start_subfile (m_cu, *fe, *m_line_header);
    }
}

void
lnp_state_machine::handle_const_add_pc ()
{
  CORE_ADDR adjust
    = (255 - m_line_header->opcode_base) / m_line_header->line_range;

  CORE_ADDR addr_adj
    = (((m_op_index + adjust)
	/ m_line_header->maximum_ops_per_instruction)
       * m_line_header->minimum_instruction_length);

  addr_adj = gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
  m_address = (unrelocated_addr) ((CORE_ADDR) m_address + addr_adj);
  m_op_index = ((m_op_index + adjust)
		% m_line_header->maximum_ops_per_instruction);
}

/* Return non-zero if we should add LINE to the line number table.
   LINE is the line to add, LAST_LINE is the last line that was added,
   LAST_SUBFILE is the subfile for LAST_LINE.
   LINE_HAS_NON_ZERO_DISCRIMINATOR is non-zero if LINE has ever
   had a non-zero discriminator.

   We have to be careful in the presence of discriminators.
   E.g., for this line:

     for (i = 0; i < 100000; i++);

   clang can emit four line number entries for that one line,
   each with a different discriminator.
   See gdb.dwarf2/dw2-single-line-discriminators.exp for an example.

   However, we want gdb to coalesce all four entries into one.
   Otherwise the user could stepi into the middle of the line and
   gdb would get confused about whether the pc really was in the
   middle of the line.

   Things are further complicated by the fact that two consecutive
   line number entries for the same line is a heuristic used by gcc
   to denote the end of the prologue.  So we can't just discard duplicate
   entries, we have to be selective about it.  The heuristic we use is
   that we only collapse consecutive entries for the same line if at least
   one of those entries has a non-zero discriminator.  PR 17276.

   Note: Addresses in the line number state machine can never go backwards
   within one sequence, thus this coalescing is ok.  */

static int
dwarf_record_line_p (struct dwarf2_cu *cu,
		     unsigned int line, unsigned int last_line,
		     int line_has_non_zero_discriminator,
		     struct subfile *last_subfile)
{
  if (cu->get_builder ()->get_current_subfile () != last_subfile)
    return 1;
  if (line != last_line)
    return 1;
  /* Same line for the same file that we've seen already.
     As a last check, for pr 17276, only record the line if the line
     has never had a non-zero discriminator.  */
  if (!line_has_non_zero_discriminator)
    return 1;
  return 0;
}

/* Use the CU's builder to record line number LINE beginning at
   address ADDRESS in the line table of subfile SUBFILE.  */

static void
dwarf_record_line_1 (struct gdbarch *gdbarch, struct subfile *subfile,
		     unsigned int line, unrelocated_addr address,
		     linetable_entry_flags flags,
		     struct dwarf2_cu *cu)
{
  unrelocated_addr addr
    = unrelocated_addr (gdbarch_addr_bits_remove (gdbarch,
						  (CORE_ADDR) address));

  if (cu != nullptr)
    {
      if (dwarf_line_debug)
	gdb_printf (gdb_stdlog, "Recording line %u, file %s, address %s\n",
		    line, lbasename (subfile->name.c_str ()),
		    paddress (gdbarch, (CORE_ADDR) address));

      cu->get_builder ()->record_line (subfile, line, addr, flags);
    }
}

/* Subroutine of dwarf_decode_lines_1 to simplify it.
   Mark the end of a set of line number records.
   The arguments are the same as for dwarf_record_line_1.
   If SUBFILE is NULL the request is ignored.  */

static void
dwarf_finish_line (struct gdbarch *gdbarch, struct subfile *subfile,
		   unrelocated_addr address, struct dwarf2_cu *cu)
{
  if (subfile == NULL)
    return;

  if (dwarf_line_debug)
    {
      gdb_printf (gdb_stdlog,
		  "Finishing current line, file %s, address %s\n",
		  lbasename (subfile->name.c_str ()),
		  paddress (gdbarch, (CORE_ADDR) address));
    }

  dwarf_record_line_1 (gdbarch, subfile, 0, address, LEF_IS_STMT, cu);
}

void
lnp_state_machine::record_line (bool end_sequence)
{
  if (dwarf_line_debug)
    {
      gdb_printf (gdb_stdlog,
		  "Processing actual line %u: file %u,"
		  " address %s, is_stmt %u, prologue_end %u,"
		  " epilogue_begin %u, discrim %u%s\n",
		  m_line, m_file,
		  paddress (m_gdbarch, (CORE_ADDR) m_address),
		  (m_flags & LEF_IS_STMT) != 0,
		  (m_flags & LEF_PROLOGUE_END) != 0,
		  (m_flags & LEF_EPILOGUE_BEGIN) != 0,
		  m_discriminator,
		  (end_sequence ? "\t(end sequence)" : ""));
    }

  file_entry *fe = current_file ();

  if (fe == NULL)
    dwarf2_debug_line_missing_file_complaint ();
  /* For now we ignore lines not starting on an instruction boundary.
     But not when processing end_sequence for compatibility with the
     previous version of the code.  */
  else if (m_op_index == 0 || end_sequence)
    {
      /* When we switch files we insert an end maker in the first file,
	 switch to the second file and add a new line entry.  The
	 problem is that the end marker inserted in the first file will
	 discard any previous line entries at the same address.  If the
	 line entries in the first file are marked as is-stmt, while
	 the new line in the second file is non-stmt, then this means
	 the end marker will discard is-stmt lines so we can have a
	 non-stmt line.  This means that there are less addresses at
	 which the user can insert a breakpoint.

	 To improve this we track the last address in m_last_address,
	 and whether we have seen an is-stmt at this address.  Then
	 when switching files, if we have seen a stmt at the current
	 address, and we are switching to create a non-stmt line, then
	 discard the new line.  */
      bool file_changed
	= m_last_subfile != m_cu->get_builder ()->get_current_subfile ();
      bool ignore_this_line
	= ((file_changed && !end_sequence && m_last_address == m_address
	    && ((m_flags & LEF_IS_STMT) == 0)
	    && m_stmt_at_address)
	   || (!end_sequence && m_line == 0));

      if ((file_changed && !ignore_this_line) || end_sequence)
	{
	  dwarf_finish_line (m_gdbarch, m_last_subfile, m_address,
			     m_currently_recording_lines ? m_cu : nullptr);
	}

      if (!end_sequence && !ignore_this_line)
	{
	  linetable_entry_flags lte_flags = m_flags;
	  if (m_cu->producer_is_codewarrior ())
	    lte_flags |= LEF_IS_STMT;

	  if (dwarf_record_line_p (m_cu, m_line, m_last_line,
				   m_line_has_non_zero_discriminator,
				   m_last_subfile))
	    {
	      buildsym_compunit *builder = m_cu->get_builder ();
	      dwarf_record_line_1 (m_gdbarch,
				   builder->get_current_subfile (),
				   m_line, m_address, lte_flags,
				   m_currently_recording_lines ? m_cu : nullptr);

	      m_last_subfile = m_cu->get_builder ()->get_current_subfile ();
	      m_last_line = m_line;
	    }
	}
    }

  /* Track whether we have seen any IS_STMT true at m_address in case we
     have multiple line table entries all at m_address.  */
  if (m_last_address != m_address)
    {
      m_stmt_at_address = false;
      m_last_address = m_address;
    }
  m_stmt_at_address |= (m_flags & LEF_IS_STMT) != 0;
}

lnp_state_machine::lnp_state_machine (struct dwarf2_cu *cu, gdbarch *arch,
				      line_header *lh)
  : m_cu (cu),
    m_gdbarch (arch),
    m_line_header (lh),
    /* Call `gdbarch_adjust_dwarf2_line' on the initial 0 address as
       if there was a line entry for it so that the backend has a
       chance to adjust it and also record it in case it needs it.
       This is currently used by MIPS code,
       cf. `mips_adjust_dwarf2_line'.  */
    m_address ((unrelocated_addr) gdbarch_adjust_dwarf2_line (arch, 0, 0)),
    m_flags (lh->default_is_stmt ? LEF_IS_STMT : (linetable_entry_flags) 0),
    m_last_address (m_address)
{
}

void
lnp_state_machine::check_line_address (struct dwarf2_cu *cu,
				       const gdb_byte *line_ptr,
				       unrelocated_addr unrelocated_lowpc,
				       unrelocated_addr address)
{
  /* Linkers resolve a symbolic relocation referencing a GC'd function to 0,
     -1 or -2 (-2 is used by certain lld versions, see
     https://github.com/llvm/llvm-project/commit/e618ccbf431f6730edb6d1467a127c3a52fd57f7).
     If ADDRESS is 0, ignoring the opcode will err if the text section is
     located at 0x0.  In this case, additionally check that if
     ADDRESS < UNRELOCATED_LOWPC.  */

  if ((address == (unrelocated_addr) 0 && address < unrelocated_lowpc)
      || address == (unrelocated_addr) -1
      || address == (unrelocated_addr) -2)
    {
      /* This line table is for a function which has been
	 GCd by the linker.  Ignore it.  PR gdb/12528 */

      struct objfile *objfile = cu->per_objfile->objfile;
      long line_offset = line_ptr - get_debug_line_section (cu)->buffer;

      complaint (_(".debug_line address at offset 0x%lx is 0 [in module %s]"),
		 line_offset, objfile_name (objfile));
      m_currently_recording_lines = false;
      /* Note: m_currently_recording_lines is left as false until we see
	 DW_LNE_end_sequence.  */
    }
}

/* Subroutine of dwarf_decode_lines to simplify it.
   Process the line number information in LH.  */

static void
dwarf_decode_lines_1 (struct line_header *lh, struct dwarf2_cu *cu,
		      unrelocated_addr lowpc)
{
  const gdb_byte *line_ptr, *extended_end;
  const gdb_byte *line_end;
  unsigned int bytes_read, extended_len;
  unsigned char op_code, extended_op;
  struct objfile *objfile = cu->per_objfile->objfile;
  bfd *abfd = objfile->obfd.get ();
  struct gdbarch *gdbarch = objfile->arch ();

  line_ptr = lh->statement_program_start;
  line_end = lh->statement_program_end;

  /* Read the statement sequences until there's nothing left.  */
  while (line_ptr < line_end)
    {
      /* The DWARF line number program state machine.  Reset the state
	 machine at the start of each sequence.  */
      lnp_state_machine state_machine (cu, gdbarch, lh);
      bool end_sequence = false;

      /* Start a subfile for the current file of the state
	 machine.  */
      const file_entry *fe = state_machine.current_file ();

      if (fe != NULL)
	dwarf2_start_subfile (cu, *fe, *lh);

      /* Decode the table.  */
      while (line_ptr < line_end && !end_sequence)
	{
	  op_code = read_1_byte (abfd, line_ptr);
	  line_ptr += 1;

	  if (op_code >= lh->opcode_base)
	    {
	      /* Special opcode.  */
	      state_machine.handle_special_opcode (op_code);
	    }
	  else switch (op_code)
	    {
	    case DW_LNS_extended_op:
	      extended_len = read_unsigned_leb128 (abfd, line_ptr,
						   &bytes_read);
	      line_ptr += bytes_read;
	      extended_end = line_ptr + extended_len;
	      extended_op = read_1_byte (abfd, line_ptr);
	      line_ptr += 1;
	      if (DW_LNE_lo_user <= extended_op
		  && extended_op <= DW_LNE_hi_user)
		{
		  /* Vendor extension, ignore.  */
		  line_ptr = extended_end;
		  break;
		}
	      switch (extended_op)
		{
		case DW_LNE_end_sequence:
		  state_machine.handle_end_sequence ();
		  end_sequence = true;
		  break;
		case DW_LNE_set_address:
		  {
		    unrelocated_addr address
		      = cu->header.read_address (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;

		    state_machine.check_line_address (cu, line_ptr, lowpc,
						      address);
		    state_machine.handle_set_address (address);
		  }
		  break;
		case DW_LNE_define_file:
		  {
		    const char *cur_file;
		    unsigned int mod_time, length;
		    dir_index dindex;

		    cur_file = read_direct_string (abfd, line_ptr,
						   &bytes_read);
		    line_ptr += bytes_read;
		    dindex = (dir_index)
		      read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;
		    mod_time =
		      read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;
		    length =
		      read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;
		    lh->add_file_name (cur_file, dindex, mod_time, length);
		  }
		  break;
		case DW_LNE_set_discriminator:
		  {
		    /* The discriminator is not interesting to the
		       debugger; just ignore it.  We still need to
		       check its value though:
		       if there are consecutive entries for the same
		       (non-prologue) line we want to coalesce them.
		       PR 17276.  */
		    unsigned int discr
		      = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;

		    state_machine.handle_set_discriminator (discr);
		  }
		  break;
		default:
		  complaint (_("mangled .debug_line section"));
		  return;
		}
	      /* Make sure that we parsed the extended op correctly.  If e.g.
		 we expected a different address size than the producer used,
		 we may have read the wrong number of bytes.  */
	      if (line_ptr != extended_end)
		{
		  complaint (_("mangled .debug_line section"));
		  return;
		}
	      break;
	    case DW_LNS_copy:
	      state_machine.handle_copy ();
	      break;
	    case DW_LNS_advance_pc:
	      {
		CORE_ADDR adjust
		  = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		line_ptr += bytes_read;

		state_machine.handle_advance_pc (adjust);
	      }
	      break;
	    case DW_LNS_advance_line:
	      {
		int line_delta
		  = read_signed_leb128 (abfd, line_ptr, &bytes_read);
		line_ptr += bytes_read;

		state_machine.handle_advance_line (line_delta);
	      }
	      break;
	    case DW_LNS_set_file:
	      {
		file_name_index file
		  = (file_name_index) read_unsigned_leb128 (abfd, line_ptr,
							    &bytes_read);
		line_ptr += bytes_read;

		state_machine.handle_set_file (file);
	      }
	      break;
	    case DW_LNS_set_column:
	      (void) read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	      line_ptr += bytes_read;
	      break;
	    case DW_LNS_negate_stmt:
	      state_machine.handle_negate_stmt ();
	      break;
	    case DW_LNS_set_basic_block:
	      break;
	    /* Add to the address register of the state machine the
	       address increment value corresponding to special opcode
	       255.  I.e., this value is scaled by the minimum
	       instruction length since special opcode 255 would have
	       scaled the increment.  */
	    case DW_LNS_const_add_pc:
	      state_machine.handle_const_add_pc ();
	      break;
	    case DW_LNS_fixed_advance_pc:
	      {
		CORE_ADDR addr_adj = read_2_bytes (abfd, line_ptr);
		line_ptr += 2;

		state_machine.handle_fixed_advance_pc (addr_adj);
	      }
	      break;
	    case DW_LNS_set_prologue_end:
	      state_machine.handle_set_prologue_end ();
	      break;
	    case DW_LNS_set_epilogue_begin:
	      state_machine.handle_set_epilogue_begin ();
	      break;
	    default:
	      {
		/* Unknown standard opcode, ignore it.  */
		int i;

		for (i = 0; i < lh->standard_opcode_lengths[op_code]; i++)
		  {
		    (void) read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;
		  }
	      }
	    }
	}

      if (!end_sequence)
	dwarf2_debug_line_missing_end_sequence_complaint ();

      /* We got a DW_LNE_end_sequence (or we ran off the end of the buffer,
	 in which case we still finish recording the last line).  */
      state_machine.record_line (true);
    }
}

/* Decode the Line Number Program (LNP) for the given line_header
   structure and CU.  The actual information extracted and the type
   of structures created from the LNP depends on the value of PST.

   FND holds the CU file name and directory, if known.
   It is used for relative paths in the line table.

   NOTE: It is important that psymtabs have the same file name (via
   strcmp) as the corresponding symtab.  Since the directory is not
   used in the name of the symtab we don't use it in the name of the
   psymtabs we create.  E.g. expand_line_sal requires this when
   finding psymtabs to expand.  A good testcase for this is
   mb-inline.exp.

   LOWPC is the lowest address in CU (or 0 if not known).

   Boolean DECODE_MAPPING specifies we need to fully decode .debug_line
   for its PC<->lines mapping information.  Otherwise only the filename
   table is read in.  */

static void
dwarf_decode_lines (struct line_header *lh, struct dwarf2_cu *cu,
		    unrelocated_addr lowpc, int decode_mapping)
{
  if (decode_mapping)
    dwarf_decode_lines_1 (lh, cu, lowpc);

  /* Make sure a symtab is created for every file, even files
     which contain only variables (i.e. no code with associated
     line numbers).  */
  buildsym_compunit *builder = cu->get_builder ();
  struct compunit_symtab *cust = builder->get_compunit_symtab ();

  for (auto &fe : lh->file_names ())
    {
      dwarf2_start_subfile (cu, fe, *lh);
      subfile *sf = builder->get_current_subfile ();

      if (sf->symtab == nullptr)
	sf->symtab = allocate_symtab (cust, sf->name.c_str (),
				      sf->name_for_id.c_str ());

      fe.symtab = sf->symtab;
    }
}

/* Start a subfile for DWARF.  FILENAME is the name of the file and
   DIRNAME the name of the source directory which contains FILENAME
   or NULL if not known.
   This routine tries to keep line numbers from identical absolute and
   relative file names in a common subfile.

   Using the `list' example from the GDB testsuite, which resides in
   /srcdir and compiling it with Irix6.2 cc in /compdir using a filename
   of /srcdir/list0.c yields the following debugging information for list0.c:

   DW_AT_name:          /srcdir/list0.c
   DW_AT_comp_dir:      /compdir
   files.files[0].name: list0.h
   files.files[0].dir:  /srcdir
   files.files[1].name: list0.c
   files.files[1].dir:  /srcdir

   The line number information for list0.c has to end up in a single
   subfile, so that `break /srcdir/list0.c:1' works as expected.
   start_subfile will ensure that this happens provided that we pass the
   concatenation of files.files[1].dir and files.files[1].name as the
   subfile's name.  */

static void
dwarf2_start_subfile (dwarf2_cu *cu, const file_entry &fe,
		      const line_header &lh)
{
  std::string filename_holder;
  const char *filename = fe.name;
  const char *dirname = lh.include_dir_at (fe.d_index);

  /* In order not to lose the line information directory,
     we concatenate it to the filename when it makes sense.
     Note that the Dwarf3 standard says (speaking of filenames in line
     information): ``The directory index is ignored for file names
     that represent full path names''.  Thus ignoring dirname in the
     `else' branch below isn't an issue.  */

  if (!IS_ABSOLUTE_PATH (filename) && dirname != NULL)
    {
      filename_holder = path_join (dirname, filename);
      filename = filename_holder.c_str ();
    }

  std::string filename_for_id = lh.file_file_name (fe);
  cu->get_builder ()->start_subfile (filename, filename_for_id.c_str ());
}

static void
var_decode_location (struct attribute *attr, struct symbol *sym,
		     struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  unit_head *cu_header = &cu->header;

  /* NOTE drow/2003-01-30: There used to be a comment and some special
     code here to turn a symbol with DW_AT_external and a
     SYMBOL_VALUE_ADDRESS of 0 into a LOC_UNRESOLVED symbol.  This was
     necessary for platforms (maybe Alpha, certainly PowerPC GNU/Linux
     with some versions of binutils) where shared libraries could have
     relocations against symbols in their debug information - the
     minimal symbol would have the right address, but the debug info
     would not.  It's no longer necessary, because we will explicitly
     apply relocations when we read in the debug information now.  */

  /* A DW_AT_location attribute with no contents indicates that a
     variable has been optimized away.  */
  if (attr->form_is_block () && attr->as_block ()->size == 0)
    {
      sym->set_loc_class_index (LOC_OPTIMIZED_OUT);
      return;
    }

  /* Handle one degenerate form of location expression specially, to
     preserve GDB's previous behavior when section offsets are
     specified.  If this is just a DW_OP_addr, DW_OP_addrx, or
     DW_OP_GNU_addr_index then mark this symbol as LOC_STATIC.  */

  if (attr->form_is_block ())
    {
      struct dwarf_block *block = attr->as_block ();

      if ((block->data[0] == DW_OP_addr
	   && block->size == 1 + cu_header->addr_size)
	  || ((block->data[0] == DW_OP_GNU_addr_index
	       || block->data[0] == DW_OP_addrx)
	      && (block->size
		  == 1 + leb128_size (&block->data[1]))))
	{
	  unsigned int dummy;

	  unrelocated_addr tem;
	  if (block->data[0] == DW_OP_addr)
	    tem = cu->header.read_address (objfile->obfd.get (),
					   block->data + 1,
					   &dummy);
	  else
	    tem = read_addr_index_from_leb128 (cu, block->data + 1, &dummy);
	  sym->set_value_address ((CORE_ADDR) tem);
	  sym->set_loc_class_index (LOC_STATIC);
	  fixup_symbol_section (sym, objfile);
	  sym->set_value_address
	    (sym->value_address ()
	     + objfile->section_offsets[sym->section_index ()]);
	  return;
	}
    }

  /* NOTE drow/2002-01-30: It might be worthwhile to have a static
     expression evaluator, and use LOC_COMPUTED only when necessary
     (i.e. when the value of a register or memory location is
     referenced, or a thread-local block, etc.).  Then again, it might
     not be worthwhile.  I'm assuming that it isn't unless performance
     or memory numbers show me otherwise.  */

  dwarf2_symbol_mark_computed (attr, sym, cu, 0);

  if (sym->computed_ops ()->location_has_loclist)
    cu->has_loclist = true;
}

/* A helper function to add an "export" symbol.  The new symbol starts
   as a clone of ORIG, but is modified to defer to the symbol named
   ORIG_NAME.  The original symbol uses the name given in the source
   code, and the symbol that is created here uses the linkage name as
   its name.  See ada-imported.c.  */

static void
add_ada_export_symbol (struct symbol *orig, const char *new_name,
		       const char *orig_name, struct dwarf2_cu *cu,
		       struct pending **list_to_add)
{
  struct symbol *copy
    = new (&cu->per_objfile->objfile->objfile_obstack) symbol (*orig);
  copy->set_linkage_name (new_name);
  SYMBOL_LOCATION_BATON (copy) = (void *) orig_name;
  copy->set_loc_class_index (copy->loc_class () == LOC_BLOCK
			  ? ada_block_index
			  : ada_imported_index);
  add_symbol_to_list (copy, list_to_add);
}

/* A helper function that decides if a given symbol is an Ada Pragma
   Import or Pragma Export.  */

static bool
is_ada_import_or_export (dwarf2_cu *cu, const char *name,
			 const char *linkagename)
{
  return (cu->lang () == language_ada
	  && linkagename != nullptr
	  && !streq (name, linkagename)
	  /* The following exclusions are necessary because symbols
	     with names or linkage names that match here will meet the
	     other criteria but are not in fact caused by Pragma
	     Import or Pragma Export, and applying the import/export
	     treatment to them will introduce problems.  Some of these
	     checks only apply to functions, but it is simpler and
	     harmless to always do them all.  */
	  && !startswith (name, "__builtin")
	  && !startswith (linkagename, "___ghost_")
	  && !startswith (linkagename, "__gnat")
	  && !startswith (linkagename, "_ada_")
	  && !streq (linkagename, "adainit"));
}

/* Given a pointer to a DWARF information entry, figure out if we need
   to make a symbol table entry for it, and if so, create a new entry
   and return a pointer to it.
   If TYPE is NULL, determine symbol type from the die, otherwise
   used the passed type.
   If SPACE is not NULL, use it to hold the new symbol.  If it is
   NULL, allocate a new symbol on the objfile's obstack.  */

static struct symbol *
new_symbol (struct die_info *die, struct type *type, struct dwarf2_cu *cu,
	    struct symbol *space)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct symbol *sym = NULL;
  const char *name;
  struct attribute *attr = NULL;
  struct attribute *attr2 = NULL;
  struct pending **list_to_add = NULL;

  int inlined_func = (die->tag == DW_TAG_inlined_subroutine);

  name = dwarf2_name (die, cu);
  if (name == nullptr && (die->tag == DW_TAG_subprogram
			  || die->tag == DW_TAG_inlined_subroutine
			  || die->tag == DW_TAG_entry_point))
    name = dw2_linkage_name (die, cu);

  if (name)
    {
      int suppress_add = 0;

      if (space)
	sym = space;
      else
	sym = new (&objfile->objfile_obstack) symbol;
      OBJSTAT (objfile, n_syms++);

      /* Cache this symbol's name and the name's demangled form (if any).  */
      sym->set_language (cu->lang (), &objfile->objfile_obstack);
      /* Fortran does not have mangling standard and the mangling does differ
	 between gfortran, iFort etc.  */
      const char *physname
	= ((cu->lang () == language_fortran || cu->lang () == language_ada)
	   ? dwarf2_full_name (name, die, cu)
	   : dwarf2_physname (name, die, cu));
      const char *linkagename = dw2_linkage_name (die, cu);

      if (linkagename == nullptr)
	sym->set_linkage_name (physname);
      else if (cu->lang () == language_ada)
	sym->set_linkage_name (linkagename);
      else
	{
	  if (physname == linkagename)
	    sym->set_demangled_name (name, &objfile->objfile_obstack);
	  else
	    sym->set_demangled_name (physname, &objfile->objfile_obstack);

	  sym->set_linkage_name (linkagename);
	}

      /* Handle DW_AT_artificial.  */
      attr = dwarf2_attr (die, DW_AT_artificial, cu);
      if (attr != nullptr)
	sym->set_is_artificial (attr->as_boolean ());

      /* Default assumptions.
	 Use the passed type or decode it from the die.  */
      sym->set_domain (UNDEF_DOMAIN);
      sym->set_loc_class_index (LOC_OPTIMIZED_OUT);
      if (type != NULL)
	sym->set_type (type);
      else
	sym->set_type (die_type (die, cu));
      attr = dwarf2_attr (die,
			  inlined_func ? DW_AT_call_line : DW_AT_decl_line,
			  cu);
      if (attr != nullptr)
	sym->set_line (attr->unsigned_constant ().value_or (0));

      struct dwarf2_cu *file_cu = cu;
      attr = dwarf2_attr (die,
			  inlined_func ? DW_AT_call_file : DW_AT_decl_file,
			  &file_cu);
      if (attr != nullptr)
	{
	  std::optional<ULONGEST> index_cst = attr->unsigned_constant ();
	  if (index_cst.has_value ())
	    {
	      file_name_index file_index = (file_name_index) *index_cst;
	      struct file_entry *fe;

	      if (file_cu->line_header == nullptr)
		{
		  file_and_directory fnd (nullptr, nullptr);
		  handle_DW_AT_stmt_list (file_cu->dies, file_cu, fnd, {}, false);
		}

	      if (file_cu->line_header != nullptr)
		fe = file_cu->line_header->file_name_at (file_index);
	      else
		fe = NULL;

	      if (fe == NULL)
		complaint (_("file index out of range"));
	      else
		sym->set_symtab (fe->symtab);
	    }
	}

      switch (die->tag)
	{
	case DW_TAG_label:
	  attr = dwarf2_attr (die, DW_AT_low_pc, cu);
	  if (attr != nullptr)
	    {
	      CORE_ADDR addr = per_objfile->relocate (attr->as_address ());
	      sym->set_section_index (SECT_OFF_TEXT (objfile));
	      sym->set_value_address (addr);
	      sym->set_loc_class_index (LOC_LABEL);
	    }
	  else
	    sym->set_loc_class_index (LOC_OPTIMIZED_OUT);
	  sym->set_type (builtin_type (objfile)->builtin_core_addr);
	  sym->set_domain (LABEL_DOMAIN);
	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_entry_point:
	  /* SYMBOL_BLOCK_VALUE (sym) will be filled in later by
	     finish_block.  */
	  sym->set_domain (FUNCTION_DOMAIN);
	  sym->set_loc_class_index (LOC_BLOCK);
	  /* DW_TAG_entry_point provides an additional entry_point to an
	     existing sub_program.  Therefore, we inherit the "external"
	     attribute from the sub_program to which the entry_point
	     belongs to.  */
	  attr2 = dwarf2_attr (die->parent, DW_AT_external, cu);
	  if (attr2 != nullptr && attr2->as_boolean ())
	    list_to_add = cu->get_builder ()->get_global_symbols ();
	  else
	    list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_subprogram:
	  /* SYMBOL_BLOCK_VALUE (sym) will be filled in later by
	     finish_block.  */
	  sym->set_domain (FUNCTION_DOMAIN);
	  sym->set_loc_class_index (LOC_BLOCK);
	  attr2 = dwarf2_attr (die, DW_AT_external, cu);
	  if ((attr2 != nullptr && attr2->as_boolean ())
	      || cu->lang () == language_ada
	      || cu->lang () == language_fortran)
	    {
	      /* Subprograms marked external are stored as a global symbol.
		 Ada and Fortran subprograms, whether marked external or
		 not, are always stored as a global symbol, because we want
		 to be able to access them globally.  For instance, we want
		 to be able to break on a nested subprogram without having
		 to specify the context.  */
	      list_to_add = cu->get_builder ()->get_global_symbols ();
	    }
	  else
	    {
	      list_to_add = cu->list_in_scope;
	    }

	  if (is_ada_import_or_export (cu, physname, linkagename))
	    {
	      /* This is either a Pragma Import or Export.  They can
		 be distinguished by the declaration flag.  */
	      sym->set_linkage_name (physname);
	      if (die_is_declaration (die, cu))
		{
		  /* For Import, create a symbol using the source
		     name, and have it refer to the linkage name.  */
		  SYMBOL_LOCATION_BATON (sym) = (void *) linkagename;
		  sym->set_loc_class_index (ada_block_index);
		}
	      else
		{
		  /* For Export, create a symbol using the source
		     name, then create a second symbol that refers
		     back to it.  */
		  add_ada_export_symbol (sym, linkagename, physname, cu,
					 list_to_add);
		}
	    }
	  break;
	case DW_TAG_inlined_subroutine:
	  /* SYMBOL_BLOCK_VALUE (sym) will be filled in later by
	     finish_block.  */
	  sym->set_domain (FUNCTION_DOMAIN);
	  sym->set_loc_class_index (LOC_BLOCK);
	  sym->set_is_inlined (1);
	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_template_value_param:
	  suppress_add = 1;
	  [[fallthrough]];
	case DW_TAG_constant:
	case DW_TAG_variable:
	case DW_TAG_member:
	  sym->set_domain (VAR_DOMAIN);
	  /* Compilation with minimal debug info may result in
	     variables with missing type entries.  Change the
	     misleading `void' type to something sensible.  */
	  if (sym->type ()->code () == TYPE_CODE_VOID)
	    sym->set_type (builtin_type (objfile)->builtin_int);

	  attr = dwarf2_attr (die, DW_AT_const_value, cu);
	  /* In the case of DW_TAG_member, we should only be called for
	     static const members.  */
	  if (die->tag == DW_TAG_member)
	    {
	      /* dwarf2_add_field uses die_is_declaration,
		 so we do the same.  */
	      gdb_assert (die_is_declaration (die, cu));
	      gdb_assert (attr);
	    }
	  if (attr != nullptr)
	    {
	      dwarf2_const_value (attr, sym, cu);
	      attr2 = dwarf2_attr (die, DW_AT_external, cu);
	      if (!suppress_add)
		{
		  if (attr2 != nullptr && attr2->as_boolean ())
		    list_to_add = cu->get_builder ()->get_global_symbols ();
		  else
		    list_to_add = cu->list_in_scope;
		}
	      break;
	    }
	  attr = dwarf2_attr (die, DW_AT_location, cu);
	  if (attr != nullptr)
	    {
	      var_decode_location (attr, sym, cu);
	      attr2 = dwarf2_attr (die, DW_AT_external, cu);

	      /* Fortran explicitly imports any global symbols to the local
		 scope by DW_TAG_common_block.  */
	      if (cu->lang () == language_fortran && die->parent
		  && die->parent->tag == DW_TAG_common_block)
		attr2 = NULL;

	      if (sym->loc_class () == LOC_STATIC
		  && sym->value_address () == 0
		  && !per_objfile->per_bfd->has_section_at_zero)
		{
		  /* When a static variable is eliminated by the linker,
		     the corresponding debug information is not stripped
		     out, but the variable address is set to null;
		     do not add such variables into symbol table.  */
		}
	      else if (attr2 != nullptr && attr2->as_boolean ())
		{
		  if (sym->loc_class () == LOC_STATIC
		      && (objfile->flags & OBJF_MAINLINE) == 0
		      && per_objfile->per_bfd->can_copy)
		    {
		      /* A global static variable might be subject to
			 copy relocation.  We first check for a local
			 minsym, though, because maybe the symbol was
			 marked hidden, in which case this would not
			 apply.  */
		      bound_minimal_symbol found
			= (lookup_minimal_symbol_linkage
			   (sym->linkage_name (), objfile, false));
		      if (found.minsym != nullptr)
			sym->maybe_copied = 1;
		    }

		  /* A variable with DW_AT_external is never static,
		     but it may be block-scoped.  */
		  list_to_add
		    = ((cu->list_in_scope
			== cu->get_builder ()->get_file_symbols ())
		       ? cu->get_builder ()->get_global_symbols ()
		       : cu->list_in_scope);
		}
	      else
		list_to_add = cu->list_in_scope;

	      if (list_to_add != nullptr
		  && is_ada_import_or_export (cu, physname, linkagename))
		{
		  /* This is a Pragma Export.  A Pragma Import won't
		     be seen here, because it will not have a location
		     and so will be handled below.  */
		  add_ada_export_symbol (sym, physname, linkagename, cu,
					 list_to_add);
		}
	    }
	  else
	    {
	      /* We do not know the address of this symbol.
		 If it is an external symbol and we have type information
		 for it, enter the symbol as a LOC_UNRESOLVED symbol.
		 The address of the variable will then be determined from
		 the minimal symbol table whenever the variable is
		 referenced.  */
	      attr2 = dwarf2_attr (die, DW_AT_external, cu);

	      /* Fortran explicitly imports any global symbols to the local
		 scope by DW_TAG_common_block.  */
	      if (cu->lang () == language_fortran && die->parent
		  && die->parent->tag == DW_TAG_common_block)
		{
		  /* SYMBOL_CLASS doesn't matter here because
		     read_common_block is going to reset it.  */
		  if (!suppress_add)
		    list_to_add = cu->list_in_scope;
		}
	      else if (is_ada_import_or_export (cu, physname, linkagename))
		{
		  /* This is a Pragma Import.  A Pragma Export won't
		     be seen here, because it will have a location and
		     so will be handled above.  */
		  sym->set_linkage_name (physname);
		  list_to_add
		    = ((cu->list_in_scope
			== cu->get_builder ()->get_file_symbols ())
		       ? cu->get_builder ()->get_global_symbols ()
		       : cu->list_in_scope);
		  SYMBOL_LOCATION_BATON (sym) = (void *) linkagename;
		  sym->set_loc_class_index (ada_imported_index);
		}
	      else if (attr2 != nullptr && attr2->as_boolean ()
		       && dwarf2_attr (die, DW_AT_type, cu) != NULL)
		{
		  /* A variable with DW_AT_external is never static, but it
		     may be block-scoped.  */
		  list_to_add
		    = ((cu->list_in_scope
			== cu->get_builder ()->get_file_symbols ())
		       ? cu->get_builder ()->get_global_symbols ()
		       : cu->list_in_scope);

		  sym->set_loc_class_index (LOC_UNRESOLVED);
		}
	      else if (!die_is_declaration (die, cu))
		{
		  /* Use the default LOC_OPTIMIZED_OUT class.  */
		  gdb_assert (sym->loc_class () == LOC_OPTIMIZED_OUT);
		  if (!suppress_add)
		    list_to_add = cu->list_in_scope;
		}
	    }
	  break;
	case DW_TAG_formal_parameter:
	  {
	    /* If we are inside a function, mark this as an argument.  If
	       not, we might be looking at an argument to an inlined function
	       when we do not have enough information to show inlined frames;
	       pretend it's a local variable in that case so that the user can
	       still see it.  */
	    sym->set_domain (VAR_DOMAIN);
	    struct context_stack *curr
	      = cu->get_builder ()->get_current_context_stack ();
	    if (curr != nullptr && curr->name != nullptr)
	      sym->set_is_argument (1);
	    attr = dwarf2_attr (die, DW_AT_location, cu);
	    if (attr != nullptr)
	      {
		var_decode_location (attr, sym, cu);
	      }
	    attr = dwarf2_attr (die, DW_AT_const_value, cu);
	    if (attr != nullptr)
	      {
		dwarf2_const_value (attr, sym, cu);
	      }

	    list_to_add = cu->list_in_scope;
	  }
	  break;
	case DW_TAG_unspecified_parameters:
	  /* From varargs functions; gdb doesn't seem to have any
	     interest in this information, so just ignore it for now.
	     (FIXME?) */
	  break;
	case DW_TAG_template_type_param:
	  suppress_add = 1;
	  [[fallthrough]];
	case DW_TAG_class_type:
	case DW_TAG_interface_type:
	case DW_TAG_structure_type:
	case DW_TAG_union_type:
	case DW_TAG_set_type:
	case DW_TAG_enumeration_type:
	  if (cu->lang () == language_c
	      || cu->lang () == language_cplus
	      || cu->lang () == language_objc
	      || cu->lang () == language_opencl
	      || cu->lang () == language_minimal)
	    {
	      /* These languages have a tag namespace.  Note that
		 there's a special hack for C++ in the matching code,
		 so we don't need to enter a separate typedef for the
		 tag.  */
	      sym->set_loc_class_index (LOC_TYPEDEF);
	      sym->set_domain (STRUCT_DOMAIN);
	    }
	  else
	    {
	      /* Other languages don't have a tag namespace.  */
	      sym->set_loc_class_index (LOC_TYPEDEF);
	      sym->set_domain (TYPE_DOMAIN);
	    }

	  /* NOTE: carlton/2003-11-10: C++ class symbols shouldn't
	     really ever be static objects: otherwise, if you try
	     to, say, break of a class's method and you're in a file
	     which doesn't mention that class, it won't work unless
	     the check for all static symbols in lookup_symbol_aux
	     saves you.  See the OtherFileClass tests in
	     gdb.c++/namespace.exp.  */

	  if (!suppress_add)
	    {
	      buildsym_compunit *builder = cu->get_builder ();
	      list_to_add
		= (cu->list_in_scope == builder->get_file_symbols ()
		   && cu->lang () == language_cplus
		   ? builder->get_global_symbols ()
		   : cu->list_in_scope);

	      /* The semantics of C++ state that "struct foo {
		 ... }" also defines a typedef for "foo".  */
	      if (cu->lang () == language_cplus
		  || cu->lang () == language_ada
		  || cu->lang () == language_d
		  || cu->lang () == language_rust)
		{
		  /* The symbol's name is already allocated along
		     with this objfile, so we don't need to
		     duplicate it for the type.  */
		  if (sym->type ()->name () == 0)
		    sym->type ()->set_name (sym->search_name ());
		}
	    }
	  break;
	case DW_TAG_unspecified_type:
	  if (cu->lang () == language_ada)
	    break;
	  [[fallthrough]];
	case DW_TAG_typedef:
	case DW_TAG_array_type:
	case DW_TAG_base_type:
	case DW_TAG_subrange_type:
	case DW_TAG_generic_subrange:
	  sym->set_loc_class_index (LOC_TYPEDEF);
	  sym->set_domain (TYPE_DOMAIN);
	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_enumerator:
	  sym->set_domain (VAR_DOMAIN);
	  attr = dwarf2_attr (die, DW_AT_const_value, cu);
	  if (attr != nullptr)
	    {
	      dwarf2_const_value (attr, sym, cu);
	    }

	  /* NOTE: carlton/2003-11-10: See comment above in the
	     DW_TAG_class_type, etc. block.  */

	  list_to_add
	    = (cu->list_in_scope == cu->get_builder ()->get_file_symbols ()
	       && cu->lang () == language_cplus
	       ? cu->get_builder ()->get_global_symbols ()
	       : cu->list_in_scope);
	  break;
	case DW_TAG_imported_declaration:
	case DW_TAG_namespace:
	  sym->set_domain (TYPE_DOMAIN);
	  sym->set_loc_class_index (LOC_TYPEDEF);
	  list_to_add = cu->get_builder ()->get_global_symbols ();
	  break;
	case DW_TAG_module:
	  sym->set_loc_class_index (LOC_TYPEDEF);
	  sym->set_domain (MODULE_DOMAIN);
	  list_to_add = cu->get_builder ()->get_global_symbols ();
	  break;
	case DW_TAG_common_block:
	  sym->set_loc_class_index (LOC_COMMON_BLOCK);
	  sym->set_domain (COMMON_BLOCK_DOMAIN);
	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_namelist:
	  sym->set_loc_class_index (LOC_STATIC);
	  sym->set_domain (VAR_DOMAIN);
	  list_to_add = cu->list_in_scope;
	  break;
	default:
	  /* Not a tag we recognize.  Hopefully we aren't processing
	     trash data, but since we must specifically ignore things
	     we don't recognize, there is nothing else we should do at
	     this point.  */
	  complaint (_("unsupported tag: '%s'"),
		     dwarf_tag_name (die->tag));
	  break;
	}

      if (suppress_add)
	{
	  sym->hash_next = objfile->template_symbols;
	  objfile->template_symbols = sym;
	  list_to_add = NULL;
	}

      if (list_to_add != NULL)
	add_symbol_to_list (sym, list_to_add);

      /* For the benefit of old versions of GCC, check for anonymous
	 namespaces based on the demangled name.  */
      if (!cu->processing_has_namespace_info
	  && cu->lang () == language_cplus)
	cp_scan_for_anonymous_namespaces (cu->get_builder (), sym, objfile);
    }
  return (sym);
}

/* Read a constant value from an attribute.  Either set *VALUE, or if
   the value does not fit in *VALUE, set *BYTES - either already
   allocated on the objfile obstack, or newly allocated on OBSTACK,
   or, set *BATON, if we translated the constant to a location
   expression.  */

static void
dwarf2_const_value_attr (const struct attribute *attr, struct type *type,
			 const char *name, struct obstack *obstack,
			 struct dwarf2_cu *cu,
			 LONGEST *value, const gdb_byte **bytes,
			 struct dwarf2_locexpr_baton **baton)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  unit_head *cu_header = &cu->header;
  struct dwarf_block *blk;
  enum bfd_endian byte_order = (bfd_big_endian (objfile->obfd.get ()) ?
				BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE);

  *value = 0;
  *bytes = NULL;
  *baton = NULL;

  switch (attr->form)
    {
    case DW_FORM_addr:
    case DW_FORM_addrx:
    case DW_FORM_GNU_addr_index:
      {
	gdb_byte *data;

	if (type->length () != cu_header->addr_size)
	  dwarf2_const_value_length_mismatch_complaint (name,
							cu_header->addr_size,
							type->length ());
	/* Symbols of this form are reasonably rare, so we just
	   piggyback on the existing location code rather than writing
	   a new implementation of symbol_computed_ops.  */
	*baton = OBSTACK_ZALLOC (obstack, struct dwarf2_locexpr_baton);
	(*baton)->per_objfile = per_objfile;
	(*baton)->per_cu = cu->per_cu;
	gdb_assert ((*baton)->per_cu);

	(*baton)->size = 2 + cu_header->addr_size;
	data = (gdb_byte *) obstack_alloc (obstack, (*baton)->size);
	(*baton)->data = data;

	data[0] = DW_OP_addr;
	store_unsigned_integer (&data[1], cu_header->addr_size,
				byte_order, (ULONGEST) attr->as_address ());
	data[cu_header->addr_size + 1] = DW_OP_stack_value;
      }
      break;
    case DW_FORM_string:
    case DW_FORM_strp:
    case DW_FORM_strx:
    case DW_FORM_GNU_str_index:
    case DW_FORM_GNU_strp_alt:
    case DW_FORM_strp_sup:
      /* The string is already allocated on the objfile obstack, point
	 directly to it.  */
      *bytes = (const gdb_byte *) attr->as_string ();
      break;
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
    case DW_FORM_exprloc:
    case DW_FORM_data16:
      blk = attr->as_block ();
      if (type->length () != blk->size)
	dwarf2_const_value_length_mismatch_complaint (name, blk->size,
						      type->length ());
      *bytes = blk->data;
      break;

      /* The DW_AT_const_value attributes are supposed to carry the
	 symbol's value "represented as it would be on the target
	 architecture."  By the time we get here, it's already been
	 converted to host endianness, so we just need to sign- or
	 zero-extend it as appropriate.  */
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_sdata:
    case DW_FORM_implicit_const:
    case DW_FORM_udata:
      *value = attr->confused_constant ().value_or (0);
      break;

    default:
      complaint (_("unsupported const value attribute form: '%s'"),
		 dwarf_form_name (attr->form));
      *value = 0;
      break;
    }
}


/* Copy constant value from an attribute to a symbol.  */

static void
dwarf2_const_value (const struct attribute *attr, struct symbol *sym,
		    struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  LONGEST value;
  const gdb_byte *bytes;
  struct dwarf2_locexpr_baton *baton;

  dwarf2_const_value_attr (attr, sym->type (),
			   sym->print_name (),
			   &objfile->objfile_obstack, cu,
			   &value, &bytes, &baton);

  if (baton != NULL)
    {
      SYMBOL_LOCATION_BATON (sym) = baton;
      sym->set_loc_class_index (dwarf2_locexpr_index);
    }
  else if (bytes != NULL)
    {
      sym->set_value_bytes (bytes);
      sym->set_loc_class_index (LOC_CONST_BYTES);
    }
  else
    {
      sym->set_value_longest (value);
      sym->set_loc_class_index (LOC_CONST);
    }
}

/* Return the type of the die in question using its DW_AT_type attribute.  */

static struct type *
die_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *type_attr;

  type_attr = dwarf2_attr (die, DW_AT_type, cu);
  if (!type_attr)
    {
      struct objfile *objfile = cu->per_objfile->objfile;
      /* A missing DW_AT_type represents a void type.  */
      return builtin_type (objfile)->builtin_void;
    }

  return lookup_die_type (die, type_attr, cu);
}

/* True iff CU's producer generates GNAT Ada auxiliary information
   that allows to find parallel types through that information instead
   of having to do expensive parallel lookups by type name.  */

static int
need_gnat_info (struct dwarf2_cu *cu)
{
  /* Assume that the Ada compiler was GNAT, which always produces
     the auxiliary information.  */
  return (cu->lang () == language_ada);
}

/* Return the auxiliary type of the die in question using its
   DW_AT_GNAT_descriptive_type attribute.  Returns NULL if the
   attribute is not present.  */

static struct type *
die_descriptive_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *type_attr;

  type_attr = dwarf2_attr (die, DW_AT_GNAT_descriptive_type, cu);
  if (!type_attr)
    return NULL;

  return lookup_die_type (die, type_attr, cu);
}

/* If DIE has a descriptive_type attribute, then set the TYPE's
   descriptive type accordingly.  */

static void
set_descriptive_type (struct type *type, struct die_info *die,
		      struct dwarf2_cu *cu)
{
  struct type *descriptive_type = die_descriptive_type (die, cu);

  if (descriptive_type)
    {
      ALLOCATE_GNAT_AUX_TYPE (type);
      TYPE_DESCRIPTIVE_TYPE (type) = descriptive_type;
    }
}

/* Return the containing type of the die in question using its
   DW_AT_containing_type attribute.  */

static struct type *
die_containing_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *type_attr;
  struct objfile *objfile = cu->per_objfile->objfile;

  type_attr = dwarf2_attr (die, DW_AT_containing_type, cu);
  if (!type_attr)
    error (_(DWARF_ERROR_PREFIX
	     "Problem turning containing type into gdb type "
	     "[in module %s]"),
	   objfile_name (objfile));

  return lookup_die_type (die, type_attr, cu);
}

/* Return an error marker type to use for the ill formed type in DIE/CU.  */

static struct type *
build_error_marker_type (struct dwarf2_cu *cu, struct die_info *die)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  char *saved;

  std::string message
    = string_printf (_("<unknown type in %s, CU %s, DIE %s>"),
		     objfile_name (objfile),
		     sect_offset_str (cu->header.sect_off),
		     sect_offset_str (die->sect_off));
  saved = obstack_strdup (&objfile->objfile_obstack, message);

  return type_allocator (objfile, cu->lang ()).new_type (TYPE_CODE_ERROR,
							 0, saved);
}

/* Look up the type of DIE in CU using its type attribute ATTR.
   ATTR must be one of: DW_AT_type, DW_AT_GNAT_descriptive_type,
   DW_AT_containing_type.
   If there is no type substitute an error marker.  */

static struct type *
lookup_die_type (struct die_info *die, const struct attribute *attr,
		 struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct type *this_type;

  gdb_assert (attr->name == DW_AT_type
	      || attr->name == DW_AT_GNAT_descriptive_type
	      || attr->name == DW_AT_containing_type);

  /* First see if we have it cached.  */

  if (attr->form_is_alt ())
    {
      const auto &section = get_section_for_ref (*attr, cu);
      sect_offset sect_off = attr->get_ref_die_offset ();
      dwarf2_per_cu *per_cu
	= dwarf2_find_containing_unit ({ &section, sect_off }, per_objfile);

      this_type = get_die_type_at_offset (sect_off, per_cu, per_objfile);
    }
  else if (attr->form_is_ref ())
    {
      sect_offset sect_off = attr->get_ref_die_offset ();

      this_type = get_die_type_at_offset (sect_off, cu->per_cu, per_objfile);
    }
  else if (attr->form == DW_FORM_ref_sig8)
    {
      ULONGEST signature = attr->as_signature ();

      return get_signatured_type (die, signature, cu);
    }
  else
    {
      complaint (_(DWARF_ERROR_PREFIX
		   "Bad type attribute %s in DIE at %s [in module %s]"),
		 dwarf_attr_name (attr->name), sect_offset_str (die->sect_off),
		 objfile_name (objfile));
      return build_error_marker_type (cu, die);
    }

  /* If not cached we need to read it in.  */

  if (this_type == NULL)
    {
      struct die_info *type_die = NULL;
      struct dwarf2_cu *type_cu = cu;

      if (attr->form_is_ref ())
	type_die = follow_die_ref (die, attr, &type_cu);
      if (type_die == NULL)
	return build_error_marker_type (cu, die);
      /* If we find the type now, it's probably because the type came
	 from an inter-CU reference and the type's CU got expanded before
	 ours.  */
      this_type = read_type_die (type_die, type_cu);
    }

  /* If we still don't have a type use an error marker.  */

  if (this_type == NULL)
    return build_error_marker_type (cu, die);

  return this_type;
}

/* Return the type in DIE, CU.
   Returns NULL for invalid types.

   This first does a lookup in die_type_hash,
   and only reads the die in if necessary.

   NOTE: This can be called when reading in partial or full symbols.  */

static struct type *
read_type_die (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *this_type;

  this_type = get_die_type (die, cu);
  if (this_type)
    return this_type;

  return read_type_die_1 (die, cu);
}

/* Read the type in DIE, CU.
   Returns NULL for invalid types.  */

static struct type *
read_type_die_1 (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *this_type = NULL;

  switch (die->tag)
    {
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
      this_type = read_structure_type (die, cu);
      break;
    case DW_TAG_enumeration_type:
      this_type = read_enumeration_type (die, cu);
      break;
    case DW_TAG_entry_point:
    case DW_TAG_subprogram:
    case DW_TAG_subroutine_type:
    case DW_TAG_inlined_subroutine:
      this_type = read_subroutine_type (die, cu);
      break;
    case DW_TAG_array_type:
      this_type = read_array_type (die, cu);
      break;
    case DW_TAG_set_type:
      this_type = read_set_type (die, cu);
      break;
    case DW_TAG_pointer_type:
      this_type = read_tag_pointer_type (die, cu);
      break;
    case DW_TAG_ptr_to_member_type:
      this_type = read_tag_ptr_to_member_type (die, cu);
      break;
    case DW_TAG_reference_type:
      this_type = read_tag_reference_type (die, cu, TYPE_CODE_REF);
      break;
    case DW_TAG_rvalue_reference_type:
      this_type = read_tag_reference_type (die, cu, TYPE_CODE_RVALUE_REF);
      break;
    case DW_TAG_const_type:
      this_type = read_tag_const_type (die, cu);
      break;
    case DW_TAG_volatile_type:
      this_type = read_tag_volatile_type (die, cu);
      break;
    case DW_TAG_restrict_type:
      this_type = read_tag_restrict_type (die, cu);
      break;
    case DW_TAG_string_type:
      this_type = read_tag_string_type (die, cu);
      break;
    case DW_TAG_typedef:
      this_type = read_typedef (die, cu);
      break;
    case DW_TAG_generic_subrange:
    case DW_TAG_subrange_type:
      this_type = read_subrange_type (die, cu);
      break;
    case DW_TAG_base_type:
      this_type = read_base_type (die, cu);
      break;
    case DW_TAG_unspecified_type:
      this_type = read_unspecified_type (die, cu);
      break;
    case DW_TAG_namespace:
      this_type = read_namespace_type (die, cu);
      break;
    case DW_TAG_module:
      this_type = read_module_type (die, cu);
      break;
    case DW_TAG_atomic_type:
      this_type = read_tag_atomic_type (die, cu);
      break;
    default:
      complaint (_("unexpected tag in read_type_die: '%s'"),
		 dwarf_tag_name (die->tag));
      break;
    }

  return this_type;
}

/* See if we can figure out if the class lives in a namespace.  We do
   this by looking for a member function; its demangled name will
   contain namespace info, if there is any.
   Return the computed name or NULL.
   Space for the result is allocated on the objfile's obstack.
   This is the full-die version of guess_partial_die_structure_name.
   In this case we know DIE has no useful parent.  */

static const char *
guess_full_die_structure_name (struct die_info *die, struct dwarf2_cu *cu)
{
  struct die_info *spec_die;
  struct dwarf2_cu *spec_cu;
  struct objfile *objfile = cu->per_objfile->objfile;

  spec_cu = cu;
  spec_die = die_specification (die, &spec_cu);
  if (spec_die != NULL)
    {
      die = spec_die;
      cu = spec_cu;
    }

  for (die_info *child : die->children ())
    {
      if (child->tag == DW_TAG_subprogram)
	{
	  const char *linkage_name = dw2_linkage_name (child, cu);

	  if (linkage_name != NULL)
	    {
	      gdb::unique_xmalloc_ptr<char> actual_name
		(cu->language_defn->class_name_from_physname (linkage_name));
	      const char *name = NULL;

	      if (actual_name != NULL)
		{
		  const char *die_name = dwarf2_name (die, cu);

		  if (die_name != NULL
		      && strcmp (die_name, actual_name.get ()) != 0)
		    {
		      /* Strip off the class name from the full name.
			 We want the prefix.  */
		      int die_name_len = strlen (die_name);
		      int actual_name_len = strlen (actual_name.get ());
		      const char *ptr = actual_name.get ();

		      /* Test for '::' as a sanity check.  */
		      if (actual_name_len > die_name_len + 2
			  && ptr[actual_name_len - die_name_len - 1] == ':')
			name = obstack_strndup (
			  &objfile->per_bfd->storage_obstack,
			  ptr, actual_name_len - die_name_len - 2);
		    }
		}
	      return name;
	    }
	}
    }

  return NULL;
}

/* GCC might emit a nameless typedef that has a linkage name.  Determine the
   prefix part in such case.  See
   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=47510.  */

static const char *
anonymous_struct_prefix (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;
  const char *base;

  if (die->tag != DW_TAG_class_type && die->tag != DW_TAG_interface_type
      && die->tag != DW_TAG_structure_type && die->tag != DW_TAG_union_type)
    return NULL;

  if (dwarf2_string_attr (die, DW_AT_name, cu) != NULL)
    return NULL;

  attr = dw2_linkage_name_attr (die, cu);
  const char *attr_name = attr->as_string ();
  if (attr == NULL || attr_name == NULL)
    return NULL;

  /* dwarf2_name had to be already called.  */
  gdb_assert (attr->canonical_string_p ());

  /* Strip the base name, keep any leading namespaces/classes.  */
  base = strrchr (attr_name, ':');
  if (base == NULL || base == attr_name || base[-1] != ':')
    return "";

  struct objfile *objfile = cu->per_objfile->objfile;
  return obstack_strndup (&objfile->per_bfd->storage_obstack,
			  attr_name,
			  &base[-1] - attr_name);
}

/* Return the name of the namespace/class that DIE is defined within,
   or "" if we can't tell.  The caller should not xfree the result.

   For example, if we're within the method foo() in the following
   code:

   namespace N {
     class C {
       void foo () {
       }
     };
   }

   then determine_prefix on foo's die will return "N::C".  */

static const char *
determine_prefix (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct die_info *parent, *spec_die;
  struct dwarf2_cu *spec_cu;
  struct type *parent_type;
  const char *retval;

  if (cu->lang () != language_cplus
      && cu->lang () != language_fortran
      && cu->lang () != language_d
      && cu->lang () != language_rust
      && cu->lang () != language_ada)
    return "";

  retval = anonymous_struct_prefix (die, cu);
  if (retval)
    return retval;

  /* We have to be careful in the presence of DW_AT_specification.
     For example, with GCC 3.4, given the code

     namespace N {
       void foo() {
	 // Definition of N::foo.
       }
     }

     then we'll have a tree of DIEs like this:

     1: DW_TAG_compile_unit
       2: DW_TAG_namespace        // N
	 3: DW_TAG_subprogram     // declaration of N::foo
       4: DW_TAG_subprogram       // definition of N::foo
	    DW_AT_specification   // refers to die #3

     Thus, when processing die #4, we have to pretend that we're in
     the context of its DW_AT_specification, namely the context of die
     #3.  */
  spec_cu = cu;
  spec_die = die_specification (die, &spec_cu);
  if (spec_die == NULL)
    parent = die->parent;
  else
    {
      parent = spec_die->parent;
      cu = spec_cu;
    }

  if (parent == NULL)
    return "";
  else if (parent->building_fullname)
    {
      const char *name;
      const char *parent_name;

      /* It has been seen on RealView 2.2 built binaries,
	 DW_TAG_template_type_param types actually _defined_ as
	 children of the parent class:

	 enum E {};
	 template class <class Enum> Class{};
	 Class<enum E> class_e;

	 1: DW_TAG_class_type (Class)
	   2: DW_TAG_enumeration_type (E)
	     3: DW_TAG_enumerator (enum1:0)
	     3: DW_TAG_enumerator (enum2:1)
	     ...
	   2: DW_TAG_template_type_param
	      DW_AT_type  DW_FORM_ref_udata (E)

	 Besides being broken debug info, it can put GDB into an
	 infinite loop.  Consider:

	 When we're building the full name for Class<E>, we'll start
	 at Class, and go look over its template type parameters,
	 finding E.  We'll then try to build the full name of E, and
	 reach here.  We're now trying to build the full name of E,
	 and look over the parent DIE for containing scope.  In the
	 broken case, if we followed the parent DIE of E, we'd again
	 find Class, and once again go look at its template type
	 arguments, etc., etc.  Simply don't consider such parent die
	 as source-level parent of this die (it can't be, the language
	 doesn't allow it), and break the loop here.  */
      name = dwarf2_name (die, cu);
      parent_name = dwarf2_name (parent, cu);
      complaint (_("template param type '%s' defined within parent '%s'"),
		 name ? name : "<unknown>",
		 parent_name ? parent_name : "<unknown>");
      return "";
    }
  else
    switch (parent->tag)
      {
      case DW_TAG_namespace:
	parent_type = read_type_die (parent, cu);
	/* GCC 4.0 and 4.1 had a bug (PR c++/28460) where they generated bogus
	   DW_TAG_namespace DIEs with a name of "::" for the global namespace.
	   Work around this problem here.  */
	if (cu->lang () == language_cplus
	    && strcmp (parent_type->name (), "::") == 0)
	  return "";
	/* We give a name to even anonymous namespaces.  */
	return parent_type->name ();
      case DW_TAG_class_type:
      case DW_TAG_interface_type:
      case DW_TAG_structure_type:
      case DW_TAG_union_type:
      case DW_TAG_module:
	parent_type = read_type_die (parent, cu);
	if (parent_type->name () != NULL)
	  return parent_type->name ();
	else
	  /* An anonymous structure is only allowed non-static data
	     members; no typedefs, no member functions, et cetera.
	     So it does not need a prefix.  */
	  return "";
      case DW_TAG_compile_unit:
      case DW_TAG_partial_unit:
	/* gcc-4.5 -gdwarf-4 can drop the enclosing namespace.  Cope.  */
	if (cu->lang () == language_cplus
	    && !per_objfile->per_bfd->types.empty ()
	    && die->child != NULL
	    && (die->tag == DW_TAG_class_type
		|| die->tag == DW_TAG_structure_type
		|| die->tag == DW_TAG_union_type))
	  {
	    const char *name = guess_full_die_structure_name (die, cu);
	    if (name != NULL)
	      return name;
	  }
	return "";
      case DW_TAG_subprogram:
	/* Nested subroutines in Fortran get a prefix with the name
	   of the parent's subroutine.  Entry points are prefixed by the
	   parent's namespace.  */
	if (cu->lang () == language_fortran)
	  {
	    if ((die->tag ==  DW_TAG_subprogram)
		&& (dwarf2_name (parent, cu) != NULL))
	      return dwarf2_name (parent, cu);
	    else if (die->tag == DW_TAG_entry_point)
	      return determine_prefix (parent, cu);
	  }
	else if (cu->lang () == language_ada
		 && (die->tag == DW_TAG_subprogram
		     || die->tag == DW_TAG_inlined_subroutine
		     || die->tag == DW_TAG_lexical_block))
	  return dwarf2_full_name (nullptr, parent, cu);
	return "";
      case DW_TAG_enumeration_type:
	parent_type = read_type_die (parent, cu);
	if (parent_type->is_declared_class ())
	  {
	    if (parent_type->name () != NULL)
	      return parent_type->name ();
	    return "";
	  }
	[[fallthrough]];
      default:
	return determine_prefix (parent, cu);
      }
}

/* Return a newly-allocated string formed by concatenating PREFIX and
   SUFFIX with appropriate separator.  If PREFIX or SUFFIX is NULL or
   empty, then simply copy the SUFFIX or PREFIX, respectively.  The CU
   argument is used to determine the language and hence, the
   appropriate separator.  */

static gdb::unique_xmalloc_ptr<char>
typename_concat (const char *prefix, const char *suffix, int physname,
		 struct dwarf2_cu *cu)
{
  const char *lead = "";
  const char *sep;

  if (suffix == NULL || suffix[0] == '\0'
      || prefix == NULL || prefix[0] == '\0')
    sep = "";
  else if (cu->lang () == language_d)
    {
      /* For D, the 'main' function could be defined in any module, but it
	 should never be prefixed.  */
      if (strcmp (suffix, "D main") == 0)
	{
	  prefix = "";
	  sep = "";
	}
      else
	sep = ".";
    }
  else if (cu->lang () == language_fortran && physname)
    {
      /* This is gfortran specific mangling.  Normally DW_AT_linkage_name or
	 DW_AT_MIPS_linkage_name is preferred and used instead.  */

      lead = "__";
      sep = "_MOD_";
    }
  else if (cu->lang () == language_ada)
    sep = "__";
  else
    sep = "::";

  if (prefix == NULL)
    prefix = "";
  if (suffix == NULL)
    suffix = "";

  return gdb::unique_xmalloc_ptr<char> (concat (lead, prefix, sep, suffix,
						nullptr));
}

/* Return a generic name for a DW_TAG_template_type_param or
   DW_TAG_template_value_param tag, missing a DW_AT_name attribute.  We do this
   per parent, so each function/class/struct template will have their own set
   of template parameters named <unnnamed0>, <unnamed1>, ... where the
   enumeration starts at 0 and represents the position of the template tag in
   the list of unnamed template tags for this parent, counting both, type and
   value tags.  */

static const char *
unnamed_template_tag_name (die_info *die, dwarf2_cu *cu)
{
  if (die->parent == nullptr)
    return nullptr;

  /* Count the parent types unnamed template type and value children until, we
     arrive at our entry.  */
  size_t nth_unnamed = 0;

  for (die_info *child : die->parent->children ())
    {
      if (child == die)
	break;

      gdb_assert (child != nullptr);
      if (child->tag == DW_TAG_template_type_param
	  || child->tag == DW_TAG_template_value_param)
	{
	  if (dwarf2_attr (child, DW_AT_name, cu) == nullptr)
	    ++nth_unnamed;
	}
    }

  const std::string name_str = "<unnamed" + std::to_string (nth_unnamed) + ">";
  return cu->per_objfile->objfile->intern (name_str.c_str ());
}

/* Get name of a die, return NULL if not found.  */

static const char *
dwarf2_canonicalize_name (const char *name, struct dwarf2_cu *cu,
			  struct objfile *objfile)
{
  if (name == nullptr)
    return name;

  if (cu->lang () == language_cplus)
    {
      gdb::unique_xmalloc_ptr<char> canon_name
	= cp_canonicalize_string (name);

      if (canon_name != nullptr)
	name = objfile->intern (canon_name.get ());
    }
  else if (cu->lang () == language_c)
    {
      gdb::unique_xmalloc_ptr<char> canon_name
	= c_canonicalize_name (name);

      if (canon_name != nullptr)
	name = objfile->intern (canon_name.get ());
    }

  return name;
}

/* Get name of a die, return NULL if not found.
   Anonymous namespaces are converted to their magic string.  */

static const char *
dwarf2_name (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;
  struct objfile *objfile = cu->per_objfile->objfile;

  attr = dwarf2_attr (die, DW_AT_name, cu);
  const char *attr_name = attr == nullptr ? nullptr : attr->as_string ();
  if (attr_name == nullptr
      && die->tag != DW_TAG_namespace
      && die->tag != DW_TAG_class_type
      && die->tag != DW_TAG_interface_type
      && die->tag != DW_TAG_structure_type
      && die->tag != DW_TAG_namelist
      && die->tag != DW_TAG_union_type
      && die->tag != DW_TAG_template_type_param
      && die->tag != DW_TAG_template_value_param
      && die->tag != DW_TAG_module)
    return NULL;

  switch (die->tag)
    {
      /* A member's name should not be canonicalized.  This is a bit
	 of a hack, in that normally it should not be possible to run
	 into this situation; however, the dw2-unusual-field-names.exp
	 test creates custom DWARF that does.  */
    case DW_TAG_member:
    case DW_TAG_compile_unit:
    case DW_TAG_partial_unit:
      /* Compilation units have a DW_AT_name that is a filename, not
	 a source language identifier.  */
    case DW_TAG_enumeration_type:
    case DW_TAG_enumerator:
      /* These tags always have simple identifiers already; no need
	 to canonicalize them.  */
      return attr_name;

    case DW_TAG_namespace:
      if (attr_name != nullptr)
	return attr_name;
      return CP_ANONYMOUS_NAMESPACE_STR;

    /* DWARF does not actually require template tags to have a name.  */
    case DW_TAG_template_type_param:
    case DW_TAG_template_value_param:
      if (attr_name == nullptr)
	return unnamed_template_tag_name (die, cu);
      [[fallthrough]];
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_namelist:
      /* Some GCC versions emit spurious DW_AT_name attributes for unnamed
	 structures or unions.  These were of the form "._%d" in GCC 4.1,
	 or simply "<anonymous struct>" or "<anonymous union>" in GCC 4.3
	 and GCC 4.4.  We work around this problem by ignoring these.  */
      if (attr_name != nullptr
	  && (startswith (attr_name, "._")
	      || startswith (attr_name, "<anonymous")))
	return NULL;

      /* GCC might emit a nameless typedef that has a linkage name.  See
	 http://gcc.gnu.org/bugzilla/show_bug.cgi?id=47510.  */
      if (!attr || attr_name == NULL)
	{
	  attr = dw2_linkage_name_attr (die, cu);
	  attr_name = attr == nullptr ? nullptr : attr->as_string ();
	  if (attr == NULL || attr_name == NULL)
	    return NULL;

	  /* Avoid demangling attr_name the second time on a second
	     call for the same DIE.  */
	  if (!attr->canonical_string_p ())
	    {
	      gdb::unique_xmalloc_ptr<char> demangled
		(gdb_demangle (attr_name, DMGL_TYPES));
	      if (demangled == nullptr)
		return nullptr;

	      attr->set_string_canonical (objfile->intern (demangled.get ()));
	      attr_name = attr->as_string ();
	    }

	  /* Strip any leading namespaces/classes, keep only the
	     base name.  DW_AT_name for named DIEs does not
	     contain the prefixes.  */
	  const char *base = strrchr (attr_name, ':');
	  if (base && base > attr_name && base[-1] == ':')
	    return &base[1];
	  else
	    return attr_name;
	}
      break;

    default:
      break;
    }

  if (!attr->canonical_string_p ())
    attr->set_string_canonical (dwarf2_canonicalize_name (attr_name, cu,
							  objfile));
  return attr->as_string ();
}

/* Return the die that this die in an extension of, or NULL if there
   is none.  *EXT_CU is the CU containing DIE on input, and the CU
   containing the return value on output.  */

static struct die_info *
dwarf2_extension (struct die_info *die, struct dwarf2_cu **ext_cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_extension, *ext_cu);
  if (attr == NULL)
    return NULL;

  return follow_die_ref (die, attr, ext_cu);
}

/* Follow reference or signature attribute ATTR of SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.  */

static struct die_info *
follow_die_ref_or_sig (struct die_info *src_die, const struct attribute *attr,
		       struct dwarf2_cu **ref_cu)
{
  struct die_info *die;

  if (attr->form_is_ref ())
    die = follow_die_ref (src_die, attr, ref_cu);
  else if (attr->form == DW_FORM_ref_sig8)
    die = follow_die_sig (src_die, attr, ref_cu);
  else
    {
      src_die->error_dump ();
      error (_(DWARF_ERROR_PREFIX
	       "Expected reference attribute [in module %s]"),
	     objfile_name ((*ref_cu)->per_objfile->objfile));
    }

  return die;
}

/* Follow reference TARGET.
   On entry *REF_CU is the CU of the source die referencing TARGET.
   On exit *REF_CU is the CU of the result.
   Returns nullptr if TARGET is invalid.  */

static die_info *
follow_die_offset (const section_and_offset &target, dwarf2_cu **ref_cu)
{
  dwarf2_cu *source_cu = *ref_cu;
  dwarf2_cu *target_cu = source_cu;
  dwarf2_per_objfile *per_objfile = source_cu->per_objfile;

  gdb_assert (source_cu->per_cu != nullptr);
  gdb_assert (source_cu->dies != nullptr);

  dwarf_read_debug_printf_v ("source CU offset: %s, target offset: %s, "
			     "source CU contains target offset: %d",
			     sect_offset_str (source_cu->per_cu->sect_off),
			     sect_offset_str (target.offset),
			     (target.section == &source_cu->section ()
			      && source_cu->header.offset_in_unit_p (target.offset)));

  if (source_cu->per_cu->is_debug_types)
    {
      /* .debug_types CUs cannot reference anything outside their CU.
	 If they need to, they have to reference a signatured type via
	 DW_FORM_ref_sig8.  */
      if (!source_cu->header.offset_in_unit_p (target.offset))
	return NULL;
    }
  else if (target.section != &source_cu->section ()
	   || !source_cu->header.offset_in_unit_p (target.offset))
    {
      dwarf2_per_cu *target_per_cu
	= dwarf2_find_containing_unit (target, per_objfile);

      dwarf_read_debug_printf_v ("target CU offset: %s, "
				 "target CU DIEs loaded: %d",
				 sect_offset_str (target_per_cu->sect_off),
				 per_objfile->get_cu (target_per_cu) != nullptr);

      /* If necessary, add it to the queue and load its DIEs.

	 Even if maybe_queue_comp_unit doesn't require us to load the CU's DIEs,
	 it doesn't mean they are currently loaded.  Since we require them
	 to be loaded, we must check for ourselves.  */
      if (maybe_queue_comp_unit (source_cu, target_per_cu, per_objfile)
	  || per_objfile->get_cu (target_per_cu) == nullptr)
	load_full_comp_unit (target_per_cu, per_objfile, false,
			     source_cu->lang ());

      target_cu = per_objfile->get_cu (target_per_cu);
      if (target_cu == nullptr)
	error (_(DWARF_ERROR_PREFIX
		 "cannot follow reference to DIE at %s"
		 " [in module %s]"),
	       sect_offset_str (target.offset),
	       objfile_name (per_objfile->objfile));
    }

  *ref_cu = target_cu;

  return target_cu->find_die (target.offset);
}

/* Follow reference attribute ATTR of SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.  */

static struct die_info *
follow_die_ref (struct die_info *src_die, const struct attribute *attr,
		struct dwarf2_cu **ref_cu)
{
  sect_offset sect_off = attr->get_ref_die_offset ();
  struct dwarf2_cu *src_cu = *ref_cu;

  if (!attr->form_is_alt () && src_die->sect_off == sect_off)
    {
      /* Self-reference, we're done.  */
      return src_die;
    }

  const dwarf2_section_info &section = get_section_for_ref (*attr, src_cu);
  die_info *die = follow_die_offset ({ &section, sect_off }, ref_cu);
  if (die == nullptr)
    error (_(DWARF_ERROR_PREFIX
	     "Cannot find DIE at %s referenced from DIE at %s [in module %s]"),
	   sect_offset_str (sect_off), sect_offset_str (src_die->sect_off),
	   objfile_name (src_cu->per_objfile->objfile));

  return die;
}

/* See read.h.  */

struct dwarf2_locexpr_baton
dwarf2_fetch_die_loc_sect_off (sect_offset sect_off, dwarf2_per_cu *per_cu,
			       dwarf2_per_objfile *per_objfile,
			       gdb::function_view<CORE_ADDR ()> get_frame_pc,
			       bool resolve_abstract_p)
{
  struct attribute *attr;
  struct dwarf2_locexpr_baton retval;
  struct objfile *objfile = per_objfile->objfile;

  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu == nullptr)
    cu = load_cu (per_cu, per_objfile, false);

  if (cu == nullptr)
    {
      /* We shouldn't get here for a dummy CU, but don't crash on the user.
	 Instead just throw an error, not much else we can do.  */
      error (_(DWARF_ERROR_PREFIX
	       "Dummy CU at %s referenced [in module %s]"),
	     sect_offset_str (sect_off), objfile_name (objfile));
    }

  die_info *die = follow_die_offset ({ &cu->section (), sect_off }, &cu);
  if (die == nullptr)
    error (_(DWARF_ERROR_PREFIX
	     "Cannot find DIE at %s referenced [in module %s]"),
	   sect_offset_str (sect_off), objfile_name (objfile));

  attr = dwarf2_attr (die, DW_AT_location, cu);
  if (!attr && resolve_abstract_p
      && (per_objfile->per_bfd->abstract_to_concrete.find (die->sect_off)
	  != per_objfile->per_bfd->abstract_to_concrete.end ()))
    {
      CORE_ADDR pc = get_frame_pc ();

      for (const auto &cand_off
	     : per_objfile->per_bfd->abstract_to_concrete[die->sect_off])
	{
	  struct dwarf2_cu *cand_cu = cu;
	  die_info *cand
	    = follow_die_offset ({ &cu->section (), cand_off }, &cand_cu);
	  if (!cand
	      || !cand->parent
	      || cand->parent->tag != DW_TAG_subprogram)
	    continue;

	  unrelocated_addr unrel_low, unrel_high;
	  get_scope_pc_bounds (cand->parent, &unrel_low, &unrel_high, cu);
	  if (unrel_low == ((unrelocated_addr) -1))
	    continue;
	  CORE_ADDR pc_low = per_objfile->relocate (unrel_low);
	  CORE_ADDR pc_high = per_objfile->relocate (unrel_high);
	  if (!(pc_low <= pc && pc < pc_high))
	    continue;

	  die = cand;
	  attr = dwarf2_attr (die, DW_AT_location, cu);
	  break;
	}
    }

  if (!attr)
    {
      /* DWARF: "If there is no such attribute, then there is no effect.".
	 DATA is ignored if SIZE is 0.  */

      retval.data = NULL;
      retval.size = 0;
    }
  else if (attr->form_is_section_offset ())
    {
      struct dwarf2_loclist_baton loclist_baton;
      CORE_ADDR pc = get_frame_pc ();
      size_t size;

      fill_in_loclist_baton (cu, &loclist_baton, attr);

      retval.data = dwarf2_find_location_expression (&loclist_baton,
						     &size, pc);
      retval.size = size;
    }
  else
    {
      if (!attr->form_is_block ())
	error (_(DWARF_ERROR_PREFIX
		 "DIE at %s is neither DW_FORM_block* nor DW_FORM_exprloc"
		 " [in module %s]"),
	       sect_offset_str (sect_off), objfile_name (objfile));

      struct dwarf_block *block = attr->as_block ();
      retval.data = block->data;
      retval.size = block->size;
    }
  retval.per_objfile = per_objfile;
  retval.per_cu = cu->per_cu;

  per_objfile->age_comp_units ();

  return retval;
}

/* See read.h.  */

struct dwarf2_locexpr_baton
dwarf2_fetch_die_loc_cu_off (cu_offset offset_in_cu, dwarf2_per_cu *per_cu,
			     dwarf2_per_objfile *per_objfile,
			     gdb::function_view<CORE_ADDR ()> get_frame_pc)
{
  sect_offset sect_off = per_cu->sect_off + to_underlying (offset_in_cu);

  return dwarf2_fetch_die_loc_sect_off (sect_off, per_cu, per_objfile,
					get_frame_pc);
}

/* Write a constant of a given type as target-ordered bytes into
   OBSTACK.  */

static const gdb_byte *
write_constant_as_bytes (struct obstack *obstack,
			 enum bfd_endian byte_order,
			 struct type *type,
			 ULONGEST value,
			 LONGEST *len)
{
  gdb_byte *result;

  *len = type->length ();
  result = (gdb_byte *) obstack_alloc (obstack, *len);
  store_unsigned_integer (result, *len, byte_order, value);

  return result;
}

/* See read.h.  */

const gdb_byte *
dwarf2_fetch_constant_bytes (sect_offset sect_off,
			     dwarf2_per_cu *per_cu,
			     dwarf2_per_objfile *per_objfile,
			     obstack *obstack,
			     LONGEST *len)
{
  struct attribute *attr;
  const gdb_byte *result = NULL;
  struct type *type;
  LONGEST value;
  enum bfd_endian byte_order;
  struct objfile *objfile = per_objfile->objfile;

  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu == nullptr)
    cu = load_cu (per_cu, per_objfile, false);

  if (cu == nullptr)
    {
      /* We shouldn't get here for a dummy CU, but don't crash on the user.
	 Instead just throw an error, not much else we can do.  */
      error (_(DWARF_ERROR_PREFIX
	       "Dummy CU at %s referenced [in module %s]"),
	     sect_offset_str (sect_off), objfile_name (objfile));
    }

  die_info *die = follow_die_offset ({ &cu->section (), sect_off }, &cu);
  if (!die)
    error (_(DWARF_ERROR_PREFIX
	     "Cannot find DIE at %s referenced [in module %s]"),
	   sect_offset_str (sect_off), objfile_name (objfile));

  attr = dwarf2_attr (die, DW_AT_const_value, cu);
  if (attr == NULL)
    return NULL;

  byte_order = (bfd_big_endian (objfile->obfd.get ())
		? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE);

  switch (attr->form)
    {
    case DW_FORM_addr:
    case DW_FORM_addrx:
    case DW_FORM_GNU_addr_index:
      {
	gdb_byte *tem;

	*len = cu->header.addr_size;
	tem = (gdb_byte *) obstack_alloc (obstack, *len);
	store_unsigned_integer (tem, *len, byte_order,
				(ULONGEST) attr->as_address ());
	result = tem;
      }
      break;
    case DW_FORM_string:
    case DW_FORM_strp:
    case DW_FORM_strx:
    case DW_FORM_GNU_str_index:
    case DW_FORM_GNU_strp_alt:
    case DW_FORM_strp_sup:
      /* The string is already allocated on the objfile obstack, point
	 directly to it.  */
      {
	const char *attr_name = attr->as_string ();
	result = (const gdb_byte *) attr_name;
	*len = strlen (attr_name);
      }
      break;
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
    case DW_FORM_exprloc:
    case DW_FORM_data16:
      {
	struct dwarf_block *block = attr->as_block ();
	result = block->data;
	*len = block->size;
      }
      break;

      /* The DW_AT_const_value attributes are supposed to carry the
	 symbol's value "represented as it would be on the target
	 architecture."  By the time we get here, it's already been
	 converted to host endianness, so we just need to sign- or
	 zero-extend it as appropriate.

	 Both GCC and LLVM agree that these are always signed, though.  */
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_sdata:
    case DW_FORM_implicit_const:
    case DW_FORM_udata:
      type = die_type (die, cu);
      value = attr->confused_constant ().value_or (0);
      result = write_constant_as_bytes (obstack, byte_order, type, value, len);
      break;

    default:
      complaint (_("unsupported const value attribute form: '%s'"),
		 dwarf_form_name (attr->form));
      break;
    }

  return result;
}

/* See read.h.  */

struct type *
dwarf2_fetch_die_type_sect_off (sect_offset sect_off, dwarf2_per_cu *per_cu,
				dwarf2_per_objfile *per_objfile,
				const char **var_name)
{
  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu == nullptr)
    cu = load_cu (per_cu, per_objfile, false);

  if (cu == nullptr)
    return nullptr;

  die_info *die = follow_die_offset ({ &cu->section (), sect_off }, &cu);
  if (!die)
    return NULL;

  if (var_name != nullptr)
    *var_name = var_decl_name (die, cu);
  return die_type (die, cu);
}

/* See read.h.  */

struct type *
dwarf2_get_die_type (cu_offset die_offset, dwarf2_per_cu *per_cu,
		     dwarf2_per_objfile *per_objfile)
{
  sect_offset die_offset_sect = per_cu->sect_off + to_underlying (die_offset);
  return get_die_type_at_offset (die_offset_sect, per_cu, per_objfile);
}

/* Follow type unit SIG_TYPE referenced by SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.
   Returns NULL if the referenced DIE isn't found.  */

static struct die_info *
follow_die_sig_1 (struct die_info *src_die, struct signatured_type *sig_type,
		  struct dwarf2_cu **ref_cu)
{
  struct dwarf2_cu *sig_cu;
  dwarf2_per_objfile *per_objfile = (*ref_cu)->per_objfile;


  /* While it might be nice to assert sig_type->type == NULL here,
     we can get here for DW_AT_imported_declaration where we need
     the DIE not the type.  */

  /* If necessary, add it to the queue and load its DIEs.

     Even if maybe_queue_comp_unit doesn't require us to load the CU's DIEs,
     it doesn't mean they are currently loaded.  Since we require them
     to be loaded, we must check for ourselves.  */
  if (maybe_queue_comp_unit (*ref_cu, sig_type, per_objfile)
      || per_objfile->get_cu (sig_type) == nullptr)
    read_signatured_type (sig_type, per_objfile);

  sig_cu = per_objfile->get_cu (sig_type);
  gdb_assert (sig_cu != NULL);
  gdb_assert (to_underlying (sig_type->type_offset_in_section) != 0);

  if (die_info *die = sig_cu->find_die (sig_type->type_offset_in_section);
      die != nullptr)
    {
      /* For .gdb_index version 7 keep track of included TUs.
	 http://sourceware.org/bugzilla/show_bug.cgi?id=15021.  */
      if (per_objfile->per_bfd->index_table != NULL
	  && !per_objfile->per_bfd->index_table->version_check ())
	(*ref_cu)->per_cu->imported_symtabs.push_back (sig_cu->per_cu);

      *ref_cu = sig_cu;
      return die;
    }

  return NULL;
}

/* Follow signatured type referenced by ATTR in SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.
   The result is the DIE of the type.
   If the referenced type cannot be found an error is thrown.  */

static struct die_info *
follow_die_sig (struct die_info *src_die, const struct attribute *attr,
		struct dwarf2_cu **ref_cu)
{
  ULONGEST signature = attr->as_signature ();
  struct signatured_type *sig_type;
  struct die_info *die;

  gdb_assert (attr->form == DW_FORM_ref_sig8);

  sig_type = lookup_signatured_type (*ref_cu, signature);
  /* sig_type will be NULL if the signatured type is missing from
     the debug info.  */
  if (sig_type == NULL)
    {
      error (_(DWARF_ERROR_PREFIX
	       "Cannot find signatured DIE %s referenced from DIE at %s"
	       " [in module %s]"),
	     hex_string (signature), sect_offset_str (src_die->sect_off),
	     objfile_name ((*ref_cu)->per_objfile->objfile));
    }

  die = follow_die_sig_1 (src_die, sig_type, ref_cu);
  if (die == NULL)
    {
      src_die->error_dump ();
      error (_(DWARF_ERROR_PREFIX
	       "Problem reading signatured DIE %s referenced from DIE at %s"
	       " [in module %s]"),
	     hex_string (signature), sect_offset_str (src_die->sect_off),
	     objfile_name ((*ref_cu)->per_objfile->objfile));
    }

  return die;
}

/* Get the type specified by SIGNATURE referenced in DIE/CU,
   reading in and processing the type unit if necessary.  */

static struct type *
get_signatured_type (struct die_info *die, ULONGEST signature,
		     struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct signatured_type *sig_type;
  struct dwarf2_cu *type_cu;
  struct die_info *type_die;
  struct type *type;

  sig_type = lookup_signatured_type (cu, signature);
  /* sig_type will be NULL if the signatured type is missing from
     the debug info.  */
  if (sig_type == NULL)
    {
      complaint (_(DWARF_ERROR_PREFIX
		   "Cannot find signatured DIE %s referenced from DIE at %s"
		   " [in module %s]"),
		 hex_string (signature), sect_offset_str (die->sect_off),
		 objfile_name (per_objfile->objfile));
      return build_error_marker_type (cu, die);
    }

  /* If we already know the type we're done.  */
  type = per_objfile->get_type_for_signatured_type (sig_type);
  if (type != nullptr)
    return type;

  type_cu = cu;
  type_die = follow_die_sig_1 (die, sig_type, &type_cu);
  if (type_die != NULL)
    {
      /* N.B. We need to call get_die_type to ensure only one type for this DIE
	 is created.  This is important, for example, because for c++ classes
	 we need TYPE_NAME set which is only done by new_symbol.  Blech.  */
      type = read_type_die (type_die, type_cu);
      if (type == NULL)
	{
	  complaint (_(DWARF_ERROR_PREFIX
		       "Cannot build signatured type %s"
		       " referenced from DIE at %s [in module %s]"),
		     hex_string (signature), sect_offset_str (die->sect_off),
		     objfile_name (per_objfile->objfile));
	  type = build_error_marker_type (cu, die);
	}
    }
  else
    {
      complaint (_(DWARF_ERROR_PREFIX
		   "Problem reading signatured DIE %s referenced"
		   " from DIE at %s [in module %s]"),
		 hex_string (signature), sect_offset_str (die->sect_off),
		 objfile_name (per_objfile->objfile));
      type = build_error_marker_type (cu, die);
    }

  per_objfile->set_type_for_signatured_type (sig_type, type);

  return type;
}

/* Get the type specified by the DW_AT_signature ATTR in DIE/CU,
   reading in and processing the type unit if necessary.  */

static struct type *
get_DW_AT_signature_type (struct die_info *die, const struct attribute *attr,
			  struct dwarf2_cu *cu) /* ARI: editCase function */
{
  /* Yes, DW_AT_signature can use a non-ref_sig8 reference.  */
  if (attr->form_is_ref ())
    {
      struct dwarf2_cu *type_cu = cu;
      struct die_info *type_die = follow_die_ref (die, attr, &type_cu);

      return read_type_die (type_die, type_cu);
    }
  else if (attr->form == DW_FORM_ref_sig8)
    {
      return get_signatured_type (die, attr->as_signature (), cu);
    }
  else
    {
      dwarf2_per_objfile *per_objfile = cu->per_objfile;

      complaint (_(DWARF_ERROR_PREFIX
		   "DW_AT_signature has bad form %s in DIE at %s"
		   " [in module %s]"),
		 dwarf_form_name (attr->form), sect_offset_str (die->sect_off),
		 objfile_name (per_objfile->objfile));
      return build_error_marker_type (cu, die);
    }
}

/* Load the DIEs associated with type unit PER_CU into memory.  */

static void
load_full_type_unit (dwarf2_per_cu *per_cu, dwarf2_per_objfile *per_objfile)
{
  struct signatured_type *sig_type;

  /* We have the per_cu, but we need the signatured_type.
     Fortunately this is an easy translation.  */
  gdb_assert (per_cu->is_debug_types);
  sig_type = (struct signatured_type *) per_cu;

  gdb_assert (per_objfile->get_cu (per_cu) == nullptr);

  read_signatured_type (sig_type, per_objfile);

  gdb_assert (per_objfile->get_cu (per_cu) != nullptr);
}

/* Read in a signatured type and build its CU and DIEs.
   If the type is a stub for the real type in a DWO file,
   read in the real type from the DWO file as well.  */

static void
read_signatured_type (signatured_type *sig_type,
		      dwarf2_per_objfile *per_objfile)
{
  gdb_assert (sig_type->is_debug_types);
  gdb_assert (per_objfile->get_cu (sig_type) == nullptr);

  cutu_reader reader (*sig_type, *per_objfile, nullptr, nullptr, false,
		      language_minimal);

  if (!reader.is_dummy ())
    {
      reader.read_all_dies ();

      /* Save this dwarf2_cu in the per_objfile.  The per_objfile owns it
	 now.  */
      per_objfile->set_cu (sig_type, reader.release_cu ());
    }

  sig_type->tu_read = 1;
}

/* See read.h.  */

bool
decode_locdesc (struct dwarf_block *blk, struct dwarf2_cu *cu,
		CORE_ADDR *result)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  size_t i;
  size_t size = blk->size;
  const gdb_byte *data = blk->data;
  CORE_ADDR stack[64];
  int stacki;
  unsigned int bytes_read;
  gdb_byte op;

  *result = 0;
  i = 0;
  stacki = 0;
  stack[stacki] = 0;
  stack[++stacki] = 0;

  while (i < size)
    {
      op = data[i++];
      switch (op)
	{
	case DW_OP_lit0:
	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	  stack[++stacki] = op - DW_OP_lit0;
	  break;

	case DW_OP_addr:
	  stack[++stacki]
	    = (CORE_ADDR) cu->header.read_address (objfile->obfd.get (),
						   &data[i],
						   &bytes_read);
	  i += bytes_read;
	  break;

	case DW_OP_const1u:
	  stack[++stacki] = read_1_byte (objfile->obfd.get (), &data[i]);
	  i += 1;
	  break;

	case DW_OP_const1s:
	  stack[++stacki] = read_1_signed_byte (objfile->obfd.get (), &data[i]);
	  i += 1;
	  break;

	case DW_OP_const2u:
	  stack[++stacki] = read_2_bytes (objfile->obfd.get (), &data[i]);
	  i += 2;
	  break;

	case DW_OP_const2s:
	  stack[++stacki] = read_2_signed_bytes (objfile->obfd.get (), &data[i]);
	  i += 2;
	  break;

	case DW_OP_const4u:
	  stack[++stacki] = read_4_bytes (objfile->obfd.get (), &data[i]);
	  i += 4;
	  break;

	case DW_OP_const4s:
	  stack[++stacki] = read_4_signed_bytes (objfile->obfd.get (), &data[i]);
	  i += 4;
	  break;

	case DW_OP_const8u:
	  stack[++stacki] = read_8_bytes (objfile->obfd.get (), &data[i]);
	  i += 8;
	  break;

	case DW_OP_constu:
	  stack[++stacki] = read_unsigned_leb128 (NULL, (data + i),
						  &bytes_read);
	  i += bytes_read;
	  break;

	case DW_OP_consts:
	  stack[++stacki] = read_signed_leb128 (NULL, (data + i), &bytes_read);
	  i += bytes_read;
	  break;

	case DW_OP_dup:
	  stack[stacki + 1] = stack[stacki];
	  stacki++;
	  break;

	case DW_OP_plus:
	  stack[stacki - 1] += stack[stacki];
	  stacki--;
	  break;

	case DW_OP_plus_uconst:
	  stack[stacki] += read_unsigned_leb128 (NULL, (data + i),
						 &bytes_read);
	  i += bytes_read;
	  break;

	case DW_OP_minus:
	  stack[stacki - 1] -= stack[stacki];
	  stacki--;
	  break;

	case DW_OP_deref:
	  /* If we're not the last op, then we definitely can't encode
	     this using GDB's location_class enum.  This is valid for partial
	     global symbols, although the variable's address will be bogus
	     in the psymtab.  */
	  if (i < size)
	    return false;
	  break;

	case DW_OP_addrx:
	case DW_OP_GNU_addr_index:
	case DW_OP_constx:
	case DW_OP_GNU_const_index:
	  stack[++stacki]
	    = (CORE_ADDR) read_addr_index_from_leb128 (cu, &data[i],
						       &bytes_read);
	  i += bytes_read;
	  break;

	default:
	  return false;
	}

      /* Enforce maximum stack depth of SIZE-1 to avoid writing
	 outside of the allocated space.  Also enforce minimum>0.  */
      if (stacki >= ARRAY_SIZE (stack) - 1)
	{
	  complaint (_("location description stack overflow"));
	  return false;
	}

      if (stacki <= 0)
	{
	  complaint (_("location description stack underflow"));
	  return false;
	}
    }

  *result = stack[stacki];
  return true;
}

/* memory allocation interface */

static struct dwarf_block *
dwarf_alloc_block (struct dwarf2_cu *cu)
{
  return XOBNEW (&cu->comp_unit_obstack, struct dwarf_block);
}



/* Macro support.  */

/* An overload of dwarf_decode_macros that finds the correct section
   and ensures it is read in before calling the other overload.  */

static void
dwarf_decode_macros (struct dwarf2_cu *cu, unsigned int offset,
		     int section_is_gnu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  const struct line_header *lh = cu->line_header;
  unsigned int offset_size = cu->header.offset_size;
  struct dwarf2_section_info *section;
  const char *section_name;

  if (cu->dwo_unit != nullptr)
    {
      if (section_is_gnu)
	{
	  section = &cu->dwo_unit->dwo_file->sections.macro;
	  section_name = ".debug_macro.dwo";
	}
      else
	{
	  section = &cu->dwo_unit->dwo_file->sections.macinfo;
	  section_name = ".debug_macinfo.dwo";
	}
    }
  else
    {
      if (section_is_gnu)
	{
	  section = &per_objfile->per_bfd->macro;
	  section_name = ".debug_macro";
	}
      else
	{
	  section = &per_objfile->per_bfd->macinfo;
	  section_name = ".debug_macinfo";
	}
    }

  section->read (objfile);
  if (section->buffer == nullptr)
    {
      complaint (_("missing %s section"), section_name);
      return;
    }

  buildsym_compunit *builder = cu->get_builder ();

  struct dwarf2_section_info *str_offsets_section;
  struct dwarf2_section_info *str_section;
  std::optional<ULONGEST> str_offsets_base;

  if (cu->dwo_unit != nullptr)
    {
      str_offsets_section = &cu->dwo_unit->dwo_file
			       ->sections.str_offsets;
      str_section = &cu->dwo_unit->dwo_file->sections.str;
      if (cu->header.version <= 4)
	str_offsets_base = 0;
      else
	{
	  bfd *abfd = str_offsets_section->get_bfd_owner ();
	  unsigned int bytes_read = 0;
	  read_initial_length (abfd, str_offsets_section->buffer, &bytes_read,
			       false);
	  const bool is_dwarf64 = bytes_read != 4;
	  str_offsets_base = is_dwarf64 ? 16 : 8;
	}
    }
  else
    {
      str_offsets_section = &per_objfile->per_bfd->str_offsets;
      str_section = &per_objfile->per_bfd->str;
      str_offsets_base = cu->str_offsets_base;
    }

  try
    {
      dwarf_decode_macros (per_objfile, builder, section, lh, offset_size,
			   offset, str_section, str_offsets_section,
			   str_offsets_base, section_is_gnu, cu);
    }
  catch (const gdb_exception_error &error)
    {
      /* Print the error and carry on with no (or partial) macro
	 information.  */
      exception_fprintf (gdb_stderr, error, _("While reading section %s: "),
			 section->get_name ());
    }
}

/* Return the .debug_loc section to use for CU.
   For DWO files use .debug_loc.dwo.  */

static struct dwarf2_section_info *
cu_debug_loc_section (struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  if (cu->dwo_unit)
    {
      struct dwo_sections *sections = &cu->dwo_unit->dwo_file->sections;

      return cu->header.version >= 5 ? &sections->loclists : &sections->loc;
    }
  return (cu->header.version >= 5 ? &per_objfile->per_bfd->loclists
				  : &per_objfile->per_bfd->loc);
}

/* Return the .debug_rnglists section to use for CU.  */
static struct dwarf2_section_info *
cu_debug_rnglists_section (struct dwarf2_cu *cu, dwarf_tag tag)
{
  if (cu->header.version < 5)
    error (_(".debug_rnglists section cannot be used in DWARF %d"),
	   cu->header.version);
  struct dwarf2_per_objfile *dwarf2_per_objfile = cu->per_objfile;

  /* Make sure we read the .debug_rnglists section from the file that
     contains the DW_AT_ranges attribute we are reading.  Normally that
     would be the .dwo file, if there is one.  However for DW_TAG_compile_unit
     or DW_TAG_skeleton unit, we always want to read from objfile/linked
     program.  */
  if (cu->dwo_unit != nullptr
      && tag != DW_TAG_compile_unit
      && tag != DW_TAG_skeleton_unit)
    {
      struct dwo_sections *sections = &cu->dwo_unit->dwo_file->sections;

      if (sections->rnglists.size > 0)
	return &sections->rnglists;
      else
	error (_(".debug_rnglists section is missing from .dwo file."));
    }
  return &dwarf2_per_objfile->per_bfd->rnglists;
}

/* A helper function that fills in a dwarf2_loclist_baton.  */

static void
fill_in_loclist_baton (struct dwarf2_cu *cu,
		       struct dwarf2_loclist_baton *baton,
		       const struct attribute *attr)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct dwarf2_section_info *section = cu_debug_loc_section (cu);

  section->read (per_objfile->objfile);

  baton->per_objfile = per_objfile;
  baton->per_cu = cu->per_cu;
  gdb_assert (baton->per_cu);
  /* We don't know how long the location list is, but make sure we
     don't run off the edge of the section.  */
  baton->size = section->size - attr->as_unsigned ();
  baton->data = section->buffer + attr->as_unsigned ();
  if (cu->base_address.has_value ())
    baton->base_address = *cu->base_address;
  else
    baton->base_address = {};
  baton->from_dwo = cu->dwo_unit != NULL;
  baton->dwarf_version = cu->header.version;
}

static void
dwarf2_symbol_mark_computed (const struct attribute *attr, struct symbol *sym,
			     struct dwarf2_cu *cu, int is_block)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct dwarf2_section_info *section = cu_debug_loc_section (cu);

  if (attr->form_is_section_offset ()
      /* .debug_loc{,.dwo} may not exist at all, or the offset may be outside
	 the section.  If so, fall through to the complaint in the
	 other branch.  */
      && attr->as_unsigned () < section->size)
    {
      struct dwarf2_loclist_baton *baton;

      baton = XOBNEW (&objfile->objfile_obstack, struct dwarf2_loclist_baton);

      fill_in_loclist_baton (cu, baton, attr);

      if (!cu->base_address.has_value ())
	complaint (_("Location list used without "
		     "specifying the CU base address."));

      sym->set_loc_class_index ((is_block
			      ? dwarf2_loclist_block_index
			      : dwarf2_loclist_index));
      SYMBOL_LOCATION_BATON (sym) = baton;
    }
  else
    {
      struct dwarf2_locexpr_baton *baton;

      baton = OBSTACK_ZALLOC (&objfile->objfile_obstack,
			      struct dwarf2_locexpr_baton);
      baton->per_objfile = per_objfile;
      baton->per_cu = cu->per_cu;
      gdb_assert (baton->per_cu);

      if (attr->form_is_block ())
	{
	  /* Note that we're just copying the block's data pointer
	     here, not the actual data.  We're still pointing into the
	     info_buffer for SYM's objfile; right now we never release
	     that buffer, but when we do clean up properly this may
	     need to change.  */
	  struct dwarf_block *block = attr->as_block ();
	  baton->size = block->size;
	  baton->data = block->data;
	}
      else
	{
	  dwarf2_invalid_attrib_class_complaint ("location description",
						 sym->natural_name ());
	  baton->size = 0;
	}

      sym->set_loc_class_index ((is_block
			      ? dwarf2_locexpr_block_index
			      : dwarf2_locexpr_index));
      SYMBOL_LOCATION_BATON (sym) = baton;
    }
}

/* See read.h.  */

void
dwarf2_per_cu::set_lang (enum language lang, dwarf_source_language dw_lang)
{
  if (unit_type () == DW_UT_partial)
    return;

  /* Set if not set already.  */
  packed<language, LANGUAGE_BYTES> new_value = lang;
  packed<language, LANGUAGE_BYTES> old_value = m_lang.exchange (new_value);
  /* If already set, verify that it's the same value.  */
  gdb_assert (old_value == language_unknown
	      || old_value == language_minimal
	      || old_value == lang);

  packed<dwarf_source_language, 2> new_dw = dw_lang;
  packed<dwarf_source_language, 2> old_dw = m_dw_lang.exchange (new_dw);
  gdb_assert (old_dw == 0 || old_dw == dw_lang);
}

/* See read.h.  */

void
dwarf2_per_cu::ensure_lang (dwarf2_per_objfile *per_objfile)
{
  if (lang (false) != language_unknown)
    return;

  /* Constructing this object will set the language as a side
     effect.  */
  cutu_reader reader (*this, *per_objfile, nullptr, per_objfile->get_cu (this),
		      true, language_minimal, nullptr);
}

/* Return the unit from ALL_UNITS that potentially contains TARGET.

   Since the unit lengths may not be known yet, this function doesn't check that
   TARGET.OFFSET actually falls within the range of the returned unit.  The
   caller is responsible for this.

   If no units possibly match TARGET, return nullptr.  */

static dwarf2_per_cu *
dwarf2_find_containing_unit (const section_and_offset &target,
			     const std::vector<dwarf2_per_cu_up> &all_units)
{
  auto it = std::lower_bound (all_units.begin (), all_units.end (), target,
			      [] (const dwarf2_per_cu_up &per_cu,
				  const section_and_offset &key)
				{
				  return all_units_less_than (*per_cu, key);
				});

  if (it == all_units.begin ())
    {
      /* TARGET falls before the first unit of the first section, or is an
	 exact match with the first.  */
      if ((*it)->section == target.section && (*it)->sect_off == target.offset)
	return it->get ();
      else
	return nullptr;
    }

  if (it != all_units.end ()
      && (*it)->section == target.section
      && (*it)->sect_off == target.offset)
    {
      /* TARGET is an exact match with the start of *IT, so *IT is what we're
	 looking for.  */
      return it->get ();
    }

  /* Otherwise, the match is the one just before, as long as it matches the
     section we're looking for.  */
  --it;

  if ((*it)->section == target.section)
    return it->get ();

  return nullptr;
}

/* See read.h.  */

dwarf2_per_cu *
dwarf2_find_containing_unit (const section_and_offset &target,
			     dwarf2_per_objfile *per_objfile)
{
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  dwarf2_per_cu *per_cu
    = dwarf2_find_containing_unit (target, per_bfd->all_units);
  auto error_out = [&target, per_bfd] ()
    {
      error (_(DWARF_ERROR_PREFIX
	       "could not find unit containing offset %s [in module %s]"),
	     sect_offset_str (target.offset), per_bfd->filename ());
    };

  if (per_cu == nullptr)
    error_out ();

  gdb_assert (per_cu->section == target.section);

  /* Some producers of dwarf2_per_cu objects (thinking of the .gdb_index reader)
     do not set the length ahead of time.  The length is needed to check if
     the target is truly within PER_CU's range, so compute it now.  Constructing
     the cutu_reader object has the side-effect of setting PER_CU's length.
     Even though it should happen too often, it could be replaced with
     something more lightweight that has the same effect.  */
  if (!per_cu->length_is_set ())
    cutu_reader (*per_cu, *per_objfile, nullptr, nullptr, false,
		 language_minimal);

  /* Now we can check if the target section offset is within PER_CU's range.  */
  if (target.offset < per_cu->sect_off
      || target.offset >= per_cu->sect_off + per_cu->length ())
    error_out ();

  return per_cu;
}

/* See read.h.  */

dwarf2_per_cu *
dwarf2_find_unit (const section_and_offset &start, dwarf2_per_bfd *per_bfd)
{
  auto it = std::lower_bound (per_bfd->all_units.begin (),
			      per_bfd->all_units.end (), start,
			      [] (const dwarf2_per_cu_up &per_cu,
				  const section_and_offset &key)
				{
				  return all_units_less_than (*per_cu, key);
				});

  if (it == per_bfd->all_units.end ())
    return nullptr;

  dwarf2_per_cu *per_cu = it->get ();

  if (per_cu->section != start.section || per_cu->sect_off != start.offset)
    return nullptr;

  return per_cu;
}

#if GDB_SELF_TEST

namespace selftests {
namespace find_containing_comp_unit {

static void
run_test ()
{
  auto dummy_per_bfd = reinterpret_cast<dwarf2_per_bfd *> (0x3000);
  auto &main_section = *reinterpret_cast<dwarf2_section_info *> (0x4000);
  auto &dwz_section = *reinterpret_cast<dwarf2_section_info *> (0x5000);
  std::vector<dwarf2_per_cu_up> units;

  /* Create one dummy unit, append it to UNITS, return a non-owning
     reference.  */
  auto create_dummy_per_unit = [&] (dwarf2_section_info &section,
				    unsigned int sect_off, bool is_dwz)
			       -> dwarf2_per_cu &
    {
      /* Omit the length, because dwarf2_find_containing_unit does not consider
	 it.  */
      return *units.emplace_back (new dwarf2_per_cu (dummy_per_bfd, &section,
						     sect_offset (sect_off),
						     0, is_dwz));
    };

  /* Create 2 units in the main file and 2 units in the supplementary (dwz)
     file.  */
  auto &main1 = create_dummy_per_unit (main_section, 10, false);
  auto &main2 = create_dummy_per_unit (main_section, 20, false);
  auto &dwz1 = create_dummy_per_unit (dwz_section, 10, false);
  auto &dwz2 = create_dummy_per_unit (dwz_section, 20, false);

  /* Check that looking up a unit at all offsets in the range [START,END[ in
     section SECTION finds EXPECTED.  */
  auto check_range = [&units] (dwarf2_section_info &section, unsigned int start,
			       unsigned int end, dwarf2_per_cu *expected)
   {
      for (unsigned int sect_off = start; sect_off < end; ++sect_off)
	{
	  section_and_offset target { &section, sect_offset (sect_off) };
	  dwarf2_per_cu *result = dwarf2_find_containing_unit (target, units);

	  SELF_CHECK (result == expected);
	}
   };

  check_range (main_section, 0, 10, nullptr);
  check_range (main_section, 10, 20, &main1);
  check_range (main_section, 20, 30, &main2);

  check_range (dwz_section, 0, 10, nullptr);
  check_range (dwz_section, 10, 20, &dwz1);
  check_range (dwz_section, 20, 30, &dwz2);
}
} /* namespace find_containing_comp_unit */
} /* namespace selftests */

#endif /* GDB_SELF_TEST */

/* Initialize basic fields of dwarf_cu CU according to DIE
   COMP_UNIT_DIE.  If COMP_UNIT_DIE is NULL, the CU is assumed to be a
   CU one with no contents; in this case default values are used for
   the fields.  */

void
cutu_reader::prepare_one_comp_unit (struct dwarf2_cu *cu,
				    enum language pretend_language)
{
  struct attribute *attr;

  if (m_top_level_die == nullptr)
    {
      cu->set_producer (nullptr);
      cu->language_defn = language_def (pretend_language);
      cu->per_cu->set_unit_type (DW_UT_compile);
      cu->per_cu->set_lang (pretend_language, (dwarf_source_language) 0);
      return;
    }

  cu->set_producer (dwarf2_string_attr (m_top_level_die, DW_AT_producer, cu));

  /* Set the language we're debugging.  */
  attr = dwarf2_attr (m_top_level_die, DW_AT_language, cu);
  enum language lang;
  dwarf_source_language dw_lang = (dwarf_source_language) 0;
  if (cu->producer_is_xlc_opencl ())
    {
      /* The XLCL doesn't generate DW_LANG_OpenCL because this
	 attribute is not standardised yet.  As a workaround for the
	 language detection we fall back to the DW_AT_producer
	 string.  */
      lang = language_opencl;
      dw_lang = DW_LANG_OpenCL;
    }
  else if (cu->producer_is_ggo ())
    {
      /* Similar hack for Go.  */
      lang = language_go;
      dw_lang = DW_LANG_Go;
    }
  else if (attr != nullptr)
    {
      std::optional<ULONGEST> lang_val = attr->unsigned_constant ();
      if (lang_val.has_value ())
	{
	  lang = dwarf_lang_to_enum_language (*lang_val);
	  if (lang_val <= DW_LANG_hi_user)
	    dw_lang = (dwarf_source_language) *lang_val;
	}
      else
	lang = language_minimal;
    }
  else
    lang = pretend_language;

  cu->language_defn = language_def (lang);

  /* Initialize the lto_artificial field.  */
  attr = dwarf2_attr (m_top_level_die, DW_AT_name, cu);
  if (attr != nullptr
      && cu->producer_is_gcc ()
      && attr->as_string () != nullptr
      && strcmp (attr->as_string (), "<artificial>") == 0)
    cu->per_cu->lto_artificial = true;

  switch (m_top_level_die->tag)
    {
    case DW_TAG_compile_unit:
      cu->per_cu->set_unit_type (DW_UT_compile);
      break;
    case DW_TAG_partial_unit:
      cu->per_cu->set_unit_type (DW_UT_partial);
      break;
    case DW_TAG_type_unit:
      cu->per_cu->set_unit_type (DW_UT_type);
      break;
    default:
      error (_(DWARF_ERROR_PREFIX "unexpected tag '%s' at offset %s"),
	     dwarf_tag_name (m_top_level_die->tag),
	     sect_offset_str (cu->per_cu->sect_off));
    }

  cu->per_cu->set_lang (lang, dw_lang);
}

/* See read.h.  */

dwarf2_cu *
dwarf2_per_objfile::get_cu (dwarf2_per_cu *per_cu)
{
  auto it = m_dwarf2_cus.find (per_cu);
  if (it == m_dwarf2_cus.end ())
    return nullptr;

  return it->second.get ();
}

/* See read.h.  */

void
dwarf2_per_objfile::set_cu (dwarf2_per_cu *per_cu, dwarf2_cu_up cu)
{
  gdb_assert (this->get_cu (per_cu) == nullptr);

  m_dwarf2_cus[per_cu] = std::move (cu);
}

/* See read.h.  */

void
dwarf2_per_objfile::age_comp_units ()
{
  dwarf_read_debug_printf_v ("running");

  /* This is not expected to be called in the middle of CU expansion.  There is
     an invariant that if a CU is in the CUs-to-expand queue, its DIEs are
     loaded in memory.  Calling age_comp_units while the queue is in use could
     make us free the DIEs for a CU that is in the queue and therefore break
     that invariant.  */
  gdb_assert (!queue.has_value ());

  /* Start by clearing all marks.  */
  for (const auto &pair : m_dwarf2_cus)
    pair.second->clear_mark ();

  /* Traverse all CUs, mark them and their dependencies if used recently
     enough.  */
  for (const auto &pair : m_dwarf2_cus)
    {
      dwarf2_cu *cu = pair.second.get ();

      cu->last_used++;
      if (cu->last_used <= dwarf_max_cache_age)
	cu->mark ();
    }

  /* Delete all CUs still not marked.  */
  for (auto it = m_dwarf2_cus.begin (); it != m_dwarf2_cus.end ();)
    {
      dwarf2_cu *cu = it->second.get ();

      if (!cu->is_marked ())
	{
	  dwarf_read_debug_printf_v ("deleting old CU %s",
				     sect_offset_str (cu->per_cu->sect_off));
	  it = m_dwarf2_cus.erase (it);
	}
      else
	it++;
    }
}

/* See read.h.  */

void
dwarf2_per_objfile::remove_cu (dwarf2_per_cu *per_cu)
{
  auto it = m_dwarf2_cus.find (per_cu);
  if (it == m_dwarf2_cus.end ())
    return;

  m_dwarf2_cus.erase (it);
}

dwarf2_per_objfile::~dwarf2_per_objfile ()
{
  remove_all_cus ();
}

/* Set the type associated with DIE to TYPE.  Save it in CU's hash
   table if necessary.  For convenience, return TYPE.

   The DIEs reading must have careful ordering to:
    * Not cause infinite loops trying to read in DIEs as a prerequisite for
      reading current DIE.
    * Not trying to dereference contents of still incompletely read in types
      while reading in other DIEs.
    * Enable referencing still incompletely read in types just by a pointer to
      the type without accessing its fields.

   Therefore caller should follow these rules:
     * Try to fetch any prerequisite types we may need to build this DIE type
       before building the type and calling set_die_type.
     * After building type call set_die_type for current DIE as soon as
       possible before fetching more types to complete the current type.
     * Make the type as complete as possible before fetching more types.  */

static struct type *
set_die_type (struct die_info *die, struct type *type, struct dwarf2_cu *cu,
	      bool skip_data_location)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct attribute *attr;
  struct dynamic_prop prop;

  /* For Ada types, make sure that the gnat-specific data is always
     initialized (if not already set).  There are a few types where
     we should not be doing so, because the type-specific area is
     already used to hold some other piece of info (eg: TYPE_CODE_FLT
     where the type-specific area is used to store the floatformat).
     But this is not a problem, because the gnat-specific information
     is actually not needed for these types.  */
  if (need_gnat_info (cu)
      && type->code () != TYPE_CODE_FUNC
      && type->code () != TYPE_CODE_FLT
      && type->code () != TYPE_CODE_METHODPTR
      && type->code () != TYPE_CODE_MEMBERPTR
      && type->code () != TYPE_CODE_METHOD
      && type->code () != TYPE_CODE_FIXED_POINT
      && !HAVE_GNAT_AUX_INFO (type))
    INIT_GNAT_SPECIFIC (type);

  /* Read DW_AT_allocated and set in type.  */
  attr = dwarf2_attr (die, DW_AT_allocated, cu);
  if (attr != NULL)
    {
      struct type *prop_type = cu->addr_sized_int_type (false);
      if (attr_to_dynamic_prop (attr, die, cu, &prop, prop_type))
	type->add_dyn_prop (DYN_PROP_ALLOCATED, prop);
    }

  /* Read DW_AT_associated and set in type.  */
  attr = dwarf2_attr (die, DW_AT_associated, cu);
  if (attr != NULL)
    {
      struct type *prop_type = cu->addr_sized_int_type (false);
      if (attr_to_dynamic_prop (attr, die, cu, &prop, prop_type))
	type->add_dyn_prop (DYN_PROP_ASSOCIATED, prop);
    }

  /* Read DW_AT_rank and set in type.  */
  attr = dwarf2_attr (die, DW_AT_rank, cu);
  if (attr != NULL)
    {
      struct type *prop_type = cu->addr_sized_int_type (false);
      if (attr_to_dynamic_prop (attr, die, cu, &prop, prop_type))
	type->add_dyn_prop (DYN_PROP_RANK, prop);
    }

  /* Read DW_AT_data_location and set in type.  */
  if (!skip_data_location)
    {
      attr = dwarf2_attr (die, DW_AT_data_location, cu);
      if (attr_to_dynamic_prop (attr, die, cu, &prop, cu->addr_type ()))
	type->add_dyn_prop (DYN_PROP_DATA_LOCATION, prop);
    }

  bool inserted
    = per_objfile->die_type_hash.emplace
       (per_cu_and_offset {cu->per_cu, die->sect_off}, type).second;
  if (!inserted)
    complaint (_("A problem internal to GDB: DIE %s has type already set"),
	       sect_offset_str (die->sect_off));

  return type;
}

/* Look up the type for the die at SECT_OFF in PER_CU in die_type_hash,
   or return NULL if the die does not have a saved type.  */

static struct type *
get_die_type_at_offset (sect_offset sect_off, dwarf2_per_cu *per_cu,
			dwarf2_per_objfile *per_objfile)
{
  auto it = per_objfile->die_type_hash.find ({per_cu, sect_off});

  return it != per_objfile->die_type_hash.end () ? it->second : nullptr;
}

/* Look up the type for DIE in CU in die_type_hash,
   or return NULL if DIE does not have a saved type.  */

static struct type *
get_die_type (struct die_info *die, struct dwarf2_cu *cu)
{
  return get_die_type_at_offset (die->sect_off, cu->per_cu, cu->per_objfile);
}

struct cmd_list_element *set_dwarf_cmdlist;
struct cmd_list_element *show_dwarf_cmdlist;

static void
show_check_physname (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Whether to check \"physname\" is %s.\n"),
	      value);
}

INIT_GDB_FILE (dwarf2_read)
{
  add_setshow_prefix_cmd ("dwarf", class_maintenance,
			  _("\
Set DWARF specific variables.\n\
Configure DWARF variables such as the cache size."),
			  _("\
Show DWARF specific variables.\n\
Show DWARF variables such as the cache size."),
			  &set_dwarf_cmdlist, &show_dwarf_cmdlist,
			  &maintenance_set_cmdlist, &maintenance_show_cmdlist);

  add_setshow_zinteger_cmd ("max-cache-age", class_obscure,
			    &dwarf_max_cache_age, _("\
Set the upper bound on the age of cached DWARF compilation units."), _("\
Show the upper bound on the age of cached DWARF compilation units."), _("\
A higher limit means that cached compilation units will be stored\n\
in memory longer, and more total memory will be used.  Zero disables\n\
caching, which can slow down startup."),
			    NULL,
			    show_dwarf_max_cache_age,
			    &set_dwarf_cmdlist,
			    &show_dwarf_cmdlist);

  add_setshow_boolean_cmd ("synchronous", class_obscure,
			    &dwarf_synchronous, _("\
Set whether DWARF is read synchronously."), _("\
Show whether DWARF is read synchronously."), _("\
By default, DWARF information is read in worker threads,\n\
and gdb will not generally wait for the reading to complete\n\
before continuing with other work, for example presenting a\n\
prompt to the user.\n\
Enabling this setting will cause the DWARF reader to always wait\n\
for debug info processing to be finished before gdb can proceed."),
			    nullptr,
			    show_dwarf_synchronous,
			    &set_dwarf_cmdlist,
			    &show_dwarf_cmdlist);

  add_setshow_zuinteger_cmd ("dwarf-read", no_class, &dwarf_read_debug, _("\
Set debugging of the DWARF reader."), _("\
Show debugging of the DWARF reader."), _("\
When enabled (non-zero), debugging messages are printed during DWARF\n\
reading and symtab expansion.  A value of 1 (one) provides basic\n\
information.  A value greater than 1 provides more verbose information."),
			    NULL,
			    NULL,
			    &setdebuglist, &showdebuglist);

  add_setshow_zuinteger_cmd ("dwarf-die", no_class, &dwarf_die_debug, _("\
Set debugging of the DWARF DIE reader."), _("\
Show debugging of the DWARF DIE reader."), _("\
When enabled (non-zero), DIEs are dumped after they are read in.\n\
The value is the maximum depth to print."),
			     NULL,
			     NULL,
			     &setdebuglist, &showdebuglist);

  add_setshow_zuinteger_cmd ("dwarf-line", no_class, &dwarf_line_debug, _("\
Set debugging of the dwarf line reader."), _("\
Show debugging of the dwarf line reader."), _("\
When enabled (non-zero), line number entries are dumped as they are read in.\n\
A value of 1 (one) provides basic information.\n\
A value greater than 1 provides more verbose information."),
			     NULL,
			     NULL,
			     &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("check-physname", no_class, &check_physname, _("\
Set cross-checking of \"physname\" code against demangler."), _("\
Show cross-checking of \"physname\" code against demangler."), _("\
When enabled, GDB's internal \"physname\" code is checked against\n\
the demangler."),
			   NULL, show_check_physname,
			   &setdebuglist, &showdebuglist);

  dwarf2_locexpr_index = register_symbol_computed_impl (LOC_COMPUTED,
							&dwarf2_locexpr_funcs);
  dwarf2_loclist_index = register_symbol_computed_impl (LOC_COMPUTED,
							&dwarf2_loclist_funcs);
  ada_imported_index = register_symbol_computed_impl (LOC_COMPUTED,
						      &ada_imported_funcs);

  dwarf2_locexpr_block_index = register_symbol_block_impl (LOC_BLOCK,
					&dwarf2_block_frame_base_locexpr_funcs);
  dwarf2_loclist_block_index = register_symbol_block_impl (LOC_BLOCK,
					&dwarf2_block_frame_base_loclist_funcs);
  ada_block_index = register_symbol_block_impl (LOC_BLOCK,
						&ada_function_alias_funcs);

#if GDB_SELF_TEST
  selftests::register_test ("dwarf2_find_containing_comp_unit",
			    selftests::find_containing_comp_unit::run_test);
#endif
}
