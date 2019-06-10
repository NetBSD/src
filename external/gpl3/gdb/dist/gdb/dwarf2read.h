/* DWARF 2 debugging format support for GDB.

   Copyright (C) 1994-2019 Free Software Foundation, Inc.

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

#ifndef DWARF2READ_H
#define DWARF2READ_H

#include <unordered_map>
#include "dwarf-index-cache.h"
#include "filename-seen-cache.h"
#include "gdb_obstack.h"

/* Hold 'maintenance (set|show) dwarf' commands.  */
extern struct cmd_list_element *set_dwarf_cmdlist;
extern struct cmd_list_element *show_dwarf_cmdlist;

typedef struct dwarf2_per_cu_data *dwarf2_per_cu_ptr;
DEF_VEC_P (dwarf2_per_cu_ptr);

/* A descriptor for dwarf sections.

   S.ASECTION, SIZE are typically initialized when the objfile is first
   scanned.  BUFFER, READIN are filled in later when the section is read.
   If the section contained compressed data then SIZE is updated to record
   the uncompressed size of the section.

   DWP file format V2 introduces a wrinkle that is easiest to handle by
   creating the concept of virtual sections contained within a real section.
   In DWP V2 the sections of the input DWO files are concatenated together
   into one section, but section offsets are kept relative to the original
   input section.
   If this is a virtual dwp-v2 section, S.CONTAINING_SECTION is a backlink to
   the real section this "virtual" section is contained in, and BUFFER,SIZE
   describe the virtual section.  */

struct dwarf2_section_info
{
  union
  {
    /* If this is a real section, the bfd section.  */
    asection *section;
    /* If this is a virtual section, pointer to the containing ("real")
       section.  */
    struct dwarf2_section_info *containing_section;
  } s;
  /* Pointer to section data, only valid if readin.  */
  const gdb_byte *buffer;
  /* The size of the section, real or virtual.  */
  bfd_size_type size;
  /* If this is a virtual section, the offset in the real section.
     Only valid if is_virtual.  */
  bfd_size_type virtual_offset;
  /* True if we have tried to read this section.  */
  char readin;
  /* True if this is a virtual section, False otherwise.
     This specifies which of s.section and s.containing_section to use.  */
  char is_virtual;
};

typedef struct dwarf2_section_info dwarf2_section_info_def;
DEF_VEC_O (dwarf2_section_info_def);

/* Read the contents of the section INFO.
   OBJFILE is the main object file, but not necessarily the file where
   the section comes from.  E.g., for DWO files the bfd of INFO is the bfd
   of the DWO file.
   If the section is compressed, uncompress it before returning.  */

void dwarf2_read_section (struct objfile *objfile, dwarf2_section_info *info);

struct tu_stats
{
  int nr_uniq_abbrev_tables;
  int nr_symtabs;
  int nr_symtab_sharers;
  int nr_stmt_less_type_units;
  int nr_all_type_units_reallocs;
};

struct dwarf2_debug_sections;
struct mapped_index;
struct mapped_debug_names;
struct signatured_type;
struct die_info;
typedef struct die_info *die_info_ptr;

/* Collection of data recorded per objfile.
   This hangs off of dwarf2_objfile_data_key.  */

struct dwarf2_per_objfile : public allocate_on_obstack
{
  /* Construct a dwarf2_per_objfile for OBJFILE.  NAMES points to the
     dwarf2 section names, or is NULL if the standard ELF names are
     used.  */
  dwarf2_per_objfile (struct objfile *objfile,
		      const dwarf2_debug_sections *names);

  ~dwarf2_per_objfile ();

  DISABLE_COPY_AND_ASSIGN (dwarf2_per_objfile);

  /* Return the CU/TU given its index.

     This is intended for loops like:

     for (i = 0; i < (dwarf2_per_objfile->n_comp_units
		      + dwarf2_per_objfile->n_type_units); ++i)
       {
         dwarf2_per_cu_data *per_cu = dwarf2_per_objfile->get_cutu (i);

         ...;
       }
  */
  dwarf2_per_cu_data *get_cutu (int index);

  /* Return the CU given its index.
     This differs from get_cutu in that it's for when you know INDEX refers to a
     CU.  */
  dwarf2_per_cu_data *get_cu (int index);

  /* Return the TU given its index.
     This differs from get_cutu in that it's for when you know INDEX refers to a
     TU.  */
  signatured_type *get_tu (int index);

  /* Free all cached compilation units.  */
  void free_cached_comp_units ();
private:
  /* This function is mapped across the sections and remembers the
     offset and size of each of the debugging sections we are
     interested in.  */
  void locate_sections (bfd *abfd, asection *sectp,
			const dwarf2_debug_sections &names);

public:
  dwarf2_section_info info {};
  dwarf2_section_info abbrev {};
  dwarf2_section_info line {};
  dwarf2_section_info loc {};
  dwarf2_section_info loclists {};
  dwarf2_section_info macinfo {};
  dwarf2_section_info macro {};
  dwarf2_section_info str {};
  dwarf2_section_info line_str {};
  dwarf2_section_info ranges {};
  dwarf2_section_info rnglists {};
  dwarf2_section_info addr {};
  dwarf2_section_info frame {};
  dwarf2_section_info eh_frame {};
  dwarf2_section_info gdb_index {};
  dwarf2_section_info debug_names {};
  dwarf2_section_info debug_aranges {};

  VEC (dwarf2_section_info_def) *types = NULL;

  /* Back link.  */
  struct objfile *objfile = NULL;

  /* Table of all the compilation units.  This is used to locate
     the target compilation unit of a particular reference.  */
  std::vector<dwarf2_per_cu_data *> all_comp_units;

  /* The .debug_types-related CUs (TUs).  */
  std::vector<signatured_type *> all_type_units;

  /* Table of struct type_unit_group objects.
     The hash key is the DW_AT_stmt_list value.  */
  htab_t type_unit_groups {};

  /* A table mapping .debug_types signatures to its signatured_type entry.
     This is NULL if the .debug_types section hasn't been read in yet.  */
  htab_t signatured_types {};

  /* Type unit statistics, to see how well the scaling improvements
     are doing.  */
  struct tu_stats tu_stats {};

  /* A chain of compilation units that are currently read in, so that
     they can be freed later.  */
  dwarf2_per_cu_data *read_in_chain = NULL;

  /* A table mapping DW_AT_dwo_name values to struct dwo_file objects.
     This is NULL if the table hasn't been allocated yet.  */
  htab_t dwo_files {};

  /* True if we've checked for whether there is a DWP file.  */
  bool dwp_checked = false;

  /* The DWP file if there is one, or NULL.  */
  std::unique_ptr<struct dwp_file> dwp_file;

  /* The shared '.dwz' file, if one exists.  This is used when the
     original data was compressed using 'dwz -m'.  */
  std::unique_ptr<struct dwz_file> dwz_file;

  /* A flag indicating whether this objfile has a section loaded at a
     VMA of 0.  */
  bool has_section_at_zero = false;

  /* True if we are using the mapped index,
     or we are faking it for OBJF_READNOW's sake.  */
  bool using_index = false;

  /* The mapped index, or NULL if .gdb_index is missing or not being used.  */
  std::unique_ptr<mapped_index> index_table;

  /* The mapped index, or NULL if .debug_names is missing or not being used.  */
  std::unique_ptr<mapped_debug_names> debug_names_table;

  /* When using index_table, this keeps track of all quick_file_names entries.
     TUs typically share line table entries with a CU, so we maintain a
     separate table of all line table entries to support the sharing.
     Note that while there can be way more TUs than CUs, we've already
     sorted all the TUs into "type unit groups", grouped by their
     DW_AT_stmt_list value.  Therefore the only sharing done here is with a
     CU and its associated TU group if there is one.  */
  htab_t quick_file_names_table {};

  /* Set during partial symbol reading, to prevent queueing of full
     symbols.  */
  bool reading_partial_symbols = false;

  /* Table mapping type DIEs to their struct type *.
     This is NULL if not allocated yet.
     The mapping is done via (CU/TU + DIE offset) -> type.  */
  htab_t die_type_hash {};

  /* The CUs we recently read.  */
  std::vector<dwarf2_per_cu_data *> just_read_cus;

  /* Table containing line_header indexed by offset and offset_in_dwz.  */
  htab_t line_header_hash {};

  /* Table containing all filenames.  This is an optional because the
     table is lazily constructed on first access.  */
  gdb::optional<filename_seen_cache> filenames_cache;

  /* If we loaded the index from an external file, this contains the
     resources associated to the open file, memory mapping, etc.  */
  std::unique_ptr<index_cache_resource> index_cache_res;

  /* Mapping from abstract origin DIE to concrete DIEs that reference it as
     DW_AT_abstract_origin.  */
  std::unordered_map<die_info_ptr, std::vector<die_info_ptr>>
    abstract_to_concrete;
};

/* Get the dwarf2_per_objfile associated to OBJFILE.  */

dwarf2_per_objfile *get_dwarf2_per_objfile (struct objfile *objfile);

/* Persistent data held for a compilation unit, even when not
   processing it.  We put a pointer to this structure in the
   read_symtab_private field of the psymtab.  */

struct dwarf2_per_cu_data
{
  /* The start offset and length of this compilation unit.
     NOTE: Unlike comp_unit_head.length, this length includes
     initial_length_size.
     If the DIE refers to a DWO file, this is always of the original die,
     not the DWO file.  */
  sect_offset sect_off;
  unsigned int length;

  /* DWARF standard version this data has been read from (such as 4 or 5).  */
  short dwarf_version;

  /* Flag indicating this compilation unit will be read in before
     any of the current compilation units are processed.  */
  unsigned int queued : 1;

  /* This flag will be set when reading partial DIEs if we need to load
     absolutely all DIEs for this compilation unit, instead of just the ones
     we think are interesting.  It gets set if we look for a DIE in the
     hash table and don't find it.  */
  unsigned int load_all_dies : 1;

  /* Non-zero if this CU is from .debug_types.
     Struct dwarf2_per_cu_data is contained in struct signatured_type iff
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

  /* The section this CU/TU lives in.
     If the DIE refers to a DWO file, this is always the original die,
     not the DWO file.  */
  struct dwarf2_section_info *section;

  /* Set to non-NULL iff this CU is currently loaded.  When it gets freed out
     of the CU cache it gets reset to NULL again.  This is left as NULL for
     dummy CUs (a CU header, but nothing else).  */
  struct dwarf2_cu *cu;

  /* The corresponding dwarf2_per_objfile.  */
  struct dwarf2_per_objfile *dwarf2_per_objfile;

  /* When dwarf2_per_objfile->using_index is true, the 'quick' field
     is active.  Otherwise, the 'psymtab' field is active.  */
  union
  {
    /* The partial symbol table associated with this compilation unit,
       or NULL for unread partial units.  */
    struct partial_symtab *psymtab;

    /* Data needed by the "quick" functions.  */
    struct dwarf2_per_cu_quick_data *quick;
  } v;

  /* The CUs we import using DW_TAG_imported_unit.  This is filled in
     while reading psymtabs, used to compute the psymtab dependencies,
     and then cleared.  Then it is filled in again while reading full
     symbols, and only deleted when the objfile is destroyed.

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
  VEC (dwarf2_per_cu_ptr) *imported_symtabs;
};

/* Entry in the signatured_types hash table.  */

struct signatured_type
{
  /* The "per_cu" object of this type.
     This struct is used iff per_cu.is_debug_types.
     N.B.: This is the first member so that it's easy to convert pointers
     between them.  */
  struct dwarf2_per_cu_data per_cu;

  /* The type's signature.  */
  ULONGEST signature;

  /* Offset in the TU of the type's DIE, as read from the TU header.
     If this TU is a DWO stub and the definition lives in a DWO file
     (specified by DW_AT_GNU_dwo_name), this value is unusable.  */
  cu_offset type_offset_in_tu;

  /* Offset in the section of the type's DIE.
     If the definition lives in a DWO file, this is the offset in the
     .debug_types.dwo section.
     The value is zero until the actual value is known.
     Zero is otherwise not a valid section offset.  */
  sect_offset type_offset_in_section;

  /* Type units are grouped by their DW_AT_stmt_list entry so that they
     can share them.  This points to the containing symtab.  */
  struct type_unit_group *type_unit_group;

  /* The type.
     The first time we encounter this type we fully read it in and install it
     in the symbol tables.  Subsequent times we only need the type.  */
  struct type *type;

  /* Containing DWO unit.
     This field is valid iff per_cu.reading_dwo_directly.  */
  struct dwo_unit *dwo_unit;
};

typedef struct signatured_type *sig_type_ptr;
DEF_VEC_P (sig_type_ptr);

#endif /* DWARF2READ_H */
