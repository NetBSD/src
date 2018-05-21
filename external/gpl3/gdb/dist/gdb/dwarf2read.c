/* DWARF 2 debugging format support for GDB.

   Copyright (C) 1994-2017 Free Software Foundation, Inc.

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
   reading off the end of the section.
   E.g., load_partial_dies, read_partial_die.  */

#include "defs.h"
#include "bfd.h"
#include "elf-bfd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "objfiles.h"
#include "dwarf2.h"
#include "buildsym.h"
#include "demangle.h"
#include "gdb-demangle.h"
#include "expression.h"
#include "filenames.h"	/* for DOSish file names */
#include "macrotab.h"
#include "language.h"
#include "complaints.h"
#include "bcache.h"
#include "dwarf2expr.h"
#include "dwarf2loc.h"
#include "cp-support.h"
#include "hashtab.h"
#include "command.h"
#include "gdbcmd.h"
#include "block.h"
#include "addrmap.h"
#include "typeprint.h"
#include "psympriv.h"
#include <sys/stat.h>
#include "completer.h"
#include "vec.h"
#include "c-lang.h"
#include "go-lang.h"
#include "valprint.h"
#include "gdbcore.h" /* for gnutarget */
#include "gdb/gdb-index.h"
#include <ctype.h>
#include "gdb_bfd.h"
#include "f-lang.h"
#include "source.h"
#include "filestuff.h"
#include "build-id.h"
#include "namespace.h"
#include "common/gdb_unlinker.h"
#include "common/function-view.h"
#include "common/gdb_optional.h"
#include "common/underlying.h"

#include <fcntl.h>
#include <sys/types.h>
#include <algorithm>

typedef struct symbol *symbolp;
DEF_VEC_P (symbolp);

/* When == 1, print basic high level tracing messages.
   When > 1, be more verbose.
   This is in contrast to the low level DIE reading of dwarf_die_debug.  */
static unsigned int dwarf_read_debug = 0;

/* When non-zero, dump DIEs after they are read in.  */
static unsigned int dwarf_die_debug = 0;

/* When non-zero, dump line number entries as they are read in.  */
static unsigned int dwarf_line_debug = 0;

/* When non-zero, cross-check physname against demangler.  */
static int check_physname = 0;

/* When non-zero, do not reject deprecated .gdb_index sections.  */
static int use_deprecated_index_sections = 0;

static const struct objfile_data *dwarf2_objfile_data_key;

/* The "aclass" indices for various kinds of computed DWARF symbols.  */

static int dwarf2_locexpr_index;
static int dwarf2_loclist_index;
static int dwarf2_locexpr_block_index;
static int dwarf2_loclist_block_index;

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

/* All offsets in the index are of this type.  It must be
   architecture-independent.  */
typedef uint32_t offset_type;

DEF_VEC_I (offset_type);

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

/* Ensure we don't use more than the alloted nuber of bits for the CU.  */
#define DW2_GDB_INDEX_CU_SET_VALUE(cu_index, value) \
  do { \
    gdb_assert (((value) & ~GDB_INDEX_CU_MASK) == 0); \
    GDB_INDEX_CU_SET_VALUE((cu_index), (value)); \
  } while (0)

/* A description of the mapped index.  The file format is described in
   a comment by the code that writes the index.  */
struct mapped_index
{
  /* Index data format version.  */
  int version;

  /* The total length of the buffer.  */
  off_t total_size;

  /* A pointer to the address table data.  */
  const gdb_byte *address_table;

  /* Size of the address table data in bytes.  */
  offset_type address_table_size;

  /* The symbol table, implemented as a hash table.  */
  const offset_type *symbol_table;

  /* Size in slots, each slot is 2 offset_types.  */
  offset_type symbol_table_slots;

  /* A pointer to the constant pool.  */
  const char *constant_pool;
};

typedef struct dwarf2_per_cu_data *dwarf2_per_cu_ptr;
DEF_VEC_P (dwarf2_per_cu_ptr);

struct tu_stats
{
  int nr_uniq_abbrev_tables;
  int nr_symtabs;
  int nr_symtab_sharers;
  int nr_stmt_less_type_units;
  int nr_all_type_units_reallocs;
};

/* Collection of data recorded per objfile.
   This hangs off of dwarf2_objfile_data_key.  */

struct dwarf2_per_objfile
{
  struct dwarf2_section_info info;
  struct dwarf2_section_info abbrev;
  struct dwarf2_section_info line;
  struct dwarf2_section_info loc;
  struct dwarf2_section_info loclists;
  struct dwarf2_section_info macinfo;
  struct dwarf2_section_info macro;
  struct dwarf2_section_info str;
  struct dwarf2_section_info line_str;
  struct dwarf2_section_info ranges;
  struct dwarf2_section_info rnglists;
  struct dwarf2_section_info addr;
  struct dwarf2_section_info frame;
  struct dwarf2_section_info eh_frame;
  struct dwarf2_section_info gdb_index;

  VEC (dwarf2_section_info_def) *types;

  /* Back link.  */
  struct objfile *objfile;

  /* Table of all the compilation units.  This is used to locate
     the target compilation unit of a particular reference.  */
  struct dwarf2_per_cu_data **all_comp_units;

  /* The number of compilation units in ALL_COMP_UNITS.  */
  int n_comp_units;

  /* The number of .debug_types-related CUs.  */
  int n_type_units;

  /* The number of elements allocated in all_type_units.
     If there are skeleton-less TUs, we add them to all_type_units lazily.  */
  int n_allocated_type_units;

  /* The .debug_types-related CUs (TUs).
     This is stored in malloc space because we may realloc it.  */
  struct signatured_type **all_type_units;

  /* Table of struct type_unit_group objects.
     The hash key is the DW_AT_stmt_list value.  */
  htab_t type_unit_groups;

  /* A table mapping .debug_types signatures to its signatured_type entry.
     This is NULL if the .debug_types section hasn't been read in yet.  */
  htab_t signatured_types;

  /* Type unit statistics, to see how well the scaling improvements
     are doing.  */
  struct tu_stats tu_stats;

  /* A chain of compilation units that are currently read in, so that
     they can be freed later.  */
  struct dwarf2_per_cu_data *read_in_chain;

  /* A table mapping DW_AT_dwo_name values to struct dwo_file objects.
     This is NULL if the table hasn't been allocated yet.  */
  htab_t dwo_files;

  /* Non-zero if we've check for whether there is a DWP file.  */
  int dwp_checked;

  /* The DWP file if there is one, or NULL.  */
  struct dwp_file *dwp_file;

  /* The shared '.dwz' file, if one exists.  This is used when the
     original data was compressed using 'dwz -m'.  */
  struct dwz_file *dwz_file;

  /* A flag indicating wether this objfile has a section loaded at a
     VMA of 0.  */
  int has_section_at_zero;

  /* True if we are using the mapped index,
     or we are faking it for OBJF_READNOW's sake.  */
  unsigned char using_index;

  /* The mapped index, or NULL if .gdb_index is missing or not being used.  */
  struct mapped_index *index_table;

  /* When using index_table, this keeps track of all quick_file_names entries.
     TUs typically share line table entries with a CU, so we maintain a
     separate table of all line table entries to support the sharing.
     Note that while there can be way more TUs than CUs, we've already
     sorted all the TUs into "type unit groups", grouped by their
     DW_AT_stmt_list value.  Therefore the only sharing done here is with a
     CU and its associated TU group if there is one.  */
  htab_t quick_file_names_table;

  /* Set during partial symbol reading, to prevent queueing of full
     symbols.  */
  int reading_partial_symbols;

  /* Table mapping type DIEs to their struct type *.
     This is NULL if not allocated yet.
     The mapping is done via (CU/TU + DIE offset) -> type.  */
  htab_t die_type_hash;

  /* The CUs we recently read.  */
  VEC (dwarf2_per_cu_ptr) *just_read_cus;

  /* Table containing line_header indexed by offset and offset_in_dwz.  */
  htab_t line_header_hash;
};

static struct dwarf2_per_objfile *dwarf2_per_objfile;

/* Default names of the debugging sections.  */

/* Note that if the debugging section has been compressed, it might
   have a name like .zdebug_info.  */

static const struct dwarf2_debug_sections dwarf2_elf_names =
{
  { ".debug_info", ".zdebug_info" },
  { ".debug_abbrev", ".zdebug_abbrev" },
  { ".debug_line", ".zdebug_line" },
  { ".debug_loc", ".zdebug_loc" },
  { ".debug_loclists", ".zdebug_loclists" },
  { ".debug_macinfo", ".zdebug_macinfo" },
  { ".debug_macro", ".zdebug_macro" },
  { ".debug_str", ".zdebug_str" },
  { ".debug_line_str", ".zdebug_line_str" },
  { ".debug_ranges", ".zdebug_ranges" },
  { ".debug_rnglists", ".zdebug_rnglists" },
  { ".debug_types", ".zdebug_types" },
  { ".debug_addr", ".zdebug_addr" },
  { ".debug_frame", ".zdebug_frame" },
  { ".eh_frame", NULL },
  { ".gdb_index", ".zgdb_index" },
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
  { ".debug_str.dwo", ".zdebug_str.dwo" },
  { ".debug_str_offsets.dwo", ".zdebug_str_offsets.dwo" },
  { ".debug_types.dwo", ".zdebug_types.dwo" },
  { ".debug_cu_index", ".zdebug_cu_index" },
  { ".debug_tu_index", ".zdebug_tu_index" },
};

/* local data types */

/* The data in a compilation unit header, after target2host
   translation, looks like this.  */
struct comp_unit_head
{
  unsigned int length;
  short version;
  unsigned char addr_size;
  unsigned char signed_addr_p;
  sect_offset abbrev_sect_off;

  /* Size of file offsets; either 4 or 8.  */
  unsigned int offset_size;

  /* Size of the length field; either 4 or 12.  */
  unsigned int initial_length_size;

  enum dwarf_unit_type unit_type;

  /* Offset to the first byte of this compilation unit header in the
     .debug_info section, for resolving relative reference dies.  */
  sect_offset sect_off;

  /* Offset to first die in this cu from the start of the cu.
     This will be the first byte following the compilation unit header.  */
  cu_offset first_die_cu_offset;

  /* 64-bit signature of this type unit - it is valid only for
     UNIT_TYPE DW_UT_type.  */
  ULONGEST signature;

  /* For types, offset in the type's DIE of the type defined by this TU.  */
  cu_offset type_cu_offset_in_tu;
};

/* Type used for delaying computation of method physnames.
   See comments for compute_delayed_physnames.  */
struct delayed_method_info
{
  /* The type to which the method is attached, i.e., its parent class.  */
  struct type *type;

  /* The index of the method in the type's function fieldlists.  */
  int fnfield_index;

  /* The index of the method in the fieldlist.  */
  int index;

  /* The name of the DIE.  */
  const char *name;

  /*  The DIE associated with this method.  */
  struct die_info *die;
};

typedef struct delayed_method_info delayed_method_info;
DEF_VEC_O (delayed_method_info);

/* Internal state when decoding a particular compilation unit.  */
struct dwarf2_cu
{
  /* The objfile containing this compilation unit.  */
  struct objfile *objfile;

  /* The header of the compilation unit.  */
  struct comp_unit_head header;

  /* Base address of this compilation unit.  */
  CORE_ADDR base_address;

  /* Non-zero if base_address has been set.  */
  int base_known;

  /* The language we are debugging.  */
  enum language language;
  const struct language_defn *language_defn;

  const char *producer;

  /* The generic symbol table building routines have separate lists for
     file scope symbols and all all other scopes (local scopes).  So
     we need to select the right one to pass to add_symbol_to_list().
     We do it by keeping a pointer to the correct list in list_in_scope.

     FIXME: The original dwarf code just treated the file scope as the
     first local scope, and all other local scopes as nested local
     scopes, and worked fine.  Check to see if we really need to
     distinguish these in buildsym.c.  */
  struct pending **list_in_scope;

  /* The abbrev table for this CU.
     Normally this points to the abbrev table in the objfile.
     But if DWO_UNIT is non-NULL this is the abbrev table in the DWO file.  */
  struct abbrev_table *abbrev_table;

  /* Hash table holding all the loaded partial DIEs
     with partial_die->offset.SECT_OFF as hash.  */
  htab_t partial_dies;

  /* Storage for things with the same lifetime as this read-in compilation
     unit, including partial DIEs.  */
  struct obstack comp_unit_obstack;

  /* When multiple dwarf2_cu structures are living in memory, this field
     chains them all together, so that they can be released efficiently.
     We will probably also want a generation counter so that most-recently-used
     compilation units are cached...  */
  struct dwarf2_per_cu_data *read_in_chain;

  /* Backlink to our per_cu entry.  */
  struct dwarf2_per_cu_data *per_cu;

  /* How many compilation units ago was this CU last referenced?  */
  int last_used;

  /* A hash table of DIE cu_offset for following references with
     die_info->offset.sect_off as hash.  */
  htab_t die_hash;

  /* Full DIEs if read in.  */
  struct die_info *dies;

  /* A set of pointers to dwarf2_per_cu_data objects for compilation
     units referenced by this one.  Only set during full symbol processing;
     partial symbol tables do not have dependencies.  */
  htab_t dependencies;

  /* Header data from the line table, during full symbol processing.  */
  struct line_header *line_header;

  /* A list of methods which need to have physnames computed
     after all type information has been read.  */
  VEC (delayed_method_info) *method_list;

  /* To be copied to symtab->call_site_htab.  */
  htab_t call_site_htab;

  /* Non-NULL if this CU came from a DWO file.
     There is an invariant here that is important to remember:
     Except for attributes copied from the top level DIE in the "main"
     (or "stub") file in preparation for reading the DWO file
     (e.g., DW_AT_GNU_addr_base), we KISS: there is only *one* CU.
     Either there isn't a DWO file (in which case this is NULL and the point
     is moot), or there is and either we're not going to read it (in which
     case this is NULL) or there is and we are reading it (in which case this
     is non-NULL).  */
  struct dwo_unit *dwo_unit;

  /* The DW_AT_addr_base attribute if present, zero otherwise
     (zero is a valid value though).
     Note this value comes from the Fission stub CU/TU's DIE.  */
  ULONGEST addr_base;

  /* The DW_AT_ranges_base attribute if present, zero otherwise
     (zero is a valid value though).
     Note this value comes from the Fission stub CU/TU's DIE.
     Also note that the value is zero in the non-DWO case so this value can
     be used without needing to know whether DWO files are in use or not.
     N.B. This does not apply to DW_AT_ranges appearing in
     DW_TAG_compile_unit dies.  This is a bit of a wart, consider if ever
     DW_AT_ranges appeared in the DW_TAG_compile_unit of DWO DIEs: then
     DW_AT_ranges_base *would* have to be applied, and we'd have to care
     whether the DW_AT_ranges attribute came from the skeleton or DWO.  */
  ULONGEST ranges_base;

  /* Mark used when releasing cached dies.  */
  unsigned int mark : 1;

  /* This CU references .debug_loc.  See the symtab->locations_valid field.
     This test is imperfect as there may exist optimized debug code not using
     any location list and still facing inlining issues if handled as
     unoptimized code.  For a future better test see GCC PR other/32998.  */
  unsigned int has_loclist : 1;

  /* These cache the results for producer_is_* fields.  CHECKED_PRODUCER is set
     if all the producer_is_* fields are valid.  This information is cached
     because profiling CU expansion showed excessive time spent in
     producer_is_gxx_lt_4_6.  */
  unsigned int checked_producer : 1;
  unsigned int producer_is_gxx_lt_4_6 : 1;
  unsigned int producer_is_gcc_lt_4_3 : 1;
  unsigned int producer_is_icc : 1;

  /* When set, the file that we're processing is known to have
     debugging info for C++ namespaces.  GCC 3.3.x did not produce
     this information, but later versions do.  */

  unsigned int processing_has_namespace_info : 1;
};

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

  /* The corresponding objfile.
     Normally we can get the objfile from dwarf2_per_objfile.
     However we can enter this file with just a "per_cu" handle.  */
  struct objfile *objfile;

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

/* A struct that can be used as a hash key for tables based on DW_AT_stmt_list.
   This includes type_unit_group and quick_file_names.  */

struct stmt_list_hash
{
  /* The DWO unit this table is from or NULL if there is none.  */
  struct dwo_unit *dwo_unit;

  /* Offset in .debug_line or .debug_line.dwo.  */
  sect_offset line_sect_off;
};

/* Each element of dwarf2_per_objfile->type_unit_groups is a pointer to
   an object of this type.  */

struct type_unit_group
{
  /* dwarf2read.c's main "handle" on a TU symtab.
     To simplify things we create an artificial CU that "includes" all the
     type units using this stmt_list so that the rest of the code still has
     a "per_cu" handle on the symtab.
     This PER_CU is recognized by having no section.  */
#define IS_TYPE_UNIT_GROUP(per_cu) ((per_cu)->section == NULL)
  struct dwarf2_per_cu_data per_cu;

  /* The TUs that share this DW_AT_stmt_list entry.
     This is added to while parsing type units to build partial symtabs,
     and is deleted afterwards and not used again.  */
  VEC (sig_type_ptr) *tus;

  /* The compunit symtab.
     Type units in a group needn't all be defined in the same source file,
     so we create an essentially anonymous symtab as the compunit symtab.  */
  struct compunit_symtab *compunit_symtab;

  /* The data used to construct the hash key.  */
  struct stmt_list_hash hash;

  /* The number of symtabs from the line header.
     The value here must match line_header.num_file_names.  */
  unsigned int num_symtabs;

  /* The symbol tables for this TU (obtained from the files listed in
     DW_AT_stmt_list).
     WARNING: The order of entries here must match the order of entries
     in the line header.  After the first TU using this type_unit_group, the
     line header for the subsequent TUs is recreated from this.  This is done
     because we need to use the same symtabs for each TU using the same
     DW_AT_stmt_list value.  Also note that symtabs may be repeated here,
     there's no guarantee the line header doesn't have duplicate entries.  */
  struct symtab **symtabs;
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
  struct dwarf2_section_info str;
  struct dwarf2_section_info str_offsets;
  /* In the case of a virtual DWO file, these two are unused.  */
  struct dwarf2_section_info info;
  VEC (dwarf2_section_info_def) *types;
};

/* CUs/TUs in DWP/DWO files.  */

struct dwo_unit
{
  /* Backlink to the containing struct dwo_file.  */
  struct dwo_file *dwo_file;

  /* The "id" that distinguishes this CU/TU.
     .debug_info calls this "dwo_id", .debug_types calls this "signature".
     Since signatures came first, we stick with it for consistency.  */
  ULONGEST signature;

  /* The section this CU/TU lives in, in the DWO file.  */
  struct dwarf2_section_info *section;

  /* Same as dwarf2_per_cu_data:{sect_off,length} but in the DWO section.  */
  sect_offset sect_off;
  unsigned int length;

  /* For types, offset in the type's DIE of the type defined by this TU.  */
  cu_offset type_offset_in_tu;
};

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
  /* The DW_AT_GNU_dwo_name attribute.
     For virtual DWO files the name is constructed from the section offsets
     of abbrev,line,loc,str_offsets so that we combine virtual DWO files
     from related CU+TUs.  */
  const char *dwo_name;

  /* The DW_AT_comp_dir attribute.  */
  const char *comp_dir;

  /* The bfd, when the file is open.  Otherwise this is NULL.
     This is unused(NULL) for virtual DWO files where we use dwp_file.dbfd.  */
  bfd *dbfd;

  /* The sections that make up this DWO file.
     Remember that for virtual DWO files in DWP V2, these are virtual
     sections (for lack of a better name).  */
  struct dwo_sections sections;

  /* The CU in the file.
     We only support one because having more than one requires hacking the
     dwo_name of each to match, which is highly unlikely to happen.
     Doing this means all TUs can share comp_dir: We also assume that
     DW_AT_comp_dir across all TUs in a DWO file will be identical.  */
  struct dwo_unit *cu;

  /* Table of TUs in the file.
     Each element is a struct dwo_unit.  */
  htab_t tus;
};

/* These sections are what may appear in a DWP file.  */

struct dwp_sections
{
  /* These are used by both DWP version 1 and 2.  */
  struct dwarf2_section_info str;
  struct dwarf2_section_info cu_index;
  struct dwarf2_section_info tu_index;

  /* These are only used by DWP version 2 files.
     In DWP version 1 the .debug_info.dwo, .debug_types.dwo, and other
     sections are referenced by section number, and are not recorded here.
     In DWP version 2 there is at most one copy of all these sections, each
     section being (effectively) comprised of the concatenation of all of the
     individual sections that exist in the version 1 format.
     To keep the code simple we treat each of these concatenated pieces as a
     section itself (a virtual section?).  */
  struct dwarf2_section_info abbrev;
  struct dwarf2_section_info info;
  struct dwarf2_section_info line;
  struct dwarf2_section_info loc;
  struct dwarf2_section_info macinfo;
  struct dwarf2_section_info macro;
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

/* Similar to virtual_v1_dwo_sections, but for DWP version 2.
   In version 2, the sections of the DWO files are concatenated together
   and stored in one section of that name.  Thus each ELF section contains
   several "virtual" sections.  */

struct virtual_v2_dwo_sections
{
  bfd_size_type abbrev_offset;
  bfd_size_type abbrev_size;

  bfd_size_type line_offset;
  bfd_size_type line_size;

  bfd_size_type loc_offset;
  bfd_size_type loc_size;

  bfd_size_type macinfo_offset;
  bfd_size_type macinfo_size;

  bfd_size_type macro_offset;
  bfd_size_type macro_size;

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
  } section_pool;
};

/* Data for one DWP file.  */

struct dwp_file
{
  /* Name of the file.  */
  const char *name;

  /* File format version.  */
  int version;

  /* The bfd.  */
  bfd *dbfd;

  /* Section info for this file.  */
  struct dwp_sections sections;

  /* Table of CUs in the file.  */
  const struct dwp_hash_table *cus;

  /* Table of TUs in the file.  */
  const struct dwp_hash_table *tus;

  /* Tables of loaded CUs/TUs.  Each entry is a struct dwo_unit *.  */
  htab_t loaded_cus;
  htab_t loaded_tus;

  /* Table to map ELF section numbers to their sections.
     This is only needed for the DWP V1 file format.  */
  unsigned int num_sections;
  asection **elf_sections;
};

/* This represents a '.dwz' file.  */

struct dwz_file
{
  /* A dwz file can only contain a few sections.  */
  struct dwarf2_section_info abbrev;
  struct dwarf2_section_info info;
  struct dwarf2_section_info str;
  struct dwarf2_section_info line;
  struct dwarf2_section_info macro;
  struct dwarf2_section_info gdb_index;

  /* The dwz's BFD.  */
  bfd *dwz_bfd;
};

/* Struct used to pass misc. parameters to read_die_and_children, et
   al.  which are used for both .debug_info and .debug_types dies.
   All parameters here are unchanging for the life of the call.  This
   struct exists to abstract away the constant parameters of die reading.  */

struct die_reader_specs
{
  /* The bfd of die_section.  */
  bfd* abfd;

  /* The CU of the DIE we are parsing.  */
  struct dwarf2_cu *cu;

  /* Non-NULL if reading a DWO file (including one packaged into a DWP).  */
  struct dwo_file *dwo_file;

  /* The section the die comes from.
     This is either .debug_info or .debug_types, or the .dwo variants.  */
  struct dwarf2_section_info *die_section;

  /* die_section->buffer.  */
  const gdb_byte *buffer;

  /* The end of the buffer.  */
  const gdb_byte *buffer_end;

  /* The value of the DW_AT_comp_dir attribute.  */
  const char *comp_dir;
};

/* Type of function passed to init_cutu_and_read_dies, et.al.  */
typedef void (die_reader_func_ftype) (const struct die_reader_specs *reader,
				      const gdb_byte *info_ptr,
				      struct die_info *comp_unit_die,
				      int has_children,
				      void *data);

/* A 1-based directory index.  This is a strong typedef to prevent
   accidentally using a directory index as a 0-based index into an
   array/vector.  */
enum class dir_index : unsigned int {};

/* Likewise, a 1-based file name index.  */
enum class file_name_index : unsigned int {};

struct file_entry
{
  file_entry () = default;

  file_entry (const char *name_, dir_index d_index_,
	      unsigned int mod_time_, unsigned int length_)
    : name (name_),
      d_index (d_index_),
      mod_time (mod_time_),
      length (length_)
  {}

  /* Return the include directory at D_INDEX stored in LH.  Returns
     NULL if D_INDEX is out of bounds.  */
  const char *include_dir (const line_header *lh) const;

  /* The file name.  Note this is an observing pointer.  The memory is
     owned by debug_line_buffer.  */
  const char *name {};

  /* The directory index (1-based).  */
  dir_index d_index {};

  unsigned int mod_time {};

  unsigned int length {};

  /* True if referenced by the Line Number Program.  */
  bool included_p {};

  /* The associated symbol table, if any.  */
  struct symtab *symtab {};
};

/* The line number information for a compilation unit (found in the
   .debug_line section) begins with a "statement program header",
   which contains the following information.  */
struct line_header
{
  line_header ()
    : offset_in_dwz {}
  {}

  /* Add an entry to the include directory table.  */
  void add_include_dir (const char *include_dir);

  /* Add an entry to the file name table.  */
  void add_file_name (const char *name, dir_index d_index,
		      unsigned int mod_time, unsigned int length);

  /* Return the include dir at INDEX (1-based).  Returns NULL if INDEX
     is out of bounds.  */
  const char *include_dir_at (dir_index index) const
  {
    /* Convert directory index number (1-based) to vector index
       (0-based).  */
    size_t vec_index = to_underlying (index) - 1;

    if (vec_index >= include_dirs.size ())
      return NULL;
    return include_dirs[vec_index];
  }

  /* Return the file name at INDEX (1-based).  Returns NULL if INDEX
     is out of bounds.  */
  file_entry *file_name_at (file_name_index index)
  {
    /* Convert file name index number (1-based) to vector index
       (0-based).  */
    size_t vec_index = to_underlying (index) - 1;

    if (vec_index >= file_names.size ())
      return NULL;
    return &file_names[vec_index];
  }

  /* Const version of the above.  */
  const file_entry *file_name_at (unsigned int index) const
  {
    if (index >= file_names.size ())
      return NULL;
    return &file_names[index];
  }

  /* Offset of line number information in .debug_line section.  */
  sect_offset sect_off {};

  /* OFFSET is for struct dwz_file associated with dwarf2_per_objfile.  */
  unsigned offset_in_dwz : 1; /* Can't initialize bitfields in-class.  */

  unsigned int total_length {};
  unsigned short version {};
  unsigned int header_length {};
  unsigned char minimum_instruction_length {};
  unsigned char maximum_ops_per_instruction {};
  unsigned char default_is_stmt {};
  int line_base {};
  unsigned char line_range {};
  unsigned char opcode_base {};

  /* standard_opcode_lengths[i] is the number of operands for the
     standard opcode whose value is i.  This means that
     standard_opcode_lengths[0] is unused, and the last meaningful
     element is standard_opcode_lengths[opcode_base - 1].  */
  std::unique_ptr<unsigned char[]> standard_opcode_lengths;

  /* The include_directories table.  Note these are observing
     pointers.  The memory is owned by debug_line_buffer.  */
  std::vector<const char *> include_dirs;

  /* The file_names table.  */
  std::vector<file_entry> file_names;

  /* The start and end of the statement program following this
     header.  These point into dwarf2_per_objfile->line_buffer.  */
  const gdb_byte *statement_program_start {}, *statement_program_end {};
};

typedef std::unique_ptr<line_header> line_header_up;

const char *
file_entry::include_dir (const line_header *lh) const
{
  return lh->include_dir_at (d_index);
}

/* When we construct a partial symbol table entry we only
   need this much information.  */
struct partial_die_info
  {
    /* Offset of this DIE.  */
    sect_offset sect_off;

    /* DWARF-2 tag for this DIE.  */
    ENUM_BITFIELD(dwarf_tag) tag : 16;

    /* Assorted flags describing the data found in this DIE.  */
    unsigned int has_children : 1;
    unsigned int is_external : 1;
    unsigned int is_declaration : 1;
    unsigned int has_type : 1;
    unsigned int has_specification : 1;
    unsigned int has_pc_info : 1;
    unsigned int may_be_inlined : 1;

    /* This DIE has been marked DW_AT_main_subprogram.  */
    unsigned int main_subprogram : 1;

    /* Flag set if the SCOPE field of this structure has been
       computed.  */
    unsigned int scope_set : 1;

    /* Flag set if the DIE has a byte_size attribute.  */
    unsigned int has_byte_size : 1;

    /* Flag set if the DIE has a DW_AT_const_value attribute.  */
    unsigned int has_const_value : 1;

    /* Flag set if any of the DIE's children are template arguments.  */
    unsigned int has_template_arguments : 1;

    /* Flag set if fixup_partial_die has been called on this die.  */
    unsigned int fixup_called : 1;

    /* Flag set if DW_TAG_imported_unit uses DW_FORM_GNU_ref_alt.  */
    unsigned int is_dwz : 1;

    /* Flag set if spec_offset uses DW_FORM_GNU_ref_alt.  */
    unsigned int spec_is_dwz : 1;

    /* The name of this DIE.  Normally the value of DW_AT_name, but
       sometimes a default name for unnamed DIEs.  */
    const char *name;

    /* The linkage name, if present.  */
    const char *linkage_name;

    /* The scope to prepend to our children.  This is generally
       allocated on the comp_unit_obstack, so will disappear
       when this compilation unit leaves the cache.  */
    const char *scope;

    /* Some data associated with the partial DIE.  The tag determines
       which field is live.  */
    union
    {
      /* The location description associated with this DIE, if any.  */
      struct dwarf_block *locdesc;
      /* The offset of an import, for DW_TAG_imported_unit.  */
      sect_offset sect_off;
    } d;

    /* If HAS_PC_INFO, the PC range associated with this DIE.  */
    CORE_ADDR lowpc;
    CORE_ADDR highpc;

    /* Pointer into the info_buffer (or types_buffer) pointing at the target of
       DW_AT_sibling, if any.  */
    /* NOTE: This member isn't strictly necessary, read_partial_die could
       return DW_AT_sibling values to its caller load_partial_dies.  */
    const gdb_byte *sibling;

    /* If HAS_SPECIFICATION, the offset of the DIE referred to by
       DW_AT_specification (or DW_AT_abstract_origin or
       DW_AT_extension).  */
    sect_offset spec_offset;

    /* Pointers to this DIE's parent, first child, and next sibling,
       if any.  */
    struct partial_die_info *die_parent, *die_child, *die_sibling;
  };

/* This data structure holds the information of an abbrev.  */
struct abbrev_info
  {
    unsigned int number;	/* number identifying abbrev */
    enum dwarf_tag tag;		/* dwarf tag */
    unsigned short has_children;		/* boolean */
    unsigned short num_attrs;	/* number of attributes */
    struct attr_abbrev *attrs;	/* an array of attribute descriptions */
    struct abbrev_info *next;	/* next in chain */
  };

struct attr_abbrev
  {
    ENUM_BITFIELD(dwarf_attribute) name : 16;
    ENUM_BITFIELD(dwarf_form) form : 16;

    /* It is valid only if FORM is DW_FORM_implicit_const.  */
    LONGEST implicit_const;
  };

/* Size of abbrev_table.abbrev_hash_table.  */
#define ABBREV_HASH_SIZE 121

/* Top level data structure to contain an abbreviation table.  */

struct abbrev_table
{
  /* Where the abbrev table came from.
     This is used as a sanity check when the table is used.  */
  sect_offset sect_off;

  /* Storage for the abbrev table.  */
  struct obstack abbrev_obstack;

  /* Hash table of abbrevs.
     This is an array of size ABBREV_HASH_SIZE allocated in abbrev_obstack.
     It could be statically allocated, but the previous code didn't so we
     don't either.  */
  struct abbrev_info **abbrevs;
};

/* Attributes have a name and a value.  */
struct attribute
  {
    ENUM_BITFIELD(dwarf_attribute) name : 16;
    ENUM_BITFIELD(dwarf_form) form : 15;

    /* Has DW_STRING already been updated by dwarf2_canonicalize_name?  This
       field should be in u.str (existing only for DW_STRING) but it is kept
       here for better struct attribute alignment.  */
    unsigned int string_is_canonical : 1;

    union
      {
	const char *str;
	struct dwarf_block *blk;
	ULONGEST unsnd;
	LONGEST snd;
	CORE_ADDR addr;
	ULONGEST signature;
      }
    u;
  };

/* This data structure holds a complete die structure.  */
struct die_info
  {
    /* DWARF-2 tag for this DIE.  */
    ENUM_BITFIELD(dwarf_tag) tag : 16;

    /* Number of attributes */
    unsigned char num_attrs;

    /* True if we're presently building the full type name for the
       type derived from this DIE.  */
    unsigned char building_fullname : 1;

    /* True if this die is in process.  PR 16581.  */
    unsigned char in_process : 1;

    /* Abbrev number */
    unsigned int abbrev;

    /* Offset in .debug_info or .debug_types section.  */
    sect_offset sect_off;

    /* The dies in a compilation unit form an n-ary tree.  PARENT
       points to this die's parent; CHILD points to the first child of
       this node; and all the children of a given node are chained
       together via their SIBLING fields.  */
    struct die_info *child;	/* Its first child, if any.  */
    struct die_info *sibling;	/* Its next sibling, if any.  */
    struct die_info *parent;	/* Its parent, if any.  */

    /* An array of attributes, with NUM_ATTRS elements.  There may be
       zero, but it's not common and zero-sized arrays are not
       sufficiently portable C.  */
    struct attribute attrs[1];
  };

/* Get at parts of an attribute structure.  */

#define DW_STRING(attr)    ((attr)->u.str)
#define DW_STRING_IS_CANONICAL(attr) ((attr)->string_is_canonical)
#define DW_UNSND(attr)     ((attr)->u.unsnd)
#define DW_BLOCK(attr)     ((attr)->u.blk)
#define DW_SND(attr)       ((attr)->u.snd)
#define DW_ADDR(attr)	   ((attr)->u.addr)
#define DW_SIGNATURE(attr) ((attr)->u.signature)

/* Blocks are a bunch of untyped bytes.  */
struct dwarf_block
  {
    size_t size;

    /* Valid only if SIZE is not zero.  */
    const gdb_byte *data;
  };

#ifndef ATTR_ALLOC_CHUNK
#define ATTR_ALLOC_CHUNK 4
#endif

/* Allocate fields for structs, unions and enums in this size.  */
#ifndef DW_FIELD_ALLOC_CHUNK
#define DW_FIELD_ALLOC_CHUNK 4
#endif

/* FIXME: We might want to set this from BFD via bfd_arch_bits_per_byte,
   but this would require a corresponding change in unpack_field_as_long
   and friends.  */
static int bits_per_byte = 8;

struct nextfield
{
  struct nextfield *next;
  int accessibility;
  int virtuality;
  struct field field;
};

struct nextfnfield
{
  struct nextfnfield *next;
  struct fn_field fnfield;
};

struct fnfieldlist
{
  const char *name;
  int length;
  struct nextfnfield *head;
};

struct typedef_field_list
{
  struct typedef_field field;
  struct typedef_field_list *next;
};

/* The routines that read and process dies for a C struct or C++ class
   pass lists of data member fields and lists of member function fields
   in an instance of a field_info structure, as defined below.  */
struct field_info
  {
    /* List of data member and baseclasses fields.  */
    struct nextfield *fields, *baseclasses;

    /* Number of fields (including baseclasses).  */
    int nfields;

    /* Number of baseclasses.  */
    int nbaseclasses;

    /* Set if the accesibility of one of the fields is not public.  */
    int non_public_fields;

    /* Member function fields array, entries are allocated in the order they
       are encountered in the object file.  */
    struct nextfnfield *fnfields;

    /* Member function fieldlist array, contains name of possibly overloaded
       member function, number of overloaded member functions and a pointer
       to the head of the member function field chain.  */
    struct fnfieldlist *fnfieldlists;

    /* Number of entries in the fnfieldlists array.  */
    int nfnfields;

    /* typedefs defined inside this class.  TYPEDEF_FIELD_LIST contains head of
       a NULL terminated list of TYPEDEF_FIELD_LIST_COUNT elements.  */
    struct typedef_field_list *typedef_field_list;
    unsigned typedef_field_list_count;
  };

/* One item on the queue of compilation units to read in full symbols
   for.  */
struct dwarf2_queue_item
{
  struct dwarf2_per_cu_data *per_cu;
  enum language pretend_language;
  struct dwarf2_queue_item *next;
};

/* The current queue.  */
static struct dwarf2_queue_item *dwarf2_queue, *dwarf2_queue_tail;

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
  fprintf_filtered (file, _("The upper bound on the age of cached "
			    "DWARF compilation units is %s.\n"),
		    value);
}

/* local function prototypes */

static const char *get_section_name (const struct dwarf2_section_info *);

static const char *get_section_file_name (const struct dwarf2_section_info *);

static void dwarf2_locate_sections (bfd *, asection *, void *);

static void dwarf2_find_base_address (struct die_info *die,
				      struct dwarf2_cu *cu);

static struct partial_symtab *create_partial_symtab
  (struct dwarf2_per_cu_data *per_cu, const char *name);

static void build_type_psymtabs_reader (const struct die_reader_specs *reader,
					const gdb_byte *info_ptr,
					struct die_info *type_unit_die,
					int has_children, void *data);

static void dwarf2_build_psymtabs_hard (struct objfile *);

static void scan_partial_symbols (struct partial_die_info *,
				  CORE_ADDR *, CORE_ADDR *,
				  int, struct dwarf2_cu *);

static void add_partial_symbol (struct partial_die_info *,
				struct dwarf2_cu *);

static void add_partial_namespace (struct partial_die_info *pdi,
				   CORE_ADDR *lowpc, CORE_ADDR *highpc,
				   int set_addrmap, struct dwarf2_cu *cu);

static void add_partial_module (struct partial_die_info *pdi, CORE_ADDR *lowpc,
				CORE_ADDR *highpc, int set_addrmap,
				struct dwarf2_cu *cu);

static void add_partial_enumeration (struct partial_die_info *enum_pdi,
				     struct dwarf2_cu *cu);

static void add_partial_subprogram (struct partial_die_info *pdi,
				    CORE_ADDR *lowpc, CORE_ADDR *highpc,
				    int need_pc, struct dwarf2_cu *cu);

static void dwarf2_read_symtab (struct partial_symtab *,
				struct objfile *);

static void psymtab_to_symtab_1 (struct partial_symtab *);

static struct abbrev_info *abbrev_table_lookup_abbrev
  (const struct abbrev_table *, unsigned int);

static struct abbrev_table *abbrev_table_read_table
  (struct dwarf2_section_info *, sect_offset);

static void abbrev_table_free (struct abbrev_table *);

static void abbrev_table_free_cleanup (void *);

static void dwarf2_read_abbrevs (struct dwarf2_cu *,
				 struct dwarf2_section_info *);

static void dwarf2_free_abbrev_table (void *);

static unsigned int peek_abbrev_code (bfd *, const gdb_byte *);

static struct partial_die_info *load_partial_dies
  (const struct die_reader_specs *, const gdb_byte *, int);

static const gdb_byte *read_partial_die (const struct die_reader_specs *,
					 struct partial_die_info *,
					 struct abbrev_info *,
					 unsigned int,
					 const gdb_byte *);

static struct partial_die_info *find_partial_die (sect_offset, int,
						  struct dwarf2_cu *);

static void fixup_partial_die (struct partial_die_info *,
			       struct dwarf2_cu *);

static const gdb_byte *read_attribute (const struct die_reader_specs *,
				       struct attribute *, struct attr_abbrev *,
				       const gdb_byte *);

static unsigned int read_1_byte (bfd *, const gdb_byte *);

static int read_1_signed_byte (bfd *, const gdb_byte *);

static unsigned int read_2_bytes (bfd *, const gdb_byte *);

static unsigned int read_4_bytes (bfd *, const gdb_byte *);

static ULONGEST read_8_bytes (bfd *, const gdb_byte *);

static CORE_ADDR read_address (bfd *, const gdb_byte *ptr, struct dwarf2_cu *,
			       unsigned int *);

static LONGEST read_initial_length (bfd *, const gdb_byte *, unsigned int *);

static LONGEST read_checked_initial_length_and_offset
  (bfd *, const gdb_byte *, const struct comp_unit_head *,
   unsigned int *, unsigned int *);

static LONGEST read_offset (bfd *, const gdb_byte *,
			    const struct comp_unit_head *,
			    unsigned int *);

static LONGEST read_offset_1 (bfd *, const gdb_byte *, unsigned int);

static sect_offset read_abbrev_offset (struct dwarf2_section_info *,
				       sect_offset);

static const gdb_byte *read_n_bytes (bfd *, const gdb_byte *, unsigned int);

static const char *read_direct_string (bfd *, const gdb_byte *, unsigned int *);

static const char *read_indirect_string (bfd *, const gdb_byte *,
					 const struct comp_unit_head *,
					 unsigned int *);

static const char *read_indirect_line_string (bfd *, const gdb_byte *,
					      const struct comp_unit_head *,
					      unsigned int *);

static const char *read_indirect_string_from_dwz (struct dwz_file *, LONGEST);

static LONGEST read_signed_leb128 (bfd *, const gdb_byte *, unsigned int *);

static CORE_ADDR read_addr_index_from_leb128 (struct dwarf2_cu *,
					      const gdb_byte *,
					      unsigned int *);

static const char *read_str_index (const struct die_reader_specs *reader,
				   ULONGEST str_index);

static void set_cu_language (unsigned int, struct dwarf2_cu *);

static struct attribute *dwarf2_attr (struct die_info *, unsigned int,
				      struct dwarf2_cu *);

static struct attribute *dwarf2_attr_no_follow (struct die_info *,
						unsigned int);

static const char *dwarf2_string_attr (struct die_info *die, unsigned int name,
                                       struct dwarf2_cu *cu);

static int dwarf2_flag_true_p (struct die_info *die, unsigned name,
                               struct dwarf2_cu *cu);

static int die_is_declaration (struct die_info *, struct dwarf2_cu *cu);

static struct die_info *die_specification (struct die_info *die,
					   struct dwarf2_cu **);

static line_header_up dwarf_decode_line_header (sect_offset sect_off,
						struct dwarf2_cu *cu);

static void dwarf_decode_lines (struct line_header *, const char *,
				struct dwarf2_cu *, struct partial_symtab *,
				CORE_ADDR, int decode_mapping);

static void dwarf2_start_subfile (const char *, const char *);

static struct compunit_symtab *dwarf2_start_symtab (struct dwarf2_cu *,
						    const char *, const char *,
						    CORE_ADDR);

static struct symbol *new_symbol (struct die_info *, struct type *,
				  struct dwarf2_cu *);

static struct symbol *new_symbol_full (struct die_info *, struct type *,
				       struct dwarf2_cu *, struct symbol *);

static void dwarf2_const_value (const struct attribute *, struct symbol *,
				struct dwarf2_cu *);

static void dwarf2_const_value_attr (const struct attribute *attr,
				     struct type *type,
				     const char *name,
				     struct obstack *obstack,
				     struct dwarf2_cu *cu, LONGEST *value,
				     const gdb_byte **bytes,
				     struct dwarf2_locexpr_baton **baton);

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

static char *typename_concat (struct obstack *obs, const char *prefix,
			      const char *suffix, int physname,
			      struct dwarf2_cu *cu);

static void read_file_scope (struct die_info *, struct dwarf2_cu *);

static void read_type_unit_scope (struct die_info *, struct dwarf2_cu *);

static void read_func_scope (struct die_info *, struct dwarf2_cu *);

static void read_lexical_block_scope (struct die_info *, struct dwarf2_cu *);

static void read_call_site_scope (struct die_info *die, struct dwarf2_cu *cu);

static int dwarf2_ranges_read (unsigned, CORE_ADDR *, CORE_ADDR *,
			       struct dwarf2_cu *, struct partial_symtab *);

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

static enum pc_bounds_kind dwarf2_get_pc_bounds (struct die_info *,
						 CORE_ADDR *, CORE_ADDR *,
						 struct dwarf2_cu *,
						 struct partial_symtab *);

static void get_scope_pc_bounds (struct die_info *,
				 CORE_ADDR *, CORE_ADDR *,
				 struct dwarf2_cu *);

static void dwarf2_record_block_ranges (struct die_info *, struct block *,
                                        CORE_ADDR, struct dwarf2_cu *);

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

static struct using_direct **using_directives (enum language);

static void read_import_statement (struct die_info *die, struct dwarf2_cu *);

static int read_namespace_alias (struct die_info *die, struct dwarf2_cu *cu);

static struct type *read_module_type (struct die_info *die,
				      struct dwarf2_cu *cu);

static const char *namespace_name (struct die_info *die,
				   int *is_anonymous, struct dwarf2_cu *);

static void process_enumeration_scope (struct die_info *, struct dwarf2_cu *);

static CORE_ADDR decode_locdesc (struct dwarf_block *, struct dwarf2_cu *);

static enum dwarf_array_dim_ordering read_array_order (struct die_info *,
						       struct dwarf2_cu *);

static struct die_info *read_die_and_siblings_1
  (const struct die_reader_specs *, const gdb_byte *, const gdb_byte **,
   struct die_info *);

static struct die_info *read_die_and_siblings (const struct die_reader_specs *,
					       const gdb_byte *info_ptr,
					       const gdb_byte **new_info_ptr,
					       struct die_info *parent);

static const gdb_byte *read_full_die_1 (const struct die_reader_specs *,
					struct die_info **, const gdb_byte *,
					int *, int);

static const gdb_byte *read_full_die (const struct die_reader_specs *,
				      struct die_info **, const gdb_byte *,
				      int *);

static void process_die (struct die_info *, struct dwarf2_cu *);

static const char *dwarf2_canonicalize_name (const char *, struct dwarf2_cu *,
					     struct obstack *);

static const char *dwarf2_name (struct die_info *die, struct dwarf2_cu *);

static const char *dwarf2_full_name (const char *name,
				     struct die_info *die,
				     struct dwarf2_cu *cu);

static const char *dwarf2_physname (const char *name, struct die_info *die,
				    struct dwarf2_cu *cu);

static struct die_info *dwarf2_extension (struct die_info *die,
					  struct dwarf2_cu **);

static const char *dwarf_tag_name (unsigned int);

static const char *dwarf_attr_name (unsigned int);

static const char *dwarf_form_name (unsigned int);

static const char *dwarf_bool_name (unsigned int);

static const char *dwarf_type_encoding_name (unsigned int);

static struct die_info *sibling_die (struct die_info *);

static void dump_die_shallow (struct ui_file *, int indent, struct die_info *);

static void dump_die_for_error (struct die_info *);

static void dump_die_1 (struct ui_file *, int level, int max_level,
			struct die_info *);

/*static*/ void dump_die (struct die_info *, int max_level);

static void store_in_ref_table (struct die_info *,
				struct dwarf2_cu *);

static sect_offset dwarf2_get_ref_die_offset (const struct attribute *);

static LONGEST dwarf2_get_attr_constant_value (const struct attribute *, int);

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

static void load_full_type_unit (struct dwarf2_per_cu_data *per_cu);

static void read_signatured_type (struct signatured_type *);

static int attr_to_dynamic_prop (const struct attribute *attr,
				 struct die_info *die, struct dwarf2_cu *cu,
				 struct dynamic_prop *prop);

/* memory allocation interface */

static struct dwarf_block *dwarf_alloc_block (struct dwarf2_cu *);

static struct die_info *dwarf_alloc_die (struct dwarf2_cu *, int);

static void dwarf_decode_macros (struct dwarf2_cu *, unsigned int, int);

static int attr_form_is_block (const struct attribute *);

static int attr_form_is_section_offset (const struct attribute *);

static int attr_form_is_constant (const struct attribute *);

static int attr_form_is_ref (const struct attribute *);

static void fill_in_loclist_baton (struct dwarf2_cu *cu,
				   struct dwarf2_loclist_baton *baton,
				   const struct attribute *attr);

static void dwarf2_symbol_mark_computed (const struct attribute *attr,
					 struct symbol *sym,
					 struct dwarf2_cu *cu,
					 int is_block);

static const gdb_byte *skip_one_die (const struct die_reader_specs *reader,
				     const gdb_byte *info_ptr,
				     struct abbrev_info *abbrev);

static void free_stack_comp_unit (void *);

static hashval_t partial_die_hash (const void *item);

static int partial_die_eq (const void *item_lhs, const void *item_rhs);

static struct dwarf2_per_cu_data *dwarf2_find_containing_comp_unit
  (sect_offset sect_off, unsigned int offset_in_dwz, struct objfile *objfile);

static void init_one_comp_unit (struct dwarf2_cu *cu,
				struct dwarf2_per_cu_data *per_cu);

static void prepare_one_comp_unit (struct dwarf2_cu *cu,
				   struct die_info *comp_unit_die,
				   enum language pretend_language);

static void free_heap_comp_unit (void *);

static void free_cached_comp_units (void *);

static void age_cached_comp_units (void);

static void free_one_cached_comp_unit (struct dwarf2_per_cu_data *);

static struct type *set_die_type (struct die_info *, struct type *,
				  struct dwarf2_cu *);

static void create_all_comp_units (struct objfile *);

static int create_all_type_units (struct objfile *);

static void load_full_comp_unit (struct dwarf2_per_cu_data *,
				 enum language);

static void process_full_comp_unit (struct dwarf2_per_cu_data *,
				    enum language);

static void process_full_type_unit (struct dwarf2_per_cu_data *,
				    enum language);

static void dwarf2_add_dependence (struct dwarf2_cu *,
				   struct dwarf2_per_cu_data *);

static void dwarf2_mark (struct dwarf2_cu *);

static void dwarf2_clear_marks (struct dwarf2_per_cu_data *);

static struct type *get_die_type_at_offset (sect_offset,
					    struct dwarf2_per_cu_data *);

static struct type *get_die_type (struct die_info *die, struct dwarf2_cu *cu);

static void dwarf2_release_queue (void *dummy);

static void queue_comp_unit (struct dwarf2_per_cu_data *per_cu,
			     enum language pretend_language);

static void process_queue (void);

/* The return type of find_file_and_directory.  Note, the enclosed
   string pointers are only valid while this object is valid.  */

struct file_and_directory
{
  /* The filename.  This is never NULL.  */
  const char *name;

  /* The compilation directory.  NULL if not known.  If we needed to
     compute a new string, this points to COMP_DIR_STORAGE, otherwise,
     points directly to the DW_AT_comp_dir string attribute owned by
     the obstack that owns the DIE.  */
  const char *comp_dir;

  /* If we needed to build a new string for comp_dir, this is what
     owns the storage.  */
  std::string comp_dir_storage;
};

static file_and_directory find_file_and_directory (struct die_info *die,
						   struct dwarf2_cu *cu);

static char *file_full_name (int file, struct line_header *lh,
			     const char *comp_dir);

/* Expected enum dwarf_unit_type for read_comp_unit_head.  */
enum class rcuh_kind { COMPILE, TYPE };

static const gdb_byte *read_and_check_comp_unit_head
  (struct comp_unit_head *header,
   struct dwarf2_section_info *section,
   struct dwarf2_section_info *abbrev_section, const gdb_byte *info_ptr,
   rcuh_kind section_kind);

static void init_cutu_and_read_dies
  (struct dwarf2_per_cu_data *this_cu, struct abbrev_table *abbrev_table,
   int use_existing_cu, int keep,
   die_reader_func_ftype *die_reader_func, void *data);

static void init_cutu_and_read_dies_simple
  (struct dwarf2_per_cu_data *this_cu,
   die_reader_func_ftype *die_reader_func, void *data);

static htab_t allocate_signatured_type_table (struct objfile *objfile);

static htab_t allocate_dwo_unit_table (struct objfile *objfile);

static struct dwo_unit *lookup_dwo_unit_in_dwp
  (struct dwp_file *dwp_file, const char *comp_dir,
   ULONGEST signature, int is_debug_types);

static struct dwp_file *get_dwp_file (void);

static struct dwo_unit *lookup_dwo_comp_unit
  (struct dwarf2_per_cu_data *, const char *, const char *, ULONGEST);

static struct dwo_unit *lookup_dwo_type_unit
  (struct signatured_type *, const char *, const char *);

static void queue_and_load_all_dwo_tus (struct dwarf2_per_cu_data *);

static void free_dwo_file_cleanup (void *);

static void process_cu_includes (void);

static void check_producer (struct dwarf2_cu *cu);

static void free_line_header_voidp (void *arg);

/* Various complaints about symbol reading that don't abort the process.  */

static void
dwarf2_statement_list_fits_in_line_number_section_complaint (void)
{
  complaint (&symfile_complaints,
	     _("statement list doesn't fit in .debug_line section"));
}

static void
dwarf2_debug_line_missing_file_complaint (void)
{
  complaint (&symfile_complaints,
	     _(".debug_line section has line data without a file"));
}

static void
dwarf2_debug_line_missing_end_sequence_complaint (void)
{
  complaint (&symfile_complaints,
	     _(".debug_line section has line "
	       "program sequence without an end"));
}

static void
dwarf2_complex_location_expr_complaint (void)
{
  complaint (&symfile_complaints, _("location expression too complex"));
}

static void
dwarf2_const_value_length_mismatch_complaint (const char *arg1, int arg2,
					      int arg3)
{
  complaint (&symfile_complaints,
	     _("const value length mismatch for '%s', got %d, expected %d"),
	     arg1, arg2, arg3);
}

static void
dwarf2_section_buffer_overflow_complaint (struct dwarf2_section_info *section)
{
  complaint (&symfile_complaints,
	     _("debug info runs off end of %s section"
	       " [in module %s]"),
	     get_section_name (section),
	     get_section_file_name (section));
}

static void
dwarf2_macro_malformed_definition_complaint (const char *arg1)
{
  complaint (&symfile_complaints,
	     _("macro debug info contains a "
	       "malformed macro definition:\n`%s'"),
	     arg1);
}

static void
dwarf2_invalid_attrib_class_complaint (const char *arg1, const char *arg2)
{
  complaint (&symfile_complaints,
	     _("invalid attribute class or form for '%s' in '%s'"),
	     arg1, arg2);
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


#if WORDS_BIGENDIAN

/* Convert VALUE between big- and little-endian.  */
static offset_type
byte_swap (offset_type value)
{
  offset_type result;

  result = (value & 0xff) << 24;
  result |= (value & 0xff00) << 8;
  result |= (value & 0xff0000) >> 8;
  result |= (value & 0xff000000) >> 24;
  return result;
}

#define MAYBE_SWAP(V)  byte_swap (V)

#else
#define MAYBE_SWAP(V) (V)
#endif /* WORDS_BIGENDIAN */

/* Read the given attribute value as an address, taking the attribute's
   form into account.  */

static CORE_ADDR
attr_value_as_address (struct attribute *attr)
{
  CORE_ADDR addr;

  if (attr->form != DW_FORM_addr && attr->form != DW_FORM_GNU_addr_index)
    {
      /* Aside from a few clearly defined exceptions, attributes that
	 contain an address must always be in DW_FORM_addr form.
	 Unfortunately, some compilers happen to be violating this
	 requirement by encoding addresses using other forms, such
	 as DW_FORM_data4 for example.  For those broken compilers,
	 we try to do our best, without any guarantee of success,
	 to interpret the address correctly.  It would also be nice
	 to generate a complaint, but that would require us to maintain
	 a list of legitimate cases where a non-address form is allowed,
	 as well as update callers to pass in at least the CU's DWARF
	 version.  This is more overhead than what we're willing to
	 expand for a pretty rare case.  */
      addr = DW_UNSND (attr);
    }
  else
    addr = DW_ADDR (attr);

  return addr;
}

/* The suffix for an index file.  */
#define INDEX_SUFFIX ".gdb-index"

/* Try to locate the sections we need for DWARF 2 debugging
   information and return true if we have enough to do something.
   NAMES points to the dwarf2 section names, or is NULL if the standard
   ELF names are used.  */

int
dwarf2_has_info (struct objfile *objfile,
                 const struct dwarf2_debug_sections *names)
{
  dwarf2_per_objfile = ((struct dwarf2_per_objfile *)
			objfile_data (objfile, dwarf2_objfile_data_key));
  if (!dwarf2_per_objfile)
    {
      /* Initialize per-objfile state.  */
      struct dwarf2_per_objfile *data
	= XOBNEW (&objfile->objfile_obstack, struct dwarf2_per_objfile);

      memset (data, 0, sizeof (*data));
      set_objfile_data (objfile, dwarf2_objfile_data_key, data);
      dwarf2_per_objfile = data;

      bfd_map_over_sections (objfile->obfd, dwarf2_locate_sections,
                             (void *) names);
      dwarf2_per_objfile->objfile = objfile;
    }
  return (!dwarf2_per_objfile->info.is_virtual
	  && dwarf2_per_objfile->info.s.section != NULL
	  && !dwarf2_per_objfile->abbrev.is_virtual
	  && dwarf2_per_objfile->abbrev.s.section != NULL);
}

/* Return the containing section of virtual section SECTION.  */

static struct dwarf2_section_info *
get_containing_section (const struct dwarf2_section_info *section)
{
  gdb_assert (section->is_virtual);
  return section->s.containing_section;
}

/* Return the bfd owner of SECTION.  */

static struct bfd *
get_section_bfd_owner (const struct dwarf2_section_info *section)
{
  if (section->is_virtual)
    {
      section = get_containing_section (section);
      gdb_assert (!section->is_virtual);
    }
  return section->s.section->owner;
}

/* Return the bfd section of SECTION.
   Returns NULL if the section is not present.  */

static asection *
get_section_bfd_section (const struct dwarf2_section_info *section)
{
  if (section->is_virtual)
    {
      section = get_containing_section (section);
      gdb_assert (!section->is_virtual);
    }
  return section->s.section;
}

/* Return the name of SECTION.  */

static const char *
get_section_name (const struct dwarf2_section_info *section)
{
  asection *sectp = get_section_bfd_section (section);

  gdb_assert (sectp != NULL);
  return bfd_section_name (get_section_bfd_owner (section), sectp);
}

/* Return the name of the file SECTION is in.  */

static const char *
get_section_file_name (const struct dwarf2_section_info *section)
{
  bfd *abfd = get_section_bfd_owner (section);

  return bfd_get_filename (abfd);
}

/* Return the id of SECTION.
   Returns 0 if SECTION doesn't exist.  */

static int
get_section_id (const struct dwarf2_section_info *section)
{
  asection *sectp = get_section_bfd_section (section);

  if (sectp == NULL)
    return 0;
  return sectp->id;
}

/* Return the flags of SECTION.
   SECTION (or containing section if this is a virtual section) must exist.  */

static int
get_section_flags (const struct dwarf2_section_info *section)
{
  asection *sectp = get_section_bfd_section (section);

  gdb_assert (sectp != NULL);
  return bfd_get_section_flags (sectp->owner, sectp);
}

/* When loading sections, we look either for uncompressed section or for
   compressed section names.  */

static int
section_is_p (const char *section_name,
              const struct dwarf2_section_names *names)
{
  if (names->normal != NULL
      && strcmp (section_name, names->normal) == 0)
    return 1;
  if (names->compressed != NULL
      && strcmp (section_name, names->compressed) == 0)
    return 1;
  return 0;
}

/* This function is mapped across the sections and remembers the
   offset and size of each of the debugging sections we are interested
   in.  */

static void
dwarf2_locate_sections (bfd *abfd, asection *sectp, void *vnames)
{
  const struct dwarf2_debug_sections *names;
  flagword aflag = bfd_get_section_flags (abfd, sectp);

  if (vnames == NULL)
    names = &dwarf2_elf_names;
  else
    names = (const struct dwarf2_debug_sections *) vnames;

  if ((aflag & SEC_HAS_CONTENTS) == 0)
    {
    }
  else if (section_is_p (sectp->name, &names->info))
    {
      dwarf2_per_objfile->info.s.section = sectp;
      dwarf2_per_objfile->info.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->abbrev))
    {
      dwarf2_per_objfile->abbrev.s.section = sectp;
      dwarf2_per_objfile->abbrev.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->line))
    {
      dwarf2_per_objfile->line.s.section = sectp;
      dwarf2_per_objfile->line.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->loc))
    {
      dwarf2_per_objfile->loc.s.section = sectp;
      dwarf2_per_objfile->loc.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->loclists))
    {
      dwarf2_per_objfile->loclists.s.section = sectp;
      dwarf2_per_objfile->loclists.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->macinfo))
    {
      dwarf2_per_objfile->macinfo.s.section = sectp;
      dwarf2_per_objfile->macinfo.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->macro))
    {
      dwarf2_per_objfile->macro.s.section = sectp;
      dwarf2_per_objfile->macro.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->str))
    {
      dwarf2_per_objfile->str.s.section = sectp;
      dwarf2_per_objfile->str.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->line_str))
    {
      dwarf2_per_objfile->line_str.s.section = sectp;
      dwarf2_per_objfile->line_str.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->addr))
    {
      dwarf2_per_objfile->addr.s.section = sectp;
      dwarf2_per_objfile->addr.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->frame))
    {
      dwarf2_per_objfile->frame.s.section = sectp;
      dwarf2_per_objfile->frame.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->eh_frame))
    {
      dwarf2_per_objfile->eh_frame.s.section = sectp;
      dwarf2_per_objfile->eh_frame.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->ranges))
    {
      dwarf2_per_objfile->ranges.s.section = sectp;
      dwarf2_per_objfile->ranges.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->rnglists))
    {
      dwarf2_per_objfile->rnglists.s.section = sectp;
      dwarf2_per_objfile->rnglists.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->types))
    {
      struct dwarf2_section_info type_section;

      memset (&type_section, 0, sizeof (type_section));
      type_section.s.section = sectp;
      type_section.size = bfd_get_section_size (sectp);

      VEC_safe_push (dwarf2_section_info_def, dwarf2_per_objfile->types,
		     &type_section);
    }
  else if (section_is_p (sectp->name, &names->gdb_index))
    {
      dwarf2_per_objfile->gdb_index.s.section = sectp;
      dwarf2_per_objfile->gdb_index.size = bfd_get_section_size (sectp);
    }

  if ((bfd_get_section_flags (abfd, sectp) & (SEC_LOAD | SEC_ALLOC))
      && bfd_section_vma (abfd, sectp) == 0)
    dwarf2_per_objfile->has_section_at_zero = 1;
}

/* A helper function that decides whether a section is empty,
   or not present.  */

static int
dwarf2_section_empty_p (const struct dwarf2_section_info *section)
{
  if (section->is_virtual)
    return section->size == 0;
  return section->s.section == NULL || section->size == 0;
}

/* Read the contents of the section INFO.
   OBJFILE is the main object file, but not necessarily the file where
   the section comes from.  E.g., for DWO files the bfd of INFO is the bfd
   of the DWO file.
   If the section is compressed, uncompress it before returning.  */

static void
dwarf2_read_section (struct objfile *objfile, struct dwarf2_section_info *info)
{
  asection *sectp;
  bfd *abfd;
  gdb_byte *buf, *retbuf;

  if (info->readin)
    return;
  info->buffer = NULL;
  info->readin = 1;

  if (dwarf2_section_empty_p (info))
    return;

  sectp = get_section_bfd_section (info);

  /* If this is a virtual section we need to read in the real one first.  */
  if (info->is_virtual)
    {
      struct dwarf2_section_info *containing_section =
	get_containing_section (info);

      gdb_assert (sectp != NULL);
      if ((sectp->flags & SEC_RELOC) != 0)
	{
	  error (_("Dwarf Error: DWP format V2 with relocations is not"
		   " supported in section %s [in module %s]"),
		 get_section_name (info), get_section_file_name (info));
	}
      dwarf2_read_section (objfile, containing_section);
      /* Other code should have already caught virtual sections that don't
	 fit.  */
      gdb_assert (info->virtual_offset + info->size
		  <= containing_section->size);
      /* If the real section is empty or there was a problem reading the
	 section we shouldn't get here.  */
      gdb_assert (containing_section->buffer != NULL);
      info->buffer = containing_section->buffer + info->virtual_offset;
      return;
    }

  /* If the section has relocations, we must read it ourselves.
     Otherwise we attach it to the BFD.  */
  if ((sectp->flags & SEC_RELOC) == 0)
    {
      info->buffer = gdb_bfd_map_section (sectp, &info->size);
      return;
    }

  buf = (gdb_byte *) obstack_alloc (&objfile->objfile_obstack, info->size);
  info->buffer = buf;

  /* When debugging .o files, we may need to apply relocations; see
     http://sourceware.org/ml/gdb-patches/2002-04/msg00136.html .
     We never compress sections in .o files, so we only need to
     try this when the section is not compressed.  */
  retbuf = symfile_relocate_debug_section (objfile, sectp, buf);
  if (retbuf != NULL)
    {
      info->buffer = retbuf;
      return;
    }

  abfd = get_section_bfd_owner (info);
  gdb_assert (abfd != NULL);

  if (bfd_seek (abfd, sectp->filepos, SEEK_SET) != 0
      || bfd_bread (buf, info->size, abfd) != info->size)
    {
      error (_("Dwarf Error: Can't read DWARF data"
	       " in section %s [in module %s]"),
	     bfd_section_name (abfd, sectp), bfd_get_filename (abfd));
    }
}

/* A helper function that returns the size of a section in a safe way.
   If you are positive that the section has been read before using the
   size, then it is safe to refer to the dwarf2_section_info object's
   "size" field directly.  In other cases, you must call this
   function, because for compressed sections the size field is not set
   correctly until the section has been read.  */

static bfd_size_type
dwarf2_section_size (struct objfile *objfile,
		     struct dwarf2_section_info *info)
{
  if (!info->readin)
    dwarf2_read_section (objfile, info);
  return info->size;
}

/* Fill in SECTP, BUFP and SIZEP with section info, given OBJFILE and
   SECTION_NAME.  */

void
dwarf2_get_section_info (struct objfile *objfile,
                         enum dwarf2_section_enum sect,
                         asection **sectp, const gdb_byte **bufp,
                         bfd_size_type *sizep)
{
  struct dwarf2_per_objfile *data
    = (struct dwarf2_per_objfile *) objfile_data (objfile,
						  dwarf2_objfile_data_key);
  struct dwarf2_section_info *info;

  /* We may see an objfile without any DWARF, in which case we just
     return nothing.  */
  if (data == NULL)
    {
      *sectp = NULL;
      *bufp = NULL;
      *sizep = 0;
      return;
    }
  switch (sect)
    {
    case DWARF2_DEBUG_FRAME:
      info = &data->frame;
      break;
    case DWARF2_EH_FRAME:
      info = &data->eh_frame;
      break;
    default:
      gdb_assert_not_reached ("unexpected section");
    }

  dwarf2_read_section (objfile, info);

  *sectp = get_section_bfd_section (info);
  *bufp = info->buffer;
  *sizep = info->size;
}

/* A helper function to find the sections for a .dwz file.  */

static void
locate_dwz_sections (bfd *abfd, asection *sectp, void *arg)
{
  struct dwz_file *dwz_file = (struct dwz_file *) arg;

  /* Note that we only support the standard ELF names, because .dwz
     is ELF-only (at the time of writing).  */
  if (section_is_p (sectp->name, &dwarf2_elf_names.abbrev))
    {
      dwz_file->abbrev.s.section = sectp;
      dwz_file->abbrev.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &dwarf2_elf_names.info))
    {
      dwz_file->info.s.section = sectp;
      dwz_file->info.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &dwarf2_elf_names.str))
    {
      dwz_file->str.s.section = sectp;
      dwz_file->str.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &dwarf2_elf_names.line))
    {
      dwz_file->line.s.section = sectp;
      dwz_file->line.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &dwarf2_elf_names.macro))
    {
      dwz_file->macro.s.section = sectp;
      dwz_file->macro.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &dwarf2_elf_names.gdb_index))
    {
      dwz_file->gdb_index.s.section = sectp;
      dwz_file->gdb_index.size = bfd_get_section_size (sectp);
    }
}

/* Open the separate '.dwz' debug file, if needed.  Return NULL if
   there is no .gnu_debugaltlink section in the file.  Error if there
   is such a section but the file cannot be found.  */

static struct dwz_file *
dwarf2_get_dwz_file (void)
{
  char *data;
  struct cleanup *cleanup;
  const char *filename;
  struct dwz_file *result;
  bfd_size_type buildid_len_arg;
  size_t buildid_len;
  bfd_byte *buildid;

  if (dwarf2_per_objfile->dwz_file != NULL)
    return dwarf2_per_objfile->dwz_file;

  bfd_set_error (bfd_error_no_error);
  data = bfd_get_alt_debug_link_info (dwarf2_per_objfile->objfile->obfd,
				      &buildid_len_arg, &buildid);
  if (data == NULL)
    {
      if (bfd_get_error () == bfd_error_no_error)
	return NULL;
      error (_("could not read '.gnu_debugaltlink' section: %s"),
	     bfd_errmsg (bfd_get_error ()));
    }
  cleanup = make_cleanup (xfree, data);
  make_cleanup (xfree, buildid);

  buildid_len = (size_t) buildid_len_arg;

  filename = (const char *) data;

  std::string abs_storage;
  if (!IS_ABSOLUTE_PATH (filename))
    {
      char *abs = gdb_realpath (objfile_name (dwarf2_per_objfile->objfile));

      make_cleanup (xfree, abs);
      abs_storage = ldirname (abs) + SLASH_STRING + filename;
      filename = abs_storage.c_str ();
    }

  /* First try the file name given in the section.  If that doesn't
     work, try to use the build-id instead.  */
  gdb_bfd_ref_ptr dwz_bfd (gdb_bfd_open (filename, gnutarget, -1));
  if (dwz_bfd != NULL)
    {
      if (!build_id_verify (dwz_bfd.get (), buildid_len, buildid))
	dwz_bfd.release ();
    }

  if (dwz_bfd == NULL)
    dwz_bfd = build_id_to_debug_bfd (buildid_len, buildid);

  if (dwz_bfd == NULL)
    error (_("could not find '.gnu_debugaltlink' file for %s"),
	   objfile_name (dwarf2_per_objfile->objfile));

  result = OBSTACK_ZALLOC (&dwarf2_per_objfile->objfile->objfile_obstack,
			   struct dwz_file);
  result->dwz_bfd = dwz_bfd.release ();

  bfd_map_over_sections (result->dwz_bfd, locate_dwz_sections, result);

  do_cleanups (cleanup);

  gdb_bfd_record_inclusion (dwarf2_per_objfile->objfile->obfd, result->dwz_bfd);
  dwarf2_per_objfile->dwz_file = result;
  return result;
}

/* DWARF quick_symbols_functions support.  */

/* TUs can share .debug_line entries, and there can be a lot more TUs than
   unique line tables, so we maintain a separate table of all .debug_line
   derived entries to support the sharing.
   All the quick functions need is the list of file names.  We discard the
   line_header when we're done and don't need to record it here.  */
struct quick_file_names
{
  /* The data used to construct the hash key.  */
  struct stmt_list_hash hash;

  /* The number of entries in file_names, real_names.  */
  unsigned int num_file_names;

  /* The file names from the line table, after being run through
     file_full_name.  */
  const char **file_names;

  /* The file names from the line table after being run through
     gdb_realpath.  These are computed lazily.  */
  const char **real_names;
};

/* When using the index (and thus not using psymtabs), each CU has an
   object of this type.  This is used to hold information needed by
   the various "quick" methods.  */
struct dwarf2_per_cu_quick_data
{
  /* The file table.  This can be NULL if there was no file table
     or it's currently not read in.
     NOTE: This points into dwarf2_per_objfile->quick_file_names_table.  */
  struct quick_file_names *file_names;

  /* The corresponding symbol table.  This is NULL if symbols for this
     CU have not yet been read.  */
  struct compunit_symtab *compunit_symtab;

  /* A temporary mark bit used when iterating over all CUs in
     expand_symtabs_matching.  */
  unsigned int mark : 1;

  /* True if we've tried to read the file table and found there isn't one.
     There will be no point in trying to read it again next time.  */
  unsigned int no_file_data : 1;
};

/* Utility hash function for a stmt_list_hash.  */

static hashval_t
hash_stmt_list_entry (const struct stmt_list_hash *stmt_list_hash)
{
  hashval_t v = 0;

  if (stmt_list_hash->dwo_unit != NULL)
    v += (uintptr_t) stmt_list_hash->dwo_unit->dwo_file;
  v += to_underlying (stmt_list_hash->line_sect_off);
  return v;
}

/* Utility equality function for a stmt_list_hash.  */

static int
eq_stmt_list_entry (const struct stmt_list_hash *lhs,
		    const struct stmt_list_hash *rhs)
{
  if ((lhs->dwo_unit != NULL) != (rhs->dwo_unit != NULL))
    return 0;
  if (lhs->dwo_unit != NULL
      && lhs->dwo_unit->dwo_file != rhs->dwo_unit->dwo_file)
    return 0;

  return lhs->line_sect_off == rhs->line_sect_off;
}

/* Hash function for a quick_file_names.  */

static hashval_t
hash_file_name_entry (const void *e)
{
  const struct quick_file_names *file_data
    = (const struct quick_file_names *) e;

  return hash_stmt_list_entry (&file_data->hash);
}

/* Equality function for a quick_file_names.  */

static int
eq_file_name_entry (const void *a, const void *b)
{
  const struct quick_file_names *ea = (const struct quick_file_names *) a;
  const struct quick_file_names *eb = (const struct quick_file_names *) b;

  return eq_stmt_list_entry (&ea->hash, &eb->hash);
}

/* Delete function for a quick_file_names.  */

static void
delete_file_name_entry (void *e)
{
  struct quick_file_names *file_data = (struct quick_file_names *) e;
  int i;

  for (i = 0; i < file_data->num_file_names; ++i)
    {
      xfree ((void*) file_data->file_names[i]);
      if (file_data->real_names)
	xfree ((void*) file_data->real_names[i]);
    }

  /* The space for the struct itself lives on objfile_obstack,
     so we don't free it here.  */
}

/* Create a quick_file_names hash table.  */

static htab_t
create_quick_file_names_table (unsigned int nr_initial_entries)
{
  return htab_create_alloc (nr_initial_entries,
			    hash_file_name_entry, eq_file_name_entry,
			    delete_file_name_entry, xcalloc, xfree);
}

/* Read in PER_CU->CU.  This function is unrelated to symtabs, symtab would
   have to be created afterwards.  You should call age_cached_comp_units after
   processing PER_CU->CU.  dw2_setup must have been already called.  */

static void
load_cu (struct dwarf2_per_cu_data *per_cu)
{
  if (per_cu->is_debug_types)
    load_full_type_unit (per_cu);
  else
    load_full_comp_unit (per_cu, language_minimal);

  if (per_cu->cu == NULL)
    return;  /* Dummy CU.  */

  dwarf2_find_base_address (per_cu->cu->dies, per_cu->cu);
}

/* Read in the symbols for PER_CU.  */

static void
dw2_do_instantiate_symtab (struct dwarf2_per_cu_data *per_cu)
{
  struct cleanup *back_to;

  /* Skip type_unit_groups, reading the type units they contain
     is handled elsewhere.  */
  if (IS_TYPE_UNIT_GROUP (per_cu))
    return;

  back_to = make_cleanup (dwarf2_release_queue, NULL);

  if (dwarf2_per_objfile->using_index
      ? per_cu->v.quick->compunit_symtab == NULL
      : (per_cu->v.psymtab == NULL || !per_cu->v.psymtab->readin))
    {
      queue_comp_unit (per_cu, language_minimal);
      load_cu (per_cu);

      /* If we just loaded a CU from a DWO, and we're working with an index
	 that may badly handle TUs, load all the TUs in that DWO as well.
	 http://sourceware.org/bugzilla/show_bug.cgi?id=15021  */
      if (!per_cu->is_debug_types
	  && per_cu->cu != NULL
	  && per_cu->cu->dwo_unit != NULL
	  && dwarf2_per_objfile->index_table != NULL
	  && dwarf2_per_objfile->index_table->version <= 7
	  /* DWP files aren't supported yet.  */
	  && get_dwp_file () == NULL)
	queue_and_load_all_dwo_tus (per_cu);
    }

  process_queue ();

  /* Age the cache, releasing compilation units that have not
     been used recently.  */
  age_cached_comp_units ();

  do_cleanups (back_to);
}

/* Ensure that the symbols for PER_CU have been read in.  OBJFILE is
   the objfile from which this CU came.  Returns the resulting symbol
   table.  */

static struct compunit_symtab *
dw2_instantiate_symtab (struct dwarf2_per_cu_data *per_cu)
{
  gdb_assert (dwarf2_per_objfile->using_index);
  if (!per_cu->v.quick->compunit_symtab)
    {
      struct cleanup *back_to = make_cleanup (free_cached_comp_units, NULL);
      scoped_restore decrementer = increment_reading_symtab ();
      dw2_do_instantiate_symtab (per_cu);
      process_cu_includes ();
      do_cleanups (back_to);
    }

  return per_cu->v.quick->compunit_symtab;
}

/* Return the CU/TU given its index.

   This is intended for loops like:

   for (i = 0; i < (dwarf2_per_objfile->n_comp_units
		    + dwarf2_per_objfile->n_type_units); ++i)
     {
       struct dwarf2_per_cu_data *per_cu = dw2_get_cutu (i);

       ...;
     }
*/

static struct dwarf2_per_cu_data *
dw2_get_cutu (int index)
{
  if (index >= dwarf2_per_objfile->n_comp_units)
    {
      index -= dwarf2_per_objfile->n_comp_units;
      gdb_assert (index < dwarf2_per_objfile->n_type_units);
      return &dwarf2_per_objfile->all_type_units[index]->per_cu;
    }

  return dwarf2_per_objfile->all_comp_units[index];
}

/* Return the CU given its index.
   This differs from dw2_get_cutu in that it's for when you know INDEX
   refers to a CU.  */

static struct dwarf2_per_cu_data *
dw2_get_cu (int index)
{
  gdb_assert (index >= 0 && index < dwarf2_per_objfile->n_comp_units);

  return dwarf2_per_objfile->all_comp_units[index];
}

/* A helper for create_cus_from_index that handles a given list of
   CUs.  */

static void
create_cus_from_index_list (struct objfile *objfile,
			    const gdb_byte *cu_list, offset_type n_elements,
			    struct dwarf2_section_info *section,
			    int is_dwz,
			    int base_offset)
{
  offset_type i;

  for (i = 0; i < n_elements; i += 2)
    {
      gdb_static_assert (sizeof (ULONGEST) >= 8);

      sect_offset sect_off
	= (sect_offset) extract_unsigned_integer (cu_list, 8, BFD_ENDIAN_LITTLE);
      ULONGEST length = extract_unsigned_integer (cu_list + 8, 8, BFD_ENDIAN_LITTLE);
      cu_list += 2 * 8;

      dwarf2_per_cu_data *the_cu
	= OBSTACK_ZALLOC (&objfile->objfile_obstack,
			  struct dwarf2_per_cu_data);
      the_cu->sect_off = sect_off;
      the_cu->length = length;
      the_cu->objfile = objfile;
      the_cu->section = section;
      the_cu->v.quick = OBSTACK_ZALLOC (&objfile->objfile_obstack,
					struct dwarf2_per_cu_quick_data);
      the_cu->is_dwz = is_dwz;
      dwarf2_per_objfile->all_comp_units[base_offset + i / 2] = the_cu;
    }
}

/* Read the CU list from the mapped index, and use it to create all
   the CU objects for this objfile.  */

static void
create_cus_from_index (struct objfile *objfile,
		       const gdb_byte *cu_list, offset_type cu_list_elements,
		       const gdb_byte *dwz_list, offset_type dwz_elements)
{
  struct dwz_file *dwz;

  dwarf2_per_objfile->n_comp_units = (cu_list_elements + dwz_elements) / 2;
  dwarf2_per_objfile->all_comp_units =
    XOBNEWVEC (&objfile->objfile_obstack, struct dwarf2_per_cu_data *,
	       dwarf2_per_objfile->n_comp_units);

  create_cus_from_index_list (objfile, cu_list, cu_list_elements,
			      &dwarf2_per_objfile->info, 0, 0);

  if (dwz_elements == 0)
    return;

  dwz = dwarf2_get_dwz_file ();
  create_cus_from_index_list (objfile, dwz_list, dwz_elements, &dwz->info, 1,
			      cu_list_elements / 2);
}

/* Create the signatured type hash table from the index.  */

static void
create_signatured_type_table_from_index (struct objfile *objfile,
					 struct dwarf2_section_info *section,
					 const gdb_byte *bytes,
					 offset_type elements)
{
  offset_type i;
  htab_t sig_types_hash;

  dwarf2_per_objfile->n_type_units
    = dwarf2_per_objfile->n_allocated_type_units
    = elements / 3;
  dwarf2_per_objfile->all_type_units =
    XNEWVEC (struct signatured_type *, dwarf2_per_objfile->n_type_units);

  sig_types_hash = allocate_signatured_type_table (objfile);

  for (i = 0; i < elements; i += 3)
    {
      struct signatured_type *sig_type;
      ULONGEST signature;
      void **slot;
      cu_offset type_offset_in_tu;

      gdb_static_assert (sizeof (ULONGEST) >= 8);
      sect_offset sect_off
	= (sect_offset) extract_unsigned_integer (bytes, 8, BFD_ENDIAN_LITTLE);
      type_offset_in_tu
	= (cu_offset) extract_unsigned_integer (bytes + 8, 8,
						BFD_ENDIAN_LITTLE);
      signature = extract_unsigned_integer (bytes + 16, 8, BFD_ENDIAN_LITTLE);
      bytes += 3 * 8;

      sig_type = OBSTACK_ZALLOC (&objfile->objfile_obstack,
				 struct signatured_type);
      sig_type->signature = signature;
      sig_type->type_offset_in_tu = type_offset_in_tu;
      sig_type->per_cu.is_debug_types = 1;
      sig_type->per_cu.section = section;
      sig_type->per_cu.sect_off = sect_off;
      sig_type->per_cu.objfile = objfile;
      sig_type->per_cu.v.quick
	= OBSTACK_ZALLOC (&objfile->objfile_obstack,
			  struct dwarf2_per_cu_quick_data);

      slot = htab_find_slot (sig_types_hash, sig_type, INSERT);
      *slot = sig_type;

      dwarf2_per_objfile->all_type_units[i / 3] = sig_type;
    }

  dwarf2_per_objfile->signatured_types = sig_types_hash;
}

/* Read the address map data from the mapped index, and use it to
   populate the objfile's psymtabs_addrmap.  */

static void
create_addrmap_from_index (struct objfile *objfile, struct mapped_index *index)
{
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  const gdb_byte *iter, *end;
  struct obstack temp_obstack;
  struct addrmap *mutable_map;
  struct cleanup *cleanup;
  CORE_ADDR baseaddr;

  obstack_init (&temp_obstack);
  cleanup = make_cleanup_obstack_free (&temp_obstack);
  mutable_map = addrmap_create_mutable (&temp_obstack);

  iter = index->address_table;
  end = iter + index->address_table_size;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  while (iter < end)
    {
      ULONGEST hi, lo, cu_index;
      lo = extract_unsigned_integer (iter, 8, BFD_ENDIAN_LITTLE);
      iter += 8;
      hi = extract_unsigned_integer (iter, 8, BFD_ENDIAN_LITTLE);
      iter += 8;
      cu_index = extract_unsigned_integer (iter, 4, BFD_ENDIAN_LITTLE);
      iter += 4;

      if (lo > hi)
	{
	  complaint (&symfile_complaints,
		     _(".gdb_index address table has invalid range (%s - %s)"),
		     hex_string (lo), hex_string (hi));
	  continue;
	}

      if (cu_index >= dwarf2_per_objfile->n_comp_units)
	{
	  complaint (&symfile_complaints,
		     _(".gdb_index address table has invalid CU number %u"),
		     (unsigned) cu_index);
	  continue;
	}

      lo = gdbarch_adjust_dwarf2_addr (gdbarch, lo + baseaddr);
      hi = gdbarch_adjust_dwarf2_addr (gdbarch, hi + baseaddr);
      addrmap_set_empty (mutable_map, lo, hi - 1, dw2_get_cutu (cu_index));
    }

  objfile->psymtabs_addrmap = addrmap_create_fixed (mutable_map,
						    &objfile->objfile_obstack);
  do_cleanups (cleanup);
}

/* The hash function for strings in the mapped index.  This is the same as
   SYMBOL_HASH_NEXT, but we keep a separate copy to maintain control over the
   implementation.  This is necessary because the hash function is tied to the
   format of the mapped index file.  The hash values do not have to match with
   SYMBOL_HASH_NEXT.
   
   Use INT_MAX for INDEX_VERSION if you generate the current index format.  */

static hashval_t
mapped_index_string_hash (int index_version, const void *p)
{
  const unsigned char *str = (const unsigned char *) p;
  hashval_t r = 0;
  unsigned char c;

  while ((c = *str++) != 0)
    {
      if (index_version >= 5)
	c = tolower (c);
      r = r * 67 + c - 113;
    }

  return r;
}

/* Find a slot in the mapped index INDEX for the object named NAME.
   If NAME is found, set *VEC_OUT to point to the CU vector in the
   constant pool and return 1.  If NAME cannot be found, return 0.  */

static int
find_slot_in_mapped_hash (struct mapped_index *index, const char *name,
			  offset_type **vec_out)
{
  struct cleanup *back_to = make_cleanup (null_cleanup, 0);
  offset_type hash;
  offset_type slot, step;
  int (*cmp) (const char *, const char *);

  if (current_language->la_language == language_cplus
      || current_language->la_language == language_fortran
      || current_language->la_language == language_d)
    {
      /* NAME is already canonical.  Drop any qualifiers as .gdb_index does
	 not contain any.  */

      if (strchr (name, '(') != NULL)
	{
	  char *without_params = cp_remove_params (name);

	  if (without_params != NULL)
	    {
	      make_cleanup (xfree, without_params);
	      name = without_params;
	    }
	}
    }

  /* Index version 4 did not support case insensitive searches.  But the
     indices for case insensitive languages are built in lowercase, therefore
     simulate our NAME being searched is also lowercased.  */
  hash = mapped_index_string_hash ((index->version == 4
                                    && case_sensitivity == case_sensitive_off
				    ? 5 : index->version),
				   name);

  slot = hash & (index->symbol_table_slots - 1);
  step = ((hash * 17) & (index->symbol_table_slots - 1)) | 1;
  cmp = (case_sensitivity == case_sensitive_on ? strcmp : strcasecmp);

  for (;;)
    {
      /* Convert a slot number to an offset into the table.  */
      offset_type i = 2 * slot;
      const char *str;
      if (index->symbol_table[i] == 0 && index->symbol_table[i + 1] == 0)
	{
	  do_cleanups (back_to);
	  return 0;
	}

      str = index->constant_pool + MAYBE_SWAP (index->symbol_table[i]);
      if (!cmp (name, str))
	{
	  *vec_out = (offset_type *) (index->constant_pool
				      + MAYBE_SWAP (index->symbol_table[i + 1]));
	  do_cleanups (back_to);
	  return 1;
	}

      slot = (slot + step) & (index->symbol_table_slots - 1);
    }
}

/* A helper function that reads the .gdb_index from SECTION and fills
   in MAP.  FILENAME is the name of the file containing the section;
   it is used for error reporting.  DEPRECATED_OK is nonzero if it is
   ok to use deprecated sections.

   CU_LIST, CU_LIST_ELEMENTS, TYPES_LIST, and TYPES_LIST_ELEMENTS are
   out parameters that are filled in with information about the CU and
   TU lists in the section.

   Returns 1 if all went well, 0 otherwise.  */

static int
read_index_from_section (struct objfile *objfile,
			 const char *filename,
			 int deprecated_ok,
			 struct dwarf2_section_info *section,
			 struct mapped_index *map,
			 const gdb_byte **cu_list,
			 offset_type *cu_list_elements,
			 const gdb_byte **types_list,
			 offset_type *types_list_elements)
{
  const gdb_byte *addr;
  offset_type version;
  offset_type *metadata;
  int i;

  if (dwarf2_section_empty_p (section))
    return 0;

  /* Older elfutils strip versions could keep the section in the main
     executable while splitting it for the separate debug info file.  */
  if ((get_section_flags (section) & SEC_HAS_CONTENTS) == 0)
    return 0;

  dwarf2_read_section (objfile, section);

  addr = section->buffer;
  /* Version check.  */
  version = MAYBE_SWAP (*(offset_type *) addr);
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
Do \"set use-deprecated-index-sections on\" before the file is read\n\
to use the section anyway."),
		   filename);
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
  if (version > 8)
    return 0;

  map->version = version;
  map->total_size = section->size;

  metadata = (offset_type *) (addr + sizeof (offset_type));

  i = 0;
  *cu_list = addr + MAYBE_SWAP (metadata[i]);
  *cu_list_elements = ((MAYBE_SWAP (metadata[i + 1]) - MAYBE_SWAP (metadata[i]))
		       / 8);
  ++i;

  *types_list = addr + MAYBE_SWAP (metadata[i]);
  *types_list_elements = ((MAYBE_SWAP (metadata[i + 1])
			   - MAYBE_SWAP (metadata[i]))
			  / 8);
  ++i;

  map->address_table = addr + MAYBE_SWAP (metadata[i]);
  map->address_table_size = (MAYBE_SWAP (metadata[i + 1])
			     - MAYBE_SWAP (metadata[i]));
  ++i;

  map->symbol_table = (offset_type *) (addr + MAYBE_SWAP (metadata[i]));
  map->symbol_table_slots = ((MAYBE_SWAP (metadata[i + 1])
			      - MAYBE_SWAP (metadata[i]))
			     / (2 * sizeof (offset_type)));
  ++i;

  map->constant_pool = (char *) (addr + MAYBE_SWAP (metadata[i]));

  return 1;
}


/* Read the index file.  If everything went ok, initialize the "quick"
   elements of all the CUs and return 1.  Otherwise, return 0.  */

static int
dwarf2_read_index (struct objfile *objfile)
{
  struct mapped_index local_map, *map;
  const gdb_byte *cu_list, *types_list, *dwz_list = NULL;
  offset_type cu_list_elements, types_list_elements, dwz_list_elements = 0;
  struct dwz_file *dwz;

  if (!read_index_from_section (objfile, objfile_name (objfile),
				use_deprecated_index_sections,
				&dwarf2_per_objfile->gdb_index, &local_map,
				&cu_list, &cu_list_elements,
				&types_list, &types_list_elements))
    return 0;

  /* Don't use the index if it's empty.  */
  if (local_map.symbol_table_slots == 0)
    return 0;

  /* If there is a .dwz file, read it so we can get its CU list as
     well.  */
  dwz = dwarf2_get_dwz_file ();
  if (dwz != NULL)
    {
      struct mapped_index dwz_map;
      const gdb_byte *dwz_types_ignore;
      offset_type dwz_types_elements_ignore;

      if (!read_index_from_section (objfile, bfd_get_filename (dwz->dwz_bfd),
				    1,
				    &dwz->gdb_index, &dwz_map,
				    &dwz_list, &dwz_list_elements,
				    &dwz_types_ignore,
				    &dwz_types_elements_ignore))
	{
	  warning (_("could not read '.gdb_index' section from %s; skipping"),
		   bfd_get_filename (dwz->dwz_bfd));
	  return 0;
	}
    }

  create_cus_from_index (objfile, cu_list, cu_list_elements, dwz_list,
			 dwz_list_elements);

  if (types_list_elements)
    {
      struct dwarf2_section_info *section;

      /* We can only handle a single .debug_types when we have an
	 index.  */
      if (VEC_length (dwarf2_section_info_def, dwarf2_per_objfile->types) != 1)
	return 0;

      section = VEC_index (dwarf2_section_info_def,
			   dwarf2_per_objfile->types, 0);

      create_signatured_type_table_from_index (objfile, section, types_list,
					       types_list_elements);
    }

  create_addrmap_from_index (objfile, &local_map);

  map = XOBNEW (&objfile->objfile_obstack, struct mapped_index);
  *map = local_map;

  dwarf2_per_objfile->index_table = map;
  dwarf2_per_objfile->using_index = 1;
  dwarf2_per_objfile->quick_file_names_table =
    create_quick_file_names_table (dwarf2_per_objfile->n_comp_units);

  return 1;
}

/* A helper for the "quick" functions which sets the global
   dwarf2_per_objfile according to OBJFILE.  */

static void
dw2_setup (struct objfile *objfile)
{
  dwarf2_per_objfile = ((struct dwarf2_per_objfile *)
		        objfile_data (objfile, dwarf2_objfile_data_key));
  gdb_assert (dwarf2_per_objfile);
}

/* die_reader_func for dw2_get_file_names.  */

static void
dw2_get_file_names_reader (const struct die_reader_specs *reader,
			   const gdb_byte *info_ptr,
			   struct die_info *comp_unit_die,
			   int has_children,
			   void *data)
{
  struct dwarf2_cu *cu = reader->cu;
  struct dwarf2_per_cu_data *this_cu = cu->per_cu;  
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_per_cu_data *lh_cu;
  struct attribute *attr;
  int i;
  void **slot;
  struct quick_file_names *qfn;

  gdb_assert (! this_cu->is_debug_types);

  /* Our callers never want to match partial units -- instead they
     will match the enclosing full CU.  */
  if (comp_unit_die->tag == DW_TAG_partial_unit)
    {
      this_cu->v.quick->no_file_data = 1;
      return;
    }

  lh_cu = this_cu;
  slot = NULL;

  line_header_up lh;
  sect_offset line_offset {};

  attr = dwarf2_attr (comp_unit_die, DW_AT_stmt_list, cu);
  if (attr)
    {
      struct quick_file_names find_entry;

      line_offset = (sect_offset) DW_UNSND (attr);

      /* We may have already read in this line header (TU line header sharing).
	 If we have we're done.  */
      find_entry.hash.dwo_unit = cu->dwo_unit;
      find_entry.hash.line_sect_off = line_offset;
      slot = htab_find_slot (dwarf2_per_objfile->quick_file_names_table,
			     &find_entry, INSERT);
      if (*slot != NULL)
	{
	  lh_cu->v.quick->file_names = (struct quick_file_names *) *slot;
	  return;
	}

      lh = dwarf_decode_line_header (line_offset, cu);
    }
  if (lh == NULL)
    {
      lh_cu->v.quick->no_file_data = 1;
      return;
    }

  qfn = XOBNEW (&objfile->objfile_obstack, struct quick_file_names);
  qfn->hash.dwo_unit = cu->dwo_unit;
  qfn->hash.line_sect_off = line_offset;
  gdb_assert (slot != NULL);
  *slot = qfn;

  file_and_directory fnd = find_file_and_directory (comp_unit_die, cu);

  qfn->num_file_names = lh->file_names.size ();
  qfn->file_names =
    XOBNEWVEC (&objfile->objfile_obstack, const char *, lh->file_names.size ());
  for (i = 0; i < lh->file_names.size (); ++i)
    qfn->file_names[i] = file_full_name (i + 1, lh.get (), fnd.comp_dir);
  qfn->real_names = NULL;

  lh_cu->v.quick->file_names = qfn;
}

/* A helper for the "quick" functions which attempts to read the line
   table for THIS_CU.  */

static struct quick_file_names *
dw2_get_file_names (struct dwarf2_per_cu_data *this_cu)
{
  /* This should never be called for TUs.  */
  gdb_assert (! this_cu->is_debug_types);
  /* Nor type unit groups.  */
  gdb_assert (! IS_TYPE_UNIT_GROUP (this_cu));

  if (this_cu->v.quick->file_names != NULL)
    return this_cu->v.quick->file_names;
  /* If we know there is no line data, no point in looking again.  */
  if (this_cu->v.quick->no_file_data)
    return NULL;

  init_cutu_and_read_dies_simple (this_cu, dw2_get_file_names_reader, NULL);

  if (this_cu->v.quick->no_file_data)
    return NULL;
  return this_cu->v.quick->file_names;
}

/* A helper for the "quick" functions which computes and caches the
   real path for a given file name from the line table.  */

static const char *
dw2_get_real_path (struct objfile *objfile,
		   struct quick_file_names *qfn, int index)
{
  if (qfn->real_names == NULL)
    qfn->real_names = OBSTACK_CALLOC (&objfile->objfile_obstack,
				      qfn->num_file_names, const char *);

  if (qfn->real_names[index] == NULL)
    qfn->real_names[index] = gdb_realpath (qfn->file_names[index]);

  return qfn->real_names[index];
}

static struct symtab *
dw2_find_last_source_symtab (struct objfile *objfile)
{
  struct compunit_symtab *cust;
  int index;

  dw2_setup (objfile);
  index = dwarf2_per_objfile->n_comp_units - 1;
  cust = dw2_instantiate_symtab (dw2_get_cutu (index));
  if (cust == NULL)
    return NULL;
  return compunit_primary_filetab (cust);
}

/* Traversal function for dw2_forget_cached_source_info.  */

static int
dw2_free_cached_file_names (void **slot, void *info)
{
  struct quick_file_names *file_data = (struct quick_file_names *) *slot;

  if (file_data->real_names)
    {
      int i;

      for (i = 0; i < file_data->num_file_names; ++i)
	{
	  xfree ((void*) file_data->real_names[i]);
	  file_data->real_names[i] = NULL;
	}
    }

  return 1;
}

static void
dw2_forget_cached_source_info (struct objfile *objfile)
{
  dw2_setup (objfile);

  htab_traverse_noresize (dwarf2_per_objfile->quick_file_names_table,
			  dw2_free_cached_file_names, NULL);
}

/* Helper function for dw2_map_symtabs_matching_filename that expands
   the symtabs and calls the iterator.  */

static int
dw2_map_expand_apply (struct objfile *objfile,
		      struct dwarf2_per_cu_data *per_cu,
		      const char *name, const char *real_path,
		      gdb::function_view<bool (symtab *)> callback)
{
  struct compunit_symtab *last_made = objfile->compunit_symtabs;

  /* Don't visit already-expanded CUs.  */
  if (per_cu->v.quick->compunit_symtab)
    return 0;

  /* This may expand more than one symtab, and we want to iterate over
     all of them.  */
  dw2_instantiate_symtab (per_cu);

  return iterate_over_some_symtabs (name, real_path, objfile->compunit_symtabs,
				    last_made, callback);
}

/* Implementation of the map_symtabs_matching_filename method.  */

static bool
dw2_map_symtabs_matching_filename
  (struct objfile *objfile, const char *name, const char *real_path,
   gdb::function_view<bool (symtab *)> callback)
{
  int i;
  const char *name_basename = lbasename (name);

  dw2_setup (objfile);

  /* The rule is CUs specify all the files, including those used by
     any TU, so there's no need to scan TUs here.  */

  for (i = 0; i < dwarf2_per_objfile->n_comp_units; ++i)
    {
      int j;
      struct dwarf2_per_cu_data *per_cu = dw2_get_cu (i);
      struct quick_file_names *file_data;

      /* We only need to look at symtabs not already expanded.  */
      if (per_cu->v.quick->compunit_symtab)
	continue;

      file_data = dw2_get_file_names (per_cu);
      if (file_data == NULL)
	continue;

      for (j = 0; j < file_data->num_file_names; ++j)
	{
	  const char *this_name = file_data->file_names[j];
	  const char *this_real_name;

	  if (compare_filenames_for_search (this_name, name))
	    {
	      if (dw2_map_expand_apply (objfile, per_cu, name, real_path,
					callback))
		return true;
	      continue;
	    }

	  /* Before we invoke realpath, which can get expensive when many
	     files are involved, do a quick comparison of the basenames.  */
	  if (! basenames_may_differ
	      && FILENAME_CMP (lbasename (this_name), name_basename) != 0)
	    continue;

	  this_real_name = dw2_get_real_path (objfile, file_data, j);
	  if (compare_filenames_for_search (this_real_name, name))
	    {
	      if (dw2_map_expand_apply (objfile, per_cu, name, real_path,
					callback))
		return true;
	      continue;
	    }

	  if (real_path != NULL)
	    {
	      gdb_assert (IS_ABSOLUTE_PATH (real_path));
	      gdb_assert (IS_ABSOLUTE_PATH (name));
	      if (this_real_name != NULL
		  && FILENAME_CMP (real_path, this_real_name) == 0)
		{
		  if (dw2_map_expand_apply (objfile, per_cu, name, real_path,
					    callback))
		    return true;
		  continue;
		}
	    }
	}
    }

  return false;
}

/* Struct used to manage iterating over all CUs looking for a symbol.  */

struct dw2_symtab_iterator
{
  /* The internalized form of .gdb_index.  */
  struct mapped_index *index;
  /* If non-zero, only look for symbols that match BLOCK_INDEX.  */
  int want_specific_block;
  /* One of GLOBAL_BLOCK or STATIC_BLOCK.
     Unused if !WANT_SPECIFIC_BLOCK.  */
  int block_index;
  /* The kind of symbol we're looking for.  */
  domain_enum domain;
  /* The list of CUs from the index entry of the symbol,
     or NULL if not found.  */
  offset_type *vec;
  /* The next element in VEC to look at.  */
  int next;
  /* The number of elements in VEC, or zero if there is no match.  */
  int length;
  /* Have we seen a global version of the symbol?
     If so we can ignore all further global instances.
     This is to work around gold/15646, inefficient gold-generated
     indices.  */
  int global_seen;
};

/* Initialize the index symtab iterator ITER.
   If WANT_SPECIFIC_BLOCK is non-zero, only look for symbols
   in block BLOCK_INDEX.  Otherwise BLOCK_INDEX is ignored.  */

static void
dw2_symtab_iter_init (struct dw2_symtab_iterator *iter,
		      struct mapped_index *index,
		      int want_specific_block,
		      int block_index,
		      domain_enum domain,
		      const char *name)
{
  iter->index = index;
  iter->want_specific_block = want_specific_block;
  iter->block_index = block_index;
  iter->domain = domain;
  iter->next = 0;
  iter->global_seen = 0;

  if (find_slot_in_mapped_hash (index, name, &iter->vec))
    iter->length = MAYBE_SWAP (*iter->vec);
  else
    {
      iter->vec = NULL;
      iter->length = 0;
    }
}

/* Return the next matching CU or NULL if there are no more.  */

static struct dwarf2_per_cu_data *
dw2_symtab_iter_next (struct dw2_symtab_iterator *iter)
{
  for ( ; iter->next < iter->length; ++iter->next)
    {
      offset_type cu_index_and_attrs =
	MAYBE_SWAP (iter->vec[iter->next + 1]);
      offset_type cu_index = GDB_INDEX_CU_VALUE (cu_index_and_attrs);
      struct dwarf2_per_cu_data *per_cu;
      int want_static = iter->block_index != GLOBAL_BLOCK;
      /* This value is only valid for index versions >= 7.  */
      int is_static = GDB_INDEX_SYMBOL_STATIC_VALUE (cu_index_and_attrs);
      gdb_index_symbol_kind symbol_kind =
	GDB_INDEX_SYMBOL_KIND_VALUE (cu_index_and_attrs);
      /* Only check the symbol attributes if they're present.
	 Indices prior to version 7 don't record them,
	 and indices >= 7 may elide them for certain symbols
	 (gold does this).  */
      int attrs_valid =
	(iter->index->version >= 7
	 && symbol_kind != GDB_INDEX_SYMBOL_KIND_NONE);

      /* Don't crash on bad data.  */
      if (cu_index >= (dwarf2_per_objfile->n_comp_units
		       + dwarf2_per_objfile->n_type_units))
	{
	  complaint (&symfile_complaints,
		     _(".gdb_index entry has bad CU index"
		       " [in module %s]"),
		     objfile_name (dwarf2_per_objfile->objfile));
	  continue;
	}

      per_cu = dw2_get_cutu (cu_index);

      /* Skip if already read in.  */
      if (per_cu->v.quick->compunit_symtab)
	continue;

      /* Check static vs global.  */
      if (attrs_valid)
	{
	  if (iter->want_specific_block
	      && want_static != is_static)
	    continue;
	  /* Work around gold/15646.  */
	  if (!is_static && iter->global_seen)
	    continue;
	  if (!is_static)
	    iter->global_seen = 1;
	}

      /* Only check the symbol's kind if it has one.  */
      if (attrs_valid)
	{
	  switch (iter->domain)
	    {
	    case VAR_DOMAIN:
	      if (symbol_kind != GDB_INDEX_SYMBOL_KIND_VARIABLE
		  && symbol_kind != GDB_INDEX_SYMBOL_KIND_FUNCTION
		  /* Some types are also in VAR_DOMAIN.  */
		  && symbol_kind != GDB_INDEX_SYMBOL_KIND_TYPE)
		continue;
	      break;
	    case STRUCT_DOMAIN:
	      if (symbol_kind != GDB_INDEX_SYMBOL_KIND_TYPE)
		continue;
	      break;
	    case LABEL_DOMAIN:
	      if (symbol_kind != GDB_INDEX_SYMBOL_KIND_OTHER)
		continue;
	      break;
	    default:
	      break;
	    }
	}

      ++iter->next;
      return per_cu;
    }

  return NULL;
}

static struct compunit_symtab *
dw2_lookup_symbol (struct objfile *objfile, int block_index,
		   const char *name, domain_enum domain)
{
  struct compunit_symtab *stab_best = NULL;
  struct mapped_index *index;

  dw2_setup (objfile);

  index = dwarf2_per_objfile->index_table;

  /* index is NULL if OBJF_READNOW.  */
  if (index)
    {
      struct dw2_symtab_iterator iter;
      struct dwarf2_per_cu_data *per_cu;

      dw2_symtab_iter_init (&iter, index, 1, block_index, domain, name);

      while ((per_cu = dw2_symtab_iter_next (&iter)) != NULL)
	{
	  struct symbol *sym, *with_opaque = NULL;
	  struct compunit_symtab *stab = dw2_instantiate_symtab (per_cu);
	  const struct blockvector *bv = COMPUNIT_BLOCKVECTOR (stab);
	  struct block *block = BLOCKVECTOR_BLOCK (bv, block_index);

	  sym = block_find_symbol (block, name, domain,
				   block_find_non_opaque_type_preferred,
				   &with_opaque);

	  /* Some caution must be observed with overloaded functions
	     and methods, since the index will not contain any overload
	     information (but NAME might contain it).  */

	  if (sym != NULL
	      && strcmp_iw (SYMBOL_SEARCH_NAME (sym), name) == 0)
	    return stab;
	  if (with_opaque != NULL
	      && strcmp_iw (SYMBOL_SEARCH_NAME (with_opaque), name) == 0)
	    stab_best = stab;

	  /* Keep looking through other CUs.  */
	}
    }

  return stab_best;
}

static void
dw2_print_stats (struct objfile *objfile)
{
  int i, total, count;

  dw2_setup (objfile);
  total = dwarf2_per_objfile->n_comp_units + dwarf2_per_objfile->n_type_units;
  count = 0;
  for (i = 0; i < total; ++i)
    {
      struct dwarf2_per_cu_data *per_cu = dw2_get_cutu (i);

      if (!per_cu->v.quick->compunit_symtab)
	++count;
    }
  printf_filtered (_("  Number of read CUs: %d\n"), total - count);
  printf_filtered (_("  Number of unread CUs: %d\n"), count);
}

/* This dumps minimal information about the index.
   It is called via "mt print objfiles".
   One use is to verify .gdb_index has been loaded by the
   gdb.dwarf2/gdb-index.exp testcase.  */

static void
dw2_dump (struct objfile *objfile)
{
  dw2_setup (objfile);
  gdb_assert (dwarf2_per_objfile->using_index);
  printf_filtered (".gdb_index:");
  if (dwarf2_per_objfile->index_table != NULL)
    {
      printf_filtered (" version %d\n",
		       dwarf2_per_objfile->index_table->version);
    }
  else
    printf_filtered (" faked for \"readnow\"\n");
  printf_filtered ("\n");
}

static void
dw2_relocate (struct objfile *objfile,
	      const struct section_offsets *new_offsets,
	      const struct section_offsets *delta)
{
  /* There's nothing to relocate here.  */
}

static void
dw2_expand_symtabs_for_function (struct objfile *objfile,
				 const char *func_name)
{
  struct mapped_index *index;

  dw2_setup (objfile);

  index = dwarf2_per_objfile->index_table;

  /* index is NULL if OBJF_READNOW.  */
  if (index)
    {
      struct dw2_symtab_iterator iter;
      struct dwarf2_per_cu_data *per_cu;

      /* Note: It doesn't matter what we pass for block_index here.  */
      dw2_symtab_iter_init (&iter, index, 0, GLOBAL_BLOCK, VAR_DOMAIN,
			    func_name);

      while ((per_cu = dw2_symtab_iter_next (&iter)) != NULL)
	dw2_instantiate_symtab (per_cu);
    }
}

static void
dw2_expand_all_symtabs (struct objfile *objfile)
{
  int i;

  dw2_setup (objfile);

  for (i = 0; i < (dwarf2_per_objfile->n_comp_units
		   + dwarf2_per_objfile->n_type_units); ++i)
    {
      struct dwarf2_per_cu_data *per_cu = dw2_get_cutu (i);

      dw2_instantiate_symtab (per_cu);
    }
}

static void
dw2_expand_symtabs_with_fullname (struct objfile *objfile,
				  const char *fullname)
{
  int i;

  dw2_setup (objfile);

  /* We don't need to consider type units here.
     This is only called for examining code, e.g. expand_line_sal.
     There can be an order of magnitude (or more) more type units
     than comp units, and we avoid them if we can.  */

  for (i = 0; i < dwarf2_per_objfile->n_comp_units; ++i)
    {
      int j;
      struct dwarf2_per_cu_data *per_cu = dw2_get_cutu (i);
      struct quick_file_names *file_data;

      /* We only need to look at symtabs not already expanded.  */
      if (per_cu->v.quick->compunit_symtab)
	continue;

      file_data = dw2_get_file_names (per_cu);
      if (file_data == NULL)
	continue;

      for (j = 0; j < file_data->num_file_names; ++j)
	{
	  const char *this_fullname = file_data->file_names[j];

	  if (filename_cmp (this_fullname, fullname) == 0)
	    {
	      dw2_instantiate_symtab (per_cu);
	      break;
	    }
	}
    }
}

static void
dw2_map_matching_symbols (struct objfile *objfile,
			  const char * name, domain_enum domain,
			  int global,
			  int (*callback) (struct block *,
					   struct symbol *, void *),
			  void *data, symbol_compare_ftype *match,
			  symbol_compare_ftype *ordered_compare)
{
  /* Currently unimplemented; used for Ada.  The function can be called if the
     current language is Ada for a non-Ada objfile using GNU index.  As Ada
     does not look for non-Ada symbols this function should just return.  */
}

static void
dw2_expand_symtabs_matching
  (struct objfile *objfile,
   gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
   enum search_domain kind)
{
  int i;
  offset_type iter;
  struct mapped_index *index;

  dw2_setup (objfile);

  /* index_table is NULL if OBJF_READNOW.  */
  if (!dwarf2_per_objfile->index_table)
    return;
  index = dwarf2_per_objfile->index_table;

  if (file_matcher != NULL)
    {
      htab_up visited_found (htab_create_alloc (10, htab_hash_pointer,
						htab_eq_pointer,
						NULL, xcalloc, xfree));
      htab_up visited_not_found (htab_create_alloc (10, htab_hash_pointer,
						    htab_eq_pointer,
						    NULL, xcalloc, xfree));

      /* The rule is CUs specify all the files, including those used by
	 any TU, so there's no need to scan TUs here.  */

      for (i = 0; i < dwarf2_per_objfile->n_comp_units; ++i)
	{
	  int j;
	  struct dwarf2_per_cu_data *per_cu = dw2_get_cu (i);
	  struct quick_file_names *file_data;
	  void **slot;

	  QUIT;

	  per_cu->v.quick->mark = 0;

	  /* We only need to look at symtabs not already expanded.  */
	  if (per_cu->v.quick->compunit_symtab)
	    continue;

	  file_data = dw2_get_file_names (per_cu);
	  if (file_data == NULL)
	    continue;

	  if (htab_find (visited_not_found.get (), file_data) != NULL)
	    continue;
	  else if (htab_find (visited_found.get (), file_data) != NULL)
	    {
	      per_cu->v.quick->mark = 1;
	      continue;
	    }

	  for (j = 0; j < file_data->num_file_names; ++j)
	    {
	      const char *this_real_name;

	      if (file_matcher (file_data->file_names[j], false))
		{
		  per_cu->v.quick->mark = 1;
		  break;
		}

	      /* Before we invoke realpath, which can get expensive when many
		 files are involved, do a quick comparison of the basenames.  */
	      if (!basenames_may_differ
		  && !file_matcher (lbasename (file_data->file_names[j]),
				    true))
		continue;

	      this_real_name = dw2_get_real_path (objfile, file_data, j);
	      if (file_matcher (this_real_name, false))
		{
		  per_cu->v.quick->mark = 1;
		  break;
		}
	    }

	  slot = htab_find_slot (per_cu->v.quick->mark
				 ? visited_found.get ()
				 : visited_not_found.get (),
				 file_data, INSERT);
	  *slot = file_data;
	}
    }

  for (iter = 0; iter < index->symbol_table_slots; ++iter)
    {
      offset_type idx = 2 * iter;
      const char *name;
      offset_type *vec, vec_len, vec_idx;
      int global_seen = 0;

      QUIT;

      if (index->symbol_table[idx] == 0 && index->symbol_table[idx + 1] == 0)
	continue;

      name = index->constant_pool + MAYBE_SWAP (index->symbol_table[idx]);

      if (!symbol_matcher (name))
	continue;

      /* The name was matched, now expand corresponding CUs that were
	 marked.  */
      vec = (offset_type *) (index->constant_pool
			     + MAYBE_SWAP (index->symbol_table[idx + 1]));
      vec_len = MAYBE_SWAP (vec[0]);
      for (vec_idx = 0; vec_idx < vec_len; ++vec_idx)
	{
	  struct dwarf2_per_cu_data *per_cu;
	  offset_type cu_index_and_attrs = MAYBE_SWAP (vec[vec_idx + 1]);
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
	    (index->version >= 7
	     && symbol_kind != GDB_INDEX_SYMBOL_KIND_NONE);

	  /* Work around gold/15646.  */
	  if (attrs_valid)
	    {
	      if (!is_static && global_seen)
		continue;
	      if (!is_static)
		global_seen = 1;
	    }

	  /* Only check the symbol's kind if it has one.  */
	  if (attrs_valid)
	    {
	      switch (kind)
		{
		case VARIABLES_DOMAIN:
		  if (symbol_kind != GDB_INDEX_SYMBOL_KIND_VARIABLE)
		    continue;
		  break;
		case FUNCTIONS_DOMAIN:
		  if (symbol_kind != GDB_INDEX_SYMBOL_KIND_FUNCTION)
		    continue;
		  break;
		case TYPES_DOMAIN:
		  if (symbol_kind != GDB_INDEX_SYMBOL_KIND_TYPE)
		    continue;
		  break;
		default:
		  break;
		}
	    }

	  /* Don't crash on bad data.  */
	  if (cu_index >= (dwarf2_per_objfile->n_comp_units
			   + dwarf2_per_objfile->n_type_units))
	    {
	      complaint (&symfile_complaints,
			 _(".gdb_index entry has bad CU index"
			   " [in module %s]"), objfile_name (objfile));
	      continue;
	    }

	  per_cu = dw2_get_cutu (cu_index);
	  if (file_matcher == NULL || per_cu->v.quick->mark)
	    {
	      int symtab_was_null =
		(per_cu->v.quick->compunit_symtab == NULL);

	      dw2_instantiate_symtab (per_cu);

	      if (expansion_notify != NULL
		  && symtab_was_null
		  && per_cu->v.quick->compunit_symtab != NULL)
		{
		  expansion_notify (per_cu->v.quick->compunit_symtab);
		}
	    }
	}
    }
}

/* A helper for dw2_find_pc_sect_compunit_symtab which finds the most specific
   symtab.  */

static struct compunit_symtab *
recursively_find_pc_sect_compunit_symtab (struct compunit_symtab *cust,
					  CORE_ADDR pc)
{
  int i;

  if (COMPUNIT_BLOCKVECTOR (cust) != NULL
      && blockvector_contains_pc (COMPUNIT_BLOCKVECTOR (cust), pc))
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

static struct compunit_symtab *
dw2_find_pc_sect_compunit_symtab (struct objfile *objfile,
				  struct bound_minimal_symbol msymbol,
				  CORE_ADDR pc,
				  struct obj_section *section,
				  int warn_if_readin)
{
  struct dwarf2_per_cu_data *data;
  struct compunit_symtab *result;

  dw2_setup (objfile);

  if (!objfile->psymtabs_addrmap)
    return NULL;

  data = (struct dwarf2_per_cu_data *) addrmap_find (objfile->psymtabs_addrmap,
						     pc);
  if (!data)
    return NULL;

  if (warn_if_readin && data->v.quick->compunit_symtab)
    warning (_("(Internal error: pc %s in read in CU, but not in symtab.)"),
	     paddress (get_objfile_arch (objfile), pc));

  result
    = recursively_find_pc_sect_compunit_symtab (dw2_instantiate_symtab (data),
						pc);
  gdb_assert (result != NULL);
  return result;
}

static void
dw2_map_symbol_filenames (struct objfile *objfile, symbol_filename_ftype *fun,
			  void *data, int need_fullname)
{
  int i;
  htab_up visited (htab_create_alloc (10, htab_hash_pointer, htab_eq_pointer,
				      NULL, xcalloc, xfree));

  dw2_setup (objfile);

  /* The rule is CUs specify all the files, including those used by
     any TU, so there's no need to scan TUs here.
     We can ignore file names coming from already-expanded CUs.  */

  for (i = 0; i < dwarf2_per_objfile->n_comp_units; ++i)
    {
      struct dwarf2_per_cu_data *per_cu = dw2_get_cutu (i);

      if (per_cu->v.quick->compunit_symtab)
	{
	  void **slot = htab_find_slot (visited.get (),
					per_cu->v.quick->file_names,
					INSERT);

	  *slot = per_cu->v.quick->file_names;
	}
    }

  for (i = 0; i < dwarf2_per_objfile->n_comp_units; ++i)
    {
      int j;
      struct dwarf2_per_cu_data *per_cu = dw2_get_cu (i);
      struct quick_file_names *file_data;
      void **slot;

      /* We only need to look at symtabs not already expanded.  */
      if (per_cu->v.quick->compunit_symtab)
	continue;

      file_data = dw2_get_file_names (per_cu);
      if (file_data == NULL)
	continue;

      slot = htab_find_slot (visited.get (), file_data, INSERT);
      if (*slot)
	{
	  /* Already visited.  */
	  continue;
	}
      *slot = file_data;

      for (j = 0; j < file_data->num_file_names; ++j)
	{
	  const char *this_real_name;

	  if (need_fullname)
	    this_real_name = dw2_get_real_path (objfile, file_data, j);
	  else
	    this_real_name = NULL;
	  (*fun) (file_data->file_names[j], this_real_name, data);
	}
    }
}

static int
dw2_has_symbols (struct objfile *objfile)
{
  return 1;
}

const struct quick_symbol_functions dwarf2_gdb_index_functions =
{
  dw2_has_symbols,
  dw2_find_last_source_symtab,
  dw2_forget_cached_source_info,
  dw2_map_symtabs_matching_filename,
  dw2_lookup_symbol,
  dw2_print_stats,
  dw2_dump,
  dw2_relocate,
  dw2_expand_symtabs_for_function,
  dw2_expand_all_symtabs,
  dw2_expand_symtabs_with_fullname,
  dw2_map_matching_symbols,
  dw2_expand_symtabs_matching,
  dw2_find_pc_sect_compunit_symtab,
  dw2_map_symbol_filenames
};

/* Initialize for reading DWARF for this objfile.  Return 0 if this
   file will use psymtabs, or 1 if using the GNU index.  */

int
dwarf2_initialize_objfile (struct objfile *objfile)
{
  /* If we're about to read full symbols, don't bother with the
     indices.  In this case we also don't care if some other debug
     format is making psymtabs, because they are all about to be
     expanded anyway.  */
  if ((objfile->flags & OBJF_READNOW))
    {
      int i;

      dwarf2_per_objfile->using_index = 1;
      create_all_comp_units (objfile);
      create_all_type_units (objfile);
      dwarf2_per_objfile->quick_file_names_table =
	create_quick_file_names_table (dwarf2_per_objfile->n_comp_units);

      for (i = 0; i < (dwarf2_per_objfile->n_comp_units
		       + dwarf2_per_objfile->n_type_units); ++i)
	{
	  struct dwarf2_per_cu_data *per_cu = dw2_get_cutu (i);

	  per_cu->v.quick = OBSTACK_ZALLOC (&objfile->objfile_obstack,
					    struct dwarf2_per_cu_quick_data);
	}

      /* Return 1 so that gdb sees the "quick" functions.  However,
	 these functions will be no-ops because we will have expanded
	 all symtabs.  */
      return 1;
    }

  if (dwarf2_read_index (objfile))
    return 1;

  return 0;
}



/* Build a partial symbol table.  */

void
dwarf2_build_psymtabs (struct objfile *objfile)
{

  if (objfile->global_psymbols.size == 0 && objfile->static_psymbols.size == 0)
    {
      init_psymbol_list (objfile, 1024);
    }

  TRY
    {
      /* This isn't really ideal: all the data we allocate on the
	 objfile's obstack is still uselessly kept around.  However,
	 freeing it seems unsafe.  */
      psymtab_discarder psymtabs (objfile);
      dwarf2_build_psymtabs_hard (objfile);
      psymtabs.keep ();
    }
  CATCH (except, RETURN_MASK_ERROR)
    {
      exception_print (gdb_stderr, except);
    }
  END_CATCH
}

/* Return the total length of the CU described by HEADER.  */

static unsigned int
get_cu_length (const struct comp_unit_head *header)
{
  return header->initial_length_size + header->length;
}

/* Return TRUE if SECT_OFF is within CU_HEADER.  */

static inline bool
offset_in_cu_p (const comp_unit_head *cu_header, sect_offset sect_off)
{
  sect_offset bottom = cu_header->sect_off;
  sect_offset top = cu_header->sect_off + get_cu_length (cu_header);

  return sect_off >= bottom && sect_off < top;
}

/* Find the base address of the compilation unit for range lists and
   location lists.  It will normally be specified by DW_AT_low_pc.
   In DWARF-3 draft 4, the base address could be overridden by
   DW_AT_entry_pc.  It's been removed, but GCC still uses this for
   compilation units with discontinuous ranges.  */

static void
dwarf2_find_base_address (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  cu->base_known = 0;
  cu->base_address = 0;

  attr = dwarf2_attr (die, DW_AT_entry_pc, cu);
  if (attr)
    {
      cu->base_address = attr_value_as_address (attr);
      cu->base_known = 1;
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
      if (attr)
	{
	  cu->base_address = attr_value_as_address (attr);
	  cu->base_known = 1;
	}
    }
}

/* Read in the comp unit header information from the debug_info at info_ptr.
   Use rcuh_kind::COMPILE as the default type if not known by the caller.
   NOTE: This leaves members offset, first_die_offset to be filled in
   by the caller.  */

static const gdb_byte *
read_comp_unit_head (struct comp_unit_head *cu_header,
		     const gdb_byte *info_ptr,
		     struct dwarf2_section_info *section,
		     rcuh_kind section_kind)
{
  int signed_addr;
  unsigned int bytes_read;
  const char *filename = get_section_file_name (section);
  bfd *abfd = get_section_bfd_owner (section);

  cu_header->length = read_initial_length (abfd, info_ptr, &bytes_read);
  cu_header->initial_length_size = bytes_read;
  cu_header->offset_size = (bytes_read == 4) ? 4 : 8;
  info_ptr += bytes_read;
  cu_header->version = read_2_bytes (abfd, info_ptr);
  info_ptr += 2;
  if (cu_header->version < 5)
    switch (section_kind)
      {
      case rcuh_kind::COMPILE:
	cu_header->unit_type = DW_UT_compile;
	break;
      case rcuh_kind::TYPE:
	cu_header->unit_type = DW_UT_type;
	break;
      default:
	internal_error (__FILE__, __LINE__,
			_("read_comp_unit_head: invalid section_kind"));
      }
  else
    {
      cu_header->unit_type = static_cast<enum dwarf_unit_type>
						 (read_1_byte (abfd, info_ptr));
      info_ptr += 1;
      switch (cu_header->unit_type)
	{
	case DW_UT_compile:
	  if (section_kind != rcuh_kind::COMPILE)
	    error (_("Dwarf Error: wrong unit_type in compilation unit header "
		   "(is DW_UT_compile, should be DW_UT_type) [in module %s]"),
		   filename);
	  break;
	case DW_UT_type:
	  section_kind = rcuh_kind::TYPE;
	  break;
	default:
	  error (_("Dwarf Error: wrong unit_type in compilation unit header "
		 "(is %d, should be %d or %d) [in module %s]"),
		 cu_header->unit_type, DW_UT_compile, DW_UT_type, filename);
	}

      cu_header->addr_size = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
    }
  cu_header->abbrev_sect_off = (sect_offset) read_offset (abfd, info_ptr,
							  cu_header,
							  &bytes_read);
  info_ptr += bytes_read;
  if (cu_header->version < 5)
    {
      cu_header->addr_size = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
    }
  signed_addr = bfd_get_sign_extend_vma (abfd);
  if (signed_addr < 0)
    internal_error (__FILE__, __LINE__,
		    _("read_comp_unit_head: dwarf from non elf file"));
  cu_header->signed_addr_p = signed_addr;

  if (section_kind == rcuh_kind::TYPE)
    {
      LONGEST type_offset;

      cu_header->signature = read_8_bytes (abfd, info_ptr);
      info_ptr += 8;

      type_offset = read_offset (abfd, info_ptr, cu_header, &bytes_read);
      info_ptr += bytes_read;
      cu_header->type_cu_offset_in_tu = (cu_offset) type_offset;
      if (to_underlying (cu_header->type_cu_offset_in_tu) != type_offset)
	error (_("Dwarf Error: Too big type_offset in compilation unit "
	       "header (is %s) [in module %s]"), plongest (type_offset),
	       filename);
    }

  return info_ptr;
}

/* Helper function that returns the proper abbrev section for
   THIS_CU.  */

static struct dwarf2_section_info *
get_abbrev_section_for_cu (struct dwarf2_per_cu_data *this_cu)
{
  struct dwarf2_section_info *abbrev;

  if (this_cu->is_dwz)
    abbrev = &dwarf2_get_dwz_file ()->abbrev;
  else
    abbrev = &dwarf2_per_objfile->abbrev;

  return abbrev;
}

/* Subroutine of read_and_check_comp_unit_head and
   read_and_check_type_unit_head to simplify them.
   Perform various error checking on the header.  */

static void
error_check_comp_unit_head (struct comp_unit_head *header,
			    struct dwarf2_section_info *section,
			    struct dwarf2_section_info *abbrev_section)
{
  const char *filename = get_section_file_name (section);

  if (header->version < 2 || header->version > 5)
    error (_("Dwarf Error: wrong version in compilation unit header "
	   "(is %d, should be 2, 3, 4 or 5) [in module %s]"), header->version,
	   filename);

  if (to_underlying (header->abbrev_sect_off)
      >= dwarf2_section_size (dwarf2_per_objfile->objfile, abbrev_section))
    error (_("Dwarf Error: bad offset (0x%x) in compilation unit header "
	   "(offset 0x%x + 6) [in module %s]"),
	   to_underlying (header->abbrev_sect_off),
	   to_underlying (header->sect_off),
	   filename);

  /* Cast to ULONGEST to use 64-bit arithmetic when possible to
     avoid potential 32-bit overflow.  */
  if (((ULONGEST) header->sect_off + get_cu_length (header))
      > section->size)
    error (_("Dwarf Error: bad length (0x%x) in compilation unit header "
	   "(offset 0x%x + 0) [in module %s]"),
	   header->length, to_underlying (header->sect_off),
	   filename);
}

/* Read in a CU/TU header and perform some basic error checking.
   The contents of the header are stored in HEADER.
   The result is a pointer to the start of the first DIE.  */

static const gdb_byte *
read_and_check_comp_unit_head (struct comp_unit_head *header,
			       struct dwarf2_section_info *section,
			       struct dwarf2_section_info *abbrev_section,
			       const gdb_byte *info_ptr,
			       rcuh_kind section_kind)
{
  const gdb_byte *beg_of_comp_unit = info_ptr;
  bfd *abfd = get_section_bfd_owner (section);

  header->sect_off = (sect_offset) (beg_of_comp_unit - section->buffer);

  info_ptr = read_comp_unit_head (header, info_ptr, section, section_kind);

  header->first_die_cu_offset = (cu_offset) (info_ptr - beg_of_comp_unit);

  error_check_comp_unit_head (header, section, abbrev_section);

  return info_ptr;
}

/* Fetch the abbreviation table offset from a comp or type unit header.  */

static sect_offset
read_abbrev_offset (struct dwarf2_section_info *section,
		    sect_offset sect_off)
{
  bfd *abfd = get_section_bfd_owner (section);
  const gdb_byte *info_ptr;
  unsigned int initial_length_size, offset_size;
  uint16_t version;

  dwarf2_read_section (dwarf2_per_objfile->objfile, section);
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

  return (sect_offset) read_offset_1 (abfd, info_ptr, offset_size);
}

/* Allocate a new partial symtab for file named NAME and mark this new
   partial symtab as being an include of PST.  */

static void
dwarf2_create_include_psymtab (const char *name, struct partial_symtab *pst,
                               struct objfile *objfile)
{
  struct partial_symtab *subpst = allocate_psymtab (name, objfile);

  if (!IS_ABSOLUTE_PATH (subpst->filename))
    {
      /* It shares objfile->objfile_obstack.  */
      subpst->dirname = pst->dirname;
    }

  subpst->textlow = 0;
  subpst->texthigh = 0;

  subpst->dependencies
    = XOBNEW (&objfile->objfile_obstack, struct partial_symtab *);
  subpst->dependencies[0] = pst;
  subpst->number_of_dependencies = 1;

  subpst->globals_offset = 0;
  subpst->n_global_syms = 0;
  subpst->statics_offset = 0;
  subpst->n_static_syms = 0;
  subpst->compunit_symtab = NULL;
  subpst->read_symtab = pst->read_symtab;
  subpst->readin = 0;

  /* No private part is necessary for include psymtabs.  This property
     can be used to differentiate between such include psymtabs and
     the regular ones.  */
  subpst->read_symtab_private = NULL;
}

/* Read the Line Number Program data and extract the list of files
   included by the source file represented by PST.  Build an include
   partial symtab for each of these included files.  */

static void
dwarf2_build_include_psymtabs (struct dwarf2_cu *cu,
			       struct die_info *die,
			       struct partial_symtab *pst)
{
  line_header_up lh;
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_stmt_list, cu);
  if (attr)
    lh = dwarf_decode_line_header ((sect_offset) DW_UNSND (attr), cu);
  if (lh == NULL)
    return;  /* No linetable, so no includes.  */

  /* NOTE: pst->dirname is DW_AT_comp_dir (if present).  */
  dwarf_decode_lines (lh.get (), pst->dirname, cu, pst, pst->textlow, 1);
}

static hashval_t
hash_signatured_type (const void *item)
{
  const struct signatured_type *sig_type
    = (const struct signatured_type *) item;

  /* This drops the top 32 bits of the signature, but is ok for a hash.  */
  return sig_type->signature;
}

static int
eq_signatured_type (const void *item_lhs, const void *item_rhs)
{
  const struct signatured_type *lhs = (const struct signatured_type *) item_lhs;
  const struct signatured_type *rhs = (const struct signatured_type *) item_rhs;

  return lhs->signature == rhs->signature;
}

/* Allocate a hash table for signatured types.  */

static htab_t
allocate_signatured_type_table (struct objfile *objfile)
{
  return htab_create_alloc_ex (41,
			       hash_signatured_type,
			       eq_signatured_type,
			       NULL,
			       &objfile->objfile_obstack,
			       hashtab_obstack_allocate,
			       dummy_obstack_deallocate);
}

/* A helper function to add a signatured type CU to a table.  */

static int
add_signatured_type_cu_to_table (void **slot, void *datum)
{
  struct signatured_type *sigt = (struct signatured_type *) *slot;
  struct signatured_type ***datap = (struct signatured_type ***) datum;

  **datap = sigt;
  ++*datap;

  return 1;
}

/* A helper for create_debug_types_hash_table.  Read types from SECTION
   and fill them into TYPES_HTAB.  It will process only type units,
   therefore DW_UT_type.  */

static void
create_debug_type_hash_table (struct dwo_file *dwo_file,
			      dwarf2_section_info *section, htab_t &types_htab,
			      rcuh_kind section_kind)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_section_info *abbrev_section;
  bfd *abfd;
  const gdb_byte *info_ptr, *end_ptr;

  abbrev_section = (dwo_file != NULL
		    ? &dwo_file->sections.abbrev
		    : &dwarf2_per_objfile->abbrev);

  if (dwarf_read_debug)
    fprintf_unfiltered (gdb_stdlog, "Reading %s for %s:\n",
			get_section_name (section),
			get_section_file_name (abbrev_section));

  dwarf2_read_section (objfile, section);
  info_ptr = section->buffer;

  if (info_ptr == NULL)
    return;

  /* We can't set abfd until now because the section may be empty or
     not present, in which case the bfd is unknown.  */
  abfd = get_section_bfd_owner (section);

  /* We don't use init_cutu_and_read_dies_simple, or some such, here
     because we don't need to read any dies: the signature is in the
     header.  */

  end_ptr = info_ptr + section->size;
  while (info_ptr < end_ptr)
    {
      struct signatured_type *sig_type;
      struct dwo_unit *dwo_tu;
      void **slot;
      const gdb_byte *ptr = info_ptr;
      struct comp_unit_head header;
      unsigned int length;

      sect_offset sect_off = (sect_offset) (ptr - section->buffer);

      /* Initialize it due to a false compiler warning.  */
      header.signature = -1;
      header.type_cu_offset_in_tu = (cu_offset) -1;

      /* We need to read the type's signature in order to build the hash
	 table, but we don't need anything else just yet.  */

      ptr = read_and_check_comp_unit_head (&header, section,
					   abbrev_section, ptr, section_kind);

      length = get_cu_length (&header);

      /* Skip dummy type units.  */
      if (ptr >= info_ptr + length
	  || peek_abbrev_code (abfd, ptr) == 0
	  || header.unit_type != DW_UT_type)
	{
	  info_ptr += length;
	  continue;
	}

      if (types_htab == NULL)
	{
	  if (dwo_file)
	    types_htab = allocate_dwo_unit_table (objfile);
	  else
	    types_htab = allocate_signatured_type_table (objfile);
	}

      if (dwo_file)
	{
	  sig_type = NULL;
	  dwo_tu = OBSTACK_ZALLOC (&objfile->objfile_obstack,
				   struct dwo_unit);
	  dwo_tu->dwo_file = dwo_file;
	  dwo_tu->signature = header.signature;
	  dwo_tu->type_offset_in_tu = header.type_cu_offset_in_tu;
	  dwo_tu->section = section;
	  dwo_tu->sect_off = sect_off;
	  dwo_tu->length = length;
	}
      else
	{
	  /* N.B.: type_offset is not usable if this type uses a DWO file.
	     The real type_offset is in the DWO file.  */
	  dwo_tu = NULL;
	  sig_type = OBSTACK_ZALLOC (&objfile->objfile_obstack,
				     struct signatured_type);
	  sig_type->signature = header.signature;
	  sig_type->type_offset_in_tu = header.type_cu_offset_in_tu;
	  sig_type->per_cu.objfile = objfile;
	  sig_type->per_cu.is_debug_types = 1;
	  sig_type->per_cu.section = section;
	  sig_type->per_cu.sect_off = sect_off;
	  sig_type->per_cu.length = length;
	}

      slot = htab_find_slot (types_htab,
			     dwo_file ? (void*) dwo_tu : (void *) sig_type,
			     INSERT);
      gdb_assert (slot != NULL);
      if (*slot != NULL)
	{
	  sect_offset dup_sect_off;

	  if (dwo_file)
	    {
	      const struct dwo_unit *dup_tu
		= (const struct dwo_unit *) *slot;

	      dup_sect_off = dup_tu->sect_off;
	    }
	  else
	    {
	      const struct signatured_type *dup_tu
		= (const struct signatured_type *) *slot;

	      dup_sect_off = dup_tu->per_cu.sect_off;
	    }

	  complaint (&symfile_complaints,
		     _("debug type entry at offset 0x%x is duplicate to"
		       " the entry at offset 0x%x, signature %s"),
		     to_underlying (sect_off), to_underlying (dup_sect_off),
		     hex_string (header.signature));
	}
      *slot = dwo_file ? (void *) dwo_tu : (void *) sig_type;

      if (dwarf_read_debug > 1)
	fprintf_unfiltered (gdb_stdlog, "  offset 0x%x, signature %s\n",
			    to_underlying (sect_off),
			    hex_string (header.signature));

      info_ptr += length;
    }
}

/* Create the hash table of all entries in the .debug_types
   (or .debug_types.dwo) section(s).
   If reading a DWO file, then DWO_FILE is a pointer to the DWO file object,
   otherwise it is NULL.

   The result is a pointer to the hash table or NULL if there are no types.

   Note: This function processes DWO files only, not DWP files.  */

static void
create_debug_types_hash_table (struct dwo_file *dwo_file,
			       VEC (dwarf2_section_info_def) *types,
			       htab_t &types_htab)
{
  int ix;
  struct dwarf2_section_info *section;

  if (VEC_empty (dwarf2_section_info_def, types))
    return;

  for (ix = 0;
       VEC_iterate (dwarf2_section_info_def, types, ix, section);
       ++ix)
    create_debug_type_hash_table (dwo_file, section, types_htab,
				  rcuh_kind::TYPE);
}

/* Create the hash table of all entries in the .debug_types section,
   and initialize all_type_units.
   The result is zero if there is an error (e.g. missing .debug_types section),
   otherwise non-zero.	*/

static int
create_all_type_units (struct objfile *objfile)
{
  htab_t types_htab = NULL;
  struct signatured_type **iter;

  create_debug_type_hash_table (NULL, &dwarf2_per_objfile->info, types_htab,
				rcuh_kind::COMPILE);
  create_debug_types_hash_table (NULL, dwarf2_per_objfile->types, types_htab);
  if (types_htab == NULL)
    {
      dwarf2_per_objfile->signatured_types = NULL;
      return 0;
    }

  dwarf2_per_objfile->signatured_types = types_htab;

  dwarf2_per_objfile->n_type_units
    = dwarf2_per_objfile->n_allocated_type_units
    = htab_elements (types_htab);
  dwarf2_per_objfile->all_type_units =
    XNEWVEC (struct signatured_type *, dwarf2_per_objfile->n_type_units);
  iter = &dwarf2_per_objfile->all_type_units[0];
  htab_traverse_noresize (types_htab, add_signatured_type_cu_to_table, &iter);
  gdb_assert (iter - &dwarf2_per_objfile->all_type_units[0]
	      == dwarf2_per_objfile->n_type_units);

  return 1;
}

/* Add an entry for signature SIG to dwarf2_per_objfile->signatured_types.
   If SLOT is non-NULL, it is the entry to use in the hash table.
   Otherwise we find one.  */

static struct signatured_type *
add_type_unit (ULONGEST sig, void **slot)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  int n_type_units = dwarf2_per_objfile->n_type_units;
  struct signatured_type *sig_type;

  gdb_assert (n_type_units <= dwarf2_per_objfile->n_allocated_type_units);
  ++n_type_units;
  if (n_type_units > dwarf2_per_objfile->n_allocated_type_units)
    {
      if (dwarf2_per_objfile->n_allocated_type_units == 0)
	dwarf2_per_objfile->n_allocated_type_units = 1;
      dwarf2_per_objfile->n_allocated_type_units *= 2;
      dwarf2_per_objfile->all_type_units
	= XRESIZEVEC (struct signatured_type *,
		      dwarf2_per_objfile->all_type_units,
		      dwarf2_per_objfile->n_allocated_type_units);
      ++dwarf2_per_objfile->tu_stats.nr_all_type_units_reallocs;
    }
  dwarf2_per_objfile->n_type_units = n_type_units;

  sig_type = OBSTACK_ZALLOC (&objfile->objfile_obstack,
			     struct signatured_type);
  dwarf2_per_objfile->all_type_units[n_type_units - 1] = sig_type;
  sig_type->signature = sig;
  sig_type->per_cu.is_debug_types = 1;
  if (dwarf2_per_objfile->using_index)
    {
      sig_type->per_cu.v.quick =
	OBSTACK_ZALLOC (&objfile->objfile_obstack,
			struct dwarf2_per_cu_quick_data);
    }

  if (slot == NULL)
    {
      slot = htab_find_slot (dwarf2_per_objfile->signatured_types,
			     sig_type, INSERT);
    }
  gdb_assert (*slot == NULL);
  *slot = sig_type;
  /* The rest of sig_type must be filled in by the caller.  */
  return sig_type;
}

/* Subroutine of lookup_dwo_signatured_type and lookup_dwp_signatured_type.
   Fill in SIG_ENTRY with DWO_ENTRY.  */

static void
fill_in_sig_entry_from_dwo_entry (struct objfile *objfile,
				  struct signatured_type *sig_entry,
				  struct dwo_unit *dwo_entry)
{
  /* Make sure we're not clobbering something we don't expect to.  */
  gdb_assert (! sig_entry->per_cu.queued);
  gdb_assert (sig_entry->per_cu.cu == NULL);
  if (dwarf2_per_objfile->using_index)
    {
      gdb_assert (sig_entry->per_cu.v.quick != NULL);
      gdb_assert (sig_entry->per_cu.v.quick->compunit_symtab == NULL);
    }
  else
      gdb_assert (sig_entry->per_cu.v.psymtab == NULL);
  gdb_assert (sig_entry->signature == dwo_entry->signature);
  gdb_assert (to_underlying (sig_entry->type_offset_in_section) == 0);
  gdb_assert (sig_entry->type_unit_group == NULL);
  gdb_assert (sig_entry->dwo_unit == NULL);

  sig_entry->per_cu.section = dwo_entry->section;
  sig_entry->per_cu.sect_off = dwo_entry->sect_off;
  sig_entry->per_cu.length = dwo_entry->length;
  sig_entry->per_cu.reading_dwo_directly = 1;
  sig_entry->per_cu.objfile = objfile;
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
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwo_file *dwo_file;
  struct dwo_unit find_dwo_entry, *dwo_entry;
  struct signatured_type find_sig_entry, *sig_entry;
  void **slot;

  gdb_assert (cu->dwo_unit && dwarf2_per_objfile->using_index);

  /* If TU skeletons have been removed then we may not have read in any
     TUs yet.  */
  if (dwarf2_per_objfile->signatured_types == NULL)
    {
      dwarf2_per_objfile->signatured_types
	= allocate_signatured_type_table (objfile);
    }

  /* We only ever need to read in one copy of a signatured type.
     Use the global signatured_types array to do our own comdat-folding
     of types.  If this is the first time we're reading this TU, and
     the TU has an entry in .gdb_index, replace the recorded data from
     .gdb_index with this TU.  */

  find_sig_entry.signature = sig;
  slot = htab_find_slot (dwarf2_per_objfile->signatured_types,
			 &find_sig_entry, INSERT);
  sig_entry = (struct signatured_type *) *slot;

  /* We can get here with the TU already read, *or* in the process of being
     read.  Don't reassign the global entry to point to this DWO if that's
     the case.  Also note that if the TU is already being read, it may not
     have come from a DWO, the program may be a mix of Fission-compiled
     code and non-Fission-compiled code.  */

  /* Have we already tried to read this TU?
     Note: sig_entry can be NULL if the skeleton TU was removed (thus it
     needn't exist in the global table yet).  */
  if (sig_entry != NULL && sig_entry->per_cu.tu_read)
    return sig_entry;

  /* Note: cu->dwo_unit is the dwo_unit that references this TU, not the
     dwo_unit of the TU itself.  */
  dwo_file = cu->dwo_unit->dwo_file;

  /* Ok, this is the first time we're reading this TU.  */
  if (dwo_file->tus == NULL)
    return NULL;
  find_dwo_entry.signature = sig;
  dwo_entry = (struct dwo_unit *) htab_find (dwo_file->tus, &find_dwo_entry);
  if (dwo_entry == NULL)
    return NULL;

  /* If the global table doesn't have an entry for this TU, add one.  */
  if (sig_entry == NULL)
    sig_entry = add_type_unit (sig, slot);

  fill_in_sig_entry_from_dwo_entry (objfile, sig_entry, dwo_entry);
  sig_entry->per_cu.tu_read = 1;
  return sig_entry;
}

/* Subroutine of lookup_signatured_type.
   Look up the type for signature SIG, and if we can't find SIG in .gdb_index
   then try the DWP file.  If the TU stub (skeleton) has been removed then
   it won't be in .gdb_index.  */

static struct signatured_type *
lookup_dwp_signatured_type (struct dwarf2_cu *cu, ULONGEST sig)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwp_file *dwp_file = get_dwp_file ();
  struct dwo_unit *dwo_entry;
  struct signatured_type find_sig_entry, *sig_entry;
  void **slot;

  gdb_assert (cu->dwo_unit && dwarf2_per_objfile->using_index);
  gdb_assert (dwp_file != NULL);

  /* If TU skeletons have been removed then we may not have read in any
     TUs yet.  */
  if (dwarf2_per_objfile->signatured_types == NULL)
    {
      dwarf2_per_objfile->signatured_types
	= allocate_signatured_type_table (objfile);
    }

  find_sig_entry.signature = sig;
  slot = htab_find_slot (dwarf2_per_objfile->signatured_types,
			 &find_sig_entry, INSERT);
  sig_entry = (struct signatured_type *) *slot;

  /* Have we already tried to read this TU?
     Note: sig_entry can be NULL if the skeleton TU was removed (thus it
     needn't exist in the global table yet).  */
  if (sig_entry != NULL)
    return sig_entry;

  if (dwp_file->tus == NULL)
    return NULL;
  dwo_entry = lookup_dwo_unit_in_dwp (dwp_file, NULL,
				      sig, 1 /* is_debug_types */);
  if (dwo_entry == NULL)
    return NULL;

  sig_entry = add_type_unit (sig, slot);
  fill_in_sig_entry_from_dwo_entry (objfile, sig_entry, dwo_entry);

  return sig_entry;
}

/* Lookup a signature based type for DW_FORM_ref_sig8.
   Returns NULL if signature SIG is not present in the table.
   It is up to the caller to complain about this.  */

static struct signatured_type *
lookup_signatured_type (struct dwarf2_cu *cu, ULONGEST sig)
{
  if (cu->dwo_unit
      && dwarf2_per_objfile->using_index)
    {
      /* We're in a DWO/DWP file, and we're using .gdb_index.
	 These cases require special processing.  */
      if (get_dwp_file () == NULL)
	return lookup_dwo_signatured_type (cu, sig);
      else
	return lookup_dwp_signatured_type (cu, sig);
    }
  else
    {
      struct signatured_type find_entry, *entry;

      if (dwarf2_per_objfile->signatured_types == NULL)
	return NULL;
      find_entry.signature = sig;
      entry = ((struct signatured_type *)
	       htab_find (dwarf2_per_objfile->signatured_types, &find_entry));
      return entry;
    }
}

/* Low level DIE reading support.  */

/* Initialize a die_reader_specs struct from a dwarf2_cu struct.  */

static void
init_cu_die_reader (struct die_reader_specs *reader,
		    struct dwarf2_cu *cu,
		    struct dwarf2_section_info *section,
		    struct dwo_file *dwo_file)
{
  gdb_assert (section->readin && section->buffer != NULL);
  reader->abfd = get_section_bfd_owner (section);
  reader->cu = cu;
  reader->dwo_file = dwo_file;
  reader->die_section = section;
  reader->buffer = section->buffer;
  reader->buffer_end = section->buffer + section->size;
  reader->comp_dir = NULL;
}

/* Subroutine of init_cutu_and_read_dies to simplify it.
   Read in the rest of a CU/TU top level DIE from DWO_UNIT.
   There's just a lot of work to do, and init_cutu_and_read_dies is big enough
   already.

   STUB_COMP_UNIT_DIE is for the stub DIE, we copy over certain attributes
   from it to the DIE in the DWO.  If NULL we are skipping the stub.
   STUB_COMP_DIR is similar to STUB_COMP_UNIT_DIE: When reading a TU directly
   from the DWO file, bypassing the stub, it contains the DW_AT_comp_dir
   attribute of the referencing CU.  At most one of STUB_COMP_UNIT_DIE and
   STUB_COMP_DIR may be non-NULL.
   *RESULT_READER,*RESULT_INFO_PTR,*RESULT_COMP_UNIT_DIE,*RESULT_HAS_CHILDREN
   are filled in with the info of the DIE from the DWO file.
   ABBREV_TABLE_PROVIDED is non-zero if the caller of init_cutu_and_read_dies
   provided an abbrev table to use.
   The result is non-zero if a valid (non-dummy) DIE was found.  */

static int
read_cutu_die_from_dwo (struct dwarf2_per_cu_data *this_cu,
			struct dwo_unit *dwo_unit,
			int abbrev_table_provided,
			struct die_info *stub_comp_unit_die,
			const char *stub_comp_dir,
			struct die_reader_specs *result_reader,
			const gdb_byte **result_info_ptr,
			struct die_info **result_comp_unit_die,
			int *result_has_children)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_cu *cu = this_cu->cu;
  struct dwarf2_section_info *section;
  bfd *abfd;
  const gdb_byte *begin_info_ptr, *info_ptr;
  ULONGEST signature; /* Or dwo_id.  */
  struct attribute *comp_dir, *stmt_list, *low_pc, *high_pc, *ranges;
  int i,num_extra_attrs;
  struct dwarf2_section_info *dwo_abbrev_section;
  struct attribute *attr;
  struct die_info *comp_unit_die;

  /* At most one of these may be provided.  */
  gdb_assert ((stub_comp_unit_die != NULL) + (stub_comp_dir != NULL) <= 1);

  /* These attributes aren't processed until later:
     DW_AT_stmt_list, DW_AT_low_pc, DW_AT_high_pc, DW_AT_ranges.
     DW_AT_comp_dir is used now, to find the DWO file, but it is also
     referenced later.  However, these attributes are found in the stub
     which we won't have later.  In order to not impose this complication
     on the rest of the code, we read them here and copy them to the
     DWO CU/TU die.  */

  stmt_list = NULL;
  low_pc = NULL;
  high_pc = NULL;
  ranges = NULL;
  comp_dir = NULL;

  if (stub_comp_unit_die != NULL)
    {
      /* For TUs in DWO files, the DW_AT_stmt_list attribute lives in the
	 DWO file.  */
      if (! this_cu->is_debug_types)
	stmt_list = dwarf2_attr (stub_comp_unit_die, DW_AT_stmt_list, cu);
      low_pc = dwarf2_attr (stub_comp_unit_die, DW_AT_low_pc, cu);
      high_pc = dwarf2_attr (stub_comp_unit_die, DW_AT_high_pc, cu);
      ranges = dwarf2_attr (stub_comp_unit_die, DW_AT_ranges, cu);
      comp_dir = dwarf2_attr (stub_comp_unit_die, DW_AT_comp_dir, cu);

      /* There should be a DW_AT_addr_base attribute here (if needed).
	 We need the value before we can process DW_FORM_GNU_addr_index.  */
      cu->addr_base = 0;
      attr = dwarf2_attr (stub_comp_unit_die, DW_AT_GNU_addr_base, cu);
      if (attr)
	cu->addr_base = DW_UNSND (attr);

      /* There should be a DW_AT_ranges_base attribute here (if needed).
	 We need the value before we can process DW_AT_ranges.  */
      cu->ranges_base = 0;
      attr = dwarf2_attr (stub_comp_unit_die, DW_AT_GNU_ranges_base, cu);
      if (attr)
	cu->ranges_base = DW_UNSND (attr);
    }
  else if (stub_comp_dir != NULL)
    {
      /* Reconstruct the comp_dir attribute to simplify the code below.  */
      comp_dir = XOBNEW (&cu->comp_unit_obstack, struct attribute);
      comp_dir->name = DW_AT_comp_dir;
      comp_dir->form = DW_FORM_string;
      DW_STRING_IS_CANONICAL (comp_dir) = 0;
      DW_STRING (comp_dir) = stub_comp_dir;
    }

  /* Set up for reading the DWO CU/TU.  */
  cu->dwo_unit = dwo_unit;
  section = dwo_unit->section;
  dwarf2_read_section (objfile, section);
  abfd = get_section_bfd_owner (section);
  begin_info_ptr = info_ptr = (section->buffer
			       + to_underlying (dwo_unit->sect_off));
  dwo_abbrev_section = &dwo_unit->dwo_file->sections.abbrev;
  init_cu_die_reader (result_reader, cu, section, dwo_unit->dwo_file);

  if (this_cu->is_debug_types)
    {
      struct signatured_type *sig_type = (struct signatured_type *) this_cu;

      info_ptr = read_and_check_comp_unit_head (&cu->header, section,
						dwo_abbrev_section,
						info_ptr, rcuh_kind::TYPE);
      /* This is not an assert because it can be caused by bad debug info.  */
      if (sig_type->signature != cu->header.signature)
	{
	  error (_("Dwarf Error: signature mismatch %s vs %s while reading"
		   " TU at offset 0x%x [in module %s]"),
		 hex_string (sig_type->signature),
		 hex_string (cu->header.signature),
		 to_underlying (dwo_unit->sect_off),
		 bfd_get_filename (abfd));
	}
      gdb_assert (dwo_unit->sect_off == cu->header.sect_off);
      /* For DWOs coming from DWP files, we don't know the CU length
	 nor the type's offset in the TU until now.  */
      dwo_unit->length = get_cu_length (&cu->header);
      dwo_unit->type_offset_in_tu = cu->header.type_cu_offset_in_tu;

      /* Establish the type offset that can be used to lookup the type.
	 For DWO files, we don't know it until now.  */
      sig_type->type_offset_in_section
	= dwo_unit->sect_off + to_underlying (dwo_unit->type_offset_in_tu);
    }
  else
    {
      info_ptr = read_and_check_comp_unit_head (&cu->header, section,
						dwo_abbrev_section,
						info_ptr, rcuh_kind::COMPILE);
      gdb_assert (dwo_unit->sect_off == cu->header.sect_off);
      /* For DWOs coming from DWP files, we don't know the CU length
	 until now.  */
      dwo_unit->length = get_cu_length (&cu->header);
    }

  /* Replace the CU's original abbrev table with the DWO's.
     Reminder: We can't read the abbrev table until we've read the header.  */
  if (abbrev_table_provided)
    {
      /* Don't free the provided abbrev table, the caller of
	 init_cutu_and_read_dies owns it.  */
      dwarf2_read_abbrevs (cu, dwo_abbrev_section);
      /* Ensure the DWO abbrev table gets freed.  */
      make_cleanup (dwarf2_free_abbrev_table, cu);
    }
  else
    {
      dwarf2_free_abbrev_table (cu);
      dwarf2_read_abbrevs (cu, dwo_abbrev_section);
      /* Leave any existing abbrev table cleanup as is.  */
    }

  /* Read in the die, but leave space to copy over the attributes
     from the stub.  This has the benefit of simplifying the rest of
     the code - all the work to maintain the illusion of a single
     DW_TAG_{compile,type}_unit DIE is done here.  */
  num_extra_attrs = ((stmt_list != NULL)
		     + (low_pc != NULL)
		     + (high_pc != NULL)
		     + (ranges != NULL)
		     + (comp_dir != NULL));
  info_ptr = read_full_die_1 (result_reader, result_comp_unit_die, info_ptr,
			      result_has_children, num_extra_attrs);

  /* Copy over the attributes from the stub to the DIE we just read in.  */
  comp_unit_die = *result_comp_unit_die;
  i = comp_unit_die->num_attrs;
  if (stmt_list != NULL)
    comp_unit_die->attrs[i++] = *stmt_list;
  if (low_pc != NULL)
    comp_unit_die->attrs[i++] = *low_pc;
  if (high_pc != NULL)
    comp_unit_die->attrs[i++] = *high_pc;
  if (ranges != NULL)
    comp_unit_die->attrs[i++] = *ranges;
  if (comp_dir != NULL)
    comp_unit_die->attrs[i++] = *comp_dir;
  comp_unit_die->num_attrs += num_extra_attrs;

  if (dwarf_die_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "Read die from %s@0x%x of %s:\n",
			  get_section_name (section),
			  (unsigned) (begin_info_ptr - section->buffer),
			  bfd_get_filename (abfd));
      dump_die (comp_unit_die, dwarf_die_debug);
    }

  /* Save the comp_dir attribute.  If there is no DWP file then we'll read
     TUs by skipping the stub and going directly to the entry in the DWO file.
     However, skipping the stub means we won't get DW_AT_comp_dir, so we have
     to get it via circuitous means.  Blech.  */
  if (comp_dir != NULL)
    result_reader->comp_dir = DW_STRING (comp_dir);

  /* Skip dummy compilation units.  */
  if (info_ptr >= begin_info_ptr + dwo_unit->length
      || peek_abbrev_code (abfd, info_ptr) == 0)
    return 0;

  *result_info_ptr = info_ptr;
  return 1;
}

/* Subroutine of init_cutu_and_read_dies to simplify it.
   Look up the DWO unit specified by COMP_UNIT_DIE of THIS_CU.
   Returns NULL if the specified DWO unit cannot be found.  */

static struct dwo_unit *
lookup_dwo_unit (struct dwarf2_per_cu_data *this_cu,
		 struct die_info *comp_unit_die)
{
  struct dwarf2_cu *cu = this_cu->cu;
  struct attribute *attr;
  ULONGEST signature;
  struct dwo_unit *dwo_unit;
  const char *comp_dir, *dwo_name;

  gdb_assert (cu != NULL);

  /* Yeah, we look dwo_name up again, but it simplifies the code.  */
  dwo_name = dwarf2_string_attr (comp_unit_die, DW_AT_GNU_dwo_name, cu);
  comp_dir = dwarf2_string_attr (comp_unit_die, DW_AT_comp_dir, cu);

  if (this_cu->is_debug_types)
    {
      struct signatured_type *sig_type;

      /* Since this_cu is the first member of struct signatured_type,
	 we can go from a pointer to one to a pointer to the other.  */
      sig_type = (struct signatured_type *) this_cu;
      signature = sig_type->signature;
      dwo_unit = lookup_dwo_type_unit (sig_type, dwo_name, comp_dir);
    }
  else
    {
      struct attribute *attr;

      attr = dwarf2_attr (comp_unit_die, DW_AT_GNU_dwo_id, cu);
      if (! attr)
	error (_("Dwarf Error: missing dwo_id for dwo_name %s"
		 " [in module %s]"),
	       dwo_name, objfile_name (this_cu->objfile));
      signature = DW_UNSND (attr);
      dwo_unit = lookup_dwo_comp_unit (this_cu, dwo_name, comp_dir,
				       signature);
    }

  return dwo_unit;
}

/* Subroutine of init_cutu_and_read_dies to simplify it.
   See it for a description of the parameters.
   Read a TU directly from a DWO file, bypassing the stub.

   Note: This function could be a little bit simpler if we shared cleanups
   with our caller, init_cutu_and_read_dies.  That's generally a fragile thing
   to do, so we keep this function self-contained.  Or we could move this
   into our caller, but it's complex enough already.  */

static void
init_tu_and_read_dwo_dies (struct dwarf2_per_cu_data *this_cu,
			   int use_existing_cu, int keep,
			   die_reader_func_ftype *die_reader_func,
			   void *data)
{
  struct dwarf2_cu *cu;
  struct signatured_type *sig_type;
  struct cleanup *cleanups, *free_cu_cleanup = NULL;
  struct die_reader_specs reader;
  const gdb_byte *info_ptr;
  struct die_info *comp_unit_die;
  int has_children;

  /* Verify we can do the following downcast, and that we have the
     data we need.  */
  gdb_assert (this_cu->is_debug_types && this_cu->reading_dwo_directly);
  sig_type = (struct signatured_type *) this_cu;
  gdb_assert (sig_type->dwo_unit != NULL);

  cleanups = make_cleanup (null_cleanup, NULL);

  if (use_existing_cu && this_cu->cu != NULL)
    {
      gdb_assert (this_cu->cu->dwo_unit == sig_type->dwo_unit);
      cu = this_cu->cu;
      /* There's no need to do the rereading_dwo_cu handling that
	 init_cutu_and_read_dies does since we don't read the stub.  */
    }
  else
    {
      /* If !use_existing_cu, this_cu->cu must be NULL.  */
      gdb_assert (this_cu->cu == NULL);
      cu = XNEW (struct dwarf2_cu);
      init_one_comp_unit (cu, this_cu);
      /* If an error occurs while loading, release our storage.  */
      free_cu_cleanup = make_cleanup (free_heap_comp_unit, cu);
    }

  /* A future optimization, if needed, would be to use an existing
     abbrev table.  When reading DWOs with skeletonless TUs, all the TUs
     could share abbrev tables.  */

  if (read_cutu_die_from_dwo (this_cu, sig_type->dwo_unit,
			      0 /* abbrev_table_provided */,
			      NULL /* stub_comp_unit_die */,
			      sig_type->dwo_unit->dwo_file->comp_dir,
			      &reader, &info_ptr,
			      &comp_unit_die, &has_children) == 0)
    {
      /* Dummy die.  */
      do_cleanups (cleanups);
      return;
    }

  /* All the "real" work is done here.  */
  die_reader_func (&reader, info_ptr, comp_unit_die, has_children, data);

  /* This duplicates the code in init_cutu_and_read_dies,
     but the alternative is making the latter more complex.
     This function is only for the special case of using DWO files directly:
     no point in overly complicating the general case just to handle this.  */
  if (free_cu_cleanup != NULL)
    {
      if (keep)
	{
	  /* We've successfully allocated this compilation unit.  Let our
	     caller clean it up when finished with it.  */
	  discard_cleanups (free_cu_cleanup);

	  /* We can only discard free_cu_cleanup and all subsequent cleanups.
	     So we have to manually free the abbrev table.  */
	  dwarf2_free_abbrev_table (cu);

	  /* Link this CU into read_in_chain.  */
	  this_cu->cu->read_in_chain = dwarf2_per_objfile->read_in_chain;
	  dwarf2_per_objfile->read_in_chain = this_cu;
	}
      else
	do_cleanups (free_cu_cleanup);
    }

  do_cleanups (cleanups);
}

/* Initialize a CU (or TU) and read its DIEs.
   If the CU defers to a DWO file, read the DWO file as well.

   ABBREV_TABLE, if non-NULL, is the abbreviation table to use.
   Otherwise the table specified in the comp unit header is read in and used.
   This is an optimization for when we already have the abbrev table.

   If USE_EXISTING_CU is non-zero, and THIS_CU->cu is non-NULL, then use it.
   Otherwise, a new CU is allocated with xmalloc.

   If KEEP is non-zero, then if we allocated a dwarf2_cu we add it to
   read_in_chain.  Otherwise the dwarf2_cu data is freed at the end.

   WARNING: If THIS_CU is a "dummy CU" (used as filler by the incremental
   linker) then DIE_READER_FUNC will not get called.  */

static void
init_cutu_and_read_dies (struct dwarf2_per_cu_data *this_cu,
			 struct abbrev_table *abbrev_table,
			 int use_existing_cu, int keep,
			 die_reader_func_ftype *die_reader_func,
			 void *data)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_section_info *section = this_cu->section;
  bfd *abfd = get_section_bfd_owner (section);
  struct dwarf2_cu *cu;
  const gdb_byte *begin_info_ptr, *info_ptr;
  struct die_reader_specs reader;
  struct die_info *comp_unit_die;
  int has_children;
  struct attribute *attr;
  struct cleanup *cleanups, *free_cu_cleanup = NULL;
  struct signatured_type *sig_type = NULL;
  struct dwarf2_section_info *abbrev_section;
  /* Non-zero if CU currently points to a DWO file and we need to
     reread it.  When this happens we need to reread the skeleton die
     before we can reread the DWO file (this only applies to CUs, not TUs).  */
  int rereading_dwo_cu = 0;

  if (dwarf_die_debug)
    fprintf_unfiltered (gdb_stdlog, "Reading %s unit at offset 0x%x\n",
			this_cu->is_debug_types ? "type" : "comp",
			to_underlying (this_cu->sect_off));

  if (use_existing_cu)
    gdb_assert (keep);

  /* If we're reading a TU directly from a DWO file, including a virtual DWO
     file (instead of going through the stub), short-circuit all of this.  */
  if (this_cu->reading_dwo_directly)
    {
      /* Narrow down the scope of possibilities to have to understand.  */
      gdb_assert (this_cu->is_debug_types);
      gdb_assert (abbrev_table == NULL);
      init_tu_and_read_dwo_dies (this_cu, use_existing_cu, keep,
				 die_reader_func, data);
      return;
    }

  cleanups = make_cleanup (null_cleanup, NULL);

  /* This is cheap if the section is already read in.  */
  dwarf2_read_section (objfile, section);

  begin_info_ptr = info_ptr = section->buffer + to_underlying (this_cu->sect_off);

  abbrev_section = get_abbrev_section_for_cu (this_cu);

  if (use_existing_cu && this_cu->cu != NULL)
    {
      cu = this_cu->cu;
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
      /* If !use_existing_cu, this_cu->cu must be NULL.  */
      gdb_assert (this_cu->cu == NULL);
      cu = XNEW (struct dwarf2_cu);
      init_one_comp_unit (cu, this_cu);
      /* If an error occurs while loading, release our storage.  */
      free_cu_cleanup = make_cleanup (free_heap_comp_unit, cu);
    }

  /* Get the header.  */
  if (to_underlying (cu->header.first_die_cu_offset) != 0 && !rereading_dwo_cu)
    {
      /* We already have the header, there's no need to read it in again.  */
      info_ptr += to_underlying (cu->header.first_die_cu_offset);
    }
  else
    {
      if (this_cu->is_debug_types)
	{
	  info_ptr = read_and_check_comp_unit_head (&cu->header, section,
						    abbrev_section, info_ptr,
						    rcuh_kind::TYPE);

	  /* Since per_cu is the first member of struct signatured_type,
	     we can go from a pointer to one to a pointer to the other.  */
	  sig_type = (struct signatured_type *) this_cu;
	  gdb_assert (sig_type->signature == cu->header.signature);
	  gdb_assert (sig_type->type_offset_in_tu
		      == cu->header.type_cu_offset_in_tu);
	  gdb_assert (this_cu->sect_off == cu->header.sect_off);

	  /* LENGTH has not been set yet for type units if we're
	     using .gdb_index.  */
	  this_cu->length = get_cu_length (&cu->header);

	  /* Establish the type offset that can be used to lookup the type.  */
	  sig_type->type_offset_in_section =
	    this_cu->sect_off + to_underlying (sig_type->type_offset_in_tu);

	  this_cu->dwarf_version = cu->header.version;
	}
      else
	{
	  info_ptr = read_and_check_comp_unit_head (&cu->header, section,
						    abbrev_section,
						    info_ptr,
						    rcuh_kind::COMPILE);

	  gdb_assert (this_cu->sect_off == cu->header.sect_off);
	  gdb_assert (this_cu->length == get_cu_length (&cu->header));
	  this_cu->dwarf_version = cu->header.version;
	}
    }

  /* Skip dummy compilation units.  */
  if (info_ptr >= begin_info_ptr + this_cu->length
      || peek_abbrev_code (abfd, info_ptr) == 0)
    {
      do_cleanups (cleanups);
      return;
    }

  /* If we don't have them yet, read the abbrevs for this compilation unit.
     And if we need to read them now, make sure they're freed when we're
     done.  Note that it's important that if the CU had an abbrev table
     on entry we don't free it when we're done: Somewhere up the call stack
     it may be in use.  */
  if (abbrev_table != NULL)
    {
      gdb_assert (cu->abbrev_table == NULL);
      gdb_assert (cu->header.abbrev_sect_off == abbrev_table->sect_off);
      cu->abbrev_table = abbrev_table;
    }
  else if (cu->abbrev_table == NULL)
    {
      dwarf2_read_abbrevs (cu, abbrev_section);
      make_cleanup (dwarf2_free_abbrev_table, cu);
    }
  else if (rereading_dwo_cu)
    {
      dwarf2_free_abbrev_table (cu);
      dwarf2_read_abbrevs (cu, abbrev_section);
    }

  /* Read the top level CU/TU die.  */
  init_cu_die_reader (&reader, cu, section, NULL);
  info_ptr = read_full_die (&reader, &comp_unit_die, info_ptr, &has_children);

  /* If we are in a DWO stub, process it and then read in the "real" CU/TU
     from the DWO file.
     Note that if USE_EXISTING_OK != 0, and THIS_CU->cu already contains a
     DWO CU, that this test will fail (the attribute will not be present).  */
  attr = dwarf2_attr (comp_unit_die, DW_AT_GNU_dwo_name, cu);
  if (attr)
    {
      struct dwo_unit *dwo_unit;
      struct die_info *dwo_comp_unit_die;

      if (has_children)
	{
	  complaint (&symfile_complaints,
		     _("compilation unit with DW_AT_GNU_dwo_name"
		       " has children (offset 0x%x) [in module %s]"),
		     to_underlying (this_cu->sect_off), bfd_get_filename (abfd));
	}
      dwo_unit = lookup_dwo_unit (this_cu, comp_unit_die);
      if (dwo_unit != NULL)
	{
	  if (read_cutu_die_from_dwo (this_cu, dwo_unit,
				      abbrev_table != NULL,
				      comp_unit_die, NULL,
				      &reader, &info_ptr,
				      &dwo_comp_unit_die, &has_children) == 0)
	    {
	      /* Dummy die.  */
	      do_cleanups (cleanups);
	      return;
	    }
	  comp_unit_die = dwo_comp_unit_die;
	}
      else
	{
	  /* Yikes, we couldn't find the rest of the DIE, we only have
	     the stub.  A complaint has already been logged.  There's
	     not much more we can do except pass on the stub DIE to
	     die_reader_func.  We don't want to throw an error on bad
	     debug info.  */
	}
    }

  /* All of the above is setup for this call.  Yikes.  */
  die_reader_func (&reader, info_ptr, comp_unit_die, has_children, data);

  /* Done, clean up.  */
  if (free_cu_cleanup != NULL)
    {
      if (keep)
	{
	  /* We've successfully allocated this compilation unit.  Let our
	     caller clean it up when finished with it.  */
	  discard_cleanups (free_cu_cleanup);

	  /* We can only discard free_cu_cleanup and all subsequent cleanups.
	     So we have to manually free the abbrev table.  */
	  dwarf2_free_abbrev_table (cu);

	  /* Link this CU into read_in_chain.  */
	  this_cu->cu->read_in_chain = dwarf2_per_objfile->read_in_chain;
	  dwarf2_per_objfile->read_in_chain = this_cu;
	}
      else
	do_cleanups (free_cu_cleanup);
    }

  do_cleanups (cleanups);
}

/* Read CU/TU THIS_CU but do not follow DW_AT_GNU_dwo_name if present.
   DWO_FILE, if non-NULL, is the DWO file to read (the caller is assumed
   to have already done the lookup to find the DWO file).

   The caller is required to fill in THIS_CU->section, THIS_CU->offset, and
   THIS_CU->is_debug_types, but nothing else.

   We fill in THIS_CU->length.

   WARNING: If THIS_CU is a "dummy CU" (used as filler by the incremental
   linker) then DIE_READER_FUNC will not get called.

   THIS_CU->cu is always freed when done.
   This is done in order to not leave THIS_CU->cu in a state where we have
   to care whether it refers to the "main" CU or the DWO CU.  */

static void
init_cutu_and_read_dies_no_follow (struct dwarf2_per_cu_data *this_cu,
				   struct dwo_file *dwo_file,
				   die_reader_func_ftype *die_reader_func,
				   void *data)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_section_info *section = this_cu->section;
  bfd *abfd = get_section_bfd_owner (section);
  struct dwarf2_section_info *abbrev_section;
  struct dwarf2_cu cu;
  const gdb_byte *begin_info_ptr, *info_ptr;
  struct die_reader_specs reader;
  struct cleanup *cleanups;
  struct die_info *comp_unit_die;
  int has_children;

  if (dwarf_die_debug)
    fprintf_unfiltered (gdb_stdlog, "Reading %s unit at offset 0x%x\n",
			this_cu->is_debug_types ? "type" : "comp",
			to_underlying (this_cu->sect_off));

  gdb_assert (this_cu->cu == NULL);

  abbrev_section = (dwo_file != NULL
		    ? &dwo_file->sections.abbrev
		    : get_abbrev_section_for_cu (this_cu));

  /* This is cheap if the section is already read in.  */
  dwarf2_read_section (objfile, section);

  init_one_comp_unit (&cu, this_cu);

  cleanups = make_cleanup (free_stack_comp_unit, &cu);

  begin_info_ptr = info_ptr = section->buffer + to_underlying (this_cu->sect_off);
  info_ptr = read_and_check_comp_unit_head (&cu.header, section,
					    abbrev_section, info_ptr,
					    (this_cu->is_debug_types
					     ? rcuh_kind::TYPE
					     : rcuh_kind::COMPILE));

  this_cu->length = get_cu_length (&cu.header);

  /* Skip dummy compilation units.  */
  if (info_ptr >= begin_info_ptr + this_cu->length
      || peek_abbrev_code (abfd, info_ptr) == 0)
    {
      do_cleanups (cleanups);
      return;
    }

  dwarf2_read_abbrevs (&cu, abbrev_section);
  make_cleanup (dwarf2_free_abbrev_table, &cu);

  init_cu_die_reader (&reader, &cu, section, dwo_file);
  info_ptr = read_full_die (&reader, &comp_unit_die, info_ptr, &has_children);

  die_reader_func (&reader, info_ptr, comp_unit_die, has_children, data);

  do_cleanups (cleanups);
}

/* Read a CU/TU, except that this does not look for DW_AT_GNU_dwo_name and
   does not lookup the specified DWO file.
   This cannot be used to read DWO files.

   THIS_CU->cu is always freed when done.
   This is done in order to not leave THIS_CU->cu in a state where we have
   to care whether it refers to the "main" CU or the DWO CU.
   We can revisit this if the data shows there's a performance issue.  */

static void
init_cutu_and_read_dies_simple (struct dwarf2_per_cu_data *this_cu,
				die_reader_func_ftype *die_reader_func,
				void *data)
{
  init_cutu_and_read_dies_no_follow (this_cu, NULL, die_reader_func, data);
}

/* Type Unit Groups.

   Type Unit Groups are a way to collapse the set of all TUs (type units) into
   a more manageable set.  The grouping is done by DW_AT_stmt_list entry
   so that all types coming from the same compilation (.o file) are grouped
   together.  A future step could be to put the types in the same symtab as
   the CU the types ultimately came from.  */

static hashval_t
hash_type_unit_group (const void *item)
{
  const struct type_unit_group *tu_group
    = (const struct type_unit_group *) item;

  return hash_stmt_list_entry (&tu_group->hash);
}

static int
eq_type_unit_group (const void *item_lhs, const void *item_rhs)
{
  const struct type_unit_group *lhs = (const struct type_unit_group *) item_lhs;
  const struct type_unit_group *rhs = (const struct type_unit_group *) item_rhs;

  return eq_stmt_list_entry (&lhs->hash, &rhs->hash);
}

/* Allocate a hash table for type unit groups.  */

static htab_t
allocate_type_unit_groups_table (void)
{
  return htab_create_alloc_ex (3,
			       hash_type_unit_group,
			       eq_type_unit_group,
			       NULL,
			       &dwarf2_per_objfile->objfile->objfile_obstack,
			       hashtab_obstack_allocate,
			       dummy_obstack_deallocate);
}

/* Type units that don't have DW_AT_stmt_list are grouped into their own
   partial symtabs.  We combine several TUs per psymtab to not let the size
   of any one psymtab grow too big.  */
#define NO_STMT_LIST_TYPE_UNIT_PSYMTAB (1 << 31)
#define NO_STMT_LIST_TYPE_UNIT_PSYMTAB_SIZE 10

/* Helper routine for get_type_unit_group.
   Create the type_unit_group object used to hold one or more TUs.  */

static struct type_unit_group *
create_type_unit_group (struct dwarf2_cu *cu, sect_offset line_offset_struct)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_per_cu_data *per_cu;
  struct type_unit_group *tu_group;

  tu_group = OBSTACK_ZALLOC (&objfile->objfile_obstack,
			     struct type_unit_group);
  per_cu = &tu_group->per_cu;
  per_cu->objfile = objfile;

  if (dwarf2_per_objfile->using_index)
    {
      per_cu->v.quick = OBSTACK_ZALLOC (&objfile->objfile_obstack,
					struct dwarf2_per_cu_quick_data);
    }
  else
    {
      unsigned int line_offset = to_underlying (line_offset_struct);
      struct partial_symtab *pst;
      char *name;

      /* Give the symtab a useful name for debug purposes.  */
      if ((line_offset & NO_STMT_LIST_TYPE_UNIT_PSYMTAB) != 0)
	name = xstrprintf ("<type_units_%d>",
			   (line_offset & ~NO_STMT_LIST_TYPE_UNIT_PSYMTAB));
      else
	name = xstrprintf ("<type_units_at_0x%x>", line_offset);

      pst = create_partial_symtab (per_cu, name);
      pst->anonymous = 1;

      xfree (name);
    }

  tu_group->hash.dwo_unit = cu->dwo_unit;
  tu_group->hash.line_sect_off = line_offset_struct;

  return tu_group;
}

/* Look up the type_unit_group for type unit CU, and create it if necessary.
   STMT_LIST is a DW_AT_stmt_list attribute.  */

static struct type_unit_group *
get_type_unit_group (struct dwarf2_cu *cu, const struct attribute *stmt_list)
{
  struct tu_stats *tu_stats = &dwarf2_per_objfile->tu_stats;
  struct type_unit_group *tu_group;
  void **slot;
  unsigned int line_offset;
  struct type_unit_group type_unit_group_for_lookup;

  if (dwarf2_per_objfile->type_unit_groups == NULL)
    {
      dwarf2_per_objfile->type_unit_groups =
	allocate_type_unit_groups_table ();
    }

  /* Do we need to create a new group, or can we use an existing one?  */

  if (stmt_list)
    {
      line_offset = DW_UNSND (stmt_list);
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

  type_unit_group_for_lookup.hash.dwo_unit = cu->dwo_unit;
  type_unit_group_for_lookup.hash.line_sect_off = (sect_offset) line_offset;
  slot = htab_find_slot (dwarf2_per_objfile->type_unit_groups,
			 &type_unit_group_for_lookup, INSERT);
  if (*slot != NULL)
    {
      tu_group = (struct type_unit_group *) *slot;
      gdb_assert (tu_group != NULL);
    }
  else
    {
      sect_offset line_offset_struct = (sect_offset) line_offset;
      tu_group = create_type_unit_group (cu, line_offset_struct);
      *slot = tu_group;
      ++tu_stats->nr_symtabs;
    }

  return tu_group;
}

/* Partial symbol tables.  */

/* Create a psymtab named NAME and assign it to PER_CU.

   The caller must fill in the following details:
   dirname, textlow, texthigh.  */

static struct partial_symtab *
create_partial_symtab (struct dwarf2_per_cu_data *per_cu, const char *name)
{
  struct objfile *objfile = per_cu->objfile;
  struct partial_symtab *pst;

  pst = start_psymtab_common (objfile, name, 0,
			      objfile->global_psymbols.next,
			      objfile->static_psymbols.next);

  pst->psymtabs_addrmap_supported = 1;

  /* This is the glue that links PST into GDB's symbol API.  */
  pst->read_symtab_private = per_cu;
  pst->read_symtab = dwarf2_read_symtab;
  per_cu->v.psymtab = pst;

  return pst;
}

/* The DATA object passed to process_psymtab_comp_unit_reader has this
   type.  */

struct process_psymtab_comp_unit_data
{
  /* True if we are reading a DW_TAG_partial_unit.  */

  int want_partial_unit;

  /* The "pretend" language that is used if the CU doesn't declare a
     language.  */

  enum language pretend_language;
};

/* die_reader_func for process_psymtab_comp_unit.  */

static void
process_psymtab_comp_unit_reader (const struct die_reader_specs *reader,
				  const gdb_byte *info_ptr,
				  struct die_info *comp_unit_die,
				  int has_children,
				  void *data)
{
  struct dwarf2_cu *cu = reader->cu;
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct dwarf2_per_cu_data *per_cu = cu->per_cu;
  CORE_ADDR baseaddr;
  CORE_ADDR best_lowpc = 0, best_highpc = 0;
  struct partial_symtab *pst;
  enum pc_bounds_kind cu_bounds_kind;
  const char *filename;
  struct process_psymtab_comp_unit_data *info
    = (struct process_psymtab_comp_unit_data *) data;

  if (comp_unit_die->tag == DW_TAG_partial_unit && !info->want_partial_unit)
    return;

  gdb_assert (! per_cu->is_debug_types);

  prepare_one_comp_unit (cu, comp_unit_die, info->pretend_language);

  cu->list_in_scope = &file_symbols;

  /* Allocate a new partial symbol table structure.  */
  filename = dwarf2_string_attr (comp_unit_die, DW_AT_name, cu);
  if (filename == NULL)
    filename = "";

  pst = create_partial_symtab (per_cu, filename);

  /* This must be done before calling dwarf2_build_include_psymtabs.  */
  pst->dirname = dwarf2_string_attr (comp_unit_die, DW_AT_comp_dir, cu);

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  dwarf2_find_base_address (comp_unit_die, cu);

  /* Possibly set the default values of LOWPC and HIGHPC from
     `DW_AT_ranges'.  */
  cu_bounds_kind = dwarf2_get_pc_bounds (comp_unit_die, &best_lowpc,
					 &best_highpc, cu, pst);
  if (cu_bounds_kind == PC_BOUNDS_HIGH_LOW && best_lowpc < best_highpc)
    /* Store the contiguous range if it is not empty; it can be empty for
       CUs with no code.  */
    addrmap_set_empty (objfile->psymtabs_addrmap,
		       gdbarch_adjust_dwarf2_addr (gdbarch,
						   best_lowpc + baseaddr),
		       gdbarch_adjust_dwarf2_addr (gdbarch,
						   best_highpc + baseaddr) - 1,
		       pst);

  /* Check if comp unit has_children.
     If so, read the rest of the partial symbols from this comp unit.
     If not, there's no more debug_info for this comp unit.  */
  if (has_children)
    {
      struct partial_die_info *first_die;
      CORE_ADDR lowpc, highpc;

      lowpc = ((CORE_ADDR) -1);
      highpc = ((CORE_ADDR) 0);

      first_die = load_partial_dies (reader, info_ptr, 1);

      scan_partial_symbols (first_die, &lowpc, &highpc,
			    cu_bounds_kind <= PC_BOUNDS_INVALID, cu);

      /* If we didn't find a lowpc, set it to highpc to avoid
	 complaints from `maint check'.	 */
      if (lowpc == ((CORE_ADDR) -1))
	lowpc = highpc;

      /* If the compilation unit didn't have an explicit address range,
	 then use the information extracted from its child dies.  */
      if (cu_bounds_kind <= PC_BOUNDS_INVALID)
	{
	  best_lowpc = lowpc;
	  best_highpc = highpc;
	}
    }
  pst->textlow = gdbarch_adjust_dwarf2_addr (gdbarch, best_lowpc + baseaddr);
  pst->texthigh = gdbarch_adjust_dwarf2_addr (gdbarch, best_highpc + baseaddr);

  end_psymtab_common (objfile, pst);

  if (!VEC_empty (dwarf2_per_cu_ptr, cu->per_cu->imported_symtabs))
    {
      int i;
      int len = VEC_length (dwarf2_per_cu_ptr, cu->per_cu->imported_symtabs);
      struct dwarf2_per_cu_data *iter;

      /* Fill in 'dependencies' here; we fill in 'users' in a
	 post-pass.  */
      pst->number_of_dependencies = len;
      pst->dependencies =
	XOBNEWVEC (&objfile->objfile_obstack, struct partial_symtab *, len);
      for (i = 0;
	   VEC_iterate (dwarf2_per_cu_ptr, cu->per_cu->imported_symtabs,
			i, iter);
	   ++i)
	pst->dependencies[i] = iter->v.psymtab;

      VEC_free (dwarf2_per_cu_ptr, cu->per_cu->imported_symtabs);
    }

  /* Get the list of files included in the current compilation unit,
     and build a psymtab for each of them.  */
  dwarf2_build_include_psymtabs (cu, comp_unit_die, pst);

  if (dwarf_read_debug)
    {
      struct gdbarch *gdbarch = get_objfile_arch (objfile);

      fprintf_unfiltered (gdb_stdlog,
			  "Psymtab for %s unit @0x%x: %s - %s"
			  ", %d global, %d static syms\n",
			  per_cu->is_debug_types ? "type" : "comp",
			  to_underlying (per_cu->sect_off),
			  paddress (gdbarch, pst->textlow),
			  paddress (gdbarch, pst->texthigh),
			  pst->n_global_syms, pst->n_static_syms);
    }
}

/* Subroutine of dwarf2_build_psymtabs_hard to simplify it.
   Process compilation unit THIS_CU for a psymtab.  */

static void
process_psymtab_comp_unit (struct dwarf2_per_cu_data *this_cu,
			   int want_partial_unit,
			   enum language pretend_language)
{
  /* If this compilation unit was already read in, free the
     cached copy in order to read it in again.	This is
     necessary because we skipped some symbols when we first
     read in the compilation unit (see load_partial_dies).
     This problem could be avoided, but the benefit is unclear.  */
  if (this_cu->cu != NULL)
    free_one_cached_comp_unit (this_cu);

  if (this_cu->is_debug_types)
    init_cutu_and_read_dies (this_cu, NULL, 0, 0, build_type_psymtabs_reader,
			     NULL);
  else
    {
      process_psymtab_comp_unit_data info;
      info.want_partial_unit = want_partial_unit;
      info.pretend_language = pretend_language;
      init_cutu_and_read_dies (this_cu, NULL, 0, 0,
			       process_psymtab_comp_unit_reader, &info);
    }

  /* Age out any secondary CUs.  */
  age_cached_comp_units ();
}

/* Reader function for build_type_psymtabs.  */

static void
build_type_psymtabs_reader (const struct die_reader_specs *reader,
			    const gdb_byte *info_ptr,
			    struct die_info *type_unit_die,
			    int has_children,
			    void *data)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_cu *cu = reader->cu;
  struct dwarf2_per_cu_data *per_cu = cu->per_cu;
  struct signatured_type *sig_type;
  struct type_unit_group *tu_group;
  struct attribute *attr;
  struct partial_die_info *first_die;
  CORE_ADDR lowpc, highpc;
  struct partial_symtab *pst;

  gdb_assert (data == NULL);
  gdb_assert (per_cu->is_debug_types);
  sig_type = (struct signatured_type *) per_cu;

  if (! has_children)
    return;

  attr = dwarf2_attr_no_follow (type_unit_die, DW_AT_stmt_list);
  tu_group = get_type_unit_group (cu, attr);

  VEC_safe_push (sig_type_ptr, tu_group->tus, sig_type);

  prepare_one_comp_unit (cu, type_unit_die, language_minimal);
  cu->list_in_scope = &file_symbols;
  pst = create_partial_symtab (per_cu, "");
  pst->anonymous = 1;

  first_die = load_partial_dies (reader, info_ptr, 1);

  lowpc = (CORE_ADDR) -1;
  highpc = (CORE_ADDR) 0;
  scan_partial_symbols (first_die, &lowpc, &highpc, 0, cu);

  end_psymtab_common (objfile, pst);
}

/* Struct used to sort TUs by their abbreviation table offset.  */

struct tu_abbrev_offset
{
  struct signatured_type *sig_type;
  sect_offset abbrev_offset;
};

/* Helper routine for build_type_psymtabs_1, passed to qsort.  */

static int
sort_tu_by_abbrev_offset (const void *ap, const void *bp)
{
  const struct tu_abbrev_offset * const *a
    = (const struct tu_abbrev_offset * const*) ap;
  const struct tu_abbrev_offset * const *b
    = (const struct tu_abbrev_offset * const*) bp;
  sect_offset aoff = (*a)->abbrev_offset;
  sect_offset boff = (*b)->abbrev_offset;

  return (aoff > boff) - (aoff < boff);
}

/* Efficiently read all the type units.
   This does the bulk of the work for build_type_psymtabs.

   The efficiency is because we sort TUs by the abbrev table they use and
   only read each abbrev table once.  In one program there are 200K TUs
   sharing 8K abbrev tables.

   The main purpose of this function is to support building the
   dwarf2_per_objfile->type_unit_groups table.
   TUs typically share the DW_AT_stmt_list of the CU they came from, so we
   can collapse the search space by grouping them by stmt_list.
   The savings can be significant, in the same program from above the 200K TUs
   share 8K stmt_list tables.

   FUNC is expected to call get_type_unit_group, which will create the
   struct type_unit_group if necessary and add it to
   dwarf2_per_objfile->type_unit_groups.  */

static void
build_type_psymtabs_1 (void)
{
  struct tu_stats *tu_stats = &dwarf2_per_objfile->tu_stats;
  struct cleanup *cleanups;
  struct abbrev_table *abbrev_table;
  sect_offset abbrev_offset;
  struct tu_abbrev_offset *sorted_by_abbrev;
  int i;

  /* It's up to the caller to not call us multiple times.  */
  gdb_assert (dwarf2_per_objfile->type_unit_groups == NULL);

  if (dwarf2_per_objfile->n_type_units == 0)
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

  if (dwarf_read_debug)
    fprintf_unfiltered (gdb_stdlog, "Building type unit groups ...\n");

  /* Sort in a separate table to maintain the order of all_type_units
     for .gdb_index: TU indices directly index all_type_units.  */
  sorted_by_abbrev = XNEWVEC (struct tu_abbrev_offset,
			      dwarf2_per_objfile->n_type_units);
  for (i = 0; i < dwarf2_per_objfile->n_type_units; ++i)
    {
      struct signatured_type *sig_type = dwarf2_per_objfile->all_type_units[i];

      sorted_by_abbrev[i].sig_type = sig_type;
      sorted_by_abbrev[i].abbrev_offset =
	read_abbrev_offset (sig_type->per_cu.section,
			    sig_type->per_cu.sect_off);
    }
  cleanups = make_cleanup (xfree, sorted_by_abbrev);
  qsort (sorted_by_abbrev, dwarf2_per_objfile->n_type_units,
	 sizeof (struct tu_abbrev_offset), sort_tu_by_abbrev_offset);

  abbrev_offset = (sect_offset) ~(unsigned) 0;
  abbrev_table = NULL;
  make_cleanup (abbrev_table_free_cleanup, &abbrev_table);

  for (i = 0; i < dwarf2_per_objfile->n_type_units; ++i)
    {
      const struct tu_abbrev_offset *tu = &sorted_by_abbrev[i];

      /* Switch to the next abbrev table if necessary.  */
      if (abbrev_table == NULL
	  || tu->abbrev_offset != abbrev_offset)
	{
	  if (abbrev_table != NULL)
	    {
	      abbrev_table_free (abbrev_table);
	      /* Reset to NULL in case abbrev_table_read_table throws
		 an error: abbrev_table_free_cleanup will get called.  */
	      abbrev_table = NULL;
	    }
	  abbrev_offset = tu->abbrev_offset;
	  abbrev_table =
	    abbrev_table_read_table (&dwarf2_per_objfile->abbrev,
				     abbrev_offset);
	  ++tu_stats->nr_uniq_abbrev_tables;
	}

      init_cutu_and_read_dies (&tu->sig_type->per_cu, abbrev_table, 0, 0,
			       build_type_psymtabs_reader, NULL);
    }

  do_cleanups (cleanups);
}

/* Print collected type unit statistics.  */

static void
print_tu_stats (void)
{
  struct tu_stats *tu_stats = &dwarf2_per_objfile->tu_stats;

  fprintf_unfiltered (gdb_stdlog, "Type unit statistics:\n");
  fprintf_unfiltered (gdb_stdlog, "  %d TUs\n",
		      dwarf2_per_objfile->n_type_units);
  fprintf_unfiltered (gdb_stdlog, "  %d uniq abbrev tables\n",
		      tu_stats->nr_uniq_abbrev_tables);
  fprintf_unfiltered (gdb_stdlog, "  %d symtabs from stmt_list entries\n",
		      tu_stats->nr_symtabs);
  fprintf_unfiltered (gdb_stdlog, "  %d symtab sharers\n",
		      tu_stats->nr_symtab_sharers);
  fprintf_unfiltered (gdb_stdlog, "  %d type units without a stmt_list\n",
		      tu_stats->nr_stmt_less_type_units);
  fprintf_unfiltered (gdb_stdlog, "  %d all_type_units reallocs\n",
		      tu_stats->nr_all_type_units_reallocs);
}

/* Traversal function for build_type_psymtabs.  */

static int
build_type_psymtab_dependencies (void **slot, void *info)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct type_unit_group *tu_group = (struct type_unit_group *) *slot;
  struct dwarf2_per_cu_data *per_cu = &tu_group->per_cu;
  struct partial_symtab *pst = per_cu->v.psymtab;
  int len = VEC_length (sig_type_ptr, tu_group->tus);
  struct signatured_type *iter;
  int i;

  gdb_assert (len > 0);
  gdb_assert (IS_TYPE_UNIT_GROUP (per_cu));

  pst->number_of_dependencies = len;
  pst->dependencies =
    XOBNEWVEC (&objfile->objfile_obstack, struct partial_symtab *, len);
  for (i = 0;
       VEC_iterate (sig_type_ptr, tu_group->tus, i, iter);
       ++i)
    {
      gdb_assert (iter->per_cu.is_debug_types);
      pst->dependencies[i] = iter->per_cu.v.psymtab;
      iter->type_unit_group = tu_group;
    }

  VEC_free (sig_type_ptr, tu_group->tus);

  return 1;
}

/* Subroutine of dwarf2_build_psymtabs_hard to simplify it.
   Build partial symbol tables for the .debug_types comp-units.  */

static void
build_type_psymtabs (struct objfile *objfile)
{
  if (! create_all_type_units (objfile))
    return;

  build_type_psymtabs_1 ();
}

/* Traversal function for process_skeletonless_type_unit.
   Read a TU in a DWO file and build partial symbols for it.  */

static int
process_skeletonless_type_unit (void **slot, void *info)
{
  struct dwo_unit *dwo_unit = (struct dwo_unit *) *slot;
  struct objfile *objfile = (struct objfile *) info;
  struct signatured_type find_entry, *entry;

  /* If this TU doesn't exist in the global table, add it and read it in.  */

  if (dwarf2_per_objfile->signatured_types == NULL)
    {
      dwarf2_per_objfile->signatured_types
	= allocate_signatured_type_table (objfile);
    }

  find_entry.signature = dwo_unit->signature;
  slot = htab_find_slot (dwarf2_per_objfile->signatured_types, &find_entry,
			 INSERT);
  /* If we've already seen this type there's nothing to do.  What's happening
     is we're doing our own version of comdat-folding here.  */
  if (*slot != NULL)
    return 1;

  /* This does the job that create_all_type_units would have done for
     this TU.  */
  entry = add_type_unit (dwo_unit->signature, slot);
  fill_in_sig_entry_from_dwo_entry (objfile, entry, dwo_unit);
  *slot = entry;

  /* This does the job that build_type_psymtabs_1 would have done.  */
  init_cutu_and_read_dies (&entry->per_cu, NULL, 0, 0,
			   build_type_psymtabs_reader, NULL);

  return 1;
}

/* Traversal function for process_skeletonless_type_units.  */

static int
process_dwo_file_for_skeletonless_type_units (void **slot, void *info)
{
  struct dwo_file *dwo_file = (struct dwo_file *) *slot;

  if (dwo_file->tus != NULL)
    {
      htab_traverse_noresize (dwo_file->tus,
			      process_skeletonless_type_unit, info);
    }

  return 1;
}

/* Scan all TUs of DWO files, verifying we've processed them.
   This is needed in case a TU was emitted without its skeleton.
   Note: This can't be done until we know what all the DWO files are.  */

static void
process_skeletonless_type_units (struct objfile *objfile)
{
  /* Skeletonless TUs in DWP files without .gdb_index is not supported yet.  */
  if (get_dwp_file () == NULL
      && dwarf2_per_objfile->dwo_files != NULL)
    {
      htab_traverse_noresize (dwarf2_per_objfile->dwo_files,
			      process_dwo_file_for_skeletonless_type_units,
			      objfile);
    }
}

/* A cleanup function that clears objfile's psymtabs_addrmap field.  */

static void
psymtabs_addrmap_cleanup (void *o)
{
  struct objfile *objfile = (struct objfile *) o;

  objfile->psymtabs_addrmap = NULL;
}

/* Compute the 'user' field for each psymtab in OBJFILE.  */

static void
set_partial_user (struct objfile *objfile)
{
  int i;

  for (i = 0; i < dwarf2_per_objfile->n_comp_units; ++i)
    {
      struct dwarf2_per_cu_data *per_cu = dw2_get_cutu (i);
      struct partial_symtab *pst = per_cu->v.psymtab;
      int j;

      if (pst == NULL)
	continue;

      for (j = 0; j < pst->number_of_dependencies; ++j)
	{
	  /* Set the 'user' field only if it is not already set.  */
	  if (pst->dependencies[j]->user == NULL)
	    pst->dependencies[j]->user = pst;
	}
    }
}

/* Build the partial symbol table by doing a quick pass through the
   .debug_info and .debug_abbrev sections.  */

static void
dwarf2_build_psymtabs_hard (struct objfile *objfile)
{
  struct cleanup *back_to, *addrmap_cleanup;
  struct obstack temp_obstack;
  int i;

  if (dwarf_read_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "Building psymtabs of objfile %s ...\n",
			  objfile_name (objfile));
    }

  dwarf2_per_objfile->reading_partial_symbols = 1;

  dwarf2_read_section (objfile, &dwarf2_per_objfile->info);

  /* Any cached compilation units will be linked by the per-objfile
     read_in_chain.  Make sure to free them when we're done.  */
  back_to = make_cleanup (free_cached_comp_units, NULL);

  build_type_psymtabs (objfile);

  create_all_comp_units (objfile);

  /* Create a temporary address map on a temporary obstack.  We later
     copy this to the final obstack.  */
  obstack_init (&temp_obstack);
  make_cleanup_obstack_free (&temp_obstack);
  objfile->psymtabs_addrmap = addrmap_create_mutable (&temp_obstack);
  addrmap_cleanup = make_cleanup (psymtabs_addrmap_cleanup, objfile);

  for (i = 0; i < dwarf2_per_objfile->n_comp_units; ++i)
    {
      struct dwarf2_per_cu_data *per_cu = dw2_get_cutu (i);

      process_psymtab_comp_unit (per_cu, 0, language_minimal);
    }

  /* This has to wait until we read the CUs, we need the list of DWOs.  */
  process_skeletonless_type_units (objfile);

  /* Now that all TUs have been processed we can fill in the dependencies.  */
  if (dwarf2_per_objfile->type_unit_groups != NULL)
    {
      htab_traverse_noresize (dwarf2_per_objfile->type_unit_groups,
			      build_type_psymtab_dependencies, NULL);
    }

  if (dwarf_read_debug)
    print_tu_stats ();

  set_partial_user (objfile);

  objfile->psymtabs_addrmap = addrmap_create_fixed (objfile->psymtabs_addrmap,
						    &objfile->objfile_obstack);
  discard_cleanups (addrmap_cleanup);

  do_cleanups (back_to);

  if (dwarf_read_debug)
    fprintf_unfiltered (gdb_stdlog, "Done building psymtabs of %s\n",
			objfile_name (objfile));
}

/* die_reader_func for load_partial_comp_unit.  */

static void
load_partial_comp_unit_reader (const struct die_reader_specs *reader,
			       const gdb_byte *info_ptr,
			       struct die_info *comp_unit_die,
			       int has_children,
			       void *data)
{
  struct dwarf2_cu *cu = reader->cu;

  prepare_one_comp_unit (cu, comp_unit_die, language_minimal);

  /* Check if comp unit has_children.
     If so, read the rest of the partial symbols from this comp unit.
     If not, there's no more debug_info for this comp unit.  */
  if (has_children)
    load_partial_dies (reader, info_ptr, 0);
}

/* Load the partial DIEs for a secondary CU into memory.
   This is also used when rereading a primary CU with load_all_dies.  */

static void
load_partial_comp_unit (struct dwarf2_per_cu_data *this_cu)
{
  init_cutu_and_read_dies (this_cu, NULL, 1, 1,
			   load_partial_comp_unit_reader, NULL);
}

static void
read_comp_units_from_section (struct objfile *objfile,
			      struct dwarf2_section_info *section,
			      struct dwarf2_section_info *abbrev_section,
			      unsigned int is_dwz,
			      int *n_allocated,
			      int *n_comp_units,
			      struct dwarf2_per_cu_data ***all_comp_units)
{
  const gdb_byte *info_ptr;
  bfd *abfd = get_section_bfd_owner (section);

  if (dwarf_read_debug)
    fprintf_unfiltered (gdb_stdlog, "Reading %s for %s\n",
			get_section_name (section),
			get_section_file_name (section));

  dwarf2_read_section (objfile, section);

  info_ptr = section->buffer;

  while (info_ptr < section->buffer + section->size)
    {
      struct dwarf2_per_cu_data *this_cu;

      sect_offset sect_off = (sect_offset) (info_ptr - section->buffer);

      comp_unit_head cu_header;
      read_and_check_comp_unit_head (&cu_header, section, abbrev_section,
				     info_ptr, rcuh_kind::COMPILE);

      /* Save the compilation unit for later lookup.  */
      if (cu_header.unit_type != DW_UT_type)
	{
	  this_cu = XOBNEW (&objfile->objfile_obstack,
			    struct dwarf2_per_cu_data);
	  memset (this_cu, 0, sizeof (*this_cu));
	}
      else
	{
	  auto sig_type = XOBNEW (&objfile->objfile_obstack,
				  struct signatured_type);
	  memset (sig_type, 0, sizeof (*sig_type));
	  sig_type->signature = cu_header.signature;
	  sig_type->type_offset_in_tu = cu_header.type_cu_offset_in_tu;
	  this_cu = &sig_type->per_cu;
	}
      this_cu->is_debug_types = (cu_header.unit_type == DW_UT_type);
      this_cu->sect_off = sect_off;
      this_cu->length = cu_header.length + cu_header.initial_length_size;
      this_cu->is_dwz = is_dwz;
      this_cu->objfile = objfile;
      this_cu->section = section;

      if (*n_comp_units == *n_allocated)
	{
	  *n_allocated *= 2;
	  *all_comp_units = XRESIZEVEC (struct dwarf2_per_cu_data *,
					*all_comp_units, *n_allocated);
	}
      (*all_comp_units)[*n_comp_units] = this_cu;
      ++*n_comp_units;

      info_ptr = info_ptr + this_cu->length;
    }
}

/* Create a list of all compilation units in OBJFILE.
   This is only done for -readnow and building partial symtabs.  */

static void
create_all_comp_units (struct objfile *objfile)
{
  int n_allocated;
  int n_comp_units;
  struct dwarf2_per_cu_data **all_comp_units;
  struct dwz_file *dwz;

  n_comp_units = 0;
  n_allocated = 10;
  all_comp_units = XNEWVEC (struct dwarf2_per_cu_data *, n_allocated);

  read_comp_units_from_section (objfile, &dwarf2_per_objfile->info,
				&dwarf2_per_objfile->abbrev, 0,
				&n_allocated, &n_comp_units, &all_comp_units);

  dwz = dwarf2_get_dwz_file ();
  if (dwz != NULL)
    read_comp_units_from_section (objfile, &dwz->info, &dwz->abbrev, 1,
				  &n_allocated, &n_comp_units,
				  &all_comp_units);

  dwarf2_per_objfile->all_comp_units = XOBNEWVEC (&objfile->objfile_obstack,
						  struct dwarf2_per_cu_data *,
						  n_comp_units);
  memcpy (dwarf2_per_objfile->all_comp_units, all_comp_units,
	  n_comp_units * sizeof (struct dwarf2_per_cu_data *));
  xfree (all_comp_units);
  dwarf2_per_objfile->n_comp_units = n_comp_units;
}

/* Process all loaded DIEs for compilation unit CU, starting at
   FIRST_DIE.  The caller should pass SET_ADDRMAP == 1 if the compilation
   unit DIE did not have PC info (DW_AT_low_pc and DW_AT_high_pc, or
   DW_AT_ranges).  See the comments of add_partial_subprogram on how
   SET_ADDRMAP is used and how *LOWPC and *HIGHPC are updated.  */

static void
scan_partial_symbols (struct partial_die_info *first_die, CORE_ADDR *lowpc,
		      CORE_ADDR *highpc, int set_addrmap,
		      struct dwarf2_cu *cu)
{
  struct partial_die_info *pdi;

  /* Now, march along the PDI's, descending into ones which have
     interesting children but skipping the children of the other ones,
     until we reach the end of the compilation unit.  */

  pdi = first_die;

  while (pdi != NULL)
    {
      fixup_partial_die (pdi, cu);

      /* Anonymous namespaces or modules have no name but have interesting
	 children, so we need to look at them.  Ditto for anonymous
	 enums.  */

      if (pdi->name != NULL || pdi->tag == DW_TAG_namespace
	  || pdi->tag == DW_TAG_module || pdi->tag == DW_TAG_enumeration_type
	  || pdi->tag == DW_TAG_imported_unit)
	{
	  switch (pdi->tag)
	    {
	    case DW_TAG_subprogram:
	      add_partial_subprogram (pdi, lowpc, highpc, set_addrmap, cu);
	      break;
	    case DW_TAG_constant:
	    case DW_TAG_variable:
	    case DW_TAG_typedef:
	    case DW_TAG_union_type:
	      if (!pdi->is_declaration)
		{
		  add_partial_symbol (pdi, cu);
		}
	      break;
	    case DW_TAG_class_type:
	    case DW_TAG_interface_type:
	    case DW_TAG_structure_type:
	      if (!pdi->is_declaration)
		{
		  add_partial_symbol (pdi, cu);
		}
	      if (cu->language == language_rust && pdi->has_children)
		scan_partial_symbols (pdi->die_child, lowpc, highpc,
				      set_addrmap, cu);
	      break;
	    case DW_TAG_enumeration_type:
	      if (!pdi->is_declaration)
		add_partial_enumeration (pdi, cu);
	      break;
	    case DW_TAG_base_type:
            case DW_TAG_subrange_type:
	      /* File scope base type definitions are added to the partial
	         symbol table.  */
	      add_partial_symbol (pdi, cu);
	      break;
	    case DW_TAG_namespace:
	      add_partial_namespace (pdi, lowpc, highpc, set_addrmap, cu);
	      break;
	    case DW_TAG_module:
	      add_partial_module (pdi, lowpc, highpc, set_addrmap, cu);
	      break;
	    case DW_TAG_imported_unit:
	      {
		struct dwarf2_per_cu_data *per_cu;

		/* For now we don't handle imported units in type units.  */
		if (cu->per_cu->is_debug_types)
		  {
		    error (_("Dwarf Error: DW_TAG_imported_unit is not"
			     " supported in type units [in module %s]"),
			   objfile_name (cu->objfile));
		  }

		per_cu = dwarf2_find_containing_comp_unit (pdi->d.sect_off,
							   pdi->is_dwz,
							   cu->objfile);

		/* Go read the partial unit, if needed.  */
		if (per_cu->v.psymtab == NULL)
		  process_psymtab_comp_unit (per_cu, 1, cu->language);

		VEC_safe_push (dwarf2_per_cu_ptr,
			       cu->per_cu->imported_symtabs, per_cu);
	      }
	      break;
	    case DW_TAG_imported_declaration:
	      add_partial_symbol (pdi, cu);
	      break;
	    default:
	      break;
	    }
	}

      /* If the die has a sibling, skip to the sibling.  */

      pdi = pdi->die_sibling;
    }
}

/* Functions used to compute the fully scoped name of a partial DIE.

   Normally, this is simple.  For C++, the parent DIE's fully scoped
   name is concatenated with "::" and the partial DIE's name.
   Enumerators are an exception; they use the scope of their parent
   enumeration type, i.e. the name of the enumeration type is not
   prepended to the enumerator.

   There are two complexities.  One is DW_AT_specification; in this
   case "parent" means the parent of the target of the specification,
   instead of the direct parent of the DIE.  The other is compilers
   which do not emit DW_TAG_namespace; in this case we try to guess
   the fully qualified name of structure types from their members'
   linkage names.  This must be done using the DIE's children rather
   than the children of any DW_AT_specification target.  We only need
   to do this for structures at the top level, i.e. if the target of
   any DW_AT_specification (if any; otherwise the DIE itself) does not
   have a parent.  */

/* Compute the scope prefix associated with PDI's parent, in
   compilation unit CU.  The result will be allocated on CU's
   comp_unit_obstack, or a copy of the already allocated PDI->NAME
   field.  NULL is returned if no prefix is necessary.  */
static const char *
partial_die_parent_scope (struct partial_die_info *pdi,
			  struct dwarf2_cu *cu)
{
  const char *grandparent_scope;
  struct partial_die_info *parent, *real_pdi;

  /* We need to look at our parent DIE; if we have a DW_AT_specification,
     then this means the parent of the specification DIE.  */

  real_pdi = pdi;
  while (real_pdi->has_specification)
    real_pdi = find_partial_die (real_pdi->spec_offset,
				 real_pdi->spec_is_dwz, cu);

  parent = real_pdi->die_parent;
  if (parent == NULL)
    return NULL;

  if (parent->scope_set)
    return parent->scope;

  fixup_partial_die (parent, cu);

  grandparent_scope = partial_die_parent_scope (parent, cu);

  /* GCC 4.0 and 4.1 had a bug (PR c++/28460) where they generated bogus
     DW_TAG_namespace DIEs with a name of "::" for the global namespace.
     Work around this problem here.  */
  if (cu->language == language_cplus
      && parent->tag == DW_TAG_namespace
      && strcmp (parent->name, "::") == 0
      && grandparent_scope == NULL)
    {
      parent->scope = NULL;
      parent->scope_set = 1;
      return NULL;
    }

  if (pdi->tag == DW_TAG_enumerator)
    /* Enumerators should not get the name of the enumeration as a prefix.  */
    parent->scope = grandparent_scope;
  else if (parent->tag == DW_TAG_namespace
      || parent->tag == DW_TAG_module
      || parent->tag == DW_TAG_structure_type
      || parent->tag == DW_TAG_class_type
      || parent->tag == DW_TAG_interface_type
      || parent->tag == DW_TAG_union_type
      || parent->tag == DW_TAG_enumeration_type)
    {
      if (grandparent_scope == NULL)
	parent->scope = parent->name;
      else
	parent->scope = typename_concat (&cu->comp_unit_obstack,
					 grandparent_scope,
					 parent->name, 0, cu);
    }
  else
    {
      /* FIXME drow/2004-04-01: What should we be doing with
	 function-local names?  For partial symbols, we should probably be
	 ignoring them.  */
      complaint (&symfile_complaints,
		 _("unhandled containing DIE tag %d for DIE at %d"),
		 parent->tag, to_underlying (pdi->sect_off));
      parent->scope = grandparent_scope;
    }

  parent->scope_set = 1;
  return parent->scope;
}

/* Return the fully scoped name associated with PDI, from compilation unit
   CU.  The result will be allocated with malloc.  */

static char *
partial_die_full_name (struct partial_die_info *pdi,
		       struct dwarf2_cu *cu)
{
  const char *parent_scope;

  /* If this is a template instantiation, we can not work out the
     template arguments from partial DIEs.  So, unfortunately, we have
     to go through the full DIEs.  At least any work we do building
     types here will be reused if full symbols are loaded later.  */
  if (pdi->has_template_arguments)
    {
      fixup_partial_die (pdi, cu);

      if (pdi->name != NULL && strchr (pdi->name, '<') == NULL)
	{
	  struct die_info *die;
	  struct attribute attr;
	  struct dwarf2_cu *ref_cu = cu;

	  /* DW_FORM_ref_addr is using section offset.  */
	  attr.name = (enum dwarf_attribute) 0;
	  attr.form = DW_FORM_ref_addr;
	  attr.u.unsnd = to_underlying (pdi->sect_off);
	  die = follow_die_ref (NULL, &attr, &ref_cu);

	  return xstrdup (dwarf2_full_name (NULL, die, ref_cu));
	}
    }

  parent_scope = partial_die_parent_scope (pdi, cu);
  if (parent_scope == NULL)
    return NULL;
  else
    return typename_concat (NULL, parent_scope, pdi->name, 0, cu);
}

static void
add_partial_symbol (struct partial_die_info *pdi, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  CORE_ADDR addr = 0;
  const char *actual_name = NULL;
  CORE_ADDR baseaddr;
  char *built_actual_name;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  built_actual_name = partial_die_full_name (pdi, cu);
  if (built_actual_name != NULL)
    actual_name = built_actual_name;

  if (actual_name == NULL)
    actual_name = pdi->name;

  switch (pdi->tag)
    {
    case DW_TAG_subprogram:
      addr = gdbarch_adjust_dwarf2_addr (gdbarch, pdi->lowpc + baseaddr);
      if (pdi->is_external || cu->language == language_ada)
	{
          /* brobecker/2007-12-26: Normally, only "external" DIEs are part
             of the global scope.  But in Ada, we want to be able to access
             nested procedures globally.  So all Ada subprograms are stored
             in the global scope.  */
	  add_psymbol_to_list (actual_name, strlen (actual_name),
			       built_actual_name != NULL,
			       VAR_DOMAIN, LOC_BLOCK,
			       &objfile->global_psymbols,
			       addr, cu->language, objfile);
	}
      else
	{
	  add_psymbol_to_list (actual_name, strlen (actual_name),
			       built_actual_name != NULL,
			       VAR_DOMAIN, LOC_BLOCK,
			       &objfile->static_psymbols,
			       addr, cu->language, objfile);
	}

      if (pdi->main_subprogram && actual_name != NULL)
	set_objfile_main_name (objfile, actual_name, cu->language);
      break;
    case DW_TAG_constant:
      {
        struct psymbol_allocation_list *list;

	if (pdi->is_external)
	  list = &objfile->global_psymbols;
	else
	  list = &objfile->static_psymbols;
	add_psymbol_to_list (actual_name, strlen (actual_name),
			     built_actual_name != NULL, VAR_DOMAIN, LOC_STATIC,
			     list, 0, cu->language, objfile);
      }
      break;
    case DW_TAG_variable:
      if (pdi->d.locdesc)
	addr = decode_locdesc (pdi->d.locdesc, cu);

      if (pdi->d.locdesc
	  && addr == 0
	  && !dwarf2_per_objfile->has_section_at_zero)
	{
	  /* A global or static variable may also have been stripped
	     out by the linker if unused, in which case its address
	     will be nullified; do not add such variables into partial
	     symbol table then.  */
	}
      else if (pdi->is_external)
	{
	  /* Global Variable.
	     Don't enter into the minimal symbol tables as there is
	     a minimal symbol table entry from the ELF symbols already.
	     Enter into partial symbol table if it has a location
	     descriptor or a type.
	     If the location descriptor is missing, new_symbol will create
	     a LOC_UNRESOLVED symbol, the address of the variable will then
	     be determined from the minimal symbol table whenever the variable
	     is referenced.
	     The address for the partial symbol table entry is not
	     used by GDB, but it comes in handy for debugging partial symbol
	     table building.  */

	  if (pdi->d.locdesc || pdi->has_type)
	    add_psymbol_to_list (actual_name, strlen (actual_name),
				 built_actual_name != NULL,
				 VAR_DOMAIN, LOC_STATIC,
				 &objfile->global_psymbols,
				 addr + baseaddr,
				 cu->language, objfile);
	}
      else
	{
	  int has_loc = pdi->d.locdesc != NULL;

	  /* Static Variable.  Skip symbols whose value we cannot know (those
	     without location descriptors or constant values).  */
	  if (!has_loc && !pdi->has_const_value)
	    {
	      xfree (built_actual_name);
	      return;
	    }

	  add_psymbol_to_list (actual_name, strlen (actual_name),
			       built_actual_name != NULL,
			       VAR_DOMAIN, LOC_STATIC,
			       &objfile->static_psymbols,
			       has_loc ? addr + baseaddr : (CORE_ADDR) 0,
			       cu->language, objfile);
	}
      break;
    case DW_TAG_typedef:
    case DW_TAG_base_type:
    case DW_TAG_subrange_type:
      add_psymbol_to_list (actual_name, strlen (actual_name),
			   built_actual_name != NULL,
			   VAR_DOMAIN, LOC_TYPEDEF,
			   &objfile->static_psymbols,
			   0, cu->language, objfile);
      break;
    case DW_TAG_imported_declaration:
    case DW_TAG_namespace:
      add_psymbol_to_list (actual_name, strlen (actual_name),
			   built_actual_name != NULL,
			   VAR_DOMAIN, LOC_TYPEDEF,
			   &objfile->global_psymbols,
			   0, cu->language, objfile);
      break;
    case DW_TAG_module:
      add_psymbol_to_list (actual_name, strlen (actual_name),
			   built_actual_name != NULL,
			   MODULE_DOMAIN, LOC_TYPEDEF,
			   &objfile->global_psymbols,
			   0, cu->language, objfile);
      break;
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_enumeration_type:
      /* Skip external references.  The DWARF standard says in the section
         about "Structure, Union, and Class Type Entries": "An incomplete
         structure, union or class type is represented by a structure,
         union or class entry that does not have a byte size attribute
         and that has a DW_AT_declaration attribute."  */
      if (!pdi->has_byte_size && pdi->is_declaration)
	{
	  xfree (built_actual_name);
	  return;
	}

      /* NOTE: carlton/2003-10-07: See comment in new_symbol about
	 static vs. global.  */
      add_psymbol_to_list (actual_name, strlen (actual_name),
			   built_actual_name != NULL,
			   STRUCT_DOMAIN, LOC_TYPEDEF,
			   cu->language == language_cplus
			   ? &objfile->global_psymbols
			   : &objfile->static_psymbols,
			   0, cu->language, objfile);

      break;
    case DW_TAG_enumerator:
      add_psymbol_to_list (actual_name, strlen (actual_name),
			   built_actual_name != NULL,
			   VAR_DOMAIN, LOC_CONST,
			   cu->language == language_cplus
			   ? &objfile->global_psymbols
			   : &objfile->static_psymbols,
			   0, cu->language, objfile);
      break;
    default:
      break;
    }

  xfree (built_actual_name);
}

/* Read a partial die corresponding to a namespace; also, add a symbol
   corresponding to that namespace to the symbol table.  NAMESPACE is
   the name of the enclosing namespace.  */

static void
add_partial_namespace (struct partial_die_info *pdi,
		       CORE_ADDR *lowpc, CORE_ADDR *highpc,
		       int set_addrmap, struct dwarf2_cu *cu)
{
  /* Add a symbol for the namespace.  */

  add_partial_symbol (pdi, cu);

  /* Now scan partial symbols in that namespace.  */

  if (pdi->has_children)
    scan_partial_symbols (pdi->die_child, lowpc, highpc, set_addrmap, cu);
}

/* Read a partial die corresponding to a Fortran module.  */

static void
add_partial_module (struct partial_die_info *pdi, CORE_ADDR *lowpc,
		    CORE_ADDR *highpc, int set_addrmap, struct dwarf2_cu *cu)
{
  /* Add a symbol for the namespace.  */

  add_partial_symbol (pdi, cu);

  /* Now scan partial symbols in that module.  */

  if (pdi->has_children)
    scan_partial_symbols (pdi->die_child, lowpc, highpc, set_addrmap, cu);
}

/* Read a partial die corresponding to a subprogram and create a partial
   symbol for that subprogram.  When the CU language allows it, this
   routine also defines a partial symbol for each nested subprogram
   that this subprogram contains.  If SET_ADDRMAP is true, record the
   covered ranges in the addrmap.  Set *LOWPC and *HIGHPC to the lowest
   and highest PC values found in PDI.

   PDI may also be a lexical block, in which case we simply search
   recursively for subprograms defined inside that lexical block.
   Again, this is only performed when the CU language allows this
   type of definitions.  */

static void
add_partial_subprogram (struct partial_die_info *pdi,
			CORE_ADDR *lowpc, CORE_ADDR *highpc,
			int set_addrmap, struct dwarf2_cu *cu)
{
  if (pdi->tag == DW_TAG_subprogram)
    {
      if (pdi->has_pc_info)
        {
          if (pdi->lowpc < *lowpc)
            *lowpc = pdi->lowpc;
          if (pdi->highpc > *highpc)
            *highpc = pdi->highpc;
	  if (set_addrmap)
	    {
	      struct objfile *objfile = cu->objfile;
	      struct gdbarch *gdbarch = get_objfile_arch (objfile);
	      CORE_ADDR baseaddr;
	      CORE_ADDR highpc;
	      CORE_ADDR lowpc;

	      baseaddr = ANOFFSET (objfile->section_offsets,
				   SECT_OFF_TEXT (objfile));
	      lowpc = gdbarch_adjust_dwarf2_addr (gdbarch,
						  pdi->lowpc + baseaddr);
	      highpc = gdbarch_adjust_dwarf2_addr (gdbarch,
						   pdi->highpc + baseaddr);
	      addrmap_set_empty (objfile->psymtabs_addrmap, lowpc, highpc - 1,
				 cu->per_cu->v.psymtab);
	    }
        }

      if (pdi->has_pc_info || (!pdi->is_external && pdi->may_be_inlined))
	{
          if (!pdi->is_declaration)
	    /* Ignore subprogram DIEs that do not have a name, they are
	       illegal.  Do not emit a complaint at this point, we will
	       do so when we convert this psymtab into a symtab.  */
	    if (pdi->name)
	      add_partial_symbol (pdi, cu);
        }
    }

  if (! pdi->has_children)
    return;

  if (cu->language == language_ada)
    {
      pdi = pdi->die_child;
      while (pdi != NULL)
	{
	  fixup_partial_die (pdi, cu);
	  if (pdi->tag == DW_TAG_subprogram
	      || pdi->tag == DW_TAG_lexical_block)
	    add_partial_subprogram (pdi, lowpc, highpc, set_addrmap, cu);
	  pdi = pdi->die_sibling;
	}
    }
}

/* Read a partial die corresponding to an enumeration type.  */

static void
add_partial_enumeration (struct partial_die_info *enum_pdi,
			 struct dwarf2_cu *cu)
{
  struct partial_die_info *pdi;

  if (enum_pdi->name != NULL)
    add_partial_symbol (enum_pdi, cu);

  pdi = enum_pdi->die_child;
  while (pdi)
    {
      if (pdi->tag != DW_TAG_enumerator || pdi->name == NULL)
	complaint (&symfile_complaints, _("malformed enumerator DIE ignored"));
      else
	add_partial_symbol (pdi, cu);
      pdi = pdi->die_sibling;
    }
}

/* Return the initial uleb128 in the die at INFO_PTR.  */

static unsigned int
peek_abbrev_code (bfd *abfd, const gdb_byte *info_ptr)
{
  unsigned int bytes_read;

  return read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
}

/* Read the initial uleb128 in the die at INFO_PTR in compilation unit CU.
   Return the corresponding abbrev, or NULL if the number is zero (indicating
   an empty DIE).  In either case *BYTES_READ will be set to the length of
   the initial number.  */

static struct abbrev_info *
peek_die_abbrev (const gdb_byte *info_ptr, unsigned int *bytes_read,
		 struct dwarf2_cu *cu)
{
  bfd *abfd = cu->objfile->obfd;
  unsigned int abbrev_number;
  struct abbrev_info *abbrev;

  abbrev_number = read_unsigned_leb128 (abfd, info_ptr, bytes_read);

  if (abbrev_number == 0)
    return NULL;

  abbrev = abbrev_table_lookup_abbrev (cu->abbrev_table, abbrev_number);
  if (!abbrev)
    {
      error (_("Dwarf Error: Could not find abbrev number %d in %s"
	       " at offset 0x%x [in module %s]"),
	     abbrev_number, cu->per_cu->is_debug_types ? "TU" : "CU",
	     to_underlying (cu->header.sect_off), bfd_get_filename (abfd));
    }

  return abbrev;
}

/* Scan the debug information for CU starting at INFO_PTR in buffer BUFFER.
   Returns a pointer to the end of a series of DIEs, terminated by an empty
   DIE.  Any children of the skipped DIEs will also be skipped.  */

static const gdb_byte *
skip_children (const struct die_reader_specs *reader, const gdb_byte *info_ptr)
{
  struct dwarf2_cu *cu = reader->cu;
  struct abbrev_info *abbrev;
  unsigned int bytes_read;

  while (1)
    {
      abbrev = peek_die_abbrev (info_ptr, &bytes_read, cu);
      if (abbrev == NULL)
	return info_ptr + bytes_read;
      else
	info_ptr = skip_one_die (reader, info_ptr + bytes_read, abbrev);
    }
}

/* Scan the debug information for CU starting at INFO_PTR in buffer BUFFER.
   INFO_PTR should point just after the initial uleb128 of a DIE, and the
   abbrev corresponding to that skipped uleb128 should be passed in
   ABBREV.  Returns a pointer to this DIE's sibling, skipping any
   children.  */

static const gdb_byte *
skip_one_die (const struct die_reader_specs *reader, const gdb_byte *info_ptr,
	      struct abbrev_info *abbrev)
{
  unsigned int bytes_read;
  struct attribute attr;
  bfd *abfd = reader->abfd;
  struct dwarf2_cu *cu = reader->cu;
  const gdb_byte *buffer = reader->buffer;
  const gdb_byte *buffer_end = reader->buffer_end;
  unsigned int form, i;

  for (i = 0; i < abbrev->num_attrs; i++)
    {
      /* The only abbrev we care about is DW_AT_sibling.  */
      if (abbrev->attrs[i].name == DW_AT_sibling)
	{
	  read_attribute (reader, &attr, &abbrev->attrs[i], info_ptr);
	  if (attr.form == DW_FORM_ref_addr)
	    complaint (&symfile_complaints,
		       _("ignoring absolute DW_AT_sibling"));
	  else
	    {
	      sect_offset off = dwarf2_get_ref_die_offset (&attr);
	      const gdb_byte *sibling_ptr = buffer + to_underlying (off);

	      if (sibling_ptr < info_ptr)
		complaint (&symfile_complaints,
			   _("DW_AT_sibling points backwards"));
	      else if (sibling_ptr > reader->buffer_end)
		dwarf2_section_buffer_overflow_complaint (reader->die_section);
	      else
		return sibling_ptr;
	    }
	}

      /* If it isn't DW_AT_sibling, skip this attribute.  */
      form = abbrev->attrs[i].form;
    skip_attribute:
      switch (form)
	{
	case DW_FORM_ref_addr:
	  /* In DWARF 2, DW_FORM_ref_addr is address sized; in DWARF 3
	     and later it is offset sized.  */
	  if (cu->header.version == 2)
	    info_ptr += cu->header.addr_size;
	  else
	    info_ptr += cu->header.offset_size;
	  break;
	case DW_FORM_GNU_ref_alt:
	  info_ptr += cu->header.offset_size;
	  break;
	case DW_FORM_addr:
	  info_ptr += cu->header.addr_size;
	  break;
	case DW_FORM_data1:
	case DW_FORM_ref1:
	case DW_FORM_flag:
	  info_ptr += 1;
	  break;
	case DW_FORM_flag_present:
	case DW_FORM_implicit_const:
	  break;
	case DW_FORM_data2:
	case DW_FORM_ref2:
	  info_ptr += 2;
	  break;
	case DW_FORM_data4:
	case DW_FORM_ref4:
	  info_ptr += 4;
	  break;
	case DW_FORM_data8:
	case DW_FORM_ref8:
	case DW_FORM_ref_sig8:
	  info_ptr += 8;
	  break;
	case DW_FORM_data16:
	  info_ptr += 16;
	  break;
	case DW_FORM_string:
	  read_direct_string (abfd, info_ptr, &bytes_read);
	  info_ptr += bytes_read;
	  break;
	case DW_FORM_sec_offset:
	case DW_FORM_strp:
	case DW_FORM_GNU_strp_alt:
	  info_ptr += cu->header.offset_size;
	  break;
	case DW_FORM_exprloc:
	case DW_FORM_block:
	  info_ptr += read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
	  info_ptr += bytes_read;
	  break;
	case DW_FORM_block1:
	  info_ptr += 1 + read_1_byte (abfd, info_ptr);
	  break;
	case DW_FORM_block2:
	  info_ptr += 2 + read_2_bytes (abfd, info_ptr);
	  break;
	case DW_FORM_block4:
	  info_ptr += 4 + read_4_bytes (abfd, info_ptr);
	  break;
	case DW_FORM_sdata:
	case DW_FORM_udata:
	case DW_FORM_ref_udata:
	case DW_FORM_GNU_addr_index:
	case DW_FORM_GNU_str_index:
	  info_ptr = safe_skip_leb128 (info_ptr, buffer_end);
	  break;
	case DW_FORM_indirect:
	  form = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
	  info_ptr += bytes_read;
	  /* We need to continue parsing from here, so just go back to
	     the top.  */
	  goto skip_attribute;

	default:
	  error (_("Dwarf Error: Cannot handle %s "
		   "in DWARF reader [in module %s]"),
		 dwarf_form_name (form),
		 bfd_get_filename (abfd));
	}
    }

  if (abbrev->has_children)
    return skip_children (reader, info_ptr);
  else
    return info_ptr;
}

/* Locate ORIG_PDI's sibling.
   INFO_PTR should point to the start of the next DIE after ORIG_PDI.  */

static const gdb_byte *
locate_pdi_sibling (const struct die_reader_specs *reader,
		    struct partial_die_info *orig_pdi,
		    const gdb_byte *info_ptr)
{
  /* Do we know the sibling already?  */

  if (orig_pdi->sibling)
    return orig_pdi->sibling;

  /* Are there any children to deal with?  */

  if (!orig_pdi->has_children)
    return info_ptr;

  /* Skip the children the long way.  */

  return skip_children (reader, info_ptr);
}

/* Expand this partial symbol table into a full symbol table.  SELF is
   not NULL.  */

static void
dwarf2_read_symtab (struct partial_symtab *self,
		    struct objfile *objfile)
{
  if (self->readin)
    {
      warning (_("bug: psymtab for %s is already read in."),
	       self->filename);
    }
  else
    {
      if (info_verbose)
	{
	  printf_filtered (_("Reading in symbols for %s..."),
			   self->filename);
	  gdb_flush (gdb_stdout);
	}

      /* Restore our global data.  */
      dwarf2_per_objfile
	= (struct dwarf2_per_objfile *) objfile_data (objfile,
						      dwarf2_objfile_data_key);

      /* If this psymtab is constructed from a debug-only objfile, the
	 has_section_at_zero flag will not necessarily be correct.  We
	 can get the correct value for this flag by looking at the data
	 associated with the (presumably stripped) associated objfile.  */
      if (objfile->separate_debug_objfile_backlink)
	{
	  struct dwarf2_per_objfile *dpo_backlink
	    = ((struct dwarf2_per_objfile *)
	       objfile_data (objfile->separate_debug_objfile_backlink,
			     dwarf2_objfile_data_key));

	  dwarf2_per_objfile->has_section_at_zero
	    = dpo_backlink->has_section_at_zero;
	}

      dwarf2_per_objfile->reading_partial_symbols = 0;

      psymtab_to_symtab_1 (self);

      /* Finish up the debug error message.  */
      if (info_verbose)
	printf_filtered (_("done.\n"));
    }

  process_cu_includes ();
}

/* Reading in full CUs.  */

/* Add PER_CU to the queue.  */

static void
queue_comp_unit (struct dwarf2_per_cu_data *per_cu,
		 enum language pretend_language)
{
  struct dwarf2_queue_item *item;

  per_cu->queued = 1;
  item = XNEW (struct dwarf2_queue_item);
  item->per_cu = per_cu;
  item->pretend_language = pretend_language;
  item->next = NULL;

  if (dwarf2_queue == NULL)
    dwarf2_queue = item;
  else
    dwarf2_queue_tail->next = item;

  dwarf2_queue_tail = item;
}

/* If PER_CU is not yet queued, add it to the queue.
   If DEPENDENT_CU is non-NULL, it has a reference to PER_CU so add a
   dependency.
   The result is non-zero if PER_CU was queued, otherwise the result is zero
   meaning either PER_CU is already queued or it is already loaded.

   N.B. There is an invariant here that if a CU is queued then it is loaded.
   The caller is required to load PER_CU if we return non-zero.  */

static int
maybe_queue_comp_unit (struct dwarf2_cu *dependent_cu,
		       struct dwarf2_per_cu_data *per_cu,
		       enum language pretend_language)
{
  /* We may arrive here during partial symbol reading, if we need full
     DIEs to process an unusual case (e.g. template arguments).  Do
     not queue PER_CU, just tell our caller to load its DIEs.  */
  if (dwarf2_per_objfile->reading_partial_symbols)
    {
      if (per_cu->cu == NULL || per_cu->cu->dies == NULL)
	return 1;
      return 0;
    }

  /* Mark the dependence relation so that we don't flush PER_CU
     too early.  */
  if (dependent_cu != NULL)
    dwarf2_add_dependence (dependent_cu, per_cu);

  /* If it's already on the queue, we have nothing to do.  */
  if (per_cu->queued)
    return 0;

  /* If the compilation unit is already loaded, just mark it as
     used.  */
  if (per_cu->cu != NULL)
    {
      per_cu->cu->last_used = 0;
      return 0;
    }

  /* Add it to the queue.  */
  queue_comp_unit (per_cu, pretend_language);

  return 1;
}

/* Process the queue.  */

static void
process_queue (void)
{
  struct dwarf2_queue_item *item, *next_item;

  if (dwarf_read_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "Expanding one or more symtabs of objfile %s ...\n",
			  objfile_name (dwarf2_per_objfile->objfile));
    }

  /* The queue starts out with one item, but following a DIE reference
     may load a new CU, adding it to the end of the queue.  */
  for (item = dwarf2_queue; item != NULL; dwarf2_queue = item = next_item)
    {
      if ((dwarf2_per_objfile->using_index
	   ? !item->per_cu->v.quick->compunit_symtab
	   : (item->per_cu->v.psymtab && !item->per_cu->v.psymtab->readin))
	  /* Skip dummy CUs.  */
	  && item->per_cu->cu != NULL)
	{
	  struct dwarf2_per_cu_data *per_cu = item->per_cu;
	  unsigned int debug_print_threshold;
	  char buf[100];

	  if (per_cu->is_debug_types)
	    {
	      struct signatured_type *sig_type =
		(struct signatured_type *) per_cu;

	      sprintf (buf, "TU %s at offset 0x%x",
		       hex_string (sig_type->signature),
		       to_underlying (per_cu->sect_off));
	      /* There can be 100s of TUs.
		 Only print them in verbose mode.  */
	      debug_print_threshold = 2;
	    }
	  else
	    {
	      sprintf (buf, "CU at offset 0x%x",
		       to_underlying (per_cu->sect_off));
	      debug_print_threshold = 1;
	    }

	  if (dwarf_read_debug >= debug_print_threshold)
	    fprintf_unfiltered (gdb_stdlog, "Expanding symtab of %s\n", buf);

	  if (per_cu->is_debug_types)
	    process_full_type_unit (per_cu, item->pretend_language);
	  else
	    process_full_comp_unit (per_cu, item->pretend_language);

	  if (dwarf_read_debug >= debug_print_threshold)
	    fprintf_unfiltered (gdb_stdlog, "Done expanding %s\n", buf);
	}

      item->per_cu->queued = 0;
      next_item = item->next;
      xfree (item);
    }

  dwarf2_queue_tail = NULL;

  if (dwarf_read_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "Done expanding symtabs of %s.\n",
			  objfile_name (dwarf2_per_objfile->objfile));
    }
}

/* Free all allocated queue entries.  This function only releases anything if
   an error was thrown; if the queue was processed then it would have been
   freed as we went along.  */

static void
dwarf2_release_queue (void *dummy)
{
  struct dwarf2_queue_item *item, *last;

  item = dwarf2_queue;
  while (item)
    {
      /* Anything still marked queued is likely to be in an
	 inconsistent state, so discard it.  */
      if (item->per_cu->queued)
	{
	  if (item->per_cu->cu != NULL)
	    free_one_cached_comp_unit (item->per_cu);
	  item->per_cu->queued = 0;
	}

      last = item;
      item = item->next;
      xfree (last);
    }

  dwarf2_queue = dwarf2_queue_tail = NULL;
}

/* Read in full symbols for PST, and anything it depends on.  */

static void
psymtab_to_symtab_1 (struct partial_symtab *pst)
{
  struct dwarf2_per_cu_data *per_cu;
  int i;

  if (pst->readin)
    return;

  for (i = 0; i < pst->number_of_dependencies; i++)
    if (!pst->dependencies[i]->readin
	&& pst->dependencies[i]->user == NULL)
      {
        /* Inform about additional files that need to be read in.  */
        if (info_verbose)
          {
	    /* FIXME: i18n: Need to make this a single string.  */
            fputs_filtered (" ", gdb_stdout);
            wrap_here ("");
            fputs_filtered ("and ", gdb_stdout);
            wrap_here ("");
            printf_filtered ("%s...", pst->dependencies[i]->filename);
            wrap_here ("");     /* Flush output.  */
            gdb_flush (gdb_stdout);
          }
        psymtab_to_symtab_1 (pst->dependencies[i]);
      }

  per_cu = (struct dwarf2_per_cu_data *) pst->read_symtab_private;

  if (per_cu == NULL)
    {
      /* It's an include file, no symbols to read for it.
         Everything is in the parent symtab.  */
      pst->readin = 1;
      return;
    }

  dw2_do_instantiate_symtab (per_cu);
}

/* Trivial hash function for die_info: the hash value of a DIE
   is its offset in .debug_info for this objfile.  */

static hashval_t
die_hash (const void *item)
{
  const struct die_info *die = (const struct die_info *) item;

  return to_underlying (die->sect_off);
}

/* Trivial comparison function for die_info structures: two DIEs
   are equal if they have the same offset.  */

static int
die_eq (const void *item_lhs, const void *item_rhs)
{
  const struct die_info *die_lhs = (const struct die_info *) item_lhs;
  const struct die_info *die_rhs = (const struct die_info *) item_rhs;

  return die_lhs->sect_off == die_rhs->sect_off;
}

/* die_reader_func for load_full_comp_unit.
   This is identical to read_signatured_type_reader,
   but is kept separate for now.  */

static void
load_full_comp_unit_reader (const struct die_reader_specs *reader,
			    const gdb_byte *info_ptr,
			    struct die_info *comp_unit_die,
			    int has_children,
			    void *data)
{
  struct dwarf2_cu *cu = reader->cu;
  enum language *language_ptr = (enum language *) data;

  gdb_assert (cu->die_hash == NULL);
  cu->die_hash =
    htab_create_alloc_ex (cu->header.length / 12,
			  die_hash,
			  die_eq,
			  NULL,
			  &cu->comp_unit_obstack,
			  hashtab_obstack_allocate,
			  dummy_obstack_deallocate);

  if (has_children)
    comp_unit_die->child = read_die_and_siblings (reader, info_ptr,
						  &info_ptr, comp_unit_die);
  cu->dies = comp_unit_die;
  /* comp_unit_die is not stored in die_hash, no need.  */

  /* We try not to read any attributes in this function, because not
     all CUs needed for references have been loaded yet, and symbol
     table processing isn't initialized.  But we have to set the CU language,
     or we won't be able to build types correctly.
     Similarly, if we do not read the producer, we can not apply
     producer-specific interpretation.  */
  prepare_one_comp_unit (cu, cu->dies, *language_ptr);
}

/* Load the DIEs associated with PER_CU into memory.  */

static void
load_full_comp_unit (struct dwarf2_per_cu_data *this_cu,
		     enum language pretend_language)
{
  gdb_assert (! this_cu->is_debug_types);

  init_cutu_and_read_dies (this_cu, NULL, 1, 1,
			   load_full_comp_unit_reader, &pretend_language);
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
  VEC_safe_push (delayed_method_info, cu->method_list, &mi);
}

/* A cleanup for freeing the delayed method list.  */

static void
free_delayed_list (void *ptr)
{
  struct dwarf2_cu *cu = (struct dwarf2_cu *) ptr;
  if (cu->method_list != NULL)
    {
      VEC_free (delayed_method_info, cu->method_list);
      cu->method_list = NULL;
    }
}

/* Compute the physnames of any methods on the CU's method list.

   The computation of method physnames is delayed in order to avoid the
   (bad) condition that one of the method's formal parameters is of an as yet
   incomplete type.  */

static void
compute_delayed_physnames (struct dwarf2_cu *cu)
{
  int i;
  struct delayed_method_info *mi;
  for (i = 0; VEC_iterate (delayed_method_info, cu->method_list, i, mi) ; ++i)
    {
      const char *physname;
      struct fn_fieldlist *fn_flp
	= &TYPE_FN_FIELDLIST (mi->type, mi->fnfield_index);
      physname = dwarf2_physname (mi->name, mi->die, cu);
      TYPE_FN_FIELD_PHYSNAME (fn_flp->fn_fields, mi->index)
	= physname ? physname : "";
    }
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
  char *package_name = NULL;
  struct pending *list;
  int i;

  for (list = global_symbols; list != NULL; list = list->next)
    {
      for (i = 0; i < list->nsyms; ++i)
	{
	  struct symbol *sym = list->symbol[i];

	  if (SYMBOL_LANGUAGE (sym) == language_go
	      && SYMBOL_CLASS (sym) == LOC_BLOCK)
	    {
	      char *this_package_name = go_symbol_package_name (sym);

	      if (this_package_name == NULL)
		continue;
	      if (package_name == NULL)
		package_name = this_package_name;
	      else
		{
		  if (strcmp (package_name, this_package_name) != 0)
		    complaint (&symfile_complaints,
			       _("Symtab %s has objects from two different Go packages: %s and %s"),
			       (symbol_symtab (sym) != NULL
				? symtab_to_filename_for_display
				    (symbol_symtab (sym))
				: objfile_name (cu->objfile)),
			       this_package_name, package_name);
		  xfree (this_package_name);
		}
	    }
	}
    }

  if (package_name != NULL)
    {
      struct objfile *objfile = cu->objfile;
      const char *saved_package_name
	= (const char *) obstack_copy0 (&objfile->per_bfd->storage_obstack,
					package_name,
					strlen (package_name));
      struct type *type = init_type (objfile, TYPE_CODE_MODULE, 0,
				     saved_package_name);
      struct symbol *sym;

      TYPE_TAG_NAME (type) = TYPE_NAME (type);

      sym = allocate_symbol (objfile);
      SYMBOL_SET_LANGUAGE (sym, language_go, &objfile->objfile_obstack);
      SYMBOL_SET_NAMES (sym, saved_package_name,
			strlen (saved_package_name), 0, objfile);
      /* This is not VAR_DOMAIN because we want a way to ensure a lookup of,
	 e.g., "main" finds the "main" module and not C's main().  */
      SYMBOL_DOMAIN (sym) = STRUCT_DOMAIN;
      SYMBOL_ACLASS_INDEX (sym) = LOC_TYPEDEF;
      SYMBOL_TYPE (sym) = type;

      add_symbol_to_list (sym, &global_symbols);

      xfree (package_name);
    }
}

/* Return the symtab for PER_CU.  This works properly regardless of
   whether we're using the index or psymtabs.  */

static struct compunit_symtab *
get_compunit_symtab (struct dwarf2_per_cu_data *per_cu)
{
  return (dwarf2_per_objfile->using_index
	  ? per_cu->v.quick->compunit_symtab
	  : per_cu->v.psymtab->compunit_symtab);
}

/* A helper function for computing the list of all symbol tables
   included by PER_CU.  */

static void
recursively_compute_inclusions (VEC (compunit_symtab_ptr) **result,
				htab_t all_children, htab_t all_type_symtabs,
				struct dwarf2_per_cu_data *per_cu,
				struct compunit_symtab *immediate_parent)
{
  void **slot;
  int ix;
  struct compunit_symtab *cust;
  struct dwarf2_per_cu_data *iter;

  slot = htab_find_slot (all_children, per_cu, INSERT);
  if (*slot != NULL)
    {
      /* This inclusion and its children have been processed.  */
      return;
    }

  *slot = per_cu;
  /* Only add a CU if it has a symbol table.  */
  cust = get_compunit_symtab (per_cu);
  if (cust != NULL)
    {
      /* If this is a type unit only add its symbol table if we haven't
	 seen it yet (type unit per_cu's can share symtabs).  */
      if (per_cu->is_debug_types)
	{
	  slot = htab_find_slot (all_type_symtabs, cust, INSERT);
	  if (*slot == NULL)
	    {
	      *slot = cust;
	      VEC_safe_push (compunit_symtab_ptr, *result, cust);
	      if (cust->user == NULL)
		cust->user = immediate_parent;
	    }
	}
      else
	{
	  VEC_safe_push (compunit_symtab_ptr, *result, cust);
	  if (cust->user == NULL)
	    cust->user = immediate_parent;
	}
    }

  for (ix = 0;
       VEC_iterate (dwarf2_per_cu_ptr, per_cu->imported_symtabs, ix, iter);
       ++ix)
    {
      recursively_compute_inclusions (result, all_children,
				      all_type_symtabs, iter, cust);
    }
}

/* Compute the compunit_symtab 'includes' fields for the compunit_symtab of
   PER_CU.  */

static void
compute_compunit_symtab_includes (struct dwarf2_per_cu_data *per_cu)
{
  gdb_assert (! per_cu->is_debug_types);

  if (!VEC_empty (dwarf2_per_cu_ptr, per_cu->imported_symtabs))
    {
      int ix, len;
      struct dwarf2_per_cu_data *per_cu_iter;
      struct compunit_symtab *compunit_symtab_iter;
      VEC (compunit_symtab_ptr) *result_symtabs = NULL;
      htab_t all_children, all_type_symtabs;
      struct compunit_symtab *cust = get_compunit_symtab (per_cu);

      /* If we don't have a symtab, we can just skip this case.  */
      if (cust == NULL)
	return;

      all_children = htab_create_alloc (1, htab_hash_pointer, htab_eq_pointer,
					NULL, xcalloc, xfree);
      all_type_symtabs = htab_create_alloc (1, htab_hash_pointer, htab_eq_pointer,
					    NULL, xcalloc, xfree);

      for (ix = 0;
	   VEC_iterate (dwarf2_per_cu_ptr, per_cu->imported_symtabs,
			ix, per_cu_iter);
	   ++ix)
	{
	  recursively_compute_inclusions (&result_symtabs, all_children,
					  all_type_symtabs, per_cu_iter,
					  cust);
	}

      /* Now we have a transitive closure of all the included symtabs.  */
      len = VEC_length (compunit_symtab_ptr, result_symtabs);
      cust->includes
	= XOBNEWVEC (&dwarf2_per_objfile->objfile->objfile_obstack,
		     struct compunit_symtab *, len + 1);
      for (ix = 0;
	   VEC_iterate (compunit_symtab_ptr, result_symtabs, ix,
			compunit_symtab_iter);
	   ++ix)
	cust->includes[ix] = compunit_symtab_iter;
      cust->includes[len] = NULL;

      VEC_free (compunit_symtab_ptr, result_symtabs);
      htab_delete (all_children);
      htab_delete (all_type_symtabs);
    }
}

/* Compute the 'includes' field for the symtabs of all the CUs we just
   read.  */

static void
process_cu_includes (void)
{
  int ix;
  struct dwarf2_per_cu_data *iter;

  for (ix = 0;
       VEC_iterate (dwarf2_per_cu_ptr, dwarf2_per_objfile->just_read_cus,
		    ix, iter);
       ++ix)
    {
      if (! iter->is_debug_types)
	compute_compunit_symtab_includes (iter);
    }

  VEC_free (dwarf2_per_cu_ptr, dwarf2_per_objfile->just_read_cus);
}

/* Generate full symbol information for PER_CU, whose DIEs have
   already been loaded into memory.  */

static void
process_full_comp_unit (struct dwarf2_per_cu_data *per_cu,
			enum language pretend_language)
{
  struct dwarf2_cu *cu = per_cu->cu;
  struct objfile *objfile = per_cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  CORE_ADDR lowpc, highpc;
  struct compunit_symtab *cust;
  struct cleanup *back_to, *delayed_list_cleanup;
  CORE_ADDR baseaddr;
  struct block *static_block;
  CORE_ADDR addr;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  buildsym_init ();
  back_to = make_cleanup (really_free_pendings, NULL);
  delayed_list_cleanup = make_cleanup (free_delayed_list, cu);

  cu->list_in_scope = &file_symbols;

  cu->language = pretend_language;
  cu->language_defn = language_def (cu->language);

  /* Do line number decoding in read_file_scope () */
  process_die (cu->dies, cu);

  /* For now fudge the Go package.  */
  if (cu->language == language_go)
    fixup_go_packaging (cu);

  /* Now that we have processed all the DIEs in the CU, all the types 
     should be complete, and it should now be safe to compute all of the
     physnames.  */
  compute_delayed_physnames (cu);
  do_cleanups (delayed_list_cleanup);

  /* Some compilers don't define a DW_AT_high_pc attribute for the
     compilation unit.  If the DW_AT_high_pc is missing, synthesize
     it, by scanning the DIE's below the compilation unit.  */
  get_scope_pc_bounds (cu->dies, &lowpc, &highpc, cu);

  addr = gdbarch_adjust_dwarf2_addr (gdbarch, highpc + baseaddr);
  static_block = end_symtab_get_static_block (addr, 0, 1);

  /* If the comp unit has DW_AT_ranges, it may have discontiguous ranges.
     Also, DW_AT_ranges may record ranges not belonging to any child DIEs
     (such as virtual method tables).  Record the ranges in STATIC_BLOCK's
     addrmap to help ensure it has an accurate map of pc values belonging to
     this comp unit.  */
  dwarf2_record_block_ranges (cu->dies, static_block, baseaddr, cu);

  cust = end_symtab_from_static_block (static_block,
				       SECT_OFF_TEXT (objfile), 0);

  if (cust != NULL)
    {
      int gcc_4_minor = producer_is_gcc_ge_4 (cu->producer);

      /* Set symtab language to language from DW_AT_language.  If the
	 compilation is from a C file generated by language preprocessors, do
	 not set the language if it was already deduced by start_subfile.  */
      if (!(cu->language == language_c
	    && COMPUNIT_FILETABS (cust)->language != language_unknown))
	COMPUNIT_FILETABS (cust)->language = cu->language;

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
      if (cu->has_loclist && gcc_4_minor >= 5)
	cust->locations_valid = 1;

      if (gcc_4_minor >= 5)
	cust->epilogue_unwind_valid = 1;

      cust->call_site_htab = cu->call_site_htab;
    }

  if (dwarf2_per_objfile->using_index)
    per_cu->v.quick->compunit_symtab = cust;
  else
    {
      struct partial_symtab *pst = per_cu->v.psymtab;
      pst->compunit_symtab = cust;
      pst->readin = 1;
    }

  /* Push it for inclusion processing later.  */
  VEC_safe_push (dwarf2_per_cu_ptr, dwarf2_per_objfile->just_read_cus, per_cu);

  do_cleanups (back_to);
}

/* Generate full symbol information for type unit PER_CU, whose DIEs have
   already been loaded into memory.  */

static void
process_full_type_unit (struct dwarf2_per_cu_data *per_cu,
			enum language pretend_language)
{
  struct dwarf2_cu *cu = per_cu->cu;
  struct objfile *objfile = per_cu->objfile;
  struct compunit_symtab *cust;
  struct cleanup *back_to, *delayed_list_cleanup;
  struct signatured_type *sig_type;

  gdb_assert (per_cu->is_debug_types);
  sig_type = (struct signatured_type *) per_cu;

  buildsym_init ();
  back_to = make_cleanup (really_free_pendings, NULL);
  delayed_list_cleanup = make_cleanup (free_delayed_list, cu);

  cu->list_in_scope = &file_symbols;

  cu->language = pretend_language;
  cu->language_defn = language_def (cu->language);

  /* The symbol tables are set up in read_type_unit_scope.  */
  process_die (cu->dies, cu);

  /* For now fudge the Go package.  */
  if (cu->language == language_go)
    fixup_go_packaging (cu);

  /* Now that we have processed all the DIEs in the CU, all the types 
     should be complete, and it should now be safe to compute all of the
     physnames.  */
  compute_delayed_physnames (cu);
  do_cleanups (delayed_list_cleanup);

  /* TUs share symbol tables.
     If this is the first TU to use this symtab, complete the construction
     of it with end_expandable_symtab.  Otherwise, complete the addition of
     this TU's symbols to the existing symtab.  */
  if (sig_type->type_unit_group->compunit_symtab == NULL)
    {
      cust = end_expandable_symtab (0, SECT_OFF_TEXT (objfile));
      sig_type->type_unit_group->compunit_symtab = cust;

      if (cust != NULL)
	{
	  /* Set symtab language to language from DW_AT_language.  If the
	     compilation is from a C file generated by language preprocessors,
	     do not set the language if it was already deduced by
	     start_subfile.  */
	  if (!(cu->language == language_c
		&& COMPUNIT_FILETABS (cust)->language != language_c))
	    COMPUNIT_FILETABS (cust)->language = cu->language;
	}
    }
  else
    {
      augment_type_symtab ();
      cust = sig_type->type_unit_group->compunit_symtab;
    }

  if (dwarf2_per_objfile->using_index)
    per_cu->v.quick->compunit_symtab = cust;
  else
    {
      struct partial_symtab *pst = per_cu->v.psymtab;
      pst->compunit_symtab = cust;
      pst->readin = 1;
    }

  do_cleanups (back_to);
}

/* Process an imported unit DIE.  */

static void
process_imported_unit_die (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  /* For now we don't handle imported units in type units.  */
  if (cu->per_cu->is_debug_types)
    {
      error (_("Dwarf Error: DW_TAG_imported_unit is not"
	       " supported in type units [in module %s]"),
	     objfile_name (cu->objfile));
    }

  attr = dwarf2_attr (die, DW_AT_import, cu);
  if (attr != NULL)
    {
      sect_offset sect_off = dwarf2_get_ref_die_offset (attr);
      bool is_dwz = (attr->form == DW_FORM_GNU_ref_alt || cu->per_cu->is_dwz);
      dwarf2_per_cu_data *per_cu
	= dwarf2_find_containing_comp_unit (sect_off, is_dwz, cu->objfile);

      /* If necessary, add it to the queue and load its DIEs.  */
      if (maybe_queue_comp_unit (cu, per_cu, cu->language))
	load_full_comp_unit (per_cu, cu->language);

      VEC_safe_push (dwarf2_per_cu_ptr, cu->per_cu->imported_symtabs,
		     per_cu);
    }
}

/* Reset the in_process bit of a die.  */

static void
reset_die_in_process (void *arg)
{
  struct die_info *die = (struct die_info *) arg;

  die->in_process = 0;
}

/* Process a die and its children.  */

static void
process_die (struct die_info *die, struct dwarf2_cu *cu)
{
  struct cleanup *in_process;

  /* We should only be processing those not already in process.  */
  gdb_assert (!die->in_process);

  die->in_process = 1;
  in_process = make_cleanup (reset_die_in_process,die);

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
    case DW_TAG_array_type:
    case DW_TAG_pointer_type:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_reference_type:
    case DW_TAG_rvalue_reference_type:
    case DW_TAG_string_type:
      break;

    case DW_TAG_base_type:
    case DW_TAG_subrange_type:
    case DW_TAG_typedef:
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
      cu->processing_has_namespace_info = 1;
      read_namespace (die, cu);
      break;
    case DW_TAG_module:
      cu->processing_has_namespace_info = 1;
      read_module (die, cu);
      break;
    case DW_TAG_imported_declaration:
      cu->processing_has_namespace_info = 1;
      if (read_namespace_alias (die, cu))
	break;
      /* The declaration is not a global namespace alias: fall through.  */
    case DW_TAG_imported_module:
      cu->processing_has_namespace_info = 1;
      if (die->child != NULL && (die->tag == DW_TAG_imported_declaration
				 || cu->language != language_fortran))
	complaint (&symfile_complaints, _("Tag '%s' has unexpected children"),
		   dwarf_tag_name (die->tag));
      read_import_statement (die, cu);
      break;

    case DW_TAG_imported_unit:
      process_imported_unit_die (die, cu);
      break;

    default:
      new_symbol (die, NULL, cu);
      break;
    }

  do_cleanups (in_process);
}

/* DWARF name computation.  */

/* A helper function for dwarf2_compute_name which determines whether DIE
   needs to have the name of the scope prepended to the name listed in the
   die.  */

static int
die_needs_namespace (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  switch (die->tag)
    {
    case DW_TAG_namespace:
    case DW_TAG_typedef:
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_enumerator:
    case DW_TAG_subprogram:
    case DW_TAG_inlined_subroutine:
    case DW_TAG_member:
    case DW_TAG_imported_declaration:
      return 1;

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

/* Compute the fully qualified name of DIE in CU.  If PHYSNAME is nonzero,
   compute the physname for the object, which include a method's:
   - formal parameters (C++),
   - receiver type (Go),

   The term "physname" is a bit confusing.
   For C++, for example, it is the demangled name.
   For Go, for example, it's the mangled name.

   For Ada, return the DIE's linkage name rather than the fully qualified
   name.  PHYSNAME is ignored..

   The result is allocated on the objfile_obstack and canonicalized.  */

static const char *
dwarf2_compute_name (const char *name,
		     struct die_info *die, struct dwarf2_cu *cu,
		     int physname)
{
  struct objfile *objfile = cu->objfile;

  if (name == NULL)
    name = dwarf2_name (die, cu);

  /* For Fortran GDB prefers DW_AT_*linkage_name for the physname if present
     but otherwise compute it by typename_concat inside GDB.
     FIXME: Actually this is not really true, or at least not always true.
     It's all very confusing.  SYMBOL_SET_NAMES doesn't try to demangle
     Fortran names because there is no mangling standard.  So new_symbol_full
     will set the demangled name to the result of dwarf2_full_name, and it is
     the demangled name that GDB uses if it exists.  */
  if (cu->language == language_ada
      || (cu->language == language_fortran && physname))
    {
      /* For Ada unit, we prefer the linkage name over the name, as
	 the former contains the exported name, which the user expects
	 to be able to reference.  Ideally, we want the user to be able
	 to reference this entity using either natural or linkage name,
	 but we haven't started looking at this enhancement yet.  */
      const char *linkage_name;

      linkage_name = dwarf2_string_attr (die, DW_AT_linkage_name, cu);
      if (linkage_name == NULL)
	linkage_name = dwarf2_string_attr (die, DW_AT_MIPS_linkage_name, cu);
      if (linkage_name != NULL)
	return linkage_name;
    }

  /* These are the only languages we know how to qualify names in.  */
  if (name != NULL
      && (cu->language == language_cplus
	  || cu->language == language_fortran || cu->language == language_d
	  || cu->language == language_rust))
    {
      if (die_needs_namespace (die, cu))
	{
	  long length;
	  const char *prefix;
	  const char *canonical_name = NULL;

	  string_file buf;

	  prefix = determine_prefix (die, cu);
	  if (*prefix != '\0')
	    {
	      char *prefixed_name = typename_concat (NULL, prefix, name,
						     physname, cu);

	      buf.puts (prefixed_name);
	      xfree (prefixed_name);
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

	  if (cu->language == language_cplus && strchr (name, '<') == NULL)
	    {
	      struct attribute *attr;
	      struct die_info *child;
	      int first = 1;

	      die->building_fullname = 1;

	      for (child = die->child; child != NULL; child = child->sibling)
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
		      complaint (&symfile_complaints,
				 _("template parameter missing DW_AT_type"));
		      buf.puts ("UNKNOWN_TYPE");
		      continue;
		    }
		  type = die_type (child, cu);

		  if (child->tag == DW_TAG_template_type_param)
		    {
		      c_print_type (type, "", &buf, -1, 0, &type_print_raw_options);
		      continue;
		    }

		  attr = dwarf2_attr (child, DW_AT_const_value, cu);
		  if (attr == NULL)
		    {
		      complaint (&symfile_complaints,
				 _("template parameter missing "
				   "DW_AT_const_value"));
		      buf.puts ("UNKNOWN_VALUE");
		      continue;
		    }

		  dwarf2_const_value_attr (attr, type, name,
					   &cu->comp_unit_obstack, cu,
					   &value, &bytes, &baton);

		  if (TYPE_NOSIGN (type))
		    /* GDB prints characters as NUMBER 'CHAR'.  If that's
		       changed, this can use value_print instead.  */
		    c_printchar (value, type, &buf);
		  else
		    {
		      struct value_print_options opts;

		      if (baton != NULL)
			v = dwarf2_evaluate_loc_desc (type, NULL,
						      baton->data,
						      baton->size,
						      baton->per_cu);
		      else if (bytes != NULL)
			{
			  v = allocate_value (type);
			  memcpy (value_contents_writeable (v), bytes,
				  TYPE_LENGTH (type));
			}
		      else
			v = value_from_longest (type, value);

		      /* Specify decimal so that we do not depend on
			 the radix.  */
		      get_formatted_print_options (&opts, 'd');
		      opts.raw = 1;
		      value_print (v, &buf, &opts);
		      release_value (v);
		      value_free (v);
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
	      && cu->language == language_cplus)
	    {
	      struct type *type = read_type_die (die, cu);

	      c_type_print_args (type, &buf, 1, cu->language,
				 &type_print_raw_options);

	      if (cu->language == language_cplus)
		{
		  /* Assume that an artificial first parameter is
		     "this", but do not crash if it is not.  RealView
		     marks unnamed (and thus unused) parameters as
		     artificial; there is no way to differentiate
		     the two cases.  */
		  if (TYPE_NFIELDS (type) > 0
		      && TYPE_FIELD_ARTIFICIAL (type, 0)
		      && TYPE_CODE (TYPE_FIELD_TYPE (type, 0)) == TYPE_CODE_PTR
		      && TYPE_CONST (TYPE_TARGET_TYPE (TYPE_FIELD_TYPE (type,
									0))))
		    buf.puts (" const");
		}
	    }

	  const std::string &intermediate_name = buf.string ();

	  if (cu->language == language_cplus)
	    canonical_name
	      = dwarf2_canonicalize_name (intermediate_name.c_str (), cu,
					  &objfile->per_bfd->storage_obstack);

	  /* If we only computed INTERMEDIATE_NAME, or if
	     INTERMEDIATE_NAME is already canonical, then we need to
	     copy it to the appropriate obstack.  */
	  if (canonical_name == NULL || canonical_name == intermediate_name.c_str ())
	    name = ((const char *)
		    obstack_copy0 (&objfile->per_bfd->storage_obstack,
				   intermediate_name.c_str (),
				   intermediate_name.length ()));
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
   allocated on the objfile_objstack or NULL if the DIE does not have a
   name.

   The output string will be canonicalized (if C++).  */

static const char *
dwarf2_physname (const char *name, struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  const char *retval, *mangled = NULL, *canon = NULL;
  struct cleanup *back_to;
  int need_copy = 1;

  /* In this case dwarf2_compute_name is just a shortcut not building anything
     on its own.  */
  if (!die_needs_namespace (die, cu))
    return dwarf2_compute_name (name, die, cu, 1);

  back_to = make_cleanup (null_cleanup, NULL);

  mangled = dwarf2_string_attr (die, DW_AT_linkage_name, cu);
  if (mangled == NULL)
    mangled = dwarf2_string_attr (die, DW_AT_MIPS_linkage_name, cu);

  /* rustc emits invalid values for DW_AT_linkage_name.  Ignore these.
     See https://github.com/rust-lang/rust/issues/32925.  */
  if (cu->language == language_rust && mangled != NULL
      && strchr (mangled, '{') != NULL)
    mangled = NULL;

  /* DW_AT_linkage_name is missing in some cases - depend on what GDB
     has computed.  */
  if (mangled != NULL)
    {
      char *demangled;

      /* Use DMGL_RET_DROP for C++ template functions to suppress their return
	 type.  It is easier for GDB users to search for such functions as
	 `name(params)' than `long name(params)'.  In such case the minimal
	 symbol names do not match the full symbol names but for template
	 functions there is never a need to look up their definition from their
	 declaration so the only disadvantage remains the minimal symbol
	 variant `long name(params)' does not have the proper inferior type.
	 */

      if (cu->language == language_go)
	{
	  /* This is a lie, but we already lie to the caller new_symbol_full.
	     new_symbol_full assumes we return the mangled name.
	     This just undoes that lie until things are cleaned up.  */
	  demangled = NULL;
	}
      else
	{
	  demangled = gdb_demangle (mangled,
				    (DMGL_PARAMS | DMGL_ANSI | DMGL_RET_DROP));
	}
      if (demangled)
	{
	  make_cleanup (xfree, demangled);
	  canon = demangled;
	}
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

	  complaint (&symfile_complaints,
		     _("Computed physname <%s> does not match demangled <%s> "
		       "(from linkage <%s>) - DIE at 0x%x [in module %s]"),
		     physname, canon, mangled, to_underlying (die->sect_off),
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
    retval = ((const char *)
	      obstack_copy0 (&objfile->per_bfd->storage_obstack,
			     retval, strlen (retval)));

  do_cleanups (back_to);
  return retval;
}

/* Inspect DIE in CU for a namespace alias.  If one exists, record
   a new symbol for it.

   Returns 1 if a namespace alias was recorded, 0 otherwise.  */

static int
read_namespace_alias (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  /* If the die does not have a name, this is not a namespace
     alias.  */
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
	  complaint (&symfile_complaints,
		     _("DIE at 0x%x has too many recursively imported "
		       "declarations"), to_underlying (d->sect_off));
	  return 0;
	}

      if (attr != NULL)
	{
	  struct type *type;
	  sect_offset sect_off = dwarf2_get_ref_die_offset (attr);

	  type = get_die_type_at_offset (sect_off, cu->per_cu);
	  if (type != NULL && TYPE_CODE (type) == TYPE_CODE_NAMESPACE)
	    {
	      /* This declaration is a global namespace alias.  Add
		 a symbol for it whose type is the aliased namespace.  */
	      new_symbol (die, type, cu);
	      return 1;
	    }
	}
    }

  return 0;
}

/* Return the using directives repository (global or local?) to use in the
   current context for LANGUAGE.

   For Ada, imported declarations can materialize renamings, which *may* be
   global.  However it is impossible (for now?) in DWARF to distinguish
   "external" imported declarations and "static" ones.  As all imported
   declarations seem to be static in all other languages, make them all CU-wide
   global only in Ada.  */

static struct using_direct **
using_directives (enum language language)
{
  if (language == language_ada && context_stack_depth == 0)
    return &global_using_directives;
  else
    return &local_using_directives;
}

/* Read the import statement specified by the given die and record it.  */

static void
read_import_statement (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct attribute *import_attr;
  struct die_info *imported_die, *child_die;
  struct dwarf2_cu *imported_cu;
  const char *imported_name;
  const char *imported_name_prefix;
  const char *canonical_name;
  const char *import_alias;
  const char *imported_declaration = NULL;
  const char *import_prefix;
  VEC (const_char_ptr) *excludes = NULL;
  struct cleanup *cleanups;

  import_attr = dwarf2_attr (die, DW_AT_import, cu);
  if (import_attr == NULL)
    {
      complaint (&symfile_complaints, _("Tag '%s' has no DW_AT_import"),
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
    canonical_name = obconcat (&objfile->objfile_obstack,
			       imported_name_prefix,
			       (cu->language == language_d ? "." : "::"),
			       imported_name, (char *) NULL);
  else
    canonical_name = imported_name;

  cleanups = make_cleanup (VEC_cleanup (const_char_ptr), &excludes);

  if (die->tag == DW_TAG_imported_module && cu->language == language_fortran)
    for (child_die = die->child; child_die && child_die->tag;
	 child_die = sibling_die (child_die))
      {
	/* DWARF-4: A Fortran use statement with a “rename list” may be
	   represented by an imported module entry with an import attribute
	   referring to the module and owned entries corresponding to those
	   entities that are renamed as part of being imported.  */

	if (child_die->tag != DW_TAG_imported_declaration)
	  {
	    complaint (&symfile_complaints,
		       _("child DW_TAG_imported_declaration expected "
			 "- DIE at 0x%x [in module %s]"),
		       to_underlying (child_die->sect_off), objfile_name (objfile));
	    continue;
	  }

	import_attr = dwarf2_attr (child_die, DW_AT_import, cu);
	if (import_attr == NULL)
	  {
	    complaint (&symfile_complaints, _("Tag '%s' has no DW_AT_import"),
		       dwarf_tag_name (child_die->tag));
	    continue;
	  }

	imported_cu = cu;
	imported_die = follow_die_ref_or_sig (child_die, import_attr,
					      &imported_cu);
	imported_name = dwarf2_name (imported_die, imported_cu);
	if (imported_name == NULL)
	  {
	    complaint (&symfile_complaints,
		       _("child DW_TAG_imported_declaration has unknown "
			 "imported name - DIE at 0x%x [in module %s]"),
		       to_underlying (child_die->sect_off), objfile_name (objfile));
	    continue;
	  }

	VEC_safe_push (const_char_ptr, excludes, imported_name);

	process_die (child_die, cu);
      }

  add_using_directive (using_directives (cu->language),
		       import_prefix,
		       canonical_name,
		       import_alias,
		       imported_declaration,
		       excludes,
		       0,
		       &objfile->objfile_obstack);

  do_cleanups (cleanups);
}

/* Check for possibly missing DW_AT_comp_dir with relative .debug_line
   directory paths.  GCC SVN r127613 (new option -fdebug-prefix-map) fixed
   this, it was first present in GCC release 4.3.0.  */

static int
producer_is_gcc_lt_4_3 (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_gcc_lt_4_3;
}

static file_and_directory
find_file_and_directory (struct die_info *die, struct dwarf2_cu *cu)
{
  file_and_directory res;

  /* Find the filename.  Do not use dwarf2_name here, since the filename
     is not a source language identifier.  */
  res.name = dwarf2_string_attr (die, DW_AT_name, cu);
  res.comp_dir = dwarf2_string_attr (die, DW_AT_comp_dir, cu);

  if (res.comp_dir == NULL
      && producer_is_gcc_lt_4_3 (cu) && res.name != NULL
      && IS_ABSOLUTE_PATH (res.name))
    {
      res.comp_dir_storage = ldirname (res.name);
      if (!res.comp_dir_storage.empty ())
	res.comp_dir = res.comp_dir_storage.c_str ();
    }
  if (res.comp_dir != NULL)
    {
      /* Irix 6.2 native cc prepends <machine>.: to the compilation
	 directory, get rid of it.  */
      const char *cp = strchr (res.comp_dir, ':');

      if (cp && cp != res.comp_dir && cp[-1] == '.' && cp[1] == '/')
	res.comp_dir = cp + 1;
    }

  if (res.name == NULL)
    res.name = "<unknown>";

  return res;
}

/* Handle DW_AT_stmt_list for a compilation unit.
   DIE is the DW_TAG_compile_unit die for CU.
   COMP_DIR is the compilation directory.  LOWPC is passed to
   dwarf_decode_lines.  See dwarf_decode_lines comments about it.  */

static void
handle_DW_AT_stmt_list (struct die_info *die, struct dwarf2_cu *cu,
			const char *comp_dir, CORE_ADDR lowpc) /* ARI: editCase function */
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct attribute *attr;
  struct line_header line_header_local;
  hashval_t line_header_local_hash;
  unsigned u;
  void **slot;
  int decode_mapping;

  gdb_assert (! cu->per_cu->is_debug_types);

  attr = dwarf2_attr (die, DW_AT_stmt_list, cu);
  if (attr == NULL)
    return;

  sect_offset line_offset = (sect_offset) DW_UNSND (attr);

  /* The line header hash table is only created if needed (it exists to
     prevent redundant reading of the line table for partial_units).
     If we're given a partial_unit, we'll need it.  If we're given a
     compile_unit, then use the line header hash table if it's already
     created, but don't create one just yet.  */

  if (dwarf2_per_objfile->line_header_hash == NULL
      && die->tag == DW_TAG_partial_unit)
    {
      dwarf2_per_objfile->line_header_hash
	= htab_create_alloc_ex (127, line_header_hash_voidp,
				line_header_eq_voidp,
				free_line_header_voidp,
				&objfile->objfile_obstack,
				hashtab_obstack_allocate,
				dummy_obstack_deallocate);
    }

  line_header_local.sect_off = line_offset;
  line_header_local.offset_in_dwz = cu->per_cu->is_dwz;
  line_header_local_hash = line_header_hash (&line_header_local);
  if (dwarf2_per_objfile->line_header_hash != NULL)
    {
      slot = htab_find_slot_with_hash (dwarf2_per_objfile->line_header_hash,
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
  line_header_up lh = dwarf_decode_line_header (line_offset, cu);
  if (lh == NULL)
    return;
  cu->line_header = lh.get ();

  if (dwarf2_per_objfile->line_header_hash == NULL)
    slot = NULL;
  else
    {
      slot = htab_find_slot_with_hash (dwarf2_per_objfile->line_header_hash,
				       &line_header_local,
				       line_header_local_hash, INSERT);
      gdb_assert (slot != NULL);
    }
  if (slot != NULL && *slot == NULL)
    {
      /* This newly decoded line number information unit will be owned
	 by line_header_hash hash table.  */
      *slot = cu->line_header;
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
  dwarf_decode_lines (cu->line_header, comp_dir, cu, NULL, lowpc,
		      decode_mapping);

  lh.release ();
}

/* Process DW_TAG_compile_unit or DW_TAG_partial_unit.  */

static void
read_file_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  CORE_ADDR lowpc = ((CORE_ADDR) -1);
  CORE_ADDR highpc = ((CORE_ADDR) 0);
  struct attribute *attr;
  struct die_info *child_die;
  CORE_ADDR baseaddr;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  get_scope_pc_bounds (die, &lowpc, &highpc, cu);

  /* If we didn't find a lowpc, set it to highpc to avoid complaints
     from finish_block.  */
  if (lowpc == ((CORE_ADDR) -1))
    lowpc = highpc;
  lowpc = gdbarch_adjust_dwarf2_addr (gdbarch, lowpc + baseaddr);

  file_and_directory fnd = find_file_and_directory (die, cu);

  prepare_one_comp_unit (cu, die, cu->language);

  /* The XLCL doesn't generate DW_LANG_OpenCL because this attribute is not
     standardised yet.  As a workaround for the language detection we fall
     back to the DW_AT_producer string.  */
  if (cu->producer && strstr (cu->producer, "IBM XL C for OpenCL") != NULL)
    cu->language = language_opencl;

  /* Similar hack for Go.  */
  if (cu->producer && strstr (cu->producer, "GNU Go ") != NULL)
    set_cu_language (DW_LANG_Go, cu);

  dwarf2_start_symtab (cu, fnd.name, fnd.comp_dir, lowpc);

  /* Decode line number information if present.  We do this before
     processing child DIEs, so that the line header table is available
     for DW_AT_decl_file.  */
  handle_DW_AT_stmt_list (die, cu, fnd.comp_dir, lowpc);

  /* Process all dies in compilation unit.  */
  if (die->child != NULL)
    {
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  process_die (child_die, cu);
	  child_die = sibling_die (child_die);
	}
    }

  /* Decode macro information, if present.  Dwarf 2 macro information
     refers to information in the line number info statement program
     header, so we can only read it if we've read the header
     successfully.  */
  attr = dwarf2_attr (die, DW_AT_macros, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_GNU_macros, cu);
  if (attr && cu->line_header)
    {
      if (dwarf2_attr (die, DW_AT_macro_info, cu))
	complaint (&symfile_complaints,
		   _("CU refers to both DW_AT_macros and DW_AT_macro_info"));

      dwarf_decode_macros (cu, DW_UNSND (attr), 1);
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_macro_info, cu);
      if (attr && cu->line_header)
	{
	  unsigned int macro_offset = DW_UNSND (attr);

	  dwarf_decode_macros (cu, macro_offset, 0);
	}
    }
}

/* TU version of handle_DW_AT_stmt_list for read_type_unit_scope.
   Create the set of symtabs used by this TU, or if this TU is sharing
   symtabs with another TU and the symtabs have already been created
   then restore those symtabs in the line header.
   We don't need the pc/line-number mapping for type units.  */

static void
setup_type_unit_groups (struct die_info *die, struct dwarf2_cu *cu)
{
  struct dwarf2_per_cu_data *per_cu = cu->per_cu;
  struct type_unit_group *tu_group;
  int first_time;
  struct attribute *attr;
  unsigned int i;
  struct signatured_type *sig_type;

  gdb_assert (per_cu->is_debug_types);
  sig_type = (struct signatured_type *) per_cu;

  attr = dwarf2_attr (die, DW_AT_stmt_list, cu);

  /* If we're using .gdb_index (includes -readnow) then
     per_cu->type_unit_group may not have been set up yet.  */
  if (sig_type->type_unit_group == NULL)
    sig_type->type_unit_group = get_type_unit_group (cu, attr);
  tu_group = sig_type->type_unit_group;

  /* If we've already processed this stmt_list there's no real need to
     do it again, we could fake it and just recreate the part we need
     (file name,index -> symtab mapping).  If data shows this optimization
     is useful we can do it then.  */
  first_time = tu_group->compunit_symtab == NULL;

  /* We have to handle the case of both a missing DW_AT_stmt_list or bad
     debug info.  */
  line_header_up lh;
  if (attr != NULL)
    {
      sect_offset line_offset = (sect_offset) DW_UNSND (attr);
      lh = dwarf_decode_line_header (line_offset, cu);
    }
  if (lh == NULL)
    {
      if (first_time)
	dwarf2_start_symtab (cu, "", NULL, 0);
      else
	{
	  gdb_assert (tu_group->symtabs == NULL);
	  restart_symtab (tu_group->compunit_symtab, "", 0);
	}
      return;
    }

  cu->line_header = lh.get ();

  if (first_time)
    {
      struct compunit_symtab *cust = dwarf2_start_symtab (cu, "", NULL, 0);

      /* Note: We don't assign tu_group->compunit_symtab yet because we're
	 still initializing it, and our caller (a few levels up)
	 process_full_type_unit still needs to know if this is the first
	 time.  */

      tu_group->num_symtabs = lh->file_names.size ();
      tu_group->symtabs = XNEWVEC (struct symtab *, lh->file_names.size ());

      for (i = 0; i < lh->file_names.size (); ++i)
	{
	  file_entry &fe = lh->file_names[i];

	  dwarf2_start_subfile (fe.name, fe.include_dir (lh.get ()));

	  if (current_subfile->symtab == NULL)
	    {
	      /* NOTE: start_subfile will recognize when it's been passed
		 a file it has already seen.  So we can't assume there's a
		 simple mapping from lh->file_names to subfiles, plus
		 lh->file_names may contain dups.  */
	      current_subfile->symtab
		= allocate_symtab (cust, current_subfile->name);
	    }

	  fe.symtab = current_subfile->symtab;
	  tu_group->symtabs[i] = fe.symtab;
	}
    }
  else
    {
      restart_symtab (tu_group->compunit_symtab, "", 0);

      for (i = 0; i < lh->file_names.size (); ++i)
	{
	  struct file_entry *fe = &lh->file_names[i];

	  fe->symtab = tu_group->symtabs[i];
	}
    }

  lh.release ();

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
  struct die_info *child_die;

  prepare_one_comp_unit (cu, die, language_minimal);

  /* Initialize (or reinitialize) the machinery for building symtabs.
     We do this before processing child DIEs, so that the line header table
     is available for DW_AT_decl_file.  */
  setup_type_unit_groups (die, cu);

  if (die->child != NULL)
    {
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  process_die (child_die, cu);
	  child_die = sibling_die (child_die);
	}
    }
}

/* DWO/DWP files.

   http://gcc.gnu.org/wiki/DebugFission
   http://gcc.gnu.org/wiki/DebugFissionDWP

   To simplify handling of both DWO files ("object" files with the DWARF info)
   and DWP files (a file with the DWOs packaged up into one file), we treat
   DWP files as having a collection of virtual DWO files.  */

static hashval_t
hash_dwo_file (const void *item)
{
  const struct dwo_file *dwo_file = (const struct dwo_file *) item;
  hashval_t hash;

  hash = htab_hash_string (dwo_file->dwo_name);
  if (dwo_file->comp_dir != NULL)
    hash += htab_hash_string (dwo_file->comp_dir);
  return hash;
}

static int
eq_dwo_file (const void *item_lhs, const void *item_rhs)
{
  const struct dwo_file *lhs = (const struct dwo_file *) item_lhs;
  const struct dwo_file *rhs = (const struct dwo_file *) item_rhs;

  if (strcmp (lhs->dwo_name, rhs->dwo_name) != 0)
    return 0;
  if (lhs->comp_dir == NULL || rhs->comp_dir == NULL)
    return lhs->comp_dir == rhs->comp_dir;
  return strcmp (lhs->comp_dir, rhs->comp_dir) == 0;
}

/* Allocate a hash table for DWO files.  */

static htab_t
allocate_dwo_file_hash_table (void)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;

  return htab_create_alloc_ex (41,
			       hash_dwo_file,
			       eq_dwo_file,
			       NULL,
			       &objfile->objfile_obstack,
			       hashtab_obstack_allocate,
			       dummy_obstack_deallocate);
}

/* Lookup DWO file DWO_NAME.  */

static void **
lookup_dwo_file_slot (const char *dwo_name, const char *comp_dir)
{
  struct dwo_file find_entry;
  void **slot;

  if (dwarf2_per_objfile->dwo_files == NULL)
    dwarf2_per_objfile->dwo_files = allocate_dwo_file_hash_table ();

  memset (&find_entry, 0, sizeof (find_entry));
  find_entry.dwo_name = dwo_name;
  find_entry.comp_dir = comp_dir;
  slot = htab_find_slot (dwarf2_per_objfile->dwo_files, &find_entry, INSERT);

  return slot;
}

static hashval_t
hash_dwo_unit (const void *item)
{
  const struct dwo_unit *dwo_unit = (const struct dwo_unit *) item;

  /* This drops the top 32 bits of the id, but is ok for a hash.  */
  return dwo_unit->signature;
}

static int
eq_dwo_unit (const void *item_lhs, const void *item_rhs)
{
  const struct dwo_unit *lhs = (const struct dwo_unit *) item_lhs;
  const struct dwo_unit *rhs = (const struct dwo_unit *) item_rhs;

  /* The signature is assumed to be unique within the DWO file.
     So while object file CU dwo_id's always have the value zero,
     that's OK, assuming each object file DWO file has only one CU,
     and that's the rule for now.  */
  return lhs->signature == rhs->signature;
}

/* Allocate a hash table for DWO CUs,TUs.
   There is one of these tables for each of CUs,TUs for each DWO file.  */

static htab_t
allocate_dwo_unit_table (struct objfile *objfile)
{
  /* Start out with a pretty small number.
     Generally DWO files contain only one CU and maybe some TUs.  */
  return htab_create_alloc_ex (3,
			       hash_dwo_unit,
			       eq_dwo_unit,
			       NULL,
			       &objfile->objfile_obstack,
			       hashtab_obstack_allocate,
			       dummy_obstack_deallocate);
}

/* Structure used to pass data to create_dwo_debug_info_hash_table_reader.  */

struct create_dwo_cu_data
{
  struct dwo_file *dwo_file;
  struct dwo_unit dwo_unit;
};

/* die_reader_func for create_dwo_cu.  */

static void
create_dwo_cu_reader (const struct die_reader_specs *reader,
		      const gdb_byte *info_ptr,
		      struct die_info *comp_unit_die,
		      int has_children,
		      void *datap)
{
  struct dwarf2_cu *cu = reader->cu;
  sect_offset sect_off = cu->per_cu->sect_off;
  struct dwarf2_section_info *section = cu->per_cu->section;
  struct create_dwo_cu_data *data = (struct create_dwo_cu_data *) datap;
  struct dwo_file *dwo_file = data->dwo_file;
  struct dwo_unit *dwo_unit = &data->dwo_unit;
  struct attribute *attr;

  attr = dwarf2_attr (comp_unit_die, DW_AT_GNU_dwo_id, cu);
  if (attr == NULL)
    {
      complaint (&symfile_complaints,
		 _("Dwarf Error: debug entry at offset 0x%x is missing"
		   " its dwo_id [in module %s]"),
		 to_underlying (sect_off), dwo_file->dwo_name);
      return;
    }

  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = DW_UNSND (attr);
  dwo_unit->section = section;
  dwo_unit->sect_off = sect_off;
  dwo_unit->length = cu->per_cu->length;

  if (dwarf_read_debug)
    fprintf_unfiltered (gdb_stdlog, "  offset 0x%x, dwo_id %s\n",
			to_underlying (sect_off),
			hex_string (dwo_unit->signature));
}

/* Create the dwo_unit for the lone CU in DWO_FILE.
   Note: This function processes DWO files only, not DWP files.  */

static struct dwo_unit *
create_dwo_cu (struct dwo_file *dwo_file)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_section_info *section = &dwo_file->sections.info;
  const gdb_byte *info_ptr, *end_ptr;
  struct create_dwo_cu_data create_dwo_cu_data;
  struct dwo_unit *dwo_unit;

  dwarf2_read_section (objfile, section);
  info_ptr = section->buffer;

  if (info_ptr == NULL)
    return NULL;

  if (dwarf_read_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "Reading %s for %s:\n",
			  get_section_name (section),
			  get_section_file_name (section));
    }

  create_dwo_cu_data.dwo_file = dwo_file;
  dwo_unit = NULL;

  end_ptr = info_ptr + section->size;
  while (info_ptr < end_ptr)
    {
      struct dwarf2_per_cu_data per_cu;

      memset (&create_dwo_cu_data.dwo_unit, 0,
	      sizeof (create_dwo_cu_data.dwo_unit));
      memset (&per_cu, 0, sizeof (per_cu));
      per_cu.objfile = objfile;
      per_cu.is_debug_types = 0;
      per_cu.sect_off = sect_offset (info_ptr - section->buffer);
      per_cu.section = section;

      init_cutu_and_read_dies_no_follow (&per_cu, dwo_file,
					 create_dwo_cu_reader,
					 &create_dwo_cu_data);

      if (create_dwo_cu_data.dwo_unit.dwo_file != NULL)
	{
	  /* If we've already found one, complain.  We only support one
	     because having more than one requires hacking the dwo_name of
	     each to match, which is highly unlikely to happen.  */
	  if (dwo_unit != NULL)
	    {
	      complaint (&symfile_complaints,
			 _("Multiple CUs in DWO file %s [in module %s]"),
			 dwo_file->dwo_name, objfile_name (objfile));
	      break;
	    }

	  dwo_unit = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct dwo_unit);
	  *dwo_unit = create_dwo_cu_data.dwo_unit;
	}

      info_ptr += per_cu.length;
    }

  return dwo_unit;
}

/* DWP file .debug_{cu,tu}_index section format:
   [ref: http://gcc.gnu.org/wiki/DebugFissionDWP]

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

   DWP Version 2:

   DWP Version 2 combines all the .debug_info, etc. sections into one,
   and the entries in the index tables are now offsets into these sections.
   CU offsets begin at 0.  TU offsets begin at the size of the .debug_info
   section.

   Index Section Contents:
    Header
    Hash Table of Signatures   dwp_hash_table.hash_table
    Parallel Table of Indices  dwp_hash_table.unit_table
    Table of Section Offsets   dwp_hash_table.v2.{section_ids,offsets}
    Table of Section Sizes     dwp_hash_table.v2.sizes

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
   refer to that section.  The section identifiers are:

    DW_SECT_INFO         1  .debug_info.dwo
    DW_SECT_TYPES        2  .debug_types.dwo
    DW_SECT_ABBREV       3  .debug_abbrev.dwo
    DW_SECT_LINE         4  .debug_line.dwo
    DW_SECT_LOC          5  .debug_loc.dwo
    DW_SECT_STR_OFFSETS  6  .debug_str_offsets.dwo
    DW_SECT_MACINFO      7  .debug_macinfo.dwo
    DW_SECT_MACRO        8  .debug_macro.dwo

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
create_dwp_hash_table (struct dwp_file *dwp_file, int is_debug_types)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  bfd *dbfd = dwp_file->dbfd;
  const gdb_byte *index_ptr, *index_end;
  struct dwarf2_section_info *index;
  uint32_t version, nr_columns, nr_units, nr_slots;
  struct dwp_hash_table *htab;

  if (is_debug_types)
    index = &dwp_file->sections.tu_index;
  else
    index = &dwp_file->sections.cu_index;

  if (dwarf2_section_empty_p (index))
    return NULL;
  dwarf2_read_section (objfile, index);

  index_ptr = index->buffer;
  index_end = index_ptr + index->size;

  version = read_4_bytes (dbfd, index_ptr);
  index_ptr += 4;
  if (version == 2)
    nr_columns = read_4_bytes (dbfd, index_ptr);
  else
    nr_columns = 0;
  index_ptr += 4;
  nr_units = read_4_bytes (dbfd, index_ptr);
  index_ptr += 4;
  nr_slots = read_4_bytes (dbfd, index_ptr);
  index_ptr += 4;

  if (version != 1 && version != 2)
    {
      error (_("Dwarf Error: unsupported DWP file version (%s)"
	       " [in module %s]"),
	     pulongest (version), dwp_file->name);
    }
  if (nr_slots != (nr_slots & -nr_slots))
    {
      error (_("Dwarf Error: number of slots in DWP hash table (%s)"
	       " is not power of 2 [in module %s]"),
	     pulongest (nr_slots), dwp_file->name);
    }

  htab = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct dwp_hash_table);
  htab->version = version;
  htab->nr_columns = nr_columns;
  htab->nr_units = nr_units;
  htab->nr_slots = nr_slots;
  htab->hash_table = index_ptr;
  htab->unit_table = htab->hash_table + sizeof (uint64_t) * nr_slots;

  /* Exit early if the table is empty.  */
  if (nr_slots == 0 || nr_units == 0
      || (version == 2 && nr_columns == 0))
    {
      /* All must be zero.  */
      if (nr_slots != 0 || nr_units != 0
	  || (version == 2 && nr_columns != 0))
	{
	  complaint (&symfile_complaints,
		     _("Empty DWP but nr_slots,nr_units,nr_columns not"
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
  else
    {
      const gdb_byte *ids_ptr = htab->unit_table + sizeof (uint32_t) * nr_slots;
      int *ids = htab->section_pool.v2.section_ids;
      /* Reverse map for error checking.  */
      int ids_seen[DW_SECT_MAX + 1];
      int i;

      if (nr_columns < 2)
	{
	  error (_("Dwarf Error: bad DWP hash table, too few columns"
		   " in section table [in module %s]"),
		 dwp_file->name);
	}
      if (nr_columns > MAX_NR_V2_DWO_SECTIONS)
	{
	  error (_("Dwarf Error: bad DWP hash table, too many columns"
		   " in section table [in module %s]"),
		 dwp_file->name);
	}
      memset (ids, 255, (DW_SECT_MAX + 1) * sizeof (int32_t));
      memset (ids_seen, 255, (DW_SECT_MAX + 1) * sizeof (int32_t));
      for (i = 0; i < nr_columns; ++i)
	{
	  int id = read_4_bytes (dbfd, ids_ptr + i * sizeof (uint32_t));

	  if (id < DW_SECT_MIN || id > DW_SECT_MAX)
	    {
	      error (_("Dwarf Error: bad DWP hash table, bad section id %d"
		       " in section table [in module %s]"),
		     id, dwp_file->name);
	    }
	  if (ids_seen[id] != -1)
	    {
	      error (_("Dwarf Error: bad DWP hash table, duplicate section"
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
	  error (_("Dwarf Error: bad DWP hash table, missing/duplicate"
		   " DWO info/types section [in module %s]"),
		 dwp_file->name);
	}
      /* Must have an abbrev section.  */
      if (ids_seen[DW_SECT_ABBREV] == -1)
	{
	  error (_("Dwarf Error: bad DWP hash table, missing DWO abbrev"
		   " section [in module %s]"),
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
	  error (_("Dwarf Error: DWP index section is corrupt (too small)"
		   " [in module %s]"),
		 dwp_file->name);
	}
    }

  return htab;
}

/* Update SECTIONS with the data from SECTP.

   This function is like the other "locate" section routines that are
   passed to bfd_map_over_sections, but in this context the sections to
   read comes from the DWP V1 hash table, not the full ELF section table.

   The result is non-zero for success, or zero if an error was found.  */

static int
locate_v1_virtual_dwo_sections (asection *sectp,
				struct virtual_v1_dwo_sections *sections)
{
  const struct dwop_section_names *names = &dwop_section_names;

  if (section_is_p (sectp->name, &names->abbrev_dwo))
    {
      /* There can be only one.  */
      if (sections->abbrev.s.section != NULL)
	return 0;
      sections->abbrev.s.section = sectp;
      sections->abbrev.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->info_dwo)
	   || section_is_p (sectp->name, &names->types_dwo))
    {
      /* There can be only one.  */
      if (sections->info_or_types.s.section != NULL)
	return 0;
      sections->info_or_types.s.section = sectp;
      sections->info_or_types.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->line_dwo))
    {
      /* There can be only one.  */
      if (sections->line.s.section != NULL)
	return 0;
      sections->line.s.section = sectp;
      sections->line.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->loc_dwo))
    {
      /* There can be only one.  */
      if (sections->loc.s.section != NULL)
	return 0;
      sections->loc.s.section = sectp;
      sections->loc.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->macinfo_dwo))
    {
      /* There can be only one.  */
      if (sections->macinfo.s.section != NULL)
	return 0;
      sections->macinfo.s.section = sectp;
      sections->macinfo.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->macro_dwo))
    {
      /* There can be only one.  */
      if (sections->macro.s.section != NULL)
	return 0;
      sections->macro.s.section = sectp;
      sections->macro.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->str_offsets_dwo))
    {
      /* There can be only one.  */
      if (sections->str_offsets.s.section != NULL)
	return 0;
      sections->str_offsets.s.section = sectp;
      sections->str_offsets.size = bfd_get_section_size (sectp);
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

static struct dwo_unit *
create_dwo_unit_in_dwp_v1 (struct dwp_file *dwp_file,
			   uint32_t unit_index,
			   const char *comp_dir,
			   ULONGEST signature, int is_debug_types)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  const struct dwp_hash_table *dwp_htab =
    is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd;
  const char *kind = is_debug_types ? "TU" : "CU";
  struct dwo_file *dwo_file;
  struct dwo_unit *dwo_unit;
  struct virtual_v1_dwo_sections sections;
  void **dwo_file_slot;
  char *virtual_dwo_name;
  struct cleanup *cleanups;
  int i;

  gdb_assert (dwp_file->version == 1);

  if (dwarf_read_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "Reading %s %s/%s in DWP V1 file: %s\n",
			  kind,
			  pulongest (unit_index), hex_string (signature),
			  dwp_file->name);
    }

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
  cleanups = make_cleanup (null_cleanup, 0);

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
	  error (_("Dwarf Error: bad DWP hash table, section number too large"
		   " [in module %s]"),
		 dwp_file->name);
	}

      sectp = dwp_file->elf_sections[section_nr];
      if (! locate_v1_virtual_dwo_sections (sectp, &sections))
	{
	  error (_("Dwarf Error: bad DWP hash table, invalid section found"
		   " [in module %s]"),
		 dwp_file->name);
	}
    }

  if (i < 2
      || dwarf2_section_empty_p (&sections.info_or_types)
      || dwarf2_section_empty_p (&sections.abbrev))
    {
      error (_("Dwarf Error: bad DWP hash table, missing DWO sections"
	       " [in module %s]"),
	     dwp_file->name);
    }
  if (i == MAX_NR_V1_DWO_SECTIONS)
    {
      error (_("Dwarf Error: bad DWP hash table, too many DWO sections"
	       " [in module %s]"),
	     dwp_file->name);
    }

  /* It's easier for the rest of the code if we fake a struct dwo_file and
     have dwo_unit "live" in that.  At least for now.

     The DWP file can be made up of a random collection of CUs and TUs.
     However, for each CU + set of TUs that came from the same original DWO
     file, we can combine them back into a virtual DWO file to save space
     (fewer struct dwo_file objects to allocate).  Remember that for really
     large apps there can be on the order of 8K CUs and 200K TUs, or more.  */

  virtual_dwo_name =
    xstrprintf ("virtual-dwo/%d-%d-%d-%d",
		get_section_id (&sections.abbrev),
		get_section_id (&sections.line),
		get_section_id (&sections.loc),
		get_section_id (&sections.str_offsets));
  make_cleanup (xfree, virtual_dwo_name);
  /* Can we use an existing virtual DWO file?  */
  dwo_file_slot = lookup_dwo_file_slot (virtual_dwo_name, comp_dir);
  /* Create one if necessary.  */
  if (*dwo_file_slot == NULL)
    {
      if (dwarf_read_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "Creating virtual DWO: %s\n",
			      virtual_dwo_name);
	}
      dwo_file = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct dwo_file);
      dwo_file->dwo_name
	= (const char *) obstack_copy0 (&objfile->objfile_obstack,
					virtual_dwo_name,
					strlen (virtual_dwo_name));
      dwo_file->comp_dir = comp_dir;
      dwo_file->sections.abbrev = sections.abbrev;
      dwo_file->sections.line = sections.line;
      dwo_file->sections.loc = sections.loc;
      dwo_file->sections.macinfo = sections.macinfo;
      dwo_file->sections.macro = sections.macro;
      dwo_file->sections.str_offsets = sections.str_offsets;
      /* The "str" section is global to the entire DWP file.  */
      dwo_file->sections.str = dwp_file->sections.str;
      /* The info or types section is assigned below to dwo_unit,
	 there's no need to record it in dwo_file.
	 Also, we can't simply record type sections in dwo_file because
	 we record a pointer into the vector in dwo_unit.  As we collect more
	 types we'll grow the vector and eventually have to reallocate space
	 for it, invalidating all copies of pointers into the previous
	 contents.  */
      *dwo_file_slot = dwo_file;
    }
  else
    {
      if (dwarf_read_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "Using existing virtual DWO: %s\n",
			      virtual_dwo_name);
	}
      dwo_file = (struct dwo_file *) *dwo_file_slot;
    }
  do_cleanups (cleanups);

  dwo_unit = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct dwo_unit);
  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = signature;
  dwo_unit->section =
    XOBNEW (&objfile->objfile_obstack, struct dwarf2_section_info);
  *dwo_unit->section = sections.info_or_types;
  /* dwo_unit->{offset,length,type_offset_in_tu} are set later.  */

  return dwo_unit;
}

/* Subroutine of create_dwo_unit_in_dwp_v2 to simplify it.
   Given a pointer to the containing section SECTION, and OFFSET,SIZE of the
   piece within that section used by a TU/CU, return a virtual section
   of just that piece.  */

static struct dwarf2_section_info
create_dwp_v2_section (struct dwarf2_section_info *section,
		       bfd_size_type offset, bfd_size_type size)
{
  struct dwarf2_section_info result;
  asection *sectp;

  gdb_assert (section != NULL);
  gdb_assert (!section->is_virtual);

  memset (&result, 0, sizeof (result));
  result.s.containing_section = section;
  result.is_virtual = 1;

  if (size == 0)
    return result;

  sectp = get_section_bfd_section (section);

  /* Flag an error if the piece denoted by OFFSET,SIZE is outside the
     bounds of the real section.  This is a pretty-rare event, so just
     flag an error (easier) instead of a warning and trying to cope.  */
  if (sectp == NULL
      || offset + size > bfd_get_section_size (sectp))
    {
      bfd *abfd = sectp->owner;

      error (_("Dwarf Error: Bad DWP V2 section info, doesn't fit"
	       " in section %s [in module %s]"),
	     sectp ? bfd_section_name (abfd, sectp) : "<unknown>",
	     objfile_name (dwarf2_per_objfile->objfile));
    }

  result.virtual_offset = offset;
  result.size = size;
  return result;
}

/* Create a dwo_unit object for the DWO unit with signature SIGNATURE.
   UNIT_INDEX is the index of the DWO unit in the DWP hash table.
   COMP_DIR is the DW_AT_comp_dir attribute of the referencing CU.
   This is for DWP version 2 files.  */

static struct dwo_unit *
create_dwo_unit_in_dwp_v2 (struct dwp_file *dwp_file,
			   uint32_t unit_index,
			   const char *comp_dir,
			   ULONGEST signature, int is_debug_types)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  const struct dwp_hash_table *dwp_htab =
    is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd;
  const char *kind = is_debug_types ? "TU" : "CU";
  struct dwo_file *dwo_file;
  struct dwo_unit *dwo_unit;
  struct virtual_v2_dwo_sections sections;
  void **dwo_file_slot;
  char *virtual_dwo_name;
  struct cleanup *cleanups;
  int i;

  gdb_assert (dwp_file->version == 2);

  if (dwarf_read_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "Reading %s %s/%s in DWP V2 file: %s\n",
			  kind,
			  pulongest (unit_index), hex_string (signature),
			  dwp_file->name);
    }

  /* Fetch the section offsets of this DWO unit.  */

  memset (&sections, 0, sizeof (sections));
  cleanups = make_cleanup (null_cleanup, 0);

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

  virtual_dwo_name =
    xstrprintf ("virtual-dwo/%ld-%ld-%ld-%ld",
		(long) (sections.abbrev_size ? sections.abbrev_offset : 0),
		(long) (sections.line_size ? sections.line_offset : 0),
		(long) (sections.loc_size ? sections.loc_offset : 0),
		(long) (sections.str_offsets_size
			? sections.str_offsets_offset : 0));
  make_cleanup (xfree, virtual_dwo_name);
  /* Can we use an existing virtual DWO file?  */
  dwo_file_slot = lookup_dwo_file_slot (virtual_dwo_name, comp_dir);
  /* Create one if necessary.  */
  if (*dwo_file_slot == NULL)
    {
      if (dwarf_read_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "Creating virtual DWO: %s\n",
			      virtual_dwo_name);
	}
      dwo_file = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct dwo_file);
      dwo_file->dwo_name
	= (const char *) obstack_copy0 (&objfile->objfile_obstack,
					virtual_dwo_name,
					strlen (virtual_dwo_name));
      dwo_file->comp_dir = comp_dir;
      dwo_file->sections.abbrev =
	create_dwp_v2_section (&dwp_file->sections.abbrev,
			       sections.abbrev_offset, sections.abbrev_size);
      dwo_file->sections.line =
	create_dwp_v2_section (&dwp_file->sections.line,
			       sections.line_offset, sections.line_size);
      dwo_file->sections.loc =
	create_dwp_v2_section (&dwp_file->sections.loc,
			       sections.loc_offset, sections.loc_size);
      dwo_file->sections.macinfo =
	create_dwp_v2_section (&dwp_file->sections.macinfo,
			       sections.macinfo_offset, sections.macinfo_size);
      dwo_file->sections.macro =
	create_dwp_v2_section (&dwp_file->sections.macro,
			       sections.macro_offset, sections.macro_size);
      dwo_file->sections.str_offsets =
	create_dwp_v2_section (&dwp_file->sections.str_offsets,
			       sections.str_offsets_offset,
			       sections.str_offsets_size);
      /* The "str" section is global to the entire DWP file.  */
      dwo_file->sections.str = dwp_file->sections.str;
      /* The info or types section is assigned below to dwo_unit,
	 there's no need to record it in dwo_file.
	 Also, we can't simply record type sections in dwo_file because
	 we record a pointer into the vector in dwo_unit.  As we collect more
	 types we'll grow the vector and eventually have to reallocate space
	 for it, invalidating all copies of pointers into the previous
	 contents.  */
      *dwo_file_slot = dwo_file;
    }
  else
    {
      if (dwarf_read_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "Using existing virtual DWO: %s\n",
			      virtual_dwo_name);
	}
      dwo_file = (struct dwo_file *) *dwo_file_slot;
    }
  do_cleanups (cleanups);

  dwo_unit = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct dwo_unit);
  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = signature;
  dwo_unit->section =
    XOBNEW (&objfile->objfile_obstack, struct dwarf2_section_info);
  *dwo_unit->section = create_dwp_v2_section (is_debug_types
					      ? &dwp_file->sections.types
					      : &dwp_file->sections.info,
					      sections.info_or_types_offset,
					      sections.info_or_types_size);
  /* dwo_unit->{offset,length,type_offset_in_tu} are set later.  */

  return dwo_unit;
}

/* Lookup the DWO unit with SIGNATURE in DWP_FILE.
   Returns NULL if the signature isn't found.  */

static struct dwo_unit *
lookup_dwo_unit_in_dwp (struct dwp_file *dwp_file, const char *comp_dir,
			ULONGEST signature, int is_debug_types)
{
  const struct dwp_hash_table *dwp_htab =
    is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd;
  uint32_t mask = dwp_htab->nr_slots - 1;
  uint32_t hash = signature & mask;
  uint32_t hash2 = ((signature >> 32) & mask) | 1;
  unsigned int i;
  void **slot;
  struct dwo_unit find_dwo_cu;

  memset (&find_dwo_cu, 0, sizeof (find_dwo_cu));
  find_dwo_cu.signature = signature;
  slot = htab_find_slot (is_debug_types
			 ? dwp_file->loaded_tus
			 : dwp_file->loaded_cus,
			 &find_dwo_cu, INSERT);

  if (*slot != NULL)
    return (struct dwo_unit *) *slot;

  /* Use a for loop so that we don't loop forever on bad debug info.  */
  for (i = 0; i < dwp_htab->nr_slots; ++i)
    {
      ULONGEST signature_in_table;

      signature_in_table =
	read_8_bytes (dbfd, dwp_htab->hash_table + hash * sizeof (uint64_t));
      if (signature_in_table == signature)
	{
	  uint32_t unit_index =
	    read_4_bytes (dbfd,
			  dwp_htab->unit_table + hash * sizeof (uint32_t));

	  if (dwp_file->version == 1)
	    {
	      *slot = create_dwo_unit_in_dwp_v1 (dwp_file, unit_index,
						 comp_dir, signature,
						 is_debug_types);
	    }
	  else
	    {
	      *slot = create_dwo_unit_in_dwp_v2 (dwp_file, unit_index,
						 comp_dir, signature,
						 is_debug_types);
	    }
	  return (struct dwo_unit *) *slot;
	}
      if (signature_in_table == 0)
	return NULL;
      hash = (hash + hash2) & mask;
    }

  error (_("Dwarf Error: bad DWP hash table, lookup didn't terminate"
	   " [in module %s]"),
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
try_open_dwop_file (const char *file_name, int is_dwp, int search_cwd)
{
  int desc, flags;
  char *absolute_name;
  /* Blech.  OPF_TRY_CWD_FIRST also disables searching the path list if
     FILE_NAME contains a '/'.  So we can't use it.  Instead prepend "."
     to debug_file_directory.  */
  char *search_path;
  static const char dirname_separator_string[] = { DIRNAME_SEPARATOR, '\0' };

  if (search_cwd)
    {
      if (*debug_file_directory != '\0')
	search_path = concat (".", dirname_separator_string,
			      debug_file_directory, (char *) NULL);
      else
	search_path = xstrdup (".");
    }
  else
    search_path = xstrdup (debug_file_directory);

  flags = OPF_RETURN_REALPATH;
  if (is_dwp)
    flags |= OPF_SEARCH_IN_PATH;
  desc = openp (search_path, flags, file_name,
		O_RDONLY | O_BINARY, &absolute_name);
  xfree (search_path);
  if (desc < 0)
    return NULL;

  gdb_bfd_ref_ptr sym_bfd (gdb_bfd_open (absolute_name, gnutarget, desc));
  xfree (absolute_name);
  if (sym_bfd == NULL)
    return NULL;
  bfd_set_cacheable (sym_bfd.get (), 1);

  if (!bfd_check_format (sym_bfd.get (), bfd_object))
    return NULL;

  /* Success.  Record the bfd as having been included by the objfile's bfd.
     This is important because things like demangled_names_hash lives in the
     objfile's per_bfd space and may have references to things like symbol
     names that live in the DWO/DWP file's per_bfd space.  PR 16426.  */
  gdb_bfd_record_inclusion (dwarf2_per_objfile->objfile->obfd, sym_bfd.get ());

  return sym_bfd;
}

/* Try to open DWO file FILE_NAME.
   COMP_DIR is the DW_AT_comp_dir attribute.
   The result is the bfd handle of the file.
   If there is a problem finding or opening the file, return NULL.
   Upon success, the canonicalized path of the file is stored in the bfd,
   same as symfile_bfd_open.  */

static gdb_bfd_ref_ptr
open_dwo_file (const char *file_name, const char *comp_dir)
{
  if (IS_ABSOLUTE_PATH (file_name))
    return try_open_dwop_file (file_name, 0 /*is_dwp*/, 0 /*search_cwd*/);

  /* Before trying the search path, try DWO_NAME in COMP_DIR.  */

  if (comp_dir != NULL)
    {
      char *path_to_try = concat (comp_dir, SLASH_STRING,
				  file_name, (char *) NULL);

      /* NOTE: If comp_dir is a relative path, this will also try the
	 search path, which seems useful.  */
      gdb_bfd_ref_ptr abfd (try_open_dwop_file (path_to_try, 0 /*is_dwp*/,
						1 /*search_cwd*/));
      xfree (path_to_try);
      if (abfd != NULL)
	return abfd;
    }

  /* That didn't work, try debug-file-directory, which, despite its name,
     is a list of paths.  */

  if (*debug_file_directory == '\0')
    return NULL;

  return try_open_dwop_file (file_name, 0 /*is_dwp*/, 1 /*search_cwd*/);
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWO debugging sections we are interested in.  */

static void
dwarf2_locate_dwo_sections (bfd *abfd, asection *sectp, void *dwo_sections_ptr)
{
  struct dwo_sections *dwo_sections = (struct dwo_sections *) dwo_sections_ptr;
  const struct dwop_section_names *names = &dwop_section_names;

  if (section_is_p (sectp->name, &names->abbrev_dwo))
    {
      dwo_sections->abbrev.s.section = sectp;
      dwo_sections->abbrev.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->info_dwo))
    {
      dwo_sections->info.s.section = sectp;
      dwo_sections->info.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->line_dwo))
    {
      dwo_sections->line.s.section = sectp;
      dwo_sections->line.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->loc_dwo))
    {
      dwo_sections->loc.s.section = sectp;
      dwo_sections->loc.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->macinfo_dwo))
    {
      dwo_sections->macinfo.s.section = sectp;
      dwo_sections->macinfo.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->macro_dwo))
    {
      dwo_sections->macro.s.section = sectp;
      dwo_sections->macro.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->str_dwo))
    {
      dwo_sections->str.s.section = sectp;
      dwo_sections->str.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->str_offsets_dwo))
    {
      dwo_sections->str_offsets.s.section = sectp;
      dwo_sections->str_offsets.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->types_dwo))
    {
      struct dwarf2_section_info type_section;

      memset (&type_section, 0, sizeof (type_section));
      type_section.s.section = sectp;
      type_section.size = bfd_get_section_size (sectp);
      VEC_safe_push (dwarf2_section_info_def, dwo_sections->types,
		     &type_section);
    }
}

/* Initialize the use of the DWO file specified by DWO_NAME and referenced
   by PER_CU.  This is for the non-DWP case.
   The result is NULL if DWO_NAME can't be found.  */

static struct dwo_file *
open_and_init_dwo_file (struct dwarf2_per_cu_data *per_cu,
			const char *dwo_name, const char *comp_dir)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwo_file *dwo_file;
  struct cleanup *cleanups;

  gdb_bfd_ref_ptr dbfd (open_dwo_file (dwo_name, comp_dir));
  if (dbfd == NULL)
    {
      if (dwarf_read_debug)
	fprintf_unfiltered (gdb_stdlog, "DWO file not found: %s\n", dwo_name);
      return NULL;
    }
  dwo_file = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct dwo_file);
  dwo_file->dwo_name = dwo_name;
  dwo_file->comp_dir = comp_dir;
  dwo_file->dbfd = dbfd.release ();

  cleanups = make_cleanup (free_dwo_file_cleanup, dwo_file);

  bfd_map_over_sections (dwo_file->dbfd, dwarf2_locate_dwo_sections,
			 &dwo_file->sections);

  dwo_file->cu = create_dwo_cu (dwo_file);

  create_debug_types_hash_table (dwo_file, dwo_file->sections.types,
				 dwo_file->tus);

  discard_cleanups (cleanups);

  if (dwarf_read_debug)
    fprintf_unfiltered (gdb_stdlog, "DWO file found: %s\n", dwo_name);

  return dwo_file;
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWP debugging sections common to version 1 and 2 that
   we are interested in.  */

static void
dwarf2_locate_common_dwp_sections (bfd *abfd, asection *sectp,
				   void *dwp_file_ptr)
{
  struct dwp_file *dwp_file = (struct dwp_file *) dwp_file_ptr;
  const struct dwop_section_names *names = &dwop_section_names;
  unsigned int elf_section_nr = elf_section_data (sectp)->this_idx;

  /* Record the ELF section number for later lookup: this is what the
     .debug_cu_index,.debug_tu_index tables use in DWP V1.  */
  gdb_assert (elf_section_nr < dwp_file->num_sections);
  dwp_file->elf_sections[elf_section_nr] = sectp;

  /* Look for specific sections that we need.  */
  if (section_is_p (sectp->name, &names->str_dwo))
    {
      dwp_file->sections.str.s.section = sectp;
      dwp_file->sections.str.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->cu_index))
    {
      dwp_file->sections.cu_index.s.section = sectp;
      dwp_file->sections.cu_index.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->tu_index))
    {
      dwp_file->sections.tu_index.s.section = sectp;
      dwp_file->sections.tu_index.size = bfd_get_section_size (sectp);
    }
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWP version 2 debugging sections that we are interested
   in.  This is split into a separate function because we don't know if we
   have version 1 or 2 until we parse the cu_index/tu_index sections.  */

static void
dwarf2_locate_v2_dwp_sections (bfd *abfd, asection *sectp, void *dwp_file_ptr)
{
  struct dwp_file *dwp_file = (struct dwp_file *) dwp_file_ptr;
  const struct dwop_section_names *names = &dwop_section_names;
  unsigned int elf_section_nr = elf_section_data (sectp)->this_idx;

  /* Record the ELF section number for later lookup: this is what the
     .debug_cu_index,.debug_tu_index tables use in DWP V1.  */
  gdb_assert (elf_section_nr < dwp_file->num_sections);
  dwp_file->elf_sections[elf_section_nr] = sectp;

  /* Look for specific sections that we need.  */
  if (section_is_p (sectp->name, &names->abbrev_dwo))
    {
      dwp_file->sections.abbrev.s.section = sectp;
      dwp_file->sections.abbrev.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->info_dwo))
    {
      dwp_file->sections.info.s.section = sectp;
      dwp_file->sections.info.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->line_dwo))
    {
      dwp_file->sections.line.s.section = sectp;
      dwp_file->sections.line.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->loc_dwo))
    {
      dwp_file->sections.loc.s.section = sectp;
      dwp_file->sections.loc.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->macinfo_dwo))
    {
      dwp_file->sections.macinfo.s.section = sectp;
      dwp_file->sections.macinfo.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->macro_dwo))
    {
      dwp_file->sections.macro.s.section = sectp;
      dwp_file->sections.macro.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->str_offsets_dwo))
    {
      dwp_file->sections.str_offsets.s.section = sectp;
      dwp_file->sections.str_offsets.size = bfd_get_section_size (sectp);
    }
  else if (section_is_p (sectp->name, &names->types_dwo))
    {
      dwp_file->sections.types.s.section = sectp;
      dwp_file->sections.types.size = bfd_get_section_size (sectp);
    }
}

/* Hash function for dwp_file loaded CUs/TUs.  */

static hashval_t
hash_dwp_loaded_cutus (const void *item)
{
  const struct dwo_unit *dwo_unit = (const struct dwo_unit *) item;

  /* This drops the top 32 bits of the signature, but is ok for a hash.  */
  return dwo_unit->signature;
}

/* Equality function for dwp_file loaded CUs/TUs.  */

static int
eq_dwp_loaded_cutus (const void *a, const void *b)
{
  const struct dwo_unit *dua = (const struct dwo_unit *) a;
  const struct dwo_unit *dub = (const struct dwo_unit *) b;

  return dua->signature == dub->signature;
}

/* Allocate a hash table for dwp_file loaded CUs/TUs.  */

static htab_t
allocate_dwp_loaded_cutus_table (struct objfile *objfile)
{
  return htab_create_alloc_ex (3,
			       hash_dwp_loaded_cutus,
			       eq_dwp_loaded_cutus,
			       NULL,
			       &objfile->objfile_obstack,
			       hashtab_obstack_allocate,
			       dummy_obstack_deallocate);
}

/* Try to open DWP file FILE_NAME.
   The result is the bfd handle of the file.
   If there is a problem finding or opening the file, return NULL.
   Upon success, the canonicalized path of the file is stored in the bfd,
   same as symfile_bfd_open.  */

static gdb_bfd_ref_ptr
open_dwp_file (const char *file_name)
{
  gdb_bfd_ref_ptr abfd (try_open_dwop_file (file_name, 1 /*is_dwp*/,
					    1 /*search_cwd*/));
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
  if (*debug_file_directory != '\0')
    {
      /* Don't implicitly search the current directory here.
	 If the user wants to search "." to handle this case,
	 it must be added to debug-file-directory.  */
      return try_open_dwop_file (lbasename (file_name), 1 /*is_dwp*/,
				 0 /*search_cwd*/);
    }

  return NULL;
}

/* Initialize the use of the DWP file for the current objfile.
   By convention the name of the DWP file is ${objfile}.dwp.
   The result is NULL if it can't be found.  */

static struct dwp_file *
open_and_init_dwp_file (void)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwp_file *dwp_file;

  /* Try to find first .dwp for the binary file before any symbolic links
     resolving.  */

  /* If the objfile is a debug file, find the name of the real binary
     file and get the name of dwp file from there.  */
  std::string dwp_name;
  if (objfile->separate_debug_objfile_backlink != NULL)
    {
      struct objfile *backlink = objfile->separate_debug_objfile_backlink;
      const char *backlink_basename = lbasename (backlink->original_name);

      dwp_name = ldirname (objfile->original_name) + SLASH_STRING + backlink_basename;
    }
  else
    dwp_name = objfile->original_name;

  dwp_name += ".dwp";

  gdb_bfd_ref_ptr dbfd (open_dwp_file (dwp_name.c_str ()));
  if (dbfd == NULL
      && strcmp (objfile->original_name, objfile_name (objfile)) != 0)
    {
      /* Try to find .dwp for the binary file after gdb_realpath resolving.  */
      dwp_name = objfile_name (objfile);
      dwp_name += ".dwp";
      dbfd = open_dwp_file (dwp_name.c_str ());
    }

  if (dbfd == NULL)
    {
      if (dwarf_read_debug)
	fprintf_unfiltered (gdb_stdlog, "DWP file not found: %s\n", dwp_name.c_str ());
      return NULL;
    }
  dwp_file = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct dwp_file);
  dwp_file->name = bfd_get_filename (dbfd.get ());
  dwp_file->dbfd = dbfd.release ();

  /* +1: section 0 is unused */
  dwp_file->num_sections = bfd_count_sections (dwp_file->dbfd) + 1;
  dwp_file->elf_sections =
    OBSTACK_CALLOC (&objfile->objfile_obstack,
		    dwp_file->num_sections, asection *);

  bfd_map_over_sections (dwp_file->dbfd, dwarf2_locate_common_dwp_sections,
			 dwp_file);

  dwp_file->cus = create_dwp_hash_table (dwp_file, 0);

  dwp_file->tus = create_dwp_hash_table (dwp_file, 1);

  /* The DWP file version is stored in the hash table.  Oh well.  */
  if (dwp_file->cus->version != dwp_file->tus->version)
    {
      /* Technically speaking, we should try to limp along, but this is
	 pretty bizarre.  We use pulongest here because that's the established
	 portability solution (e.g, we cannot use %u for uint32_t).  */
      error (_("Dwarf Error: DWP file CU version %s doesn't match"
	       " TU version %s [in DWP file %s]"),
	     pulongest (dwp_file->cus->version),
	     pulongest (dwp_file->tus->version), dwp_name.c_str ());
    }
  dwp_file->version = dwp_file->cus->version;

  if (dwp_file->version == 2)
    bfd_map_over_sections (dwp_file->dbfd, dwarf2_locate_v2_dwp_sections,
			   dwp_file);

  dwp_file->loaded_cus = allocate_dwp_loaded_cutus_table (objfile);
  dwp_file->loaded_tus = allocate_dwp_loaded_cutus_table (objfile);

  if (dwarf_read_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "DWP file found: %s\n", dwp_file->name);
      fprintf_unfiltered (gdb_stdlog,
			  "    %s CUs, %s TUs\n",
			  pulongest (dwp_file->cus ? dwp_file->cus->nr_units : 0),
			  pulongest (dwp_file->tus ? dwp_file->tus->nr_units : 0));
    }

  return dwp_file;
}

/* Wrapper around open_and_init_dwp_file, only open it once.  */

static struct dwp_file *
get_dwp_file (void)
{
  if (! dwarf2_per_objfile->dwp_checked)
    {
      dwarf2_per_objfile->dwp_file = open_and_init_dwp_file ();
      dwarf2_per_objfile->dwp_checked = 1;
    }
  return dwarf2_per_objfile->dwp_file;
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

static struct dwo_unit *
lookup_dwo_cutu (struct dwarf2_per_cu_data *this_unit,
		 const char *dwo_name, const char *comp_dir,
		 ULONGEST signature, int is_debug_types)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  const char *kind = is_debug_types ? "TU" : "CU";
  void **dwo_file_slot;
  struct dwo_file *dwo_file;
  struct dwp_file *dwp_file;

  /* First see if there's a DWP file.
     If we have a DWP file but didn't find the DWO inside it, don't
     look for the original DWO file.  It makes gdb behave differently
     depending on whether one is debugging in the build tree.  */

  dwp_file = get_dwp_file ();
  if (dwp_file != NULL)
    {
      const struct dwp_hash_table *dwp_htab =
	is_debug_types ? dwp_file->tus : dwp_file->cus;

      if (dwp_htab != NULL)
	{
	  struct dwo_unit *dwo_cutu =
	    lookup_dwo_unit_in_dwp (dwp_file, comp_dir,
				    signature, is_debug_types);

	  if (dwo_cutu != NULL)
	    {
	      if (dwarf_read_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "Virtual DWO %s %s found: @%s\n",
				      kind, hex_string (signature),
				      host_address_to_string (dwo_cutu));
		}
	      return dwo_cutu;
	    }
	}
    }
  else
    {
      /* No DWP file, look for the DWO file.  */

      dwo_file_slot = lookup_dwo_file_slot (dwo_name, comp_dir);
      if (*dwo_file_slot == NULL)
	{
	  /* Read in the file and build a table of the CUs/TUs it contains.  */
	  *dwo_file_slot = open_and_init_dwo_file (this_unit, dwo_name, comp_dir);
	}
      /* NOTE: This will be NULL if unable to open the file.  */
      dwo_file = (struct dwo_file *) *dwo_file_slot;

      if (dwo_file != NULL)
	{
	  struct dwo_unit *dwo_cutu = NULL;

	  if (is_debug_types && dwo_file->tus)
	    {
	      struct dwo_unit find_dwo_cutu;

	      memset (&find_dwo_cutu, 0, sizeof (find_dwo_cutu));
	      find_dwo_cutu.signature = signature;
	      dwo_cutu
		= (struct dwo_unit *) htab_find (dwo_file->tus, &find_dwo_cutu);
	    }
	  else if (!is_debug_types && dwo_file->cu)
	    {
	      if (signature == dwo_file->cu->signature)
		dwo_cutu = dwo_file->cu;
	    }

	  if (dwo_cutu != NULL)
	    {
	      if (dwarf_read_debug)
		{
		  fprintf_unfiltered (gdb_stdlog, "DWO %s %s(%s) found: @%s\n",
				      kind, dwo_name, hex_string (signature),
				      host_address_to_string (dwo_cutu));
		}
	      return dwo_cutu;
	    }
	}
    }

  /* We didn't find it.  This could mean a dwo_id mismatch, or
     someone deleted the DWO/DWP file, or the search path isn't set up
     correctly to find the file.  */

  if (dwarf_read_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "DWO %s %s(%s) not found\n",
			  kind, dwo_name, hex_string (signature));
    }

  /* This is a warning and not a complaint because it can be caused by
     pilot error (e.g., user accidentally deleting the DWO).  */
  {
    /* Print the name of the DWP file if we looked there, helps the user
       better diagnose the problem.  */
    char *dwp_text = NULL;
    struct cleanup *cleanups;

    if (dwp_file != NULL)
      dwp_text = xstrprintf (" [in DWP file %s]", lbasename (dwp_file->name));
    cleanups = make_cleanup (xfree, dwp_text);

    warning (_("Could not find DWO %s %s(%s)%s referenced by %s at offset 0x%x"
	       " [in module %s]"),
	     kind, dwo_name, hex_string (signature),
	     dwp_text != NULL ? dwp_text : "",
	     this_unit->is_debug_types ? "TU" : "CU",
	     to_underlying (this_unit->sect_off), objfile_name (objfile));

    do_cleanups (cleanups);
  }
  return NULL;
}

/* Lookup the DWO CU DWO_NAME/SIGNATURE referenced from THIS_CU.
   See lookup_dwo_cutu_unit for details.  */

static struct dwo_unit *
lookup_dwo_comp_unit (struct dwarf2_per_cu_data *this_cu,
		      const char *dwo_name, const char *comp_dir,
		      ULONGEST signature)
{
  return lookup_dwo_cutu (this_cu, dwo_name, comp_dir, signature, 0);
}

/* Lookup the DWO TU DWO_NAME/SIGNATURE referenced from THIS_TU.
   See lookup_dwo_cutu_unit for details.  */

static struct dwo_unit *
lookup_dwo_type_unit (struct signatured_type *this_tu,
		      const char *dwo_name, const char *comp_dir)
{
  return lookup_dwo_cutu (&this_tu->per_cu, dwo_name, comp_dir, this_tu->signature, 1);
}

/* Traversal function for queue_and_load_all_dwo_tus.  */

static int
queue_and_load_dwo_tu (void **slot, void *info)
{
  struct dwo_unit *dwo_unit = (struct dwo_unit *) *slot;
  struct dwarf2_per_cu_data *per_cu = (struct dwarf2_per_cu_data *) info;
  ULONGEST signature = dwo_unit->signature;
  struct signatured_type *sig_type =
    lookup_dwo_signatured_type (per_cu->cu, signature);

  if (sig_type != NULL)
    {
      struct dwarf2_per_cu_data *sig_cu = &sig_type->per_cu;

      /* We pass NULL for DEPENDENT_CU because we don't yet know if there's
	 a real dependency of PER_CU on SIG_TYPE.  That is detected later
	 while processing PER_CU.  */
      if (maybe_queue_comp_unit (NULL, sig_cu, per_cu->cu->language))
	load_full_type_unit (sig_cu);
      VEC_safe_push (dwarf2_per_cu_ptr, per_cu->imported_symtabs, sig_cu);
    }

  return 1;
}

/* Queue all TUs contained in the DWO of PER_CU to be read in.
   The DWO may have the only definition of the type, though it may not be
   referenced anywhere in PER_CU.  Thus we have to load *all* its TUs.
   http://sourceware.org/bugzilla/show_bug.cgi?id=15021  */

static void
queue_and_load_all_dwo_tus (struct dwarf2_per_cu_data *per_cu)
{
  struct dwo_unit *dwo_unit;
  struct dwo_file *dwo_file;

  gdb_assert (!per_cu->is_debug_types);
  gdb_assert (get_dwp_file () == NULL);
  gdb_assert (per_cu->cu != NULL);

  dwo_unit = per_cu->cu->dwo_unit;
  gdb_assert (dwo_unit != NULL);

  dwo_file = dwo_unit->dwo_file;
  if (dwo_file->tus != NULL)
    htab_traverse_noresize (dwo_file->tus, queue_and_load_dwo_tu, per_cu);
}

/* Free all resources associated with DWO_FILE.
   Close the DWO file and munmap the sections.
   All memory should be on the objfile obstack.  */

static void
free_dwo_file (struct dwo_file *dwo_file, struct objfile *objfile)
{

  /* Note: dbfd is NULL for virtual DWO files.  */
  gdb_bfd_unref (dwo_file->dbfd);

  VEC_free (dwarf2_section_info_def, dwo_file->sections.types);
}

/* Wrapper for free_dwo_file for use in cleanups.  */

static void
free_dwo_file_cleanup (void *arg)
{
  struct dwo_file *dwo_file = (struct dwo_file *) arg;
  struct objfile *objfile = dwarf2_per_objfile->objfile;

  free_dwo_file (dwo_file, objfile);
}

/* Traversal function for free_dwo_files.  */

static int
free_dwo_file_from_slot (void **slot, void *info)
{
  struct dwo_file *dwo_file = (struct dwo_file *) *slot;
  struct objfile *objfile = (struct objfile *) info;

  free_dwo_file (dwo_file, objfile);

  return 1;
}

/* Free all resources associated with DWO_FILES.  */

static void
free_dwo_files (htab_t dwo_files, struct objfile *objfile)
{
  htab_traverse_noresize (dwo_files, free_dwo_file_from_slot, objfile);
}

/* Read in various DIEs.  */

/* qsort helper for inherit_abstract_dies.  */

static int
unsigned_int_compar (const void *ap, const void *bp)
{
  unsigned int a = *(unsigned int *) ap;
  unsigned int b = *(unsigned int *) bp;

  return (a > b) - (b > a);
}

/* DW_AT_abstract_origin inherits whole DIEs (not just their attributes).
   Inherit only the children of the DW_AT_abstract_origin DIE not being
   already referenced by DW_AT_abstract_origin from the children of the
   current DIE.  */

static void
inherit_abstract_dies (struct die_info *die, struct dwarf2_cu *cu)
{
  struct die_info *child_die;
  unsigned die_children_count;
  /* CU offsets which were referenced by children of the current DIE.  */
  sect_offset *offsets;
  sect_offset *offsets_end, *offsetp;
  /* Parent of DIE - referenced by DW_AT_abstract_origin.  */
  struct die_info *origin_die;
  /* Iterator of the ORIGIN_DIE children.  */
  struct die_info *origin_child_die;
  struct cleanup *cleanups;
  struct attribute *attr;
  struct dwarf2_cu *origin_cu;
  struct pending **origin_previous_list_in_scope;

  attr = dwarf2_attr (die, DW_AT_abstract_origin, cu);
  if (!attr)
    return;

  /* Note that following die references may follow to a die in a
     different cu.  */

  origin_cu = cu;
  origin_die = follow_die_ref (die, attr, &origin_cu);

  /* We're inheriting ORIGIN's children into the scope we'd put DIE's
     symbols in.  */
  origin_previous_list_in_scope = origin_cu->list_in_scope;
  origin_cu->list_in_scope = cu->list_in_scope;

  if (die->tag != origin_die->tag
      && !(die->tag == DW_TAG_inlined_subroutine
	   && origin_die->tag == DW_TAG_subprogram))
    complaint (&symfile_complaints,
	       _("DIE 0x%x and its abstract origin 0x%x have different tags"),
	       to_underlying (die->sect_off),
	       to_underlying (origin_die->sect_off));

  child_die = die->child;
  die_children_count = 0;
  while (child_die && child_die->tag)
    {
      child_die = sibling_die (child_die);
      die_children_count++;
    }
  offsets = XNEWVEC (sect_offset, die_children_count);
  cleanups = make_cleanup (xfree, offsets);

  offsets_end = offsets;
  for (child_die = die->child;
       child_die && child_die->tag;
       child_die = sibling_die (child_die))
    {
      struct die_info *child_origin_die;
      struct dwarf2_cu *child_origin_cu;

      /* We are trying to process concrete instance entries:
	 DW_TAG_call_site DIEs indeed have a DW_AT_abstract_origin tag, but
	 it's not relevant to our analysis here. i.e. detecting DIEs that are
	 present in the abstract instance but not referenced in the concrete
	 one.  */
      if (child_die->tag == DW_TAG_call_site
          || child_die->tag == DW_TAG_GNU_call_site)
	continue;

      /* For each CHILD_DIE, find the corresponding child of
	 ORIGIN_DIE.  If there is more than one layer of
	 DW_AT_abstract_origin, follow them all; there shouldn't be,
	 but GCC versions at least through 4.4 generate this (GCC PR
	 40573).  */
      child_origin_die = child_die;
      child_origin_cu = cu;
      while (1)
	{
	  attr = dwarf2_attr (child_origin_die, DW_AT_abstract_origin,
			      child_origin_cu);
	  if (attr == NULL)
	    break;
	  child_origin_die = follow_die_ref (child_origin_die, attr,
					     &child_origin_cu);
	}

      /* According to DWARF3 3.3.8.2 #3 new entries without their abstract
	 counterpart may exist.  */
      if (child_origin_die != child_die)
	{
	  if (child_die->tag != child_origin_die->tag
	      && !(child_die->tag == DW_TAG_inlined_subroutine
		   && child_origin_die->tag == DW_TAG_subprogram))
	    complaint (&symfile_complaints,
		       _("Child DIE 0x%x and its abstract origin 0x%x have "
			 "different tags"),
		       to_underlying (child_die->sect_off),
		       to_underlying (child_origin_die->sect_off));
	  if (child_origin_die->parent != origin_die)
	    complaint (&symfile_complaints,
		       _("Child DIE 0x%x and its abstract origin 0x%x have "
			 "different parents"),
		       to_underlying (child_die->sect_off),
		       to_underlying (child_origin_die->sect_off));
	  else
	    *offsets_end++ = child_origin_die->sect_off;
	}
    }
  qsort (offsets, offsets_end - offsets, sizeof (*offsets),
	 unsigned_int_compar);
  for (offsetp = offsets + 1; offsetp < offsets_end; offsetp++)
    if (offsetp[-1] == *offsetp)
      complaint (&symfile_complaints,
		 _("Multiple children of DIE 0x%x refer "
		   "to DIE 0x%x as their abstract origin"),
		 to_underlying (die->sect_off), to_underlying (*offsetp));

  offsetp = offsets;
  origin_child_die = origin_die->child;
  while (origin_child_die && origin_child_die->tag)
    {
      /* Is ORIGIN_CHILD_DIE referenced by any of the DIE children?  */
      while (offsetp < offsets_end
	     && *offsetp < origin_child_die->sect_off)
	offsetp++;
      if (offsetp >= offsets_end
	  || *offsetp > origin_child_die->sect_off)
	{
	  /* Found that ORIGIN_CHILD_DIE is really not referenced.
	     Check whether we're already processing ORIGIN_CHILD_DIE.
	     This can happen with mutually referenced abstract_origins.
	     PR 16581.  */
	  if (!origin_child_die->in_process)
	    process_die (origin_child_die, origin_cu);
	}
      origin_child_die = sibling_die (origin_child_die);
    }
  origin_cu->list_in_scope = origin_previous_list_in_scope;

  do_cleanups (cleanups);
}

static void
read_func_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct context_stack *newobj;
  CORE_ADDR lowpc;
  CORE_ADDR highpc;
  struct die_info *child_die;
  struct attribute *attr, *call_line, *call_file;
  const char *name;
  CORE_ADDR baseaddr;
  struct block *block;
  int inlined_func = (die->tag == DW_TAG_inlined_subroutine);
  VEC (symbolp) *template_args = NULL;
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

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  name = dwarf2_name (die, cu);

  /* Ignore functions with missing or empty names.  These are actually
     illegal according to the DWARF standard.  */
  if (name == NULL)
    {
      complaint (&symfile_complaints,
		 _("missing name for subprogram DIE at %d"),
		 to_underlying (die->sect_off));
      return;
    }

  /* Ignore functions with missing or invalid low and high pc attributes.  */
  if (dwarf2_get_pc_bounds (die, &lowpc, &highpc, cu, NULL)
      <= PC_BOUNDS_INVALID)
    {
      attr = dwarf2_attr (die, DW_AT_external, cu);
      if (!attr || !DW_UNSND (attr))
	complaint (&symfile_complaints,
		   _("cannot get low and high bounds "
		     "for subprogram DIE at %d"),
		   to_underlying (die->sect_off));
      return;
    }

  lowpc = gdbarch_adjust_dwarf2_addr (gdbarch, lowpc + baseaddr);
  highpc = gdbarch_adjust_dwarf2_addr (gdbarch, highpc + baseaddr);

  /* If we have any template arguments, then we must allocate a
     different sort of symbol.  */
  for (child_die = die->child; child_die; child_die = sibling_die (child_die))
    {
      if (child_die->tag == DW_TAG_template_type_param
	  || child_die->tag == DW_TAG_template_value_param)
	{
	  templ_func = allocate_template_symbol (objfile);
	  templ_func->base.is_cplus_template_function = 1;
	  break;
	}
    }

  newobj = push_context (0, lowpc);
  newobj->name = new_symbol_full (die, read_type_die (die, cu), cu,
			       (struct symbol *) templ_func);

  /* If there is a location expression for DW_AT_frame_base, record
     it.  */
  attr = dwarf2_attr (die, DW_AT_frame_base, cu);
  if (attr)
    dwarf2_symbol_mark_computed (attr, newobj->name, cu, 1);

  /* If there is a location for the static link, record it.  */
  newobj->static_link = NULL;
  attr = dwarf2_attr (die, DW_AT_static_link, cu);
  if (attr)
    {
      newobj->static_link
	= XOBNEW (&objfile->objfile_obstack, struct dynamic_prop);
      attr_to_dynamic_prop (attr, die, cu, newobj->static_link);
    }

  cu->list_in_scope = &local_symbols;

  if (die->child != NULL)
    {
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  if (child_die->tag == DW_TAG_template_type_param
	      || child_die->tag == DW_TAG_template_value_param)
	    {
	      struct symbol *arg = new_symbol (child_die, NULL, cu);

	      if (arg != NULL)
		VEC_safe_push (symbolp, template_args, arg);
	    }
	  else
	    process_die (child_die, cu);
	  child_die = sibling_die (child_die);
	}
    }

  inherit_abstract_dies (die, cu);

  /* If we have a DW_AT_specification, we might need to import using
     directives from the context of the specification DIE.  See the
     comment in determine_prefix.  */
  if (cu->language == language_cplus
      && dwarf2_attr (die, DW_AT_specification, cu))
    {
      struct dwarf2_cu *spec_cu = cu;
      struct die_info *spec_die = die_specification (die, &spec_cu);

      while (spec_die)
	{
	  child_die = spec_die->child;
	  while (child_die && child_die->tag)
	    {
	      if (child_die->tag == DW_TAG_imported_module)
		process_die (child_die, spec_cu);
	      child_die = sibling_die (child_die);
	    }

	  /* In some cases, GCC generates specification DIEs that
	     themselves contain DW_AT_specification attributes.  */
	  spec_die = die_specification (spec_die, &spec_cu);
	}
    }

  newobj = pop_context ();
  /* Make a block for the local symbols within.  */
  block = finish_block (newobj->name, &local_symbols, newobj->old_blocks,
			newobj->static_link, lowpc, highpc);

  /* For C++, set the block's scope.  */
  if ((cu->language == language_cplus
       || cu->language == language_fortran
       || cu->language == language_d
       || cu->language == language_rust)
      && cu->processing_has_namespace_info)
    block_set_scope (block, determine_prefix (die, cu),
		     &objfile->objfile_obstack);

  /* If we have address ranges, record them.  */
  dwarf2_record_block_ranges (die, block, baseaddr, cu);

  gdbarch_make_symbol_special (gdbarch, newobj->name, objfile);

  /* Attach template arguments to function.  */
  if (! VEC_empty (symbolp, template_args))
    {
      gdb_assert (templ_func != NULL);

      templ_func->n_template_arguments = VEC_length (symbolp, template_args);
      templ_func->template_arguments
        = XOBNEWVEC (&objfile->objfile_obstack, struct symbol *,
		     templ_func->n_template_arguments);
      memcpy (templ_func->template_arguments,
	      VEC_address (symbolp, template_args),
	      (templ_func->n_template_arguments * sizeof (struct symbol *)));
      VEC_free (symbolp, template_args);
    }

  /* In C++, we can have functions nested inside functions (e.g., when
     a function declares a class that has methods).  This means that
     when we finish processing a function scope, we may need to go
     back to building a containing block's symbol lists.  */
  local_symbols = newobj->locals;
  local_using_directives = newobj->local_using_directives;

  /* If we've finished processing a top-level function, subsequent
     symbols go in the file symbol list.  */
  if (outermost_context_p ())
    cu->list_in_scope = &file_symbols;
}

/* Process all the DIES contained within a lexical block scope.  Start
   a new scope, process the dies, and then close the scope.  */

static void
read_lexical_block_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct context_stack *newobj;
  CORE_ADDR lowpc, highpc;
  struct die_info *child_die;
  CORE_ADDR baseaddr;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  /* Ignore blocks with missing or invalid low and high pc attributes.  */
  /* ??? Perhaps consider discontiguous blocks defined by DW_AT_ranges
     as multiple lexical blocks?  Handling children in a sane way would
     be nasty.  Might be easier to properly extend generic blocks to
     describe ranges.  */
  switch (dwarf2_get_pc_bounds (die, &lowpc, &highpc, cu, NULL))
    {
    case PC_BOUNDS_NOT_PRESENT:
      /* DW_TAG_lexical_block has no attributes, process its children as if
	 there was no wrapping by that DW_TAG_lexical_block.
	 GCC does no longer produces such DWARF since GCC r224161.  */
      for (child_die = die->child;
	   child_die != NULL && child_die->tag;
	   child_die = sibling_die (child_die))
	process_die (child_die, cu);
      return;
    case PC_BOUNDS_INVALID:
      return;
    }
  lowpc = gdbarch_adjust_dwarf2_addr (gdbarch, lowpc + baseaddr);
  highpc = gdbarch_adjust_dwarf2_addr (gdbarch, highpc + baseaddr);

  push_context (0, lowpc);
  if (die->child != NULL)
    {
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  process_die (child_die, cu);
	  child_die = sibling_die (child_die);
	}
    }
  inherit_abstract_dies (die, cu);
  newobj = pop_context ();

  if (local_symbols != NULL || local_using_directives != NULL)
    {
      struct block *block
        = finish_block (0, &local_symbols, newobj->old_blocks, NULL,
			newobj->start_addr, highpc);

      /* Note that recording ranges after traversing children, as we
         do here, means that recording a parent's ranges entails
         walking across all its children's ranges as they appear in
         the address map, which is quadratic behavior.

         It would be nicer to record the parent's ranges before
         traversing its children, simply overriding whatever you find
         there.  But since we don't even decide whether to create a
         block until after we've traversed its children, that's hard
         to do.  */
      dwarf2_record_block_ranges (die, block, baseaddr, cu);
    }
  local_symbols = newobj->locals;
  local_using_directives = newobj->local_using_directives;
}

/* Read in DW_TAG_call_site and insert it to CU->call_site_htab.  */

static void
read_call_site_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  CORE_ADDR pc, baseaddr;
  struct attribute *attr;
  struct call_site *call_site, call_site_local;
  void **slot;
  int nparams;
  struct die_info *child_die;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  attr = dwarf2_attr (die, DW_AT_call_return_pc, cu);
  if (attr == NULL)
    {
      /* This was a pre-DWARF-5 GNU extension alias
	 for DW_AT_call_return_pc.  */
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
    }
  if (!attr)
    {
      complaint (&symfile_complaints,
		 _("missing DW_AT_call_return_pc for DW_TAG_call_site "
		   "DIE 0x%x [in module %s]"),
		 to_underlying (die->sect_off), objfile_name (objfile));
      return;
    }
  pc = attr_value_as_address (attr) + baseaddr;
  pc = gdbarch_adjust_dwarf2_addr (gdbarch, pc);

  if (cu->call_site_htab == NULL)
    cu->call_site_htab = htab_create_alloc_ex (16, core_addr_hash, core_addr_eq,
					       NULL, &objfile->objfile_obstack,
					       hashtab_obstack_allocate, NULL);
  call_site_local.pc = pc;
  slot = htab_find_slot (cu->call_site_htab, &call_site_local, INSERT);
  if (*slot != NULL)
    {
      complaint (&symfile_complaints,
		 _("Duplicate PC %s for DW_TAG_call_site "
		   "DIE 0x%x [in module %s]"),
		 paddress (gdbarch, pc), to_underlying (die->sect_off),
		 objfile_name (objfile));
      return;
    }

  /* Count parameters at the caller.  */

  nparams = 0;
  for (child_die = die->child; child_die && child_die->tag;
       child_die = sibling_die (child_die))
    {
      if (child_die->tag != DW_TAG_call_site_parameter
          && child_die->tag != DW_TAG_GNU_call_site_parameter)
	{
	  complaint (&symfile_complaints,
		     _("Tag %d is not DW_TAG_call_site_parameter in "
		       "DW_TAG_call_site child DIE 0x%x [in module %s]"),
		     child_die->tag, to_underlying (child_die->sect_off),
		     objfile_name (objfile));
	  continue;
	}

      nparams++;
    }

  call_site
    = ((struct call_site *)
       obstack_alloc (&objfile->objfile_obstack,
		      sizeof (*call_site)
		      + (sizeof (*call_site->parameter) * (nparams - 1))));
  *slot = call_site;
  memset (call_site, 0, sizeof (*call_site) - sizeof (*call_site->parameter));
  call_site->pc = pc;

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
	      gdb_assert (TYPE_CODE (func_type) == TYPE_CODE_FUNC);

	      /* Enlist this call site to the function.  */
	      call_site->tail_call_next = TYPE_TAIL_CALL_LIST (func_type);
	      TYPE_TAIL_CALL_LIST (func_type) = call_site;
	    }
	  else
	    complaint (&symfile_complaints,
		       _("Cannot find function owning DW_TAG_call_site "
			 "DIE 0x%x [in module %s]"),
		       to_underlying (die->sect_off), objfile_name (objfile));
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
  SET_FIELD_DWARF_BLOCK (call_site->target, NULL);
  if (!attr || (attr_form_is_block (attr) && DW_BLOCK (attr)->size == 0))
    /* Keep NULL DWARF_BLOCK.  */;
  else if (attr_form_is_block (attr))
    {
      struct dwarf2_locexpr_baton *dlbaton;

      dlbaton = XOBNEW (&objfile->objfile_obstack, struct dwarf2_locexpr_baton);
      dlbaton->data = DW_BLOCK (attr)->data;
      dlbaton->size = DW_BLOCK (attr)->size;
      dlbaton->per_cu = cu->per_cu;

      SET_FIELD_DWARF_BLOCK (call_site->target, dlbaton);
    }
  else if (attr_form_is_ref (attr))
    {
      struct dwarf2_cu *target_cu = cu;
      struct die_info *target_die;

      target_die = follow_die_ref (die, attr, &target_cu);
      gdb_assert (target_cu->objfile == objfile);
      if (die_is_declaration (target_die, target_cu))
	{
	  const char *target_physname;

	  /* Prefer the mangled name; otherwise compute the demangled one.  */
	  target_physname = dwarf2_string_attr (target_die,
						DW_AT_linkage_name,
						target_cu);
	  if (target_physname == NULL)
	    target_physname = dwarf2_string_attr (target_die,
						 DW_AT_MIPS_linkage_name,
						 target_cu);
	  if (target_physname == NULL)
	    target_physname = dwarf2_physname (NULL, target_die, target_cu);
	  if (target_physname == NULL)
	    complaint (&symfile_complaints,
		       _("DW_AT_call_target target DIE has invalid "
		         "physname, for referencing DIE 0x%x [in module %s]"),
		       to_underlying (die->sect_off), objfile_name (objfile));
	  else
	    SET_FIELD_PHYSNAME (call_site->target, target_physname);
	}
      else
	{
	  CORE_ADDR lowpc;

	  /* DW_AT_entry_pc should be preferred.  */
	  if (dwarf2_get_pc_bounds (target_die, &lowpc, NULL, target_cu, NULL)
	      <= PC_BOUNDS_INVALID)
	    complaint (&symfile_complaints,
		       _("DW_AT_call_target target DIE has invalid "
		         "low pc, for referencing DIE 0x%x [in module %s]"),
		       to_underlying (die->sect_off), objfile_name (objfile));
	  else
	    {
	      lowpc = gdbarch_adjust_dwarf2_addr (gdbarch, lowpc + baseaddr);
	      SET_FIELD_PHYSADDR (call_site->target, lowpc);
	    }
	}
    }
  else
    complaint (&symfile_complaints,
	       _("DW_TAG_call_site DW_AT_call_target is neither "
		 "block nor reference, for DIE 0x%x [in module %s]"),
	       to_underlying (die->sect_off), objfile_name (objfile));

  call_site->per_cu = cu->per_cu;

  for (child_die = die->child;
       child_die && child_die->tag;
       child_die = sibling_die (child_die))
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
      if (loc == NULL && origin != NULL && attr_form_is_ref (origin))
	{
	  parameter->kind = CALL_SITE_PARAMETER_PARAM_OFFSET;

	  sect_offset sect_off
	    = (sect_offset) dwarf2_get_ref_die_offset (origin);
	  if (!offset_in_cu_p (&cu->header, sect_off))
	    {
	      /* As DW_OP_GNU_parameter_ref uses CU-relative offset this
		 binding can be done only inside one CU.  Such referenced DIE
		 therefore cannot be even moved to DW_TAG_partial_unit.  */
	      complaint (&symfile_complaints,
			 _("DW_AT_call_parameter offset is not in CU for "
			   "DW_TAG_call_site child DIE 0x%x [in module %s]"),
			 to_underlying (child_die->sect_off),
			 objfile_name (objfile));
	      continue;
	    }
	  parameter->u.param_cu_off
	    = (cu_offset) (sect_off - cu->header.sect_off);
	}
      else if (loc == NULL || origin != NULL || !attr_form_is_block (loc))
	{
	  complaint (&symfile_complaints,
		     _("No DW_FORM_block* DW_AT_location for "
		       "DW_TAG_call_site child DIE 0x%x [in module %s]"),
		     to_underlying (child_die->sect_off), objfile_name (objfile));
	  continue;
	}
      else
	{
	  parameter->u.dwarf_reg = dwarf_block_to_dwarf_reg
	    (DW_BLOCK (loc)->data, &DW_BLOCK (loc)->data[DW_BLOCK (loc)->size]);
	  if (parameter->u.dwarf_reg != -1)
	    parameter->kind = CALL_SITE_PARAMETER_DWARF_REG;
	  else if (dwarf_block_to_sp_offset (gdbarch, DW_BLOCK (loc)->data,
				    &DW_BLOCK (loc)->data[DW_BLOCK (loc)->size],
					     &parameter->u.fb_offset))
	    parameter->kind = CALL_SITE_PARAMETER_FB_OFFSET;
	  else
	    {
	      complaint (&symfile_complaints,
			 _("Only single DW_OP_reg or DW_OP_fbreg is supported "
			   "for DW_FORM_block* DW_AT_location is supported for "
			   "DW_TAG_call_site child DIE 0x%x "
			   "[in module %s]"),
			 to_underlying (child_die->sect_off),
			 objfile_name (objfile));
	      continue;
	    }
	}

      attr = dwarf2_attr (child_die, DW_AT_call_value, cu);
      if (attr == NULL)
	attr = dwarf2_attr (child_die, DW_AT_GNU_call_site_value, cu);
      if (!attr_form_is_block (attr))
	{
	  complaint (&symfile_complaints,
		     _("No DW_FORM_block* DW_AT_call_value for "
		       "DW_TAG_call_site child DIE 0x%x [in module %s]"),
		     to_underlying (child_die->sect_off),
		     objfile_name (objfile));
	  continue;
	}
      parameter->value = DW_BLOCK (attr)->data;
      parameter->value_size = DW_BLOCK (attr)->size;

      /* Parameters are not pre-cleared by memset above.  */
      parameter->data_value = NULL;
      parameter->data_value_size = 0;
      call_site->parameter_count++;

      attr = dwarf2_attr (child_die, DW_AT_call_data_value, cu);
      if (attr == NULL)
	attr = dwarf2_attr (child_die, DW_AT_GNU_call_site_data_value, cu);
      if (attr)
	{
	  if (!attr_form_is_block (attr))
	    complaint (&symfile_complaints,
		       _("No DW_FORM_block* DW_AT_call_data_value for "
			 "DW_TAG_call_site child DIE 0x%x [in module %s]"),
		       to_underlying (child_die->sect_off),
		       objfile_name (objfile));
	  else
	    {
	      parameter->data_value = DW_BLOCK (attr)->data;
	      parameter->data_value_size = DW_BLOCK (attr)->size;
	    }
	}
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
			 Callback &&callback)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct comp_unit_head *cu_header = &cu->header;
  bfd *obfd = objfile->obfd;
  unsigned int addr_size = cu_header->addr_size;
  CORE_ADDR mask = ~(~(CORE_ADDR)1 << (addr_size * 8 - 1));
  /* Base address selection entry.  */
  CORE_ADDR base;
  int found_base;
  unsigned int dummy;
  const gdb_byte *buffer;
  CORE_ADDR low = 0;
  CORE_ADDR high = 0;
  CORE_ADDR baseaddr;
  bool overflow = false;

  found_base = cu->base_known;
  base = cu->base_address;

  dwarf2_read_section (objfile, &dwarf2_per_objfile->rnglists);
  if (offset >= dwarf2_per_objfile->rnglists.size)
    {
      complaint (&symfile_complaints,
		 _("Offset %d out of bounds for DW_AT_ranges attribute"),
		 offset);
      return false;
    }
  buffer = dwarf2_per_objfile->rnglists.buffer + offset;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  while (1)
    {
      /* Initialize it due to a false compiler warning.  */
      CORE_ADDR range_beginning = 0, range_end = 0;
      const gdb_byte *buf_end = (dwarf2_per_objfile->rnglists.buffer
				 + dwarf2_per_objfile->rnglists.size);
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
	  base = read_address (obfd, buffer, cu, &bytes_read);
	  found_base = 1;
	  buffer += bytes_read;
	  break;
	case DW_RLE_start_length:
	  if (buffer + cu->header.addr_size > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_beginning = read_address (obfd, buffer, cu, &bytes_read);
	  buffer += bytes_read;
	  range_end = (range_beginning
		       + read_unsigned_leb128 (obfd, buffer, &bytes_read));
	  buffer += bytes_read;
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  break;
	case DW_RLE_offset_pair:
	  range_beginning = read_unsigned_leb128 (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_end = read_unsigned_leb128 (obfd, buffer, &bytes_read);
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
	  range_beginning = read_address (obfd, buffer, cu, &bytes_read);
	  buffer += bytes_read;
	  range_end = read_address (obfd, buffer, cu, &bytes_read);
	  buffer += bytes_read;
	  break;
	default:
	  complaint (&symfile_complaints,
		     _("Invalid .debug_rnglists data (no base address)"));
	  return false;
	}
      if (rlet == DW_RLE_end_of_list || overflow)
	break;
      if (rlet == DW_RLE_base_address)
	continue;

      if (!found_base)
	{
	  /* We have no valid base address for the ranges
	     data.  */
	  complaint (&symfile_complaints,
		     _("Invalid .debug_rnglists data (no base address)"));
	  return false;
	}

      if (range_beginning > range_end)
	{
	  /* Inverted range entries are invalid.  */
	  complaint (&symfile_complaints,
		     _("Invalid .debug_rnglists data (inverted range)"));
	  return false;
	}

      /* Empty range entries have no effect.  */
      if (range_beginning == range_end)
	continue;

      range_beginning += base;
      range_end += base;

      /* A not-uncommon case of bad debug info.
	 Don't pollute the addrmap with bad data.  */
      if (range_beginning + baseaddr == 0
	  && !dwarf2_per_objfile->has_section_at_zero)
	{
	  complaint (&symfile_complaints,
		     _(".debug_rnglists entry has start address of zero"
		       " [in module %s]"), objfile_name (objfile));
	  continue;
	}

      callback (range_beginning, range_end);
    }

  if (overflow)
    {
      complaint (&symfile_complaints,
		 _("Offset %d is not terminated "
		   "for DW_AT_ranges attribute"),
		 offset);
      return false;
    }

  return true;
}

/* Call CALLBACK from DW_AT_ranges attribute value OFFSET reading .debug_ranges.
   Callback's type should be:
    void (CORE_ADDR range_beginning, CORE_ADDR range_end)
   Return 1 if the attributes are present and valid, otherwise, return 0.  */

template <typename Callback>
static int
dwarf2_ranges_process (unsigned offset, struct dwarf2_cu *cu,
		       Callback &&callback)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct comp_unit_head *cu_header = &cu->header;
  bfd *obfd = objfile->obfd;
  unsigned int addr_size = cu_header->addr_size;
  CORE_ADDR mask = ~(~(CORE_ADDR)1 << (addr_size * 8 - 1));
  /* Base address selection entry.  */
  CORE_ADDR base;
  int found_base;
  unsigned int dummy;
  const gdb_byte *buffer;
  CORE_ADDR baseaddr;

  if (cu_header->version >= 5)
    return dwarf2_rnglists_process (offset, cu, callback);

  found_base = cu->base_known;
  base = cu->base_address;

  dwarf2_read_section (objfile, &dwarf2_per_objfile->ranges);
  if (offset >= dwarf2_per_objfile->ranges.size)
    {
      complaint (&symfile_complaints,
		 _("Offset %d out of bounds for DW_AT_ranges attribute"),
		 offset);
      return 0;
    }
  buffer = dwarf2_per_objfile->ranges.buffer + offset;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  while (1)
    {
      CORE_ADDR range_beginning, range_end;

      range_beginning = read_address (obfd, buffer, cu, &dummy);
      buffer += addr_size;
      range_end = read_address (obfd, buffer, cu, &dummy);
      buffer += addr_size;
      offset += 2 * addr_size;

      /* An end of list marker is a pair of zero addresses.  */
      if (range_beginning == 0 && range_end == 0)
	/* Found the end of list entry.  */
	break;

      /* Each base address selection entry is a pair of 2 values.
	 The first is the largest possible address, the second is
	 the base address.  Check for a base address here.  */
      if ((range_beginning & mask) == mask)
	{
	  /* If we found the largest possible address, then we already
	     have the base address in range_end.  */
	  base = range_end;
	  found_base = 1;
	  continue;
	}

      if (!found_base)
	{
	  /* We have no valid base address for the ranges
	     data.  */
	  complaint (&symfile_complaints,
		     _("Invalid .debug_ranges data (no base address)"));
	  return 0;
	}

      if (range_beginning > range_end)
	{
	  /* Inverted range entries are invalid.  */
	  complaint (&symfile_complaints,
		     _("Invalid .debug_ranges data (inverted range)"));
	  return 0;
	}

      /* Empty range entries have no effect.  */
      if (range_beginning == range_end)
	continue;

      range_beginning += base;
      range_end += base;

      /* A not-uncommon case of bad debug info.
	 Don't pollute the addrmap with bad data.  */
      if (range_beginning + baseaddr == 0
	  && !dwarf2_per_objfile->has_section_at_zero)
	{
	  complaint (&symfile_complaints,
		     _(".debug_ranges entry has start address of zero"
		       " [in module %s]"), objfile_name (objfile));
	  continue;
	}

      callback (range_beginning, range_end);
    }

  return 1;
}

/* Get low and high pc attributes from DW_AT_ranges attribute value OFFSET.
   Return 1 if the attributes are present and valid, otherwise, return 0.
   If RANGES_PST is not NULL we should setup `objfile->psymtabs_addrmap'.  */

static int
dwarf2_ranges_read (unsigned offset, CORE_ADDR *low_return,
		    CORE_ADDR *high_return, struct dwarf2_cu *cu,
		    struct partial_symtab *ranges_pst)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  const CORE_ADDR baseaddr = ANOFFSET (objfile->section_offsets,
				       SECT_OFF_TEXT (objfile));
  int low_set = 0;
  CORE_ADDR low = 0;
  CORE_ADDR high = 0;
  int retval;

  retval = dwarf2_ranges_process (offset, cu,
    [&] (CORE_ADDR range_beginning, CORE_ADDR range_end)
    {
      if (ranges_pst != NULL)
	{
	  CORE_ADDR lowpc;
	  CORE_ADDR highpc;

	  lowpc = gdbarch_adjust_dwarf2_addr (gdbarch,
					      range_beginning + baseaddr);
	  highpc = gdbarch_adjust_dwarf2_addr (gdbarch,
					       range_end + baseaddr);
	  addrmap_set_empty (objfile->psymtabs_addrmap, lowpc, highpc - 1,
			     ranges_pst);
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

/* Get low and high pc attributes from a die.  See enum pc_bounds_kind
   definition for the return value.  *LOWPC and *HIGHPC are set iff
   neither PC_BOUNDS_NOT_PRESENT nor PC_BOUNDS_INVALID are returned.  */

static enum pc_bounds_kind
dwarf2_get_pc_bounds (struct die_info *die, CORE_ADDR *lowpc,
		      CORE_ADDR *highpc, struct dwarf2_cu *cu,
		      struct partial_symtab *pst)
{
  struct attribute *attr;
  struct attribute *attr_high;
  CORE_ADDR low = 0;
  CORE_ADDR high = 0;
  enum pc_bounds_kind ret;

  attr_high = dwarf2_attr (die, DW_AT_high_pc, cu);
  if (attr_high)
    {
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
      if (attr)
        {
	  low = attr_value_as_address (attr);
	  high = attr_value_as_address (attr_high);
	  if (cu->header.version >= 4 && attr_form_is_constant (attr_high))
	    high += low;
	}
      else
	/* Found high w/o low attribute.  */
	return PC_BOUNDS_INVALID;

      /* Found consecutive range of addresses.  */
      ret = PC_BOUNDS_HIGH_LOW;
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_ranges, cu);
      if (attr != NULL)
	{
	  /* DW_AT_ranges_base does not apply to DIEs from the DWO skeleton.
	     We take advantage of the fact that DW_AT_ranges does not appear
	     in DW_TAG_compile_unit of DWO files.  */
	  int need_ranges_base = die->tag != DW_TAG_compile_unit;
	  unsigned int ranges_offset = (DW_UNSND (attr)
					+ (need_ranges_base
					   ? cu->ranges_base
					   : 0));

	  /* Value of the DW_AT_ranges attribute is the offset in the
	     .debug_ranges section.  */
	  if (!dwarf2_ranges_read (ranges_offset, &low, &high, cu, pst))
	    return PC_BOUNDS_INVALID;
	  /* Found discontinuous range of addresses.  */
	  ret = PC_BOUNDS_RANGES;
	}
      else
	return PC_BOUNDS_NOT_PRESENT;
    }

  /* read_partial_die has also the strict LOW < HIGH requirement.  */
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
  if (low == 0 && !dwarf2_per_objfile->has_section_at_zero)
    return PC_BOUNDS_INVALID;

  *lowpc = low;
  if (highpc)
    *highpc = high;
  return ret;
}

/* Assuming that DIE represents a subprogram DIE or a lexical block, get
   its low and high PC addresses.  Do nothing if these addresses could not
   be determined.  Otherwise, set LOWPC to the low address if it is smaller,
   and HIGHPC to the high address if greater than HIGHPC.  */

static void
dwarf2_get_subprogram_pc_bounds (struct die_info *die,
                                 CORE_ADDR *lowpc, CORE_ADDR *highpc,
                                 struct dwarf2_cu *cu)
{
  CORE_ADDR low, high;
  struct die_info *child = die->child;

  if (dwarf2_get_pc_bounds (die, &low, &high, cu, NULL) >= PC_BOUNDS_RANGES)
    {
      *lowpc = std::min (*lowpc, low);
      *highpc = std::max (*highpc, high);
    }

  /* If the language does not allow nested subprograms (either inside
     subprograms or lexical blocks), we're done.  */
  if (cu->language != language_ada)
    return;

  /* Check all the children of the given DIE.  If it contains nested
     subprograms, then check their pc bounds.  Likewise, we need to
     check lexical blocks as well, as they may also contain subprogram
     definitions.  */
  while (child && child->tag)
    {
      if (child->tag == DW_TAG_subprogram
          || child->tag == DW_TAG_lexical_block)
        dwarf2_get_subprogram_pc_bounds (child, lowpc, highpc, cu);
      child = sibling_die (child);
    }
}

/* Get the low and high pc's represented by the scope DIE, and store
   them in *LOWPC and *HIGHPC.  If the correct values can't be
   determined, set *LOWPC to -1 and *HIGHPC to 0.  */

static void
get_scope_pc_bounds (struct die_info *die,
		     CORE_ADDR *lowpc, CORE_ADDR *highpc,
		     struct dwarf2_cu *cu)
{
  CORE_ADDR best_low = (CORE_ADDR) -1;
  CORE_ADDR best_high = (CORE_ADDR) 0;
  CORE_ADDR current_low, current_high;

  if (dwarf2_get_pc_bounds (die, &current_low, &current_high, cu, NULL)
      >= PC_BOUNDS_RANGES)
    {
      best_low = current_low;
      best_high = current_high;
    }
  else
    {
      struct die_info *child = die->child;

      while (child && child->tag)
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

	    if (current_low != ((CORE_ADDR) -1))
	      {
		best_low = std::min (best_low, current_low);
		best_high = std::max (best_high, current_high);
	      }
	    break;
	  default:
	    /* Ignore.  */
	    break;
	  }

	  child = sibling_die (child);
	}
    }

  *lowpc = best_low;
  *highpc = best_high;
}

/* Record the address ranges for BLOCK, offset by BASEADDR, as given
   in DIE.  */

static void
dwarf2_record_block_ranges (struct die_info *die, struct block *block,
                            CORE_ADDR baseaddr, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct attribute *attr;
  struct attribute *attr_high;

  attr_high = dwarf2_attr (die, DW_AT_high_pc, cu);
  if (attr_high)
    {
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
      if (attr)
        {
          CORE_ADDR low = attr_value_as_address (attr);
	  CORE_ADDR high = attr_value_as_address (attr_high);

	  if (cu->header.version >= 4 && attr_form_is_constant (attr_high))
	    high += low;

	  low = gdbarch_adjust_dwarf2_addr (gdbarch, low + baseaddr);
	  high = gdbarch_adjust_dwarf2_addr (gdbarch, high + baseaddr);
	  record_block_range (block, low, high - 1);
        }
    }

  attr = dwarf2_attr (die, DW_AT_ranges, cu);
  if (attr)
    {
      bfd *obfd = objfile->obfd;
      /* DW_AT_ranges_base does not apply to DIEs from the DWO skeleton.
	 We take advantage of the fact that DW_AT_ranges does not appear
	 in DW_TAG_compile_unit of DWO files.  */
      int need_ranges_base = die->tag != DW_TAG_compile_unit;

      /* The value of the DW_AT_ranges attribute is the offset of the
         address range list in the .debug_ranges section.  */
      unsigned long offset = (DW_UNSND (attr)
			      + (need_ranges_base ? cu->ranges_base : 0));
      const gdb_byte *buffer;

      /* For some target architectures, but not others, the
         read_address function sign-extends the addresses it returns.
         To recognize base address selection entries, we need a
         mask.  */
      unsigned int addr_size = cu->header.addr_size;
      CORE_ADDR base_select_mask = ~(~(CORE_ADDR)1 << (addr_size * 8 - 1));

      /* The base address, to which the next pair is relative.  Note
         that this 'base' is a DWARF concept: most entries in a range
         list are relative, to reduce the number of relocs against the
         debugging information.  This is separate from this function's
         'baseaddr' argument, which GDB uses to relocate debugging
         information from a shared library based on the address at
         which the library was loaded.  */
      CORE_ADDR base = cu->base_address;
      int base_known = cu->base_known;

      dwarf2_ranges_process (offset, cu,
	[&] (CORE_ADDR start, CORE_ADDR end)
	{
	  start += baseaddr;
	  end += baseaddr;
	  start = gdbarch_adjust_dwarf2_addr (gdbarch, start);
	  end = gdbarch_adjust_dwarf2_addr (gdbarch, end);
	  record_block_range (block, start, end - 1);
	});
    }
}

/* Check whether the producer field indicates either of GCC < 4.6, or the
   Intel C/C++ compiler, and cache the result in CU.  */

static void
check_producer (struct dwarf2_cu *cu)
{
  int major, minor;

  if (cu->producer == NULL)
    {
      /* For unknown compilers expect their behavior is DWARF version
	 compliant.

	 GCC started to support .debug_types sections by -gdwarf-4 since
	 gcc-4.5.x.  As the .debug_types sections are missing DW_AT_producer
	 for their space efficiency GDB cannot workaround gcc-4.5.x -gdwarf-4
	 combination.  gcc-4.5.x -gdwarf-4 binaries have DW_AT_accessibility
	 interpreted incorrectly by GDB now - GCC PR debug/48229.  */
    }
  else if (producer_is_gcc (cu->producer, &major, &minor))
    {
      cu->producer_is_gxx_lt_4_6 = major < 4 || (major == 4 && minor < 6);
      cu->producer_is_gcc_lt_4_3 = major < 4 || (major == 4 && minor < 3);
    }
  else if (startswith (cu->producer, "Intel(R) C"))
    cu->producer_is_icc = 1;
  else
    {
      /* For other non-GCC compilers, expect their behavior is DWARF version
	 compliant.  */
    }

  cu->checked_producer = 1;
}

/* Check for GCC PR debug/45124 fix which is not present in any G++ version up
   to 4.5.any while it is present already in G++ 4.6.0 - the PR has been fixed
   during 4.6.0 experimental.  */

static int
producer_is_gxx_lt_4_6 (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_gxx_lt_4_6;
}

/* Return the default accessibility type if it is not overriden by
   DW_AT_accessibility.  */

static enum dwarf_access_attribute
dwarf2_default_access_attribute (struct die_info *die, struct dwarf2_cu *cu)
{
  if (cu->header.version < 3 || producer_is_gxx_lt_4_6 (cu))
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

/* Look for DW_AT_data_member_location.  Set *OFFSET to the byte
   offset.  If the attribute was not found return 0, otherwise return
   1.  If it was found but could not properly be handled, set *OFFSET
   to 0.  */

static int
handle_data_member_location (struct die_info *die, struct dwarf2_cu *cu,
			     LONGEST *offset)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_data_member_location, cu);
  if (attr != NULL)
    {
      *offset = 0;

      /* Note that we do not check for a section offset first here.
	 This is because DW_AT_data_member_location is new in DWARF 4,
	 so if we see it, we can assume that a constant form is really
	 a constant and not a section offset.  */
      if (attr_form_is_constant (attr))
	*offset = dwarf2_get_attr_constant_value (attr, 0);
      else if (attr_form_is_section_offset (attr))
	dwarf2_complex_location_expr_complaint ();
      else if (attr_form_is_block (attr))
	*offset = decode_locdesc (DW_BLOCK (attr), cu);
      else
	dwarf2_complex_location_expr_complaint ();

      return 1;
    }

  return 0;
}

/* Add an aggregate field to the field list.  */

static void
dwarf2_add_field (struct field_info *fip, struct die_info *die,
		  struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct nextfield *new_field;
  struct attribute *attr;
  struct field *fp;
  const char *fieldname = "";

  /* Allocate a new field list entry and link it in.  */
  new_field = XNEW (struct nextfield);
  make_cleanup (xfree, new_field);
  memset (new_field, 0, sizeof (struct nextfield));

  if (die->tag == DW_TAG_inheritance)
    {
      new_field->next = fip->baseclasses;
      fip->baseclasses = new_field;
    }
  else
    {
      new_field->next = fip->fields;
      fip->fields = new_field;
    }
  fip->nfields++;

  attr = dwarf2_attr (die, DW_AT_accessibility, cu);
  if (attr)
    new_field->accessibility = DW_UNSND (attr);
  else
    new_field->accessibility = dwarf2_default_access_attribute (die, cu);
  if (new_field->accessibility != DW_ACCESS_public)
    fip->non_public_fields = 1;

  attr = dwarf2_attr (die, DW_AT_virtuality, cu);
  if (attr)
    new_field->virtuality = DW_UNSND (attr);
  else
    new_field->virtuality = DW_VIRTUALITY_none;

  fp = &new_field->field;

  if (die->tag == DW_TAG_member && ! die_is_declaration (die, cu))
    {
      LONGEST offset;

      /* Data member other than a C++ static data member.  */

      /* Get type of field.  */
      fp->type = die_type (die, cu);

      SET_FIELD_BITPOS (*fp, 0);

      /* Get bit size of field (zero if none).  */
      attr = dwarf2_attr (die, DW_AT_bit_size, cu);
      if (attr)
	{
	  FIELD_BITSIZE (*fp) = DW_UNSND (attr);
	}
      else
	{
	  FIELD_BITSIZE (*fp) = 0;
	}

      /* Get bit offset of field.  */
      if (handle_data_member_location (die, cu, &offset))
	SET_FIELD_BITPOS (*fp, offset * bits_per_byte);
      attr = dwarf2_attr (die, DW_AT_bit_offset, cu);
      if (attr)
	{
	  if (gdbarch_bits_big_endian (gdbarch))
	    {
	      /* For big endian bits, the DW_AT_bit_offset gives the
	         additional bit offset from the MSB of the containing
	         anonymous object to the MSB of the field.  We don't
	         have to do anything special since we don't need to
	         know the size of the anonymous object.  */
	      SET_FIELD_BITPOS (*fp, FIELD_BITPOS (*fp) + DW_UNSND (attr));
	    }
	  else
	    {
	      /* For little endian bits, compute the bit offset to the
	         MSB of the anonymous object, subtract off the number of
	         bits from the MSB of the field to the MSB of the
	         object, and then subtract off the number of bits of
	         the field itself.  The result is the bit offset of
	         the LSB of the field.  */
	      int anonymous_size;
	      int bit_offset = DW_UNSND (attr);

	      attr = dwarf2_attr (die, DW_AT_byte_size, cu);
	      if (attr)
		{
		  /* The size of the anonymous object containing
		     the bit field is explicit, so use the
		     indicated size (in bytes).  */
		  anonymous_size = DW_UNSND (attr);
		}
	      else
		{
		  /* The size of the anonymous object containing
		     the bit field must be inferred from the type
		     attribute of the data member containing the
		     bit field.  */
		  anonymous_size = TYPE_LENGTH (fp->type);
		}
	      SET_FIELD_BITPOS (*fp,
				(FIELD_BITPOS (*fp)
				 + anonymous_size * bits_per_byte
				 - bit_offset - FIELD_BITSIZE (*fp)));
	    }
	}
      attr = dwarf2_attr (die, DW_AT_data_bit_offset, cu);
      if (attr != NULL)
	SET_FIELD_BITPOS (*fp, (FIELD_BITPOS (*fp)
				+ dwarf2_get_attr_constant_value (attr, 0)));

      /* Get name of field.  */
      fieldname = dwarf2_name (die, cu);
      if (fieldname == NULL)
	fieldname = "";

      /* The name is already allocated along with this objfile, so we don't
	 need to duplicate it for the type.  */
      fp->name = fieldname;

      /* Change accessibility for artificial fields (e.g. virtual table
         pointer or virtual base class pointer) to private.  */
      if (dwarf2_attr (die, DW_AT_artificial, cu))
	{
	  FIELD_ARTIFICIAL (*fp) = 1;
	  new_field->accessibility = DW_ACCESS_private;
	  fip->non_public_fields = 1;
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
      SET_FIELD_PHYSNAME (*fp, physname ? physname : "");
      FIELD_TYPE (*fp) = die_type (die, cu);
      FIELD_NAME (*fp) = fieldname;
    }
  else if (die->tag == DW_TAG_inheritance)
    {
      LONGEST offset;

      /* C++ base class field.  */
      if (handle_data_member_location (die, cu, &offset))
	SET_FIELD_BITPOS (*fp, offset * bits_per_byte);
      FIELD_BITSIZE (*fp) = 0;
      FIELD_TYPE (*fp) = die_type (die, cu);
      FIELD_NAME (*fp) = type_name_no_tag (fp->type);
      fip->nbaseclasses++;
    }
}

/* Add a typedef defined in the scope of the FIP's class.  */

static void
dwarf2_add_typedef (struct field_info *fip, struct die_info *die,
		    struct dwarf2_cu *cu)
{
  struct typedef_field_list *new_field;
  struct typedef_field *fp;

  /* Allocate a new field list entry and link it in.  */
  new_field = XCNEW (struct typedef_field_list);
  make_cleanup (xfree, new_field);

  gdb_assert (die->tag == DW_TAG_typedef);

  fp = &new_field->field;

  /* Get name of field.  */
  fp->name = dwarf2_name (die, cu);
  if (fp->name == NULL)
    return;

  fp->type = read_type_die (die, cu);

  new_field->next = fip->typedef_field_list;
  fip->typedef_field_list = new_field;
  fip->typedef_field_list_count++;
}

/* Create the vector of fields, and attach it to the type.  */

static void
dwarf2_attach_fields_to_type (struct field_info *fip, struct type *type,
			      struct dwarf2_cu *cu)
{
  int nfields = fip->nfields;

  /* Record the field count, allocate space for the array of fields,
     and create blank accessibility bitfields if necessary.  */
  TYPE_NFIELDS (type) = nfields;
  TYPE_FIELDS (type) = (struct field *)
    TYPE_ALLOC (type, sizeof (struct field) * nfields);
  memset (TYPE_FIELDS (type), 0, sizeof (struct field) * nfields);

  if (fip->non_public_fields && cu->language != language_ada)
    {
      ALLOCATE_CPLUS_STRUCT_TYPE (type);

      TYPE_FIELD_PRIVATE_BITS (type) =
	(B_TYPE *) TYPE_ALLOC (type, B_BYTES (nfields));
      B_CLRALL (TYPE_FIELD_PRIVATE_BITS (type), nfields);

      TYPE_FIELD_PROTECTED_BITS (type) =
	(B_TYPE *) TYPE_ALLOC (type, B_BYTES (nfields));
      B_CLRALL (TYPE_FIELD_PROTECTED_BITS (type), nfields);

      TYPE_FIELD_IGNORE_BITS (type) =
	(B_TYPE *) TYPE_ALLOC (type, B_BYTES (nfields));
      B_CLRALL (TYPE_FIELD_IGNORE_BITS (type), nfields);
    }

  /* If the type has baseclasses, allocate and clear a bit vector for
     TYPE_FIELD_VIRTUAL_BITS.  */
  if (fip->nbaseclasses && cu->language != language_ada)
    {
      int num_bytes = B_BYTES (fip->nbaseclasses);
      unsigned char *pointer;

      ALLOCATE_CPLUS_STRUCT_TYPE (type);
      pointer = (unsigned char *) TYPE_ALLOC (type, num_bytes);
      TYPE_FIELD_VIRTUAL_BITS (type) = pointer;
      B_CLRALL (TYPE_FIELD_VIRTUAL_BITS (type), fip->nbaseclasses);
      TYPE_N_BASECLASSES (type) = fip->nbaseclasses;
    }

  /* Copy the saved-up fields into the field vector.  Start from the head of
     the list, adding to the tail of the field array, so that they end up in
     the same order in the array in which they were added to the list.  */
  while (nfields-- > 0)
    {
      struct nextfield *fieldp;

      if (fip->fields)
	{
	  fieldp = fip->fields;
	  fip->fields = fieldp->next;
	}
      else
	{
	  fieldp = fip->baseclasses;
	  fip->baseclasses = fieldp->next;
	}

      TYPE_FIELD (type, nfields) = fieldp->field;
      switch (fieldp->accessibility)
	{
	case DW_ACCESS_private:
	  if (cu->language != language_ada)
	    SET_TYPE_FIELD_PRIVATE (type, nfields);
	  break;

	case DW_ACCESS_protected:
	  if (cu->language != language_ada)
	    SET_TYPE_FIELD_PROTECTED (type, nfields);
	  break;

	case DW_ACCESS_public:
	  break;

	default:
	  /* Unknown accessibility.  Complain and treat it as public.  */
	  {
	    complaint (&symfile_complaints, _("unsupported accessibility %d"),
		       fieldp->accessibility);
	  }
	  break;
	}
      if (nfields < fip->nbaseclasses)
	{
	  switch (fieldp->virtuality)
	    {
	    case DW_VIRTUALITY_virtual:
	    case DW_VIRTUALITY_pure_virtual:
	      if (cu->language == language_ada)
		error (_("unexpected virtuality in component of Ada type"));
	      SET_TYPE_FIELD_VIRTUAL (type, nfields);
	      break;
	    }
	}
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
  struct objfile *objfile = cu->objfile;
  struct attribute *attr;
  struct fnfieldlist *flp;
  int i;
  struct fn_field *fnp;
  const char *fieldname;
  struct nextfnfield *new_fnfield;
  struct type *this_type;
  enum dwarf_access_attribute accessibility;

  if (cu->language == language_ada)
    error (_("unexpected member function in Ada type"));

  /* Get name of member function.  */
  fieldname = dwarf2_name (die, cu);
  if (fieldname == NULL)
    return;

  /* Look up member function name in fieldlist.  */
  for (i = 0; i < fip->nfnfields; i++)
    {
      if (strcmp (fip->fnfieldlists[i].name, fieldname) == 0)
	break;
    }

  /* Create new list element if necessary.  */
  if (i < fip->nfnfields)
    flp = &fip->fnfieldlists[i];
  else
    {
      if ((fip->nfnfields % DW_FIELD_ALLOC_CHUNK) == 0)
	{
	  fip->fnfieldlists = (struct fnfieldlist *)
	    xrealloc (fip->fnfieldlists,
		      (fip->nfnfields + DW_FIELD_ALLOC_CHUNK)
		      * sizeof (struct fnfieldlist));
	  if (fip->nfnfields == 0)
	    make_cleanup (free_current_contents, &fip->fnfieldlists);
	}
      flp = &fip->fnfieldlists[fip->nfnfields];
      flp->name = fieldname;
      flp->length = 0;
      flp->head = NULL;
      i = fip->nfnfields++;
    }

  /* Create a new member function field and chain it to the field list
     entry.  */
  new_fnfield = XNEW (struct nextfnfield);
  make_cleanup (xfree, new_fnfield);
  memset (new_fnfield, 0, sizeof (struct nextfnfield));
  new_fnfield->next = flp->head;
  flp->head = new_fnfield;
  flp->length++;

  /* Fill in the member function field info.  */
  fnp = &new_fnfield->fnfield;

  /* Delay processing of the physname until later.  */
  if (cu->language == language_cplus)
    {
      add_to_method_list (type, i, flp->length - 1, fieldname,
			  die, cu);
    }
  else
    {
      const char *physname = dwarf2_physname (fieldname, die, cu);
      fnp->physname = physname ? physname : "";
    }

  fnp->type = alloc_type (objfile);
  this_type = read_type_die (die, cu);
  if (this_type && TYPE_CODE (this_type) == TYPE_CODE_FUNC)
    {
      int nparams = TYPE_NFIELDS (this_type);

      /* TYPE is the domain of this method, and THIS_TYPE is the type
	   of the method itself (TYPE_CODE_METHOD).  */
      smash_to_method_type (fnp->type, type,
			    TYPE_TARGET_TYPE (this_type),
			    TYPE_FIELDS (this_type),
			    TYPE_NFIELDS (this_type),
			    TYPE_VARARGS (this_type));

      /* Handle static member functions.
         Dwarf2 has no clean way to discern C++ static and non-static
         member functions.  G++ helps GDB by marking the first
         parameter for non-static member functions (which is the this
         pointer) as artificial.  We obtain this information from
         read_subroutine_type via TYPE_FIELD_ARTIFICIAL.  */
      if (nparams == 0 || TYPE_FIELD_ARTIFICIAL (this_type, 0) == 0)
	fnp->voffset = VOFFSET_STATIC;
    }
  else
    complaint (&symfile_complaints, _("member function type missing for '%s'"),
	       dwarf2_full_name (fieldname, die, cu));

  /* Get fcontext from DW_AT_containing_type if present.  */
  if (dwarf2_attr (die, DW_AT_containing_type, cu) != NULL)
    fnp->fcontext = die_containing_type (die, cu);

  /* dwarf2 doesn't have stubbed physical names, so the setting of is_const and
     is_volatile is irrelevant, as it is needed by gdb_mangle_name only.  */

  /* Get accessibility.  */
  attr = dwarf2_attr (die, DW_AT_accessibility, cu);
  if (attr)
    accessibility = (enum dwarf_access_attribute) DW_UNSND (attr);
  else
    accessibility = dwarf2_default_access_attribute (die, cu);
  switch (accessibility)
    {
    case DW_ACCESS_private:
      fnp->is_private = 1;
      break;
    case DW_ACCESS_protected:
      fnp->is_protected = 1;
      break;
    }

  /* Check for artificial methods.  */
  attr = dwarf2_attr (die, DW_AT_artificial, cu);
  if (attr && DW_UNSND (attr) != 0)
    fnp->is_artificial = 1;

  fnp->is_constructor = dwarf2_is_constructor (die, cu);

  /* Get index in virtual function table if it is a virtual member
     function.  For older versions of GCC, this is an offset in the
     appropriate virtual table, as specified by DW_AT_containing_type.
     For everyone else, it is an expression to be evaluated relative
     to the object address.  */

  attr = dwarf2_attr (die, DW_AT_vtable_elem_location, cu);
  if (attr)
    {
      if (attr_form_is_block (attr) && DW_BLOCK (attr)->size > 0)
        {
	  if (DW_BLOCK (attr)->data[0] == DW_OP_constu)
	    {
	      /* Old-style GCC.  */
	      fnp->voffset = decode_locdesc (DW_BLOCK (attr), cu) + 2;
	    }
	  else if (DW_BLOCK (attr)->data[0] == DW_OP_deref
		   || (DW_BLOCK (attr)->size > 1
		       && DW_BLOCK (attr)->data[0] == DW_OP_deref_size
		       && DW_BLOCK (attr)->data[1] == cu->header.addr_size))
	    {
	      fnp->voffset = decode_locdesc (DW_BLOCK (attr), cu);
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
	      if (TYPE_NFIELDS (this_type) == 0
		  || !TYPE_FIELD_ARTIFICIAL (this_type, 0))
		{
		  complaint (&symfile_complaints,
			     _("cannot determine context for virtual member "
			       "function \"%s\" (offset %d)"),
			     fieldname, to_underlying (die->sect_off));
		}
	      else
		{
		  fnp->fcontext
		    = TYPE_TARGET_TYPE (TYPE_FIELD_TYPE (this_type, 0));
		}
	    }
	}
      else if (attr_form_is_section_offset (attr))
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
      if (attr && DW_UNSND (attr))
	{
	  /* GCC does this, as of 2008-08-25; PR debug/37237.  */
	  complaint (&symfile_complaints,
		     _("Member function \"%s\" (offset %d) is virtual "
		       "but the vtable offset is not specified"),
		     fieldname, to_underlying (die->sect_off));
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
  struct fnfieldlist *flp;
  int i;

  if (cu->language == language_ada)
    error (_("unexpected member functions in Ada type"));

  ALLOCATE_CPLUS_STRUCT_TYPE (type);
  TYPE_FN_FIELDLISTS (type) = (struct fn_fieldlist *)
    TYPE_ALLOC (type, sizeof (struct fn_fieldlist) * fip->nfnfields);

  for (i = 0, flp = fip->fnfieldlists; i < fip->nfnfields; i++, flp++)
    {
      struct nextfnfield *nfp = flp->head;
      struct fn_fieldlist *fn_flp = &TYPE_FN_FIELDLIST (type, i);
      int k;

      TYPE_FN_FIELDLIST_NAME (type, i) = flp->name;
      TYPE_FN_FIELDLIST_LENGTH (type, i) = flp->length;
      fn_flp->fn_fields = (struct fn_field *)
	TYPE_ALLOC (type, sizeof (struct fn_field) * flp->length);
      for (k = flp->length; (k--, nfp); nfp = nfp->next)
	fn_flp->fn_fields[k] = nfp->fnfield;
    }

  TYPE_NFN_FIELDS (type) = fip->nfnfields;
}

/* Returns non-zero if NAME is the name of a vtable member in CU's
   language, zero otherwise.  */
static int
is_vtable_name (const char *name, struct dwarf2_cu *cu)
{
  static const char vptr[] = "_vptr";
  static const char vtable[] = "vtable";

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
  if (TYPE_CODE (type) != TYPE_CODE_STRUCT || TYPE_NFIELDS (type) != 2)
    return;

  /* Check for __pfn and __delta members.  */
  if (TYPE_FIELD_NAME (type, 0) == NULL
      || strcmp (TYPE_FIELD_NAME (type, 0), "__pfn") != 0
      || TYPE_FIELD_NAME (type, 1) == NULL
      || strcmp (TYPE_FIELD_NAME (type, 1), "__delta") != 0)
    return;

  /* Find the type of the method.  */
  pfn_type = TYPE_FIELD_TYPE (type, 0);
  if (pfn_type == NULL
      || TYPE_CODE (pfn_type) != TYPE_CODE_PTR
      || TYPE_CODE (TYPE_TARGET_TYPE (pfn_type)) != TYPE_CODE_FUNC)
    return;

  /* Look for the "this" argument.  */
  pfn_type = TYPE_TARGET_TYPE (pfn_type);
  if (TYPE_NFIELDS (pfn_type) == 0
      /* || TYPE_FIELD_TYPE (pfn_type, 0) == NULL */
      || TYPE_CODE (TYPE_FIELD_TYPE (pfn_type, 0)) != TYPE_CODE_PTR)
    return;

  self_type = TYPE_TARGET_TYPE (TYPE_FIELD_TYPE (pfn_type, 0));
  new_type = alloc_type (objfile);
  smash_to_method_type (new_type, self_type, TYPE_TARGET_TYPE (pfn_type),
			TYPE_FIELDS (pfn_type), TYPE_NFIELDS (pfn_type),
			TYPE_VARARGS (pfn_type));
  smash_to_methodptr_type (type, new_type);
}

/* Return non-zero if the CU's PRODUCER string matches the Intel C/C++ compiler
   (icc).  */

static int
producer_is_icc (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_icc;
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
  struct objfile *objfile = cu->objfile;
  struct type *type;
  struct attribute *attr;
  const char *name;

  /* If the definition of this type lives in .debug_types, read that type.
     Don't follow DW_AT_specification though, that will take us back up
     the chain and we want to go down.  */
  attr = dwarf2_attr_no_follow (die, DW_AT_signature);
  if (attr)
    {
      type = get_DW_AT_signature_type (die, attr, cu);

      /* The type's CU may not be the same as CU.
	 Ensure TYPE is recorded with CU in die_type_hash.  */
      return set_die_type (die, type, cu);
    }

  type = alloc_type (objfile);
  INIT_CPLUS_SPECIFIC (type);

  name = dwarf2_name (die, cu);
  if (name != NULL)
    {
      if (cu->language == language_cplus
	  || cu->language == language_d
	  || cu->language == language_rust)
	{
	  const char *full_name = dwarf2_full_name (name, die, cu);

	  /* dwarf2_full_name might have already finished building the DIE's
	     type.  If so, there is no need to continue.  */
	  if (get_die_type (die, cu) != NULL)
	    return get_die_type (die, cu);

	  TYPE_TAG_NAME (type) = full_name;
	  if (die->tag == DW_TAG_structure_type
	      || die->tag == DW_TAG_class_type)
	    TYPE_NAME (type) = TYPE_TAG_NAME (type);
	}
      else
	{
	  /* The name is already allocated along with this objfile, so
	     we don't need to duplicate it for the type.  */
	  TYPE_TAG_NAME (type) = name;
	  if (die->tag == DW_TAG_class_type)
	    TYPE_NAME (type) = TYPE_TAG_NAME (type);
	}
    }

  if (die->tag == DW_TAG_structure_type)
    {
      TYPE_CODE (type) = TYPE_CODE_STRUCT;
    }
  else if (die->tag == DW_TAG_union_type)
    {
      TYPE_CODE (type) = TYPE_CODE_UNION;
    }
  else
    {
      TYPE_CODE (type) = TYPE_CODE_STRUCT;
    }

  if (cu->language == language_cplus && die->tag == DW_TAG_class_type)
    TYPE_DECLARED_CLASS (type) = 1;

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr)
    {
      if (attr_form_is_constant (attr))
        TYPE_LENGTH (type) = DW_UNSND (attr);
      else
	{
	  /* For the moment, dynamic type sizes are not supported
	     by GDB's struct type.  The actual size is determined
	     on-demand when resolving the type of a given object,
	     so set the type's length to zero for now.  Otherwise,
	     we record an expression as the length, and that expression
	     could lead to a very large value, which could eventually
	     lead to us trying to allocate that much memory when creating
	     a value of that type.  */
          TYPE_LENGTH (type) = 0;
	}
    }
  else
    {
      TYPE_LENGTH (type) = 0;
    }

  if (producer_is_icc (cu) && (TYPE_LENGTH (type) == 0))
    {
      /* ICC does not output the required DW_AT_declaration
	 on incomplete types, but gives them a size of zero.  */
      TYPE_STUB (type) = 1;
    }
  else
    TYPE_STUB_SUPPORTED (type) = 1;

  if (die_is_declaration (die, cu))
    TYPE_STUB (type) = 1;
  else if (attr == NULL && die->child == NULL
	   && producer_is_realview (cu->producer))
    /* RealView does not output the required DW_AT_declaration
       on incomplete types.  */
    TYPE_STUB (type) = 1;

  /* We need to add the type field to the die immediately so we don't
     infinitely recurse when dealing with pointers to the structure
     type within the structure itself.  */
  set_die_type (die, type, cu);

  /* set_die_type should be already done.  */
  set_descriptive_type (type, die, cu);

  return type;
}

/* Finish creating a structure or union type, including filling in
   its members and creating a symbol for it.  */

static void
process_structure_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct die_info *child_die;
  struct type *type;

  type = get_die_type (die, cu);
  if (type == NULL)
    type = read_structure_type (die, cu);

  if (die->child != NULL && ! die_is_declaration (die, cu))
    {
      struct field_info fi;
      VEC (symbolp) *template_args = NULL;
      struct cleanup *back_to = make_cleanup (null_cleanup, 0);

      memset (&fi, 0, sizeof (struct field_info));

      child_die = die->child;

      while (child_die && child_die->tag)
	{
	  if (child_die->tag == DW_TAG_member
	      || child_die->tag == DW_TAG_variable)
	    {
	      /* NOTE: carlton/2002-11-05: A C++ static data member
		 should be a DW_TAG_member that is a declaration, but
		 all versions of G++ as of this writing (so through at
		 least 3.2.1) incorrectly generate DW_TAG_variable
		 tags for them instead.  */
	      dwarf2_add_field (&fi, child_die, cu);
	    }
	  else if (child_die->tag == DW_TAG_subprogram)
	    {
	      /* Rust doesn't have member functions in the C++ sense.
		 However, it does emit ordinary functions as children
		 of a struct DIE.  */
	      if (cu->language == language_rust)
		read_func_scope (child_die, cu);
	      else
		{
		  /* C++ member function.  */
		  dwarf2_add_member_fn (&fi, child_die, type, cu);
		}
	    }
	  else if (child_die->tag == DW_TAG_inheritance)
	    {
	      /* C++ base class field.  */
	      dwarf2_add_field (&fi, child_die, cu);
	    }
	  else if (child_die->tag == DW_TAG_typedef)
	    dwarf2_add_typedef (&fi, child_die, cu);
	  else if (child_die->tag == DW_TAG_template_type_param
		   || child_die->tag == DW_TAG_template_value_param)
	    {
	      struct symbol *arg = new_symbol (child_die, NULL, cu);

	      if (arg != NULL)
		VEC_safe_push (symbolp, template_args, arg);
	    }

	  child_die = sibling_die (child_die);
	}

      /* Attach template arguments to type.  */
      if (! VEC_empty (symbolp, template_args))
	{
	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  TYPE_N_TEMPLATE_ARGUMENTS (type)
	    = VEC_length (symbolp, template_args);
	  TYPE_TEMPLATE_ARGUMENTS (type)
	    = XOBNEWVEC (&objfile->objfile_obstack,
			 struct symbol *,
			 TYPE_N_TEMPLATE_ARGUMENTS (type));
	  memcpy (TYPE_TEMPLATE_ARGUMENTS (type),
		  VEC_address (symbolp, template_args),
		  (TYPE_N_TEMPLATE_ARGUMENTS (type)
		   * sizeof (struct symbol *)));
	  VEC_free (symbolp, template_args);
	}

      /* Attach fields and member functions to the type.  */
      if (fi.nfields)
	dwarf2_attach_fields_to_type (&fi, type, cu);
      if (fi.nfnfields)
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
		  for (i = TYPE_NFIELDS (t) - 1;
		       i >= TYPE_N_BASECLASSES (t);
		       --i)
		    {
		      const char *fieldname = TYPE_FIELD_NAME (t, i);

                      if (is_vtable_name (fieldname, cu))
			{
			  set_type_vptr_fieldno (type, i);
			  break;
			}
		    }

		  /* Complain if virtual function table field not found.  */
		  if (i < TYPE_N_BASECLASSES (t))
		    complaint (&symfile_complaints,
			       _("virtual function table pointer "
				 "not found when defining class '%s'"),
			       TYPE_TAG_NAME (type) ? TYPE_TAG_NAME (type) :
			       "");
		}
	      else
		{
		  set_type_vptr_fieldno (type, TYPE_VPTR_FIELDNO (t));
		}
	    }
	  else if (cu->producer
		   && startswith (cu->producer, "IBM(R) XL C/C++ Advanced Edition"))
	    {
	      /* The IBM XLC compiler does not provide direct indication
	         of the containing type, but the vtable pointer is
	         always named __vfp.  */

	      int i;

	      for (i = TYPE_NFIELDS (type) - 1;
		   i >= TYPE_N_BASECLASSES (type);
		   --i)
		{
		  if (strcmp (TYPE_FIELD_NAME (type, i), "__vfp") == 0)
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
      if (fi.typedef_field_list)
	{
	  int i = fi.typedef_field_list_count;

	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  TYPE_TYPEDEF_FIELD_ARRAY (type)
	    = ((struct typedef_field *)
	       TYPE_ALLOC (type, sizeof (TYPE_TYPEDEF_FIELD (type, 0)) * i));
	  TYPE_TYPEDEF_FIELD_COUNT (type) = i;

	  /* Reverse the list order to keep the debug info elements order.  */
	  while (--i >= 0)
	    {
	      struct typedef_field *dest, *src;

	      dest = &TYPE_TYPEDEF_FIELD (type, i);
	      src = &fi.typedef_field_list->field;
	      fi.typedef_field_list = fi.typedef_field_list->next;
	      *dest = *src;
	    }
	}

      do_cleanups (back_to);
    }

  quirk_gcc_member_function_pointer (type, objfile);

  /* NOTE: carlton/2004-03-16: GCC 3.4 (or at least one of its
     snapshots) has been known to create a die giving a declaration
     for a class that has, as a child, a die giving a definition for a
     nested class.  So we have to process our children even if the
     current die is a declaration.  Normally, of course, a declaration
     won't have any children at all.  */

  child_die = die->child;

  while (child_die != NULL && child_die->tag)
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

      child_die = sibling_die (child_die);
    }

  /* Do not consider external references.  According to the DWARF standard,
     these DIEs are identified by the fact that they have no byte_size
     attribute, and a declaration attribute.  */
  if (dwarf2_attr (die, DW_AT_byte_size, cu) != NULL
      || !die_is_declaration (die, cu))
    new_symbol (die, type, cu);
}

/* Assuming DIE is an enumeration type, and TYPE is its associated type,
   update TYPE using some information only available in DIE's children.  */

static void
update_enumeration_type_from_children (struct die_info *die,
				       struct type *type,
				       struct dwarf2_cu *cu)
{
  struct obstack obstack;
  struct die_info *child_die;
  int unsigned_enum = 1;
  int flag_enum = 1;
  ULONGEST mask = 0;
  struct cleanup *old_chain;

  obstack_init (&obstack);
  old_chain = make_cleanup_obstack_free (&obstack);

  for (child_die = die->child;
       child_die != NULL && child_die->tag;
       child_die = sibling_die (child_die))
    {
      struct attribute *attr;
      LONGEST value;
      const gdb_byte *bytes;
      struct dwarf2_locexpr_baton *baton;
      const char *name;

      if (child_die->tag != DW_TAG_enumerator)
	continue;

      attr = dwarf2_attr (child_die, DW_AT_const_value, cu);
      if (attr == NULL)
	continue;

      name = dwarf2_name (child_die, cu);
      if (name == NULL)
	name = "<anonymous enumerator>";

      dwarf2_const_value_attr (attr, type, name, &obstack, cu,
			       &value, &bytes, &baton);
      if (value < 0)
	{
	  unsigned_enum = 0;
	  flag_enum = 0;
	}
      else if ((mask & value) != 0)
	flag_enum = 0;
      else
	mask |= value;

      /* If we already know that the enum type is neither unsigned, nor
	 a flag type, no need to look at the rest of the enumerates.  */
      if (!unsigned_enum && !flag_enum)
	break;
    }

  if (unsigned_enum)
    TYPE_UNSIGNED (type) = 1;
  if (flag_enum)
    TYPE_FLAG_ENUM (type) = 1;

  do_cleanups (old_chain);
}

/* Given a DW_AT_enumeration_type die, set its type.  We do not
   complete the type's fields yet, or create any symbols.  */

static struct type *
read_enumeration_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct type *type;
  struct attribute *attr;
  const char *name;

  /* If the definition of this type lives in .debug_types, read that type.
     Don't follow DW_AT_specification though, that will take us back up
     the chain and we want to go down.  */
  attr = dwarf2_attr_no_follow (die, DW_AT_signature);
  if (attr)
    {
      type = get_DW_AT_signature_type (die, attr, cu);

      /* The type's CU may not be the same as CU.
	 Ensure TYPE is recorded with CU in die_type_hash.  */
      return set_die_type (die, type, cu);
    }

  type = alloc_type (objfile);

  TYPE_CODE (type) = TYPE_CODE_ENUM;
  name = dwarf2_full_name (NULL, die, cu);
  if (name != NULL)
    TYPE_TAG_NAME (type) = name;

  attr = dwarf2_attr (die, DW_AT_type, cu);
  if (attr != NULL)
    {
      struct type *underlying_type = die_type (die, cu);

      TYPE_TARGET_TYPE (type) = underlying_type;
    }

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr)
    {
      TYPE_LENGTH (type) = DW_UNSND (attr);
    }
  else
    {
      TYPE_LENGTH (type) = 0;
    }

  /* The enumeration DIE can be incomplete.  In Ada, any type can be
     declared as private in the package spec, and then defined only
     inside the package body.  Such types are known as Taft Amendment
     Types.  When another package uses such a type, an incomplete DIE
     may be generated by the compiler.  */
  if (die_is_declaration (die, cu))
    TYPE_STUB (type) = 1;

  /* Finish the creation of this type by using the enum's children.
     We must call this even when the underlying type has been provided
     so that we can determine if we're looking at a "flag" enum.  */
  update_enumeration_type_from_children (die, type, cu);

  /* If this type has an underlying type that is not a stub, then we
     may use its attributes.  We always use the "unsigned" attribute
     in this situation, because ordinarily we guess whether the type
     is unsigned -- but the guess can be wrong and the underlying type
     can tell us the reality.  However, we defer to a local size
     attribute if one exists, because this lets the compiler override
     the underlying type if needed.  */
  if (TYPE_TARGET_TYPE (type) != NULL && !TYPE_STUB (TYPE_TARGET_TYPE (type)))
    {
      TYPE_UNSIGNED (type) = TYPE_UNSIGNED (TYPE_TARGET_TYPE (type));
      if (TYPE_LENGTH (type) == 0)
	TYPE_LENGTH (type) = TYPE_LENGTH (TYPE_TARGET_TYPE (type));
    }

  TYPE_DECLARED_CLASS (type) = dwarf2_flag_true_p (die, DW_AT_enum_class, cu);

  return set_die_type (die, type, cu);
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
      struct die_info *child_die;
      struct symbol *sym;
      struct field *fields = NULL;
      int num_fields = 0;
      const char *name;

      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  if (child_die->tag != DW_TAG_enumerator)
	    {
	      process_die (child_die, cu);
	    }
	  else
	    {
	      name = dwarf2_name (child_die, cu);
	      if (name)
		{
		  sym = new_symbol (child_die, this_type, cu);

		  if ((num_fields % DW_FIELD_ALLOC_CHUNK) == 0)
		    {
		      fields = (struct field *)
			xrealloc (fields,
				  (num_fields + DW_FIELD_ALLOC_CHUNK)
				  * sizeof (struct field));
		    }

		  FIELD_NAME (fields[num_fields]) = SYMBOL_LINKAGE_NAME (sym);
		  FIELD_TYPE (fields[num_fields]) = NULL;
		  SET_FIELD_ENUMVAL (fields[num_fields], SYMBOL_VALUE (sym));
		  FIELD_BITSIZE (fields[num_fields]) = 0;

		  num_fields++;
		}
	    }

	  child_die = sibling_die (child_die);
	}

      if (num_fields)
	{
	  TYPE_NFIELDS (this_type) = num_fields;
	  TYPE_FIELDS (this_type) = (struct field *)
	    TYPE_ALLOC (this_type, sizeof (struct field) * num_fields);
	  memcpy (TYPE_FIELDS (this_type), fields,
		  sizeof (struct field) * num_fields);
	  xfree (fields);
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

/* Extract all information from a DW_TAG_array_type DIE and put it in
   the DIE's type field.  For now, this only handles one dimensional
   arrays.  */

static struct type *
read_array_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct die_info *child_die;
  struct type *type;
  struct type *element_type, *range_type, *index_type;
  struct type **range_types = NULL;
  struct attribute *attr;
  int ndim = 0;
  struct cleanup *back_to;
  const char *name;
  unsigned int bit_stride = 0;

  element_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  attr = dwarf2_attr (die, DW_AT_byte_stride, cu);
  if (attr != NULL)
    bit_stride = DW_UNSND (attr) * 8;

  attr = dwarf2_attr (die, DW_AT_bit_stride, cu);
  if (attr != NULL)
    bit_stride = DW_UNSND (attr);

  /* Irix 6.2 native cc creates array types without children for
     arrays with unspecified length.  */
  if (die->child == NULL)
    {
      index_type = objfile_type (objfile)->builtin_int;
      range_type = create_static_range_type (NULL, index_type, 0, -1);
      type = create_array_type_with_stride (NULL, element_type, range_type,
					    bit_stride);
      return set_die_type (die, type, cu);
    }

  back_to = make_cleanup (null_cleanup, NULL);
  child_die = die->child;
  while (child_die && child_die->tag)
    {
      if (child_die->tag == DW_TAG_subrange_type)
	{
	  struct type *child_type = read_type_die (child_die, cu);

          if (child_type != NULL)
            {
	      /* The range type was succesfully read.  Save it for the
                 array type creation.  */
              if ((ndim % DW_FIELD_ALLOC_CHUNK) == 0)
                {
                  range_types = (struct type **)
                    xrealloc (range_types, (ndim + DW_FIELD_ALLOC_CHUNK)
                              * sizeof (struct type *));
                  if (ndim == 0)
                    make_cleanup (free_current_contents, &range_types);
	        }
	      range_types[ndim++] = child_type;
            }
	}
      child_die = sibling_die (child_die);
    }

  /* Dwarf2 dimensions are output from left to right, create the
     necessary array types in backwards order.  */

  type = element_type;

  if (read_array_order (die, cu) == DW_ORD_col_major)
    {
      int i = 0;

      while (i < ndim)
	type = create_array_type_with_stride (NULL, type, range_types[i++],
					      bit_stride);
    }
  else
    {
      while (ndim-- > 0)
	type = create_array_type_with_stride (NULL, type, range_types[ndim],
					      bit_stride);
    }

  /* Understand Dwarf2 support for vector types (like they occur on
     the PowerPC w/ AltiVec).  Gcc just adds another attribute to the
     array type.  This is not part of the Dwarf2/3 standard yet, but a
     custom vendor extension.  The main difference between a regular
     array and the vector variant is that vectors are passed by value
     to functions.  */
  attr = dwarf2_attr (die, DW_AT_GNU_vector, cu);
  if (attr)
    make_vector_type (type);

  /* The DIE may have DW_AT_byte_size set.  For example an OpenCL
     implementation may choose to implement triple vectors using this
     attribute.  */
  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr)
    {
      if (DW_UNSND (attr) >= TYPE_LENGTH (type))
	TYPE_LENGTH (type) = DW_UNSND (attr);
      else
	complaint (&symfile_complaints,
		   _("DW_AT_byte_size for array type smaller "
		     "than the total size of elements"));
    }

  name = dwarf2_name (die, cu);
  if (name)
    TYPE_NAME (type) = name;

  /* Install the type in the die.  */
  set_die_type (die, type, cu);

  /* set_die_type should be already done.  */
  set_descriptive_type (type, die, cu);

  do_cleanups (back_to);

  return type;
}

static enum dwarf_array_dim_ordering
read_array_order (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_ordering, cu);

  if (attr)
    return (enum dwarf_array_dim_ordering) DW_SND (attr);

  /* GNU F77 is a special case, as at 08/2004 array type info is the
     opposite order to the dwarf2 specification, but data is still
     laid out as per normal fortran.

     FIXME: dsl/2004-8-20: If G77 is ever fixed, this will also need
     version checking.  */

  if (cu->language == language_fortran
      && cu->producer && strstr (cu->producer, "GNU F77"))
    {
      return DW_ORD_row_major;
    }

  switch (cu->language_defn->la_array_ordering)
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

  set_type = create_set_type (NULL, domain_type);

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr)
    TYPE_LENGTH (set_type) = DW_UNSND (attr);

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
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_locexpr_baton *baton;
  gdb_byte *ptr;
  unsigned int cu_off;
  enum bfd_endian byte_order = gdbarch_byte_order (get_objfile_arch (objfile));
  LONGEST offset = 0;

  gdb_assert (common_loc && member_loc);
  gdb_assert (attr_form_is_block (common_loc));
  gdb_assert (attr_form_is_block (member_loc)
	      || attr_form_is_constant (member_loc));

  baton = XOBNEW (&objfile->objfile_obstack, struct dwarf2_locexpr_baton);
  baton->per_cu = cu->per_cu;
  gdb_assert (baton->per_cu);

  baton->size = 5 /* DW_OP_call4 */ + 1 /* DW_OP_plus */;

  if (attr_form_is_constant (member_loc))
    {
      offset = dwarf2_get_attr_constant_value (member_loc, 0);
      baton->size += 1 /* DW_OP_addr */ + cu->header.addr_size;
    }
  else
    baton->size += DW_BLOCK (member_loc)->size;

  ptr = (gdb_byte *) obstack_alloc (&objfile->objfile_obstack, baton->size);
  baton->data = ptr;

  *ptr++ = DW_OP_call4;
  cu_off = common_die->sect_off - cu->per_cu->sect_off;
  store_unsigned_integer (ptr, 4, byte_order, cu_off);
  ptr += 4;

  if (attr_form_is_constant (member_loc))
    {
      *ptr++ = DW_OP_addr;
      store_unsigned_integer (ptr, cu->header.addr_size, byte_order, offset);
      ptr += cu->header.addr_size;
    }
  else
    {
      /* We have to copy the data here, because DW_OP_call4 will only
	 use a DW_AT_location attribute.  */
      memcpy (ptr, DW_BLOCK (member_loc)->data, DW_BLOCK (member_loc)->size);
      ptr += DW_BLOCK (member_loc)->size;
    }

  *ptr++ = DW_OP_plus;
  gdb_assert (ptr - baton->data == baton->size);

  SYMBOL_LOCATION_BATON (sym) = baton;
  SYMBOL_ACLASS_INDEX (sym) = dwarf2_locexpr_index;
}

/* Create appropriate locally-scoped variables for all the
   DW_TAG_common_block entries.  Also create a struct common_block
   listing all such variables for `info common'.  COMMON_BLOCK_DOMAIN
   is used to sepate the common blocks name namespace from regular
   variable names.  */

static void
read_common_block (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_location, cu);
  if (attr)
    {
      /* Support the .debug_loc offsets.  */
      if (attr_form_is_block (attr))
        {
	  /* Ok.  */
        }
      else if (attr_form_is_section_offset (attr))
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
      struct objfile *objfile = cu->objfile;
      struct die_info *child_die;
      size_t n_entries = 0, size;
      struct common_block *common_block;
      struct symbol *sym;

      for (child_die = die->child;
	   child_die && child_die->tag;
	   child_die = sibling_die (child_die))
	++n_entries;

      size = (sizeof (struct common_block)
	      + (n_entries - 1) * sizeof (struct symbol *));
      common_block
	= (struct common_block *) obstack_alloc (&objfile->objfile_obstack,
						 size);
      memset (common_block->contents, 0, n_entries * sizeof (struct symbol *));
      common_block->n_entries = 0;

      for (child_die = die->child;
	   child_die && child_die->tag;
	   child_die = sibling_die (child_die))
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
		  complaint (&symfile_complaints,
			     _("Variable in common block has "
			       "DW_AT_data_member_location "
			       "- DIE at 0x%x [in module %s]"),
			     to_underlying (child_die->sect_off),
			     objfile_name (cu->objfile));

		  if (attr_form_is_section_offset (member_loc))
		    dwarf2_complex_location_expr_complaint ();
		  else if (attr_form_is_constant (member_loc)
			   || attr_form_is_block (member_loc))
		    {
		      if (attr)
			mark_common_block_symbol_computed (sym, die, attr,
							   member_loc, cu);
		    }
		  else
		    dwarf2_complex_location_expr_complaint ();
		}
	    }
	}

      sym = new_symbol (die, objfile_type (objfile)->builtin_void, cu);
      SYMBOL_VALUE_COMMON_BLOCK (sym) = common_block;
    }
}

/* Create a type for a C++ namespace.  */

static struct type *
read_namespace_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
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
  if (previous_prefix[0] != '\0')
    name = typename_concat (&objfile->objfile_obstack,
			    previous_prefix, name, 0, cu);

  /* Create the type.  */
  type = init_type (objfile, TYPE_CODE_NAMESPACE, 0, name);
  TYPE_TAG_NAME (type) = TYPE_NAME (type);

  return set_die_type (die, type, cu);
}

/* Read a namespace scope.  */

static void
read_namespace (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
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

	  add_using_directive (using_directives (cu->language),
			       previous_prefix, TYPE_NAME (type), NULL,
			       NULL, NULL, 0, &objfile->objfile_obstack);
	}
    }

  if (die->child != NULL)
    {
      struct die_info *child_die = die->child;

      while (child_die && child_die->tag)
	{
	  process_die (child_die, cu);
	  child_die = sibling_die (child_die);
	}
    }
}

/* Read a Fortran module as type.  This DIE can be only a declaration used for
   imported module.  Still we need that type as local Fortran "use ... only"
   declaration imports depend on the created type in determine_prefix.  */

static struct type *
read_module_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  const char *module_name;
  struct type *type;

  module_name = dwarf2_name (die, cu);
  if (!module_name)
    complaint (&symfile_complaints,
	       _("DW_TAG_module has no name, offset 0x%x"),
               to_underlying (die->sect_off));
  type = init_type (objfile, TYPE_CODE_MODULE, 0, module_name);

  /* determine_prefix uses TYPE_TAG_NAME.  */
  TYPE_TAG_NAME (type) = TYPE_NAME (type);

  return set_die_type (die, type, cu);
}

/* Read a Fortran module.  */

static void
read_module (struct die_info *die, struct dwarf2_cu *cu)
{
  struct die_info *child_die = die->child;
  struct type *type;

  type = read_type_die (die, cu);
  new_symbol (die, type, cu);

  while (child_die && child_die->tag)
    {
      process_die (child_die, cu);
      child_die = sibling_die (child_die);
    }
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
  struct gdbarch *gdbarch = get_objfile_arch (cu->objfile);
  struct comp_unit_head *cu_header = &cu->header;
  struct type *type;
  struct attribute *attr_byte_size;
  struct attribute *attr_address_class;
  int byte_size, addr_class;
  struct type *target_type;

  target_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  type = lookup_pointer_type (target_type);

  attr_byte_size = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr_byte_size)
    byte_size = DW_UNSND (attr_byte_size);
  else
    byte_size = cu_header->addr_size;

  attr_address_class = dwarf2_attr (die, DW_AT_address_class, cu);
  if (attr_address_class)
    addr_class = DW_UNSND (attr_address_class);
  else
    addr_class = DW_ADDR_none;

  /* If the pointer size or address class is different than the
     default, create a type variant marked as such and set the
     length accordingly.  */
  if (TYPE_LENGTH (type) != byte_size || addr_class != DW_ADDR_none)
    {
      if (gdbarch_address_class_type_flags_p (gdbarch))
	{
	  int type_flags;

	  type_flags = gdbarch_address_class_type_flags
			 (gdbarch, byte_size, addr_class);
	  gdb_assert ((type_flags & ~TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL)
		      == 0);
	  type = make_type_with_address_space (type, type_flags);
	}
      else if (TYPE_LENGTH (type) != byte_size)
	{
	  complaint (&symfile_complaints,
		     _("invalid pointer size %d"), byte_size);
	}
      else
	{
	  /* Should we also complain about unhandled address classes?  */
	}
    }

  TYPE_LENGTH (type) = byte_size;
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

  if (TYPE_CODE (check_typedef (to_type)) == TYPE_CODE_METHOD)
    type = lookup_methodptr_type (to_type);
  else if (TYPE_CODE (check_typedef (to_type)) == TYPE_CODE_FUNC)
    {
      struct type *new_type = alloc_type (cu->objfile);

      smash_to_method_type (new_type, domain, TYPE_TARGET_TYPE (to_type),
			    TYPE_FIELDS (to_type), TYPE_NFIELDS (to_type),
			    TYPE_VARARGS (to_type));
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
  struct comp_unit_head *cu_header = &cu->header;
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
  if (attr)
    {
      TYPE_LENGTH (type) = DW_UNSND (attr);
    }
  else
    {
      TYPE_LENGTH (type) = cu_header->addr_size;
    }
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

  while (TYPE_CODE (TYPE_TARGET_TYPE (inner_array)) == TYPE_CODE_ARRAY)
    {
      TYPE_TARGET_TYPE (inner_array) =
	copy_type (TYPE_TARGET_TYPE (inner_array));
      inner_array = TYPE_TARGET_TYPE (inner_array);
    }

  el_type = TYPE_TARGET_TYPE (inner_array);
  cnst |= TYPE_CONST (el_type);
  voltl |= TYPE_VOLATILE (el_type);
  TYPE_TARGET_TYPE (inner_array) = make_cv_type (cnst, voltl, el_type, NULL);

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
  if (TYPE_CODE (base_type) == TYPE_CODE_ARRAY)
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
  if (TYPE_CODE (base_type) == TYPE_CODE_ARRAY)
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
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct type *type, *range_type, *index_type, *char_type;
  struct attribute *attr;
  unsigned int length;

  attr = dwarf2_attr (die, DW_AT_string_length, cu);
  if (attr)
    {
      length = DW_UNSND (attr);
    }
  else
    {
      /* Check for the DW_AT_byte_size attribute.  */
      attr = dwarf2_attr (die, DW_AT_byte_size, cu);
      if (attr)
        {
          length = DW_UNSND (attr);
        }
      else
        {
          length = 1;
        }
    }

  index_type = objfile_type (objfile)->builtin_int;
  range_type = create_static_range_type (NULL, index_type, 1, length);
  char_type = language_string_char_type (cu->language_defn, gdbarch);
  type = create_string_type (NULL, char_type, range_type);

  return set_die_type (die, type, cu);
}

/* Assuming that DIE corresponds to a function, returns nonzero
   if the function is prototyped.  */

static int
prototyped_function_p (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_prototyped, cu);
  if (attr && (DW_UNSND (attr) != 0))
    return 1;

  /* The DWARF standard implies that the DW_AT_prototyped attribute
     is only meaninful for C, but the concept also extends to other
     languages that allow unprototyped functions (Eg: Objective C).
     For all other languages, assume that functions are always
     prototyped.  */
  if (cu->language != language_c
      && cu->language != language_objc
      && cu->language != language_opencl)
    return 1;

  /* RealView does not emit DW_AT_prototyped.  We can not distinguish
     prototyped and unprototyped functions; default to prototyped,
     since that is more common in modern code (and RealView warns
     about unprototyped functions).  */
  if (producer_is_realview (cu->producer))
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
  struct objfile *objfile = cu->objfile;
  struct type *type;		/* Type that this function returns.  */
  struct type *ftype;		/* Function that returns above type.  */
  struct attribute *attr;

  type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  ftype = get_die_type (die, cu);
  if (ftype)
    return ftype;

  ftype = lookup_function_type (type);

  if (prototyped_function_p (die, cu))
    TYPE_PROTOTYPED (ftype) = 1;

  /* Store the calling convention in the type if it's available in
     the subroutine die.  Otherwise set the calling convention to
     the default value DW_CC_normal.  */
  attr = dwarf2_attr (die, DW_AT_calling_convention, cu);
  if (attr)
    TYPE_CALLING_CONVENTION (ftype) = DW_UNSND (attr);
  else if (cu->producer && strstr (cu->producer, "IBM XL C for OpenCL"))
    TYPE_CALLING_CONVENTION (ftype) = DW_CC_GDB_IBM_OpenCL;
  else
    TYPE_CALLING_CONVENTION (ftype) = DW_CC_normal;

  /* Record whether the function returns normally to its caller or not
     if the DWARF producer set that information.  */
  attr = dwarf2_attr (die, DW_AT_noreturn, cu);
  if (attr && (DW_UNSND (attr) != 0))
    TYPE_NO_RETURN (ftype) = 1;

  /* We need to add the subroutine type to the die immediately so
     we don't infinitely recurse when dealing with parameters
     declared as the same subroutine type.  */
  set_die_type (die, ftype, cu);

  if (die->child != NULL)
    {
      struct type *void_type = objfile_type (objfile)->builtin_void;
      struct die_info *child_die;
      int nparams, iparams;

      /* Count the number of parameters.
         FIXME: GDB currently ignores vararg functions, but knows about
         vararg member functions.  */
      nparams = 0;
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  if (child_die->tag == DW_TAG_formal_parameter)
	    nparams++;
	  else if (child_die->tag == DW_TAG_unspecified_parameters)
	    TYPE_VARARGS (ftype) = 1;
	  child_die = sibling_die (child_die);
	}

      /* Allocate storage for parameters and fill them in.  */
      TYPE_NFIELDS (ftype) = nparams;
      TYPE_FIELDS (ftype) = (struct field *)
	TYPE_ZALLOC (ftype, nparams * sizeof (struct field));

      /* TYPE_FIELD_TYPE must never be NULL.  Pre-fill the array to ensure it
	 even if we error out during the parameters reading below.  */
      for (iparams = 0; iparams < nparams; iparams++)
	TYPE_FIELD_TYPE (ftype, iparams) = void_type;

      iparams = 0;
      child_die = die->child;
      while (child_die && child_die->tag)
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
	      if (attr)
		TYPE_FIELD_ARTIFICIAL (ftype, iparams) = DW_UNSND (attr);
	      else
		TYPE_FIELD_ARTIFICIAL (ftype, iparams) = 0;
	      arg_type = die_type (child_die, cu);

	      /* RealView does not mark THIS as const, which the testsuite
		 expects.  GCC marks THIS as const in method definitions,
		 but not in the class specifications (GCC PR 43053).  */
	      if (cu->language == language_cplus && !TYPE_CONST (arg_type)
		  && TYPE_FIELD_ARTIFICIAL (ftype, iparams))
		{
		  int is_this = 0;
		  struct dwarf2_cu *arg_cu = cu;
		  const char *name = dwarf2_name (child_die, cu);

		  attr = dwarf2_attr (die, DW_AT_object_pointer, cu);
		  if (attr)
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

	      TYPE_FIELD_TYPE (ftype, iparams) = arg_type;
	      iparams++;
	    }
	  child_die = sibling_die (child_die);
	}
    }

  return ftype;
}

static struct type *
read_typedef (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  const char *name = NULL;
  struct type *this_type, *target_type;

  name = dwarf2_full_name (NULL, die, cu);
  this_type = init_type (objfile, TYPE_CODE_TYPEDEF, 0, name);
  TYPE_TARGET_STUB (this_type) = 1;
  set_die_type (die, this_type, cu);
  target_type = die_type (die, cu);
  if (target_type != this_type)
    TYPE_TARGET_TYPE (this_type) = target_type;
  else
    {
      /* Self-referential typedefs are, it seems, not allowed by the DWARF
	 spec and cause infinite loops in GDB.  */
      complaint (&symfile_complaints,
		 _("Self-referential DW_TAG_typedef "
		   "- DIE at 0x%x [in module %s]"),
		 to_underlying (die->sect_off), objfile_name (objfile));
      TYPE_TARGET_TYPE (this_type) = NULL;
    }
  return this_type;
}

/* Allocate a floating-point type of size BITS and name NAME.  Pass NAME_HINT
   (which may be different from NAME) to the architecture back-end to allow
   it to guess the correct format if necessary.  */

static struct type *
dwarf2_init_float_type (struct objfile *objfile, int bits, const char *name,
			const char *name_hint)
{
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  const struct floatformat **format;
  struct type *type;

  format = gdbarch_floatformat_for_type (gdbarch, name_hint, bits);
  if (format)
    type = init_float_type (objfile, bits, name, format);
  else
    type = init_type (objfile, TYPE_CODE_ERROR, bits / TARGET_CHAR_BIT, name);

  return type;
}

/* Find a representation of a given base type and install
   it in the TYPE field of the die.  */

static struct type *
read_base_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct type *type;
  struct attribute *attr;
  int encoding = 0, bits = 0;
  const char *name;

  attr = dwarf2_attr (die, DW_AT_encoding, cu);
  if (attr)
    {
      encoding = DW_UNSND (attr);
    }
  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr)
    {
      bits = DW_UNSND (attr) * TARGET_CHAR_BIT;
    }
  name = dwarf2_name (die, cu);
  if (!name)
    {
      complaint (&symfile_complaints,
		 _("DW_AT_name missing from DW_TAG_base_type"));
    }

  switch (encoding)
    {
      case DW_ATE_address:
	/* Turn DW_ATE_address into a void * pointer.  */
	type = init_type (objfile, TYPE_CODE_VOID, 1, NULL);
	type = init_pointer_type (objfile, bits, name, type);
	break;
      case DW_ATE_boolean:
	type = init_boolean_type (objfile, bits, 1, name);
	break;
      case DW_ATE_complex_float:
	type = dwarf2_init_float_type (objfile, bits / 2, NULL, name);
	type = init_complex_type (objfile, name, type);
	break;
      case DW_ATE_decimal_float:
	type = init_decfloat_type (objfile, bits, name);
	break;
      case DW_ATE_float:
	type = dwarf2_init_float_type (objfile, bits, name, name);
	break;
      case DW_ATE_signed:
	type = init_integer_type (objfile, bits, 0, name);
	break;
      case DW_ATE_unsigned:
	if (cu->language == language_fortran
	    && name
	    && startswith (name, "character("))
	  type = init_character_type (objfile, bits, 1, name);
	else
	  type = init_integer_type (objfile, bits, 1, name);
	break;
      case DW_ATE_signed_char:
	if (cu->language == language_ada || cu->language == language_m2
	    || cu->language == language_pascal
	    || cu->language == language_fortran)
	  type = init_character_type (objfile, bits, 0, name);
	else
	  type = init_integer_type (objfile, bits, 0, name);
	break;
      case DW_ATE_unsigned_char:
	if (cu->language == language_ada || cu->language == language_m2
	    || cu->language == language_pascal
	    || cu->language == language_fortran
	    || cu->language == language_rust)
	  type = init_character_type (objfile, bits, 1, name);
	else
	  type = init_integer_type (objfile, bits, 1, name);
	break;
      case DW_ATE_UTF:
	{
	  gdbarch *arch = get_objfile_arch (objfile);

	  if (bits == 16)
	    type = builtin_type (arch)->builtin_char16;
	  else if (bits == 32)
	    type = builtin_type (arch)->builtin_char32;
	  else
	    {
	      complaint (&symfile_complaints,
			 _("unsupported DW_ATE_UTF bit size: '%d'"),
			 bits);
	      type = init_integer_type (objfile, bits, 1, name);
	    }
	  return set_die_type (die, type, cu);
	}
	break;

      default:
	complaint (&symfile_complaints, _("unsupported DW_AT_encoding: '%s'"),
		   dwarf_type_encoding_name (encoding));
	type = init_type (objfile, TYPE_CODE_ERROR,
			  bits / TARGET_CHAR_BIT, name);
	break;
    }

  if (name && strcmp (name, "char") == 0)
    TYPE_NOSIGN (type) = 1;

  return set_die_type (die, type, cu);
}

/* Parse dwarf attribute if it's a block, reference or constant and put the
   resulting value of the attribute into struct bound_prop.
   Returns 1 if ATTR could be resolved into PROP, 0 otherwise.  */

static int
attr_to_dynamic_prop (const struct attribute *attr, struct die_info *die,
		      struct dwarf2_cu *cu, struct dynamic_prop *prop)
{
  struct dwarf2_property_baton *baton;
  struct obstack *obstack = &cu->objfile->objfile_obstack;

  if (attr == NULL || prop == NULL)
    return 0;

  if (attr_form_is_block (attr))
    {
      baton = XOBNEW (obstack, struct dwarf2_property_baton);
      baton->referenced_type = NULL;
      baton->locexpr.per_cu = cu->per_cu;
      baton->locexpr.size = DW_BLOCK (attr)->size;
      baton->locexpr.data = DW_BLOCK (attr)->data;
      prop->data.baton = baton;
      prop->kind = PROP_LOCEXPR;
      gdb_assert (prop->data.baton != NULL);
    }
  else if (attr_form_is_ref (attr))
    {
      struct dwarf2_cu *target_cu = cu;
      struct die_info *target_die;
      struct attribute *target_attr;

      target_die = follow_die_ref (die, attr, &target_cu);
      target_attr = dwarf2_attr (target_die, DW_AT_location, target_cu);
      if (target_attr == NULL)
	target_attr = dwarf2_attr (target_die, DW_AT_data_member_location,
				   target_cu);
      if (target_attr == NULL)
	return 0;

      switch (target_attr->name)
	{
	  case DW_AT_location:
	    if (attr_form_is_section_offset (target_attr))
	      {
		baton = XOBNEW (obstack, struct dwarf2_property_baton);
		baton->referenced_type = die_type (target_die, target_cu);
		fill_in_loclist_baton (cu, &baton->loclist, target_attr);
		prop->data.baton = baton;
		prop->kind = PROP_LOCLIST;
		gdb_assert (prop->data.baton != NULL);
	      }
	    else if (attr_form_is_block (target_attr))
	      {
		baton = XOBNEW (obstack, struct dwarf2_property_baton);
		baton->referenced_type = die_type (target_die, target_cu);
		baton->locexpr.per_cu = cu->per_cu;
		baton->locexpr.size = DW_BLOCK (target_attr)->size;
		baton->locexpr.data = DW_BLOCK (target_attr)->data;
		prop->data.baton = baton;
		prop->kind = PROP_LOCEXPR;
		gdb_assert (prop->data.baton != NULL);
	      }
	    else
	      {
		dwarf2_invalid_attrib_class_complaint ("DW_AT_location",
						       "dynamic property");
		return 0;
	      }
	    break;
	  case DW_AT_data_member_location:
	    {
	      LONGEST offset;

	      if (!handle_data_member_location (target_die, target_cu,
						&offset))
		return 0;

	      baton = XOBNEW (obstack, struct dwarf2_property_baton);
	      baton->referenced_type = read_type_die (target_die->parent,
						      target_cu);
	      baton->offset_info.offset = offset;
	      baton->offset_info.type = die_type (target_die, target_cu);
	      prop->data.baton = baton;
	      prop->kind = PROP_ADDR_OFFSET;
	      break;
	    }
	}
    }
  else if (attr_form_is_constant (attr))
    {
      prop->data.const_val = dwarf2_get_attr_constant_value (attr, 0);
      prop->kind = PROP_CONST;
    }
  else
    {
      dwarf2_invalid_attrib_class_complaint (dwarf_form_name (attr->form),
					     dwarf2_name (die, cu));
      return 0;
    }

  return 1;
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
  LONGEST negative_mask;

  orig_base_type = die_type (die, cu);
  /* If ORIG_BASE_TYPE is a typedef, it will not be TYPE_UNSIGNED,
     whereas the real type might be.  So, we use ORIG_BASE_TYPE when
     creating the range type, but we use the result of check_typedef
     when examining properties of the type.  */
  base_type = check_typedef (orig_base_type);

  /* The die_type call above may have already set the type for this DIE.  */
  range_type = get_die_type (die, cu);
  if (range_type)
    return range_type;

  low.kind = PROP_CONST;
  high.kind = PROP_CONST;
  high.data.const_val = 0;

  /* Set LOW_DEFAULT_IS_VALID if current language and DWARF version allow
     omitting DW_AT_lower_bound.  */
  switch (cu->language)
    {
    case language_c:
    case language_cplus:
      low.data.const_val = 0;
      low_default_is_valid = 1;
      break;
    case language_fortran:
      low.data.const_val = 1;
      low_default_is_valid = 1;
      break;
    case language_d:
    case language_objc:
    case language_rust:
      low.data.const_val = 0;
      low_default_is_valid = (cu->header.version >= 4);
      break;
    case language_ada:
    case language_m2:
    case language_pascal:
      low.data.const_val = 1;
      low_default_is_valid = (cu->header.version >= 4);
      break;
    default:
      low.data.const_val = 0;
      low_default_is_valid = 0;
      break;
    }

  attr = dwarf2_attr (die, DW_AT_lower_bound, cu);
  if (attr)
    attr_to_dynamic_prop (attr, die, cu, &low);
  else if (!low_default_is_valid)
    complaint (&symfile_complaints, _("Missing DW_AT_lower_bound "
				      "- DIE at 0x%x [in module %s]"),
	       to_underlying (die->sect_off), objfile_name (cu->objfile));

  attr = dwarf2_attr (die, DW_AT_upper_bound, cu);
  if (!attr_to_dynamic_prop (attr, die, cu, &high))
    {
      attr = dwarf2_attr (die, DW_AT_count, cu);
      if (attr_to_dynamic_prop (attr, die, cu, &high))
	{
	  /* If bounds are constant do the final calculation here.  */
	  if (low.kind == PROP_CONST && high.kind == PROP_CONST)
	    high.data.const_val = low.data.const_val + high.data.const_val - 1;
	  else
	    high_bound_is_count = 1;
	}
    }

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
  if (TYPE_CODE (base_type) == TYPE_CODE_VOID)
    {
      struct objfile *objfile = cu->objfile;
      struct gdbarch *gdbarch = get_objfile_arch (objfile);
      int addr_size = gdbarch_addr_bit (gdbarch) /8;
      struct type *int_type = objfile_type (objfile)->builtin_int;

      /* Test "int", "long int", and "long long int" objfile types,
	 and select the first one having a size above or equal to the
	 architecture address size.  */
      if (int_type && TYPE_LENGTH (int_type) >= addr_size)
	base_type = int_type;
      else
	{
	  int_type = objfile_type (objfile)->builtin_long;
	  if (int_type && TYPE_LENGTH (int_type) >= addr_size)
	    base_type = int_type;
	  else
	    {
	      int_type = objfile_type (objfile)->builtin_long_long;
	      if (int_type && TYPE_LENGTH (int_type) >= addr_size)
		base_type = int_type;
	    }
	}
    }

  /* Normally, the DWARF producers are expected to use a signed
     constant form (Eg. DW_FORM_sdata) to express negative bounds.
     But this is unfortunately not always the case, as witnessed
     with GCC, for instance, where the ambiguous DW_FORM_dataN form
     is used instead.  To work around that ambiguity, we treat
     the bounds as signed, and thus sign-extend their values, when
     the base type is signed.  */
  negative_mask =
    -((LONGEST) 1 << (TYPE_LENGTH (base_type) * TARGET_CHAR_BIT - 1));
  if (low.kind == PROP_CONST
      && !TYPE_UNSIGNED (base_type) && (low.data.const_val & negative_mask))
    low.data.const_val |= negative_mask;
  if (high.kind == PROP_CONST
      && !TYPE_UNSIGNED (base_type) && (high.data.const_val & negative_mask))
    high.data.const_val |= negative_mask;

  range_type = create_range_type (NULL, orig_base_type, &low, &high);

  if (high_bound_is_count)
    TYPE_RANGE_DATA (range_type)->flag_upper_bound_is_count = 1;

  /* Ada expects an empty array on no boundary attributes.  */
  if (attr == NULL && cu->language != language_ada)
    TYPE_HIGH_BOUND_KIND (range_type) = PROP_UNDEFINED;

  name = dwarf2_name (die, cu);
  if (name)
    TYPE_NAME (range_type) = name;

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr)
    TYPE_LENGTH (range_type) = DW_UNSND (attr);

  set_die_type (die, range_type, cu);

  /* set_die_type should be already done.  */
  set_descriptive_type (range_type, die, cu);

  return range_type;
}

static struct type *
read_unspecified_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *type;

  /* For now, we only support the C meaning of an unspecified type: void.  */

  type = init_type (cu->objfile, TYPE_CODE_VOID, 0, NULL);
  TYPE_NAME (type) = dwarf2_name (die, cu);

  return set_die_type (die, type, cu);
}

/* Read a single die and all its descendents.  Set the die's sibling
   field to NULL; set other fields in the die correctly, and set all
   of the descendents' fields correctly.  Set *NEW_INFO_PTR to the
   location of the info_ptr after reading all of those dies.  PARENT
   is the parent of the die in question.  */

static struct die_info *
read_die_and_children (const struct die_reader_specs *reader,
		       const gdb_byte *info_ptr,
		       const gdb_byte **new_info_ptr,
		       struct die_info *parent)
{
  struct die_info *die;
  const gdb_byte *cur_ptr;
  int has_children;

  cur_ptr = read_full_die_1 (reader, &die, info_ptr, &has_children, 0);
  if (die == NULL)
    {
      *new_info_ptr = cur_ptr;
      return NULL;
    }
  store_in_ref_table (die, reader->cu);

  if (has_children)
    die->child = read_die_and_siblings_1 (reader, cur_ptr, new_info_ptr, die);
  else
    {
      die->child = NULL;
      *new_info_ptr = cur_ptr;
    }

  die->sibling = NULL;
  die->parent = parent;
  return die;
}

/* Read a die, all of its descendents, and all of its siblings; set
   all of the fields of all of the dies correctly.  Arguments are as
   in read_die_and_children.  */

static struct die_info *
read_die_and_siblings_1 (const struct die_reader_specs *reader,
			 const gdb_byte *info_ptr,
			 const gdb_byte **new_info_ptr,
			 struct die_info *parent)
{
  struct die_info *first_die, *last_sibling;
  const gdb_byte *cur_ptr;

  cur_ptr = info_ptr;
  first_die = last_sibling = NULL;

  while (1)
    {
      struct die_info *die
	= read_die_and_children (reader, cur_ptr, &cur_ptr, parent);

      if (die == NULL)
	{
	  *new_info_ptr = cur_ptr;
	  return first_die;
	}

      if (!first_die)
	first_die = die;
      else
	last_sibling->sibling = die;

      last_sibling = die;
    }
}

/* Read a die, all of its descendents, and all of its siblings; set
   all of the fields of all of the dies correctly.  Arguments are as
   in read_die_and_children.
   This the main entry point for reading a DIE and all its children.  */

static struct die_info *
read_die_and_siblings (const struct die_reader_specs *reader,
		       const gdb_byte *info_ptr,
		       const gdb_byte **new_info_ptr,
		       struct die_info *parent)
{
  struct die_info *die = read_die_and_siblings_1 (reader, info_ptr,
						  new_info_ptr, parent);

  if (dwarf_die_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "Read die from %s@0x%x of %s:\n",
			  get_section_name (reader->die_section),
			  (unsigned) (info_ptr - reader->die_section->buffer),
			  bfd_get_filename (reader->abfd));
      dump_die (die, dwarf_die_debug);
    }

  return die;
}

/* Read a die and all its attributes, leave space for NUM_EXTRA_ATTRS
   attributes.
   The caller is responsible for filling in the extra attributes
   and updating (*DIEP)->num_attrs.
   Set DIEP to point to a newly allocated die with its information,
   except for its child, sibling, and parent fields.
   Set HAS_CHILDREN to tell whether the die has children or not.  */

static const gdb_byte *
read_full_die_1 (const struct die_reader_specs *reader,
		 struct die_info **diep, const gdb_byte *info_ptr,
		 int *has_children, int num_extra_attrs)
{
  unsigned int abbrev_number, bytes_read, i;
  struct abbrev_info *abbrev;
  struct die_info *die;
  struct dwarf2_cu *cu = reader->cu;
  bfd *abfd = reader->abfd;

  sect_offset sect_off = (sect_offset) (info_ptr - reader->buffer);
  abbrev_number = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
  info_ptr += bytes_read;
  if (!abbrev_number)
    {
      *diep = NULL;
      *has_children = 0;
      return info_ptr;
    }

  abbrev = abbrev_table_lookup_abbrev (cu->abbrev_table, abbrev_number);
  if (!abbrev)
    error (_("Dwarf Error: could not find abbrev number %d [in module %s]"),
	   abbrev_number,
	   bfd_get_filename (abfd));

  die = dwarf_alloc_die (cu, abbrev->num_attrs + num_extra_attrs);
  die->sect_off = sect_off;
  die->tag = abbrev->tag;
  die->abbrev = abbrev_number;

  /* Make the result usable.
     The caller needs to update num_attrs after adding the extra
     attributes.  */
  die->num_attrs = abbrev->num_attrs;

  for (i = 0; i < abbrev->num_attrs; ++i)
    info_ptr = read_attribute (reader, &die->attrs[i], &abbrev->attrs[i],
			       info_ptr);

  *diep = die;
  *has_children = abbrev->has_children;
  return info_ptr;
}

/* Read a die and all its attributes.
   Set DIEP to point to a newly allocated die with its information,
   except for its child, sibling, and parent fields.
   Set HAS_CHILDREN to tell whether the die has children or not.  */

static const gdb_byte *
read_full_die (const struct die_reader_specs *reader,
	       struct die_info **diep, const gdb_byte *info_ptr,
	       int *has_children)
{
  const gdb_byte *result;

  result = read_full_die_1 (reader, diep, info_ptr, has_children, 0);

  if (dwarf_die_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "Read die from %s@0x%x of %s:\n",
			  get_section_name (reader->die_section),
			  (unsigned) (info_ptr - reader->die_section->buffer),
			  bfd_get_filename (reader->abfd));
      dump_die (*diep, dwarf_die_debug);
    }

  return result;
}

/* Abbreviation tables.

   In DWARF version 2, the description of the debugging information is
   stored in a separate .debug_abbrev section.  Before we read any
   dies from a section we read in all abbreviations and install them
   in a hash table.  */

/* Allocate space for a struct abbrev_info object in ABBREV_TABLE.  */

static struct abbrev_info *
abbrev_table_alloc_abbrev (struct abbrev_table *abbrev_table)
{
  struct abbrev_info *abbrev;

  abbrev = XOBNEW (&abbrev_table->abbrev_obstack, struct abbrev_info);
  memset (abbrev, 0, sizeof (struct abbrev_info));

  return abbrev;
}

/* Add an abbreviation to the table.  */

static void
abbrev_table_add_abbrev (struct abbrev_table *abbrev_table,
			 unsigned int abbrev_number,
			 struct abbrev_info *abbrev)
{
  unsigned int hash_number;

  hash_number = abbrev_number % ABBREV_HASH_SIZE;
  abbrev->next = abbrev_table->abbrevs[hash_number];
  abbrev_table->abbrevs[hash_number] = abbrev;
}

/* Look up an abbrev in the table.
   Returns NULL if the abbrev is not found.  */

static struct abbrev_info *
abbrev_table_lookup_abbrev (const struct abbrev_table *abbrev_table,
			    unsigned int abbrev_number)
{
  unsigned int hash_number;
  struct abbrev_info *abbrev;

  hash_number = abbrev_number % ABBREV_HASH_SIZE;
  abbrev = abbrev_table->abbrevs[hash_number];

  while (abbrev)
    {
      if (abbrev->number == abbrev_number)
	return abbrev;
      abbrev = abbrev->next;
    }
  return NULL;
}

/* Read in an abbrev table.  */

static struct abbrev_table *
abbrev_table_read_table (struct dwarf2_section_info *section,
			 sect_offset sect_off)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  bfd *abfd = get_section_bfd_owner (section);
  struct abbrev_table *abbrev_table;
  const gdb_byte *abbrev_ptr;
  struct abbrev_info *cur_abbrev;
  unsigned int abbrev_number, bytes_read, abbrev_name;
  unsigned int abbrev_form;
  struct attr_abbrev *cur_attrs;
  unsigned int allocated_attrs;

  abbrev_table = XNEW (struct abbrev_table);
  abbrev_table->sect_off = sect_off;
  obstack_init (&abbrev_table->abbrev_obstack);
  abbrev_table->abbrevs =
    XOBNEWVEC (&abbrev_table->abbrev_obstack, struct abbrev_info *,
	       ABBREV_HASH_SIZE);
  memset (abbrev_table->abbrevs, 0,
	  ABBREV_HASH_SIZE * sizeof (struct abbrev_info *));

  dwarf2_read_section (objfile, section);
  abbrev_ptr = section->buffer + to_underlying (sect_off);
  abbrev_number = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
  abbrev_ptr += bytes_read;

  allocated_attrs = ATTR_ALLOC_CHUNK;
  cur_attrs = XNEWVEC (struct attr_abbrev, allocated_attrs);

  /* Loop until we reach an abbrev number of 0.  */
  while (abbrev_number)
    {
      cur_abbrev = abbrev_table_alloc_abbrev (abbrev_table);

      /* read in abbrev header */
      cur_abbrev->number = abbrev_number;
      cur_abbrev->tag
	= (enum dwarf_tag) read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;
      cur_abbrev->has_children = read_1_byte (abfd, abbrev_ptr);
      abbrev_ptr += 1;

      /* now read in declarations */
      for (;;)
	{
	  LONGEST implicit_const;

	  abbrev_name = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
	  abbrev_ptr += bytes_read;
	  abbrev_form = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
	  abbrev_ptr += bytes_read;
	  if (abbrev_form == DW_FORM_implicit_const)
	    {
	      implicit_const = read_signed_leb128 (abfd, abbrev_ptr,
						   &bytes_read);
	      abbrev_ptr += bytes_read;
	    }
	  else
	    {
	      /* Initialize it due to a false compiler warning.  */
	      implicit_const = -1;
	    }

	  if (abbrev_name == 0)
	    break;

	  if (cur_abbrev->num_attrs == allocated_attrs)
	    {
	      allocated_attrs += ATTR_ALLOC_CHUNK;
	      cur_attrs
		= XRESIZEVEC (struct attr_abbrev, cur_attrs, allocated_attrs);
	    }

	  cur_attrs[cur_abbrev->num_attrs].name
	    = (enum dwarf_attribute) abbrev_name;
	  cur_attrs[cur_abbrev->num_attrs].form
	    = (enum dwarf_form) abbrev_form;
	  cur_attrs[cur_abbrev->num_attrs].implicit_const = implicit_const;
	  ++cur_abbrev->num_attrs;
	}

      cur_abbrev->attrs =
	XOBNEWVEC (&abbrev_table->abbrev_obstack, struct attr_abbrev,
		   cur_abbrev->num_attrs);
      memcpy (cur_abbrev->attrs, cur_attrs,
	      cur_abbrev->num_attrs * sizeof (struct attr_abbrev));

      abbrev_table_add_abbrev (abbrev_table, abbrev_number, cur_abbrev);

      /* Get next abbreviation.
         Under Irix6 the abbreviations for a compilation unit are not
         always properly terminated with an abbrev number of 0.
         Exit loop if we encounter an abbreviation which we have
         already read (which means we are about to read the abbreviations
         for the next compile unit) or if the end of the abbreviation
         table is reached.  */
      if ((unsigned int) (abbrev_ptr - section->buffer) >= section->size)
	break;
      abbrev_number = read_unsigned_leb128 (abfd, abbrev_ptr, &bytes_read);
      abbrev_ptr += bytes_read;
      if (abbrev_table_lookup_abbrev (abbrev_table, abbrev_number) != NULL)
	break;
    }

  xfree (cur_attrs);
  return abbrev_table;
}

/* Free the resources held by ABBREV_TABLE.  */

static void
abbrev_table_free (struct abbrev_table *abbrev_table)
{
  obstack_free (&abbrev_table->abbrev_obstack, NULL);
  xfree (abbrev_table);
}

/* Same as abbrev_table_free but as a cleanup.
   We pass in a pointer to the pointer to the table so that we can
   set the pointer to NULL when we're done.  It also simplifies
   build_type_psymtabs_1.  */

static void
abbrev_table_free_cleanup (void *table_ptr)
{
  struct abbrev_table **abbrev_table_ptr = (struct abbrev_table **) table_ptr;

  if (*abbrev_table_ptr != NULL)
    abbrev_table_free (*abbrev_table_ptr);
  *abbrev_table_ptr = NULL;
}

/* Read the abbrev table for CU from ABBREV_SECTION.  */

static void
dwarf2_read_abbrevs (struct dwarf2_cu *cu,
		     struct dwarf2_section_info *abbrev_section)
{
  cu->abbrev_table =
    abbrev_table_read_table (abbrev_section, cu->header.abbrev_sect_off);
}

/* Release the memory used by the abbrev table for a compilation unit.  */

static void
dwarf2_free_abbrev_table (void *ptr_to_cu)
{
  struct dwarf2_cu *cu = (struct dwarf2_cu *) ptr_to_cu;

  if (cu->abbrev_table != NULL)
    abbrev_table_free (cu->abbrev_table);
  /* Set this to NULL so that we SEGV if we try to read it later,
     and also because free_comp_unit verifies this is NULL.  */
  cu->abbrev_table = NULL;
}

/* Returns nonzero if TAG represents a type that we might generate a partial
   symbol for.  */

static int
is_type_tag_for_partial (int tag)
{
  switch (tag)
    {
#if 0
    /* Some types that would be reasonable to generate partial symbols for,
       that we don't at present.  */
    case DW_TAG_array_type:
    case DW_TAG_file_type:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_set_type:
    case DW_TAG_string_type:
    case DW_TAG_subroutine_type:
#endif
    case DW_TAG_base_type:
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_structure_type:
    case DW_TAG_subrange_type:
    case DW_TAG_typedef:
    case DW_TAG_union_type:
      return 1;
    default:
      return 0;
    }
}

/* Load all DIEs that are interesting for partial symbols into memory.  */

static struct partial_die_info *
load_partial_dies (const struct die_reader_specs *reader,
		   const gdb_byte *info_ptr, int building_psymtab)
{
  struct dwarf2_cu *cu = reader->cu;
  struct objfile *objfile = cu->objfile;
  struct partial_die_info *part_die;
  struct partial_die_info *parent_die, *last_die, *first_die = NULL;
  struct abbrev_info *abbrev;
  unsigned int bytes_read;
  unsigned int load_all = 0;
  int nesting_level = 1;

  parent_die = NULL;
  last_die = NULL;

  gdb_assert (cu->per_cu != NULL);
  if (cu->per_cu->load_all_dies)
    load_all = 1;

  cu->partial_dies
    = htab_create_alloc_ex (cu->header.length / 12,
			    partial_die_hash,
			    partial_die_eq,
			    NULL,
			    &cu->comp_unit_obstack,
			    hashtab_obstack_allocate,
			    dummy_obstack_deallocate);

  part_die = XOBNEW (&cu->comp_unit_obstack, struct partial_die_info);

  while (1)
    {
      abbrev = peek_die_abbrev (info_ptr, &bytes_read, cu);

      /* A NULL abbrev means the end of a series of children.  */
      if (abbrev == NULL)
	{
	  if (--nesting_level == 0)
	    {
	      /* PART_DIE was probably the last thing allocated on the
		 comp_unit_obstack, so we could call obstack_free
		 here.  We don't do that because the waste is small,
		 and will be cleaned up when we're done with this
		 compilation unit.  This way, we're also more robust
		 against other users of the comp_unit_obstack.  */
	      return first_die;
	    }
	  info_ptr += bytes_read;
	  last_die = parent_die;
	  parent_die = parent_die->die_parent;
	  continue;
	}

      /* Check for template arguments.  We never save these; if
	 they're seen, we just mark the parent, and go on our way.  */
      if (parent_die != NULL
	  && cu->language == language_cplus
	  && (abbrev->tag == DW_TAG_template_type_param
	      || abbrev->tag == DW_TAG_template_value_param))
	{
	  parent_die->has_template_arguments = 1;

	  if (!load_all)
	    {
	      /* We don't need a partial DIE for the template argument.  */
	      info_ptr = skip_one_die (reader, info_ptr + bytes_read, abbrev);
	      continue;
	    }
	}

      /* We only recurse into c++ subprograms looking for template arguments.
	 Skip their other children.  */
      if (!load_all
	  && cu->language == language_cplus
	  && parent_die != NULL
	  && parent_die->tag == DW_TAG_subprogram)
	{
	  info_ptr = skip_one_die (reader, info_ptr + bytes_read, abbrev);
	  continue;
	}

      /* Check whether this DIE is interesting enough to save.  Normally
	 we would not be interested in members here, but there may be
	 later variables referencing them via DW_AT_specification (for
	 static members).  */
      if (!load_all
	  && !is_type_tag_for_partial (abbrev->tag)
	  && abbrev->tag != DW_TAG_constant
	  && abbrev->tag != DW_TAG_enumerator
	  && abbrev->tag != DW_TAG_subprogram
	  && abbrev->tag != DW_TAG_lexical_block
	  && abbrev->tag != DW_TAG_variable
	  && abbrev->tag != DW_TAG_namespace
	  && abbrev->tag != DW_TAG_module
	  && abbrev->tag != DW_TAG_member
	  && abbrev->tag != DW_TAG_imported_unit
	  && abbrev->tag != DW_TAG_imported_declaration)
	{
	  /* Otherwise we skip to the next sibling, if any.  */
	  info_ptr = skip_one_die (reader, info_ptr + bytes_read, abbrev);
	  continue;
	}

      info_ptr = read_partial_die (reader, part_die, abbrev, bytes_read,
				   info_ptr);

      /* This two-pass algorithm for processing partial symbols has a
	 high cost in cache pressure.  Thus, handle some simple cases
	 here which cover the majority of C partial symbols.  DIEs
	 which neither have specification tags in them, nor could have
	 specification tags elsewhere pointing at them, can simply be
	 processed and discarded.

	 This segment is also optional; scan_partial_symbols and
	 add_partial_symbol will handle these DIEs if we chain
	 them in normally.  When compilers which do not emit large
	 quantities of duplicate debug information are more common,
	 this code can probably be removed.  */

      /* Any complete simple types at the top level (pretty much all
	 of them, for a language without namespaces), can be processed
	 directly.  */
      if (parent_die == NULL
	  && part_die->has_specification == 0
	  && part_die->is_declaration == 0
	  && ((part_die->tag == DW_TAG_typedef && !part_die->has_children)
	      || part_die->tag == DW_TAG_base_type
	      || part_die->tag == DW_TAG_subrange_type))
	{
	  if (building_psymtab && part_die->name != NULL)
	    add_psymbol_to_list (part_die->name, strlen (part_die->name), 0,
				 VAR_DOMAIN, LOC_TYPEDEF,
				 &objfile->static_psymbols,
				 0, cu->language, objfile);
	  info_ptr = locate_pdi_sibling (reader, part_die, info_ptr);
	  continue;
	}

      /* The exception for DW_TAG_typedef with has_children above is
	 a workaround of GCC PR debug/47510.  In the case of this complaint
	 type_name_no_tag_or_error will error on such types later.

	 GDB skipped children of DW_TAG_typedef by the shortcut above and then
	 it could not find the child DIEs referenced later, this is checked
	 above.  In correct DWARF DW_TAG_typedef should have no children.  */

      if (part_die->tag == DW_TAG_typedef && part_die->has_children)
	complaint (&symfile_complaints,
		   _("DW_TAG_typedef has childen - GCC PR debug/47510 bug "
		     "- DIE at 0x%x [in module %s]"),
		   to_underlying (part_die->sect_off), objfile_name (objfile));

      /* If we're at the second level, and we're an enumerator, and
	 our parent has no specification (meaning possibly lives in a
	 namespace elsewhere), then we can add the partial symbol now
	 instead of queueing it.  */
      if (part_die->tag == DW_TAG_enumerator
	  && parent_die != NULL
	  && parent_die->die_parent == NULL
	  && parent_die->tag == DW_TAG_enumeration_type
	  && parent_die->has_specification == 0)
	{
	  if (part_die->name == NULL)
	    complaint (&symfile_complaints,
		       _("malformed enumerator DIE ignored"));
	  else if (building_psymtab)
	    add_psymbol_to_list (part_die->name, strlen (part_die->name), 0,
				 VAR_DOMAIN, LOC_CONST,
				 cu->language == language_cplus
				 ? &objfile->global_psymbols
				 : &objfile->static_psymbols,
				 0, cu->language, objfile);

	  info_ptr = locate_pdi_sibling (reader, part_die, info_ptr);
	  continue;
	}

      /* We'll save this DIE so link it in.  */
      part_die->die_parent = parent_die;
      part_die->die_sibling = NULL;
      part_die->die_child = NULL;

      if (last_die && last_die == parent_die)
	last_die->die_child = part_die;
      else if (last_die)
	last_die->die_sibling = part_die;

      last_die = part_die;

      if (first_die == NULL)
	first_die = part_die;

      /* Maybe add the DIE to the hash table.  Not all DIEs that we
	 find interesting need to be in the hash table, because we
	 also have the parent/sibling/child chains; only those that we
	 might refer to by offset later during partial symbol reading.

	 For now this means things that might have be the target of a
	 DW_AT_specification, DW_AT_abstract_origin, or
	 DW_AT_extension.  DW_AT_extension will refer only to
	 namespaces; DW_AT_abstract_origin refers to functions (and
	 many things under the function DIE, but we do not recurse
	 into function DIEs during partial symbol reading) and
	 possibly variables as well; DW_AT_specification refers to
	 declarations.  Declarations ought to have the DW_AT_declaration
	 flag.  It happens that GCC forgets to put it in sometimes, but
	 only for functions, not for types.

	 Adding more things than necessary to the hash table is harmless
	 except for the performance cost.  Adding too few will result in
	 wasted time in find_partial_die, when we reread the compilation
	 unit with load_all_dies set.  */

      if (load_all
	  || abbrev->tag == DW_TAG_constant
	  || abbrev->tag == DW_TAG_subprogram
	  || abbrev->tag == DW_TAG_variable
	  || abbrev->tag == DW_TAG_namespace
	  || part_die->is_declaration)
	{
	  void **slot;

	  slot = htab_find_slot_with_hash (cu->partial_dies, part_die,
					   to_underlying (part_die->sect_off),
					   INSERT);
	  *slot = part_die;
	}

      part_die = XOBNEW (&cu->comp_unit_obstack, struct partial_die_info);

      /* For some DIEs we want to follow their children (if any).  For C
	 we have no reason to follow the children of structures; for other
	 languages we have to, so that we can get at method physnames
	 to infer fully qualified class names, for DW_AT_specification,
	 and for C++ template arguments.  For C++, we also look one level
	 inside functions to find template arguments (if the name of the
	 function does not already contain the template arguments).

	 For Ada, we need to scan the children of subprograms and lexical
	 blocks as well because Ada allows the definition of nested
	 entities that could be interesting for the debugger, such as
	 nested subprograms for instance.  */
      if (last_die->has_children
	  && (load_all
	      || last_die->tag == DW_TAG_namespace
	      || last_die->tag == DW_TAG_module
	      || last_die->tag == DW_TAG_enumeration_type
	      || (cu->language == language_cplus
		  && last_die->tag == DW_TAG_subprogram
		  && (last_die->name == NULL
		      || strchr (last_die->name, '<') == NULL))
	      || (cu->language != language_c
		  && (last_die->tag == DW_TAG_class_type
		      || last_die->tag == DW_TAG_interface_type
		      || last_die->tag == DW_TAG_structure_type
		      || last_die->tag == DW_TAG_union_type))
	      || (cu->language == language_ada
		  && (last_die->tag == DW_TAG_subprogram
		      || last_die->tag == DW_TAG_lexical_block))))
	{
	  nesting_level++;
	  parent_die = last_die;
	  continue;
	}

      /* Otherwise we skip to the next sibling, if any.  */
      info_ptr = locate_pdi_sibling (reader, last_die, info_ptr);

      /* Back to the top, do it again.  */
    }
}

/* Read a minimal amount of information into the minimal die structure.  */

static const gdb_byte *
read_partial_die (const struct die_reader_specs *reader,
		  struct partial_die_info *part_die,
		  struct abbrev_info *abbrev, unsigned int abbrev_len,
		  const gdb_byte *info_ptr)
{
  struct dwarf2_cu *cu = reader->cu;
  struct objfile *objfile = cu->objfile;
  const gdb_byte *buffer = reader->buffer;
  unsigned int i;
  struct attribute attr;
  int has_low_pc_attr = 0;
  int has_high_pc_attr = 0;
  int high_pc_relative = 0;

  memset (part_die, 0, sizeof (struct partial_die_info));

  part_die->sect_off = (sect_offset) (info_ptr - buffer);

  info_ptr += abbrev_len;

  if (abbrev == NULL)
    return info_ptr;

  part_die->tag = abbrev->tag;
  part_die->has_children = abbrev->has_children;

  for (i = 0; i < abbrev->num_attrs; ++i)
    {
      info_ptr = read_attribute (reader, &attr, &abbrev->attrs[i], info_ptr);

      /* Store the data if it is of an attribute we want to keep in a
         partial symbol table.  */
      switch (attr.name)
	{
	case DW_AT_name:
	  switch (part_die->tag)
	    {
	    case DW_TAG_compile_unit:
	    case DW_TAG_partial_unit:
	    case DW_TAG_type_unit:
	      /* Compilation units have a DW_AT_name that is a filename, not
		 a source language identifier.  */
	    case DW_TAG_enumeration_type:
	    case DW_TAG_enumerator:
	      /* These tags always have simple identifiers already; no need
		 to canonicalize them.  */
	      part_die->name = DW_STRING (&attr);
	      break;
	    default:
	      part_die->name
		= dwarf2_canonicalize_name (DW_STRING (&attr), cu,
					    &objfile->per_bfd->storage_obstack);
	      break;
	    }
	  break;
	case DW_AT_linkage_name:
	case DW_AT_MIPS_linkage_name:
	  /* Note that both forms of linkage name might appear.  We
	     assume they will be the same, and we only store the last
	     one we see.  */
	  if (cu->language == language_ada)
	    part_die->name = DW_STRING (&attr);
	  part_die->linkage_name = DW_STRING (&attr);
	  break;
	case DW_AT_low_pc:
	  has_low_pc_attr = 1;
	  part_die->lowpc = attr_value_as_address (&attr);
	  break;
	case DW_AT_high_pc:
	  has_high_pc_attr = 1;
	  part_die->highpc = attr_value_as_address (&attr);
	  if (cu->header.version >= 4 && attr_form_is_constant (&attr))
		high_pc_relative = 1;
	  break;
	case DW_AT_location:
          /* Support the .debug_loc offsets.  */
          if (attr_form_is_block (&attr))
            {
	       part_die->d.locdesc = DW_BLOCK (&attr);
            }
          else if (attr_form_is_section_offset (&attr))
            {
	      dwarf2_complex_location_expr_complaint ();
            }
          else
            {
	      dwarf2_invalid_attrib_class_complaint ("DW_AT_location",
						     "partial symbol information");
            }
	  break;
	case DW_AT_external:
	  part_die->is_external = DW_UNSND (&attr);
	  break;
	case DW_AT_declaration:
	  part_die->is_declaration = DW_UNSND (&attr);
	  break;
	case DW_AT_type:
	  part_die->has_type = 1;
	  break;
	case DW_AT_abstract_origin:
	case DW_AT_specification:
	case DW_AT_extension:
	  part_die->has_specification = 1;
	  part_die->spec_offset = dwarf2_get_ref_die_offset (&attr);
	  part_die->spec_is_dwz = (attr.form == DW_FORM_GNU_ref_alt
				   || cu->per_cu->is_dwz);
	  break;
	case DW_AT_sibling:
	  /* Ignore absolute siblings, they might point outside of
	     the current compile unit.  */
	  if (attr.form == DW_FORM_ref_addr)
	    complaint (&symfile_complaints,
		       _("ignoring absolute DW_AT_sibling"));
	  else
	    {
	      sect_offset off = dwarf2_get_ref_die_offset (&attr);
	      const gdb_byte *sibling_ptr = buffer + to_underlying (off);

	      if (sibling_ptr < info_ptr)
		complaint (&symfile_complaints,
			   _("DW_AT_sibling points backwards"));
	      else if (sibling_ptr > reader->buffer_end)
		dwarf2_section_buffer_overflow_complaint (reader->die_section);
	      else
		part_die->sibling = sibling_ptr;
	    }
	  break;
        case DW_AT_byte_size:
          part_die->has_byte_size = 1;
          break;
        case DW_AT_const_value:
          part_die->has_const_value = 1;
          break;
	case DW_AT_calling_convention:
	  /* DWARF doesn't provide a way to identify a program's source-level
	     entry point.  DW_AT_calling_convention attributes are only meant
	     to describe functions' calling conventions.

	     However, because it's a necessary piece of information in
	     Fortran, and before DWARF 4 DW_CC_program was the only
	     piece of debugging information whose definition refers to
	     a 'main program' at all, several compilers marked Fortran
	     main programs with DW_CC_program --- even when those
	     functions use the standard calling conventions.

	     Although DWARF now specifies a way to provide this
	     information, we support this practice for backward
	     compatibility.  */
	  if (DW_UNSND (&attr) == DW_CC_program
	      && cu->language == language_fortran)
	    part_die->main_subprogram = 1;
	  break;
	case DW_AT_inline:
	  if (DW_UNSND (&attr) == DW_INL_inlined
	      || DW_UNSND (&attr) == DW_INL_declared_inlined)
	    part_die->may_be_inlined = 1;
	  break;

	case DW_AT_import:
	  if (part_die->tag == DW_TAG_imported_unit)
	    {
	      part_die->d.sect_off = dwarf2_get_ref_die_offset (&attr);
	      part_die->is_dwz = (attr.form == DW_FORM_GNU_ref_alt
				  || cu->per_cu->is_dwz);
	    }
	  break;

	case DW_AT_main_subprogram:
	  part_die->main_subprogram = DW_UNSND (&attr);
	  break;

	default:
	  break;
	}
    }

  if (high_pc_relative)
    part_die->highpc += part_die->lowpc;

  if (has_low_pc_attr && has_high_pc_attr)
    {
      /* When using the GNU linker, .gnu.linkonce. sections are used to
	 eliminate duplicate copies of functions and vtables and such.
	 The linker will arbitrarily choose one and discard the others.
	 The AT_*_pc values for such functions refer to local labels in
	 these sections.  If the section from that file was discarded, the
	 labels are not in the output, so the relocs get a value of 0.
	 If this is a discarded function, mark the pc bounds as invalid,
	 so that GDB will ignore it.  */
      if (part_die->lowpc == 0 && !dwarf2_per_objfile->has_section_at_zero)
	{
	  struct gdbarch *gdbarch = get_objfile_arch (objfile);

	  complaint (&symfile_complaints,
		     _("DW_AT_low_pc %s is zero "
		       "for DIE at 0x%x [in module %s]"),
		     paddress (gdbarch, part_die->lowpc),
		     to_underlying (part_die->sect_off), objfile_name (objfile));
	}
      /* dwarf2_get_pc_bounds has also the strict low < high requirement.  */
      else if (part_die->lowpc >= part_die->highpc)
	{
	  struct gdbarch *gdbarch = get_objfile_arch (objfile);

	  complaint (&symfile_complaints,
		     _("DW_AT_low_pc %s is not < DW_AT_high_pc %s "
		       "for DIE at 0x%x [in module %s]"),
		     paddress (gdbarch, part_die->lowpc),
		     paddress (gdbarch, part_die->highpc),
		     to_underlying (part_die->sect_off),
		     objfile_name (objfile));
	}
      else
	part_die->has_pc_info = 1;
    }

  return info_ptr;
}

/* Find a cached partial DIE at OFFSET in CU.  */

static struct partial_die_info *
find_partial_die_in_comp_unit (sect_offset sect_off, struct dwarf2_cu *cu)
{
  struct partial_die_info *lookup_die = NULL;
  struct partial_die_info part_die;

  part_die.sect_off = sect_off;
  lookup_die = ((struct partial_die_info *)
		htab_find_with_hash (cu->partial_dies, &part_die,
				     to_underlying (sect_off)));

  return lookup_die;
}

/* Find a partial DIE at OFFSET, which may or may not be in CU,
   except in the case of .debug_types DIEs which do not reference
   outside their CU (they do however referencing other types via
   DW_FORM_ref_sig8).  */

static struct partial_die_info *
find_partial_die (sect_offset sect_off, int offset_in_dwz, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct dwarf2_per_cu_data *per_cu = NULL;
  struct partial_die_info *pd = NULL;

  if (offset_in_dwz == cu->per_cu->is_dwz
      && offset_in_cu_p (&cu->header, sect_off))
    {
      pd = find_partial_die_in_comp_unit (sect_off, cu);
      if (pd != NULL)
	return pd;
      /* We missed recording what we needed.
	 Load all dies and try again.  */
      per_cu = cu->per_cu;
    }
  else
    {
      /* TUs don't reference other CUs/TUs (except via type signatures).  */
      if (cu->per_cu->is_debug_types)
	{
	  error (_("Dwarf Error: Type Unit at offset 0x%x contains"
		   " external reference to offset 0x%x [in module %s].\n"),
		 to_underlying (cu->header.sect_off), to_underlying (sect_off),
		 bfd_get_filename (objfile->obfd));
	}
      per_cu = dwarf2_find_containing_comp_unit (sect_off, offset_in_dwz,
						 objfile);

      if (per_cu->cu == NULL || per_cu->cu->partial_dies == NULL)
	load_partial_comp_unit (per_cu);

      per_cu->cu->last_used = 0;
      pd = find_partial_die_in_comp_unit (sect_off, per_cu->cu);
    }

  /* If we didn't find it, and not all dies have been loaded,
     load them all and try again.  */

  if (pd == NULL && per_cu->load_all_dies == 0)
    {
      per_cu->load_all_dies = 1;

      /* This is nasty.  When we reread the DIEs, somewhere up the call chain
	 THIS_CU->cu may already be in use.  So we can't just free it and
	 replace its DIEs with the ones we read in.  Instead, we leave those
	 DIEs alone (which can still be in use, e.g. in scan_partial_symbols),
	 and clobber THIS_CU->cu->partial_dies with the hash table for the new
	 set.  */
      load_partial_comp_unit (per_cu);

      pd = find_partial_die_in_comp_unit (sect_off, per_cu->cu);
    }

  if (pd == NULL)
    internal_error (__FILE__, __LINE__,
		    _("could not find partial DIE 0x%x "
		      "in cache [from module %s]\n"),
		    to_underlying (sect_off), bfd_get_filename (objfile->obfd));
  return pd;
}

/* See if we can figure out if the class lives in a namespace.  We do
   this by looking for a member function; its demangled name will
   contain namespace info, if there is any.  */

static void
guess_partial_die_structure_name (struct partial_die_info *struct_pdi,
				  struct dwarf2_cu *cu)
{
  /* NOTE: carlton/2003-10-07: Getting the info this way changes
     what template types look like, because the demangler
     frequently doesn't give the same name as the debug info.  We
     could fix this by only using the demangled name to get the
     prefix (but see comment in read_structure_type).  */

  struct partial_die_info *real_pdi;
  struct partial_die_info *child_pdi;

  /* If this DIE (this DIE's specification, if any) has a parent, then
     we should not do this.  We'll prepend the parent's fully qualified
     name when we create the partial symbol.  */

  real_pdi = struct_pdi;
  while (real_pdi->has_specification)
    real_pdi = find_partial_die (real_pdi->spec_offset,
				 real_pdi->spec_is_dwz, cu);

  if (real_pdi->die_parent != NULL)
    return;

  for (child_pdi = struct_pdi->die_child;
       child_pdi != NULL;
       child_pdi = child_pdi->die_sibling)
    {
      if (child_pdi->tag == DW_TAG_subprogram
	  && child_pdi->linkage_name != NULL)
	{
	  char *actual_class_name
	    = language_class_name_from_physname (cu->language_defn,
						 child_pdi->linkage_name);
	  if (actual_class_name != NULL)
	    {
	      struct_pdi->name
		= ((const char *)
		   obstack_copy0 (&cu->objfile->per_bfd->storage_obstack,
				  actual_class_name,
				  strlen (actual_class_name)));
	      xfree (actual_class_name);
	    }
	  break;
	}
    }
}

/* Adjust PART_DIE before generating a symbol for it.  This function
   may set the is_external flag or change the DIE's name.  */

static void
fixup_partial_die (struct partial_die_info *part_die,
		   struct dwarf2_cu *cu)
{
  /* Once we've fixed up a die, there's no point in doing so again.
     This also avoids a memory leak if we were to call
     guess_partial_die_structure_name multiple times.  */
  if (part_die->fixup_called)
    return;

  /* If we found a reference attribute and the DIE has no name, try
     to find a name in the referred to DIE.  */

  if (part_die->name == NULL && part_die->has_specification)
    {
      struct partial_die_info *spec_die;

      spec_die = find_partial_die (part_die->spec_offset,
				   part_die->spec_is_dwz, cu);

      fixup_partial_die (spec_die, cu);

      if (spec_die->name)
	{
	  part_die->name = spec_die->name;

	  /* Copy DW_AT_external attribute if it is set.  */
	  if (spec_die->is_external)
	    part_die->is_external = spec_die->is_external;
	}
    }

  /* Set default names for some unnamed DIEs.  */

  if (part_die->name == NULL && part_die->tag == DW_TAG_namespace)
    part_die->name = CP_ANONYMOUS_NAMESPACE_STR;

  /* If there is no parent die to provide a namespace, and there are
     children, see if we can determine the namespace from their linkage
     name.  */
  if (cu->language == language_cplus
      && !VEC_empty (dwarf2_section_info_def, dwarf2_per_objfile->types)
      && part_die->die_parent == NULL
      && part_die->has_children
      && (part_die->tag == DW_TAG_class_type
	  || part_die->tag == DW_TAG_structure_type
	  || part_die->tag == DW_TAG_union_type))
    guess_partial_die_structure_name (part_die, cu);

  /* GCC might emit a nameless struct or union that has a linkage
     name.  See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=47510.  */
  if (part_die->name == NULL
      && (part_die->tag == DW_TAG_class_type
	  || part_die->tag == DW_TAG_interface_type
	  || part_die->tag == DW_TAG_structure_type
	  || part_die->tag == DW_TAG_union_type)
      && part_die->linkage_name != NULL)
    {
      char *demangled;

      demangled = gdb_demangle (part_die->linkage_name, DMGL_TYPES);
      if (demangled)
	{
	  const char *base;

	  /* Strip any leading namespaces/classes, keep only the base name.
	     DW_AT_name for named DIEs does not contain the prefixes.  */
	  base = strrchr (demangled, ':');
	  if (base && base > demangled && base[-1] == ':')
	    base++;
	  else
	    base = demangled;

	  part_die->name
	    = ((const char *)
	       obstack_copy0 (&cu->objfile->per_bfd->storage_obstack,
			      base, strlen (base)));
	  xfree (demangled);
	}
    }

  part_die->fixup_called = 1;
}

/* Read an attribute value described by an attribute form.  */

static const gdb_byte *
read_attribute_value (const struct die_reader_specs *reader,
		      struct attribute *attr, unsigned form,
		      LONGEST implicit_const, const gdb_byte *info_ptr)
{
  struct dwarf2_cu *cu = reader->cu;
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  bfd *abfd = reader->abfd;
  struct comp_unit_head *cu_header = &cu->header;
  unsigned int bytes_read;
  struct dwarf_block *blk;

  attr->form = (enum dwarf_form) form;
  switch (form)
    {
    case DW_FORM_ref_addr:
      if (cu->header.version == 2)
	DW_UNSND (attr) = read_address (abfd, info_ptr, cu, &bytes_read);
      else
	DW_UNSND (attr) = read_offset (abfd, info_ptr,
				       &cu->header, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_GNU_ref_alt:
      DW_UNSND (attr) = read_offset (abfd, info_ptr, &cu->header, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_addr:
      DW_ADDR (attr) = read_address (abfd, info_ptr, cu, &bytes_read);
      DW_ADDR (attr) = gdbarch_adjust_dwarf2_addr (gdbarch, DW_ADDR (attr));
      info_ptr += bytes_read;
      break;
    case DW_FORM_block2:
      blk = dwarf_alloc_block (cu);
      blk->size = read_2_bytes (abfd, info_ptr);
      info_ptr += 2;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      DW_BLOCK (attr) = blk;
      break;
    case DW_FORM_block4:
      blk = dwarf_alloc_block (cu);
      blk->size = read_4_bytes (abfd, info_ptr);
      info_ptr += 4;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      DW_BLOCK (attr) = blk;
      break;
    case DW_FORM_data2:
      DW_UNSND (attr) = read_2_bytes (abfd, info_ptr);
      info_ptr += 2;
      break;
    case DW_FORM_data4:
      DW_UNSND (attr) = read_4_bytes (abfd, info_ptr);
      info_ptr += 4;
      break;
    case DW_FORM_data8:
      DW_UNSND (attr) = read_8_bytes (abfd, info_ptr);
      info_ptr += 8;
      break;
    case DW_FORM_data16:
      blk = dwarf_alloc_block (cu);
      blk->size = 16;
      blk->data = read_n_bytes (abfd, info_ptr, 16);
      info_ptr += 16;
      DW_BLOCK (attr) = blk;
      break;
    case DW_FORM_sec_offset:
      DW_UNSND (attr) = read_offset (abfd, info_ptr, &cu->header, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_string:
      DW_STRING (attr) = read_direct_string (abfd, info_ptr, &bytes_read);
      DW_STRING_IS_CANONICAL (attr) = 0;
      info_ptr += bytes_read;
      break;
    case DW_FORM_strp:
      if (!cu->per_cu->is_dwz)
	{
	  DW_STRING (attr) = read_indirect_string (abfd, info_ptr, cu_header,
						   &bytes_read);
	  DW_STRING_IS_CANONICAL (attr) = 0;
	  info_ptr += bytes_read;
	  break;
	}
      /* FALLTHROUGH */
    case DW_FORM_line_strp:
      if (!cu->per_cu->is_dwz)
	{
	  DW_STRING (attr) = read_indirect_line_string (abfd, info_ptr,
							cu_header, &bytes_read);
	  DW_STRING_IS_CANONICAL (attr) = 0;
	  info_ptr += bytes_read;
	  break;
	}
      /* FALLTHROUGH */
    case DW_FORM_GNU_strp_alt:
      {
	struct dwz_file *dwz = dwarf2_get_dwz_file ();
	LONGEST str_offset = read_offset (abfd, info_ptr, cu_header,
					  &bytes_read);

	DW_STRING (attr) = read_indirect_string_from_dwz (dwz, str_offset);
	DW_STRING_IS_CANONICAL (attr) = 0;
	info_ptr += bytes_read;
      }
      break;
    case DW_FORM_exprloc:
    case DW_FORM_block:
      blk = dwarf_alloc_block (cu);
      blk->size = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      DW_BLOCK (attr) = blk;
      break;
    case DW_FORM_block1:
      blk = dwarf_alloc_block (cu);
      blk->size = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      DW_BLOCK (attr) = blk;
      break;
    case DW_FORM_data1:
      DW_UNSND (attr) = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
      break;
    case DW_FORM_flag:
      DW_UNSND (attr) = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
      break;
    case DW_FORM_flag_present:
      DW_UNSND (attr) = 1;
      break;
    case DW_FORM_sdata:
      DW_SND (attr) = read_signed_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_udata:
      DW_UNSND (attr) = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_ref1:
      DW_UNSND (attr) = (to_underlying (cu->header.sect_off)
			 + read_1_byte (abfd, info_ptr));
      info_ptr += 1;
      break;
    case DW_FORM_ref2:
      DW_UNSND (attr) = (to_underlying (cu->header.sect_off)
			 + read_2_bytes (abfd, info_ptr));
      info_ptr += 2;
      break;
    case DW_FORM_ref4:
      DW_UNSND (attr) = (to_underlying (cu->header.sect_off)
			 + read_4_bytes (abfd, info_ptr));
      info_ptr += 4;
      break;
    case DW_FORM_ref8:
      DW_UNSND (attr) = (to_underlying (cu->header.sect_off)
			 + read_8_bytes (abfd, info_ptr));
      info_ptr += 8;
      break;
    case DW_FORM_ref_sig8:
      DW_SIGNATURE (attr) = read_8_bytes (abfd, info_ptr);
      info_ptr += 8;
      break;
    case DW_FORM_ref_udata:
      DW_UNSND (attr) = (to_underlying (cu->header.sect_off)
			 + read_unsigned_leb128 (abfd, info_ptr, &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_indirect:
      form = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      if (form == DW_FORM_implicit_const)
	{
	  implicit_const = read_signed_leb128 (abfd, info_ptr, &bytes_read);
	  info_ptr += bytes_read;
	}
      info_ptr = read_attribute_value (reader, attr, form, implicit_const,
				       info_ptr);
      break;
    case DW_FORM_implicit_const:
      DW_SND (attr) = implicit_const;
      break;
    case DW_FORM_GNU_addr_index:
      if (reader->dwo_file == NULL)
	{
	  /* For now flag a hard error.
	     Later we can turn this into a complaint.  */
	  error (_("Dwarf Error: %s found in non-DWO CU [in module %s]"),
		 dwarf_form_name (form),
		 bfd_get_filename (abfd));
	}
      DW_ADDR (attr) = read_addr_index_from_leb128 (cu, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      break;
    case DW_FORM_GNU_str_index:
      if (reader->dwo_file == NULL)
	{
	  /* For now flag a hard error.
	     Later we can turn this into a complaint if warranted.  */
	  error (_("Dwarf Error: %s found in non-DWO CU [in module %s]"),
		 dwarf_form_name (form),
		 bfd_get_filename (abfd));
	}
      {
	ULONGEST str_index =
	  read_unsigned_leb128 (abfd, info_ptr, &bytes_read);

	DW_STRING (attr) = read_str_index (reader, str_index);
	DW_STRING_IS_CANONICAL (attr) = 0;
	info_ptr += bytes_read;
      }
      break;
    default:
      error (_("Dwarf Error: Cannot handle %s in DWARF reader [in module %s]"),
	     dwarf_form_name (form),
	     bfd_get_filename (abfd));
    }

  /* Super hack.  */
  if (cu->per_cu->is_dwz && attr_form_is_ref (attr))
    attr->form = DW_FORM_GNU_ref_alt;

  /* We have seen instances where the compiler tried to emit a byte
     size attribute of -1 which ended up being encoded as an unsigned
     0xffffffff.  Although 0xffffffff is technically a valid size value,
     an object of this size seems pretty unlikely so we can relatively
     safely treat these cases as if the size attribute was invalid and
     treat them as zero by default.  */
  if (attr->name == DW_AT_byte_size
      && form == DW_FORM_data4
      && DW_UNSND (attr) >= 0xffffffff)
    {
      complaint
        (&symfile_complaints,
         _("Suspicious DW_AT_byte_size value treated as zero instead of %s"),
         hex_string (DW_UNSND (attr)));
      DW_UNSND (attr) = 0;
    }

  return info_ptr;
}

/* Read an attribute described by an abbreviated attribute.  */

static const gdb_byte *
read_attribute (const struct die_reader_specs *reader,
		struct attribute *attr, struct attr_abbrev *abbrev,
		const gdb_byte *info_ptr)
{
  attr->name = abbrev->name;
  return read_attribute_value (reader, attr, abbrev->form,
			       abbrev->implicit_const, info_ptr);
}

/* Read dwarf information from a buffer.  */

static unsigned int
read_1_byte (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_8 (abfd, buf);
}

static int
read_1_signed_byte (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_signed_8 (abfd, buf);
}

static unsigned int
read_2_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_16 (abfd, buf);
}

static int
read_2_signed_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_signed_16 (abfd, buf);
}

static unsigned int
read_4_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_32 (abfd, buf);
}

static int
read_4_signed_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_signed_32 (abfd, buf);
}

static ULONGEST
read_8_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_64 (abfd, buf);
}

static CORE_ADDR
read_address (bfd *abfd, const gdb_byte *buf, struct dwarf2_cu *cu,
	      unsigned int *bytes_read)
{
  struct comp_unit_head *cu_header = &cu->header;
  CORE_ADDR retval = 0;

  if (cu_header->signed_addr_p)
    {
      switch (cu_header->addr_size)
	{
	case 2:
	  retval = bfd_get_signed_16 (abfd, buf);
	  break;
	case 4:
	  retval = bfd_get_signed_32 (abfd, buf);
	  break;
	case 8:
	  retval = bfd_get_signed_64 (abfd, buf);
	  break;
	default:
	  internal_error (__FILE__, __LINE__,
			  _("read_address: bad switch, signed [in module %s]"),
			  bfd_get_filename (abfd));
	}
    }
  else
    {
      switch (cu_header->addr_size)
	{
	case 2:
	  retval = bfd_get_16 (abfd, buf);
	  break;
	case 4:
	  retval = bfd_get_32 (abfd, buf);
	  break;
	case 8:
	  retval = bfd_get_64 (abfd, buf);
	  break;
	default:
	  internal_error (__FILE__, __LINE__,
			  _("read_address: bad switch, "
			    "unsigned [in module %s]"),
			  bfd_get_filename (abfd));
	}
    }

  *bytes_read = cu_header->addr_size;
  return retval;
}

/* Read the initial length from a section.  The (draft) DWARF 3
   specification allows the initial length to take up either 4 bytes
   or 12 bytes.  If the first 4 bytes are 0xffffffff, then the next 8
   bytes describe the length and all offsets will be 8 bytes in length
   instead of 4.

   An older, non-standard 64-bit format is also handled by this
   function.  The older format in question stores the initial length
   as an 8-byte quantity without an escape value.  Lengths greater
   than 2^32 aren't very common which means that the initial 4 bytes
   is almost always zero.  Since a length value of zero doesn't make
   sense for the 32-bit format, this initial zero can be considered to
   be an escape value which indicates the presence of the older 64-bit
   format.  As written, the code can't detect (old format) lengths
   greater than 4GB.  If it becomes necessary to handle lengths
   somewhat larger than 4GB, we could allow other small values (such
   as the non-sensical values of 1, 2, and 3) to also be used as
   escape values indicating the presence of the old format.

   The value returned via bytes_read should be used to increment the
   relevant pointer after calling read_initial_length().

   [ Note:  read_initial_length() and read_offset() are based on the
     document entitled "DWARF Debugging Information Format", revision
     3, draft 8, dated November 19, 2001.  This document was obtained
     from:

	http://reality.sgiweb.org/davea/dwarf3-draft8-011125.pdf

     This document is only a draft and is subject to change.  (So beware.)

     Details regarding the older, non-standard 64-bit format were
     determined empirically by examining 64-bit ELF files produced by
     the SGI toolchain on an IRIX 6.5 machine.

     - Kevin, July 16, 2002
   ] */

static LONGEST
read_initial_length (bfd *abfd, const gdb_byte *buf, unsigned int *bytes_read)
{
  LONGEST length = bfd_get_32 (abfd, buf);

  if (length == 0xffffffff)
    {
      length = bfd_get_64 (abfd, buf + 4);
      *bytes_read = 12;
    }
  else if (length == 0)
    {
      /* Handle the (non-standard) 64-bit DWARF2 format used by IRIX.  */
      length = bfd_get_64 (abfd, buf);
      *bytes_read = 8;
    }
  else
    {
      *bytes_read = 4;
    }

  return length;
}

/* Cover function for read_initial_length.
   Returns the length of the object at BUF, and stores the size of the
   initial length in *BYTES_READ and stores the size that offsets will be in
   *OFFSET_SIZE.
   If the initial length size is not equivalent to that specified in
   CU_HEADER then issue a complaint.
   This is useful when reading non-comp-unit headers.  */

static LONGEST
read_checked_initial_length_and_offset (bfd *abfd, const gdb_byte *buf,
					const struct comp_unit_head *cu_header,
					unsigned int *bytes_read,
					unsigned int *offset_size)
{
  LONGEST length = read_initial_length (abfd, buf, bytes_read);

  gdb_assert (cu_header->initial_length_size == 4
	      || cu_header->initial_length_size == 8
	      || cu_header->initial_length_size == 12);

  if (cu_header->initial_length_size != *bytes_read)
    complaint (&symfile_complaints,
	       _("intermixed 32-bit and 64-bit DWARF sections"));

  *offset_size = (*bytes_read == 4) ? 4 : 8;
  return length;
}

/* Read an offset from the data stream.  The size of the offset is
   given by cu_header->offset_size.  */

static LONGEST
read_offset (bfd *abfd, const gdb_byte *buf,
	     const struct comp_unit_head *cu_header,
             unsigned int *bytes_read)
{
  LONGEST offset = read_offset_1 (abfd, buf, cu_header->offset_size);

  *bytes_read = cu_header->offset_size;
  return offset;
}

/* Read an offset from the data stream.  */

static LONGEST
read_offset_1 (bfd *abfd, const gdb_byte *buf, unsigned int offset_size)
{
  LONGEST retval = 0;

  switch (offset_size)
    {
    case 4:
      retval = bfd_get_32 (abfd, buf);
      break;
    case 8:
      retval = bfd_get_64 (abfd, buf);
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("read_offset_1: bad switch [in module %s]"),
		      bfd_get_filename (abfd));
    }

  return retval;
}

static const gdb_byte *
read_n_bytes (bfd *abfd, const gdb_byte *buf, unsigned int size)
{
  /* If the size of a host char is 8 bits, we can return a pointer
     to the buffer, otherwise we have to copy the data to a buffer
     allocated on the temporary obstack.  */
  gdb_assert (HOST_CHAR_BIT == 8);
  return buf;
}

static const char *
read_direct_string (bfd *abfd, const gdb_byte *buf,
		    unsigned int *bytes_read_ptr)
{
  /* If the size of a host char is 8 bits, we can return a pointer
     to the string, otherwise we have to copy the string to a buffer
     allocated on the temporary obstack.  */
  gdb_assert (HOST_CHAR_BIT == 8);
  if (*buf == '\0')
    {
      *bytes_read_ptr = 1;
      return NULL;
    }
  *bytes_read_ptr = strlen ((const char *) buf) + 1;
  return (const char *) buf;
}

/* Return pointer to string at section SECT offset STR_OFFSET with error
   reporting strings FORM_NAME and SECT_NAME.  */

static const char *
read_indirect_string_at_offset_from (bfd *abfd, LONGEST str_offset,
				     struct dwarf2_section_info *sect,
				     const char *form_name,
				     const char *sect_name)
{
  dwarf2_read_section (dwarf2_per_objfile->objfile, sect);
  if (sect->buffer == NULL)
    error (_("%s used without %s section [in module %s]"),
	   form_name, sect_name, bfd_get_filename (abfd));
  if (str_offset >= sect->size)
    error (_("%s pointing outside of %s section [in module %s]"),
	   form_name, sect_name, bfd_get_filename (abfd));
  gdb_assert (HOST_CHAR_BIT == 8);
  if (sect->buffer[str_offset] == '\0')
    return NULL;
  return (const char *) (sect->buffer + str_offset);
}

/* Return pointer to string at .debug_str offset STR_OFFSET.  */

static const char *
read_indirect_string_at_offset (bfd *abfd, LONGEST str_offset)
{
  return read_indirect_string_at_offset_from (abfd, str_offset,
					      &dwarf2_per_objfile->str,
					      "DW_FORM_strp", ".debug_str");
}

/* Return pointer to string at .debug_line_str offset STR_OFFSET.  */

static const char *
read_indirect_line_string_at_offset (bfd *abfd, LONGEST str_offset)
{
  return read_indirect_string_at_offset_from (abfd, str_offset,
					      &dwarf2_per_objfile->line_str,
					      "DW_FORM_line_strp",
					      ".debug_line_str");
}

/* Read a string at offset STR_OFFSET in the .debug_str section from
   the .dwz file DWZ.  Throw an error if the offset is too large.  If
   the string consists of a single NUL byte, return NULL; otherwise
   return a pointer to the string.  */

static const char *
read_indirect_string_from_dwz (struct dwz_file *dwz, LONGEST str_offset)
{
  dwarf2_read_section (dwarf2_per_objfile->objfile, &dwz->str);

  if (dwz->str.buffer == NULL)
    error (_("DW_FORM_GNU_strp_alt used without .debug_str "
	     "section [in module %s]"),
	   bfd_get_filename (dwz->dwz_bfd));
  if (str_offset >= dwz->str.size)
    error (_("DW_FORM_GNU_strp_alt pointing outside of "
	     ".debug_str section [in module %s]"),
	   bfd_get_filename (dwz->dwz_bfd));
  gdb_assert (HOST_CHAR_BIT == 8);
  if (dwz->str.buffer[str_offset] == '\0')
    return NULL;
  return (const char *) (dwz->str.buffer + str_offset);
}

/* Return pointer to string at .debug_str offset as read from BUF.
   BUF is assumed to be in a compilation unit described by CU_HEADER.
   Return *BYTES_READ_PTR count of bytes read from BUF.  */

static const char *
read_indirect_string (bfd *abfd, const gdb_byte *buf,
		      const struct comp_unit_head *cu_header,
		      unsigned int *bytes_read_ptr)
{
  LONGEST str_offset = read_offset (abfd, buf, cu_header, bytes_read_ptr);

  return read_indirect_string_at_offset (abfd, str_offset);
}

/* Return pointer to string at .debug_line_str offset as read from BUF.
   BUF is assumed to be in a compilation unit described by CU_HEADER.
   Return *BYTES_READ_PTR count of bytes read from BUF.  */

static const char *
read_indirect_line_string (bfd *abfd, const gdb_byte *buf,
			   const struct comp_unit_head *cu_header,
			   unsigned int *bytes_read_ptr)
{
  LONGEST str_offset = read_offset (abfd, buf, cu_header, bytes_read_ptr);

  return read_indirect_line_string_at_offset (abfd, str_offset);
}

ULONGEST
read_unsigned_leb128 (bfd *abfd, const gdb_byte *buf,
			  unsigned int *bytes_read_ptr)
{
  ULONGEST result;
  unsigned int num_read;
  int shift;
  unsigned char byte;

  result = 0;
  shift = 0;
  num_read = 0;
  while (1)
    {
      byte = bfd_get_8 (abfd, buf);
      buf++;
      num_read++;
      result |= ((ULONGEST) (byte & 127) << shift);
      if ((byte & 128) == 0)
	{
	  break;
	}
      shift += 7;
    }
  *bytes_read_ptr = num_read;
  return result;
}

static LONGEST
read_signed_leb128 (bfd *abfd, const gdb_byte *buf,
		    unsigned int *bytes_read_ptr)
{
  LONGEST result;
  int shift, num_read;
  unsigned char byte;

  result = 0;
  shift = 0;
  num_read = 0;
  while (1)
    {
      byte = bfd_get_8 (abfd, buf);
      buf++;
      num_read++;
      result |= ((LONGEST) (byte & 127) << shift);
      shift += 7;
      if ((byte & 128) == 0)
	{
	  break;
	}
    }
  if ((shift < 8 * sizeof (result)) && (byte & 0x40))
    result |= -(((LONGEST) 1) << shift);
  *bytes_read_ptr = num_read;
  return result;
}

/* Given index ADDR_INDEX in .debug_addr, fetch the value.
   ADDR_BASE is the DW_AT_GNU_addr_base attribute or zero.
   ADDR_SIZE is the size of addresses from the CU header.  */

static CORE_ADDR
read_addr_index_1 (unsigned int addr_index, ULONGEST addr_base, int addr_size)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  bfd *abfd = objfile->obfd;
  const gdb_byte *info_ptr;

  dwarf2_read_section (objfile, &dwarf2_per_objfile->addr);
  if (dwarf2_per_objfile->addr.buffer == NULL)
    error (_("DW_FORM_addr_index used without .debug_addr section [in module %s]"),
	   objfile_name (objfile));
  if (addr_base + addr_index * addr_size >= dwarf2_per_objfile->addr.size)
    error (_("DW_FORM_addr_index pointing outside of "
	     ".debug_addr section [in module %s]"),
	   objfile_name (objfile));
  info_ptr = (dwarf2_per_objfile->addr.buffer
	      + addr_base + addr_index * addr_size);
  if (addr_size == 4)
    return bfd_get_32 (abfd, info_ptr);
  else
    return bfd_get_64 (abfd, info_ptr);
}

/* Given index ADDR_INDEX in .debug_addr, fetch the value.  */

static CORE_ADDR
read_addr_index (struct dwarf2_cu *cu, unsigned int addr_index)
{
  return read_addr_index_1 (addr_index, cu->addr_base, cu->header.addr_size);
}

/* Given a pointer to an leb128 value, fetch the value from .debug_addr.  */

static CORE_ADDR
read_addr_index_from_leb128 (struct dwarf2_cu *cu, const gdb_byte *info_ptr,
			     unsigned int *bytes_read)
{
  bfd *abfd = cu->objfile->obfd;
  unsigned int addr_index = read_unsigned_leb128 (abfd, info_ptr, bytes_read);

  return read_addr_index (cu, addr_index);
}

/* Data structure to pass results from dwarf2_read_addr_index_reader
   back to dwarf2_read_addr_index.  */

struct dwarf2_read_addr_index_data
{
  ULONGEST addr_base;
  int addr_size;
};

/* die_reader_func for dwarf2_read_addr_index.  */

static void
dwarf2_read_addr_index_reader (const struct die_reader_specs *reader,
			       const gdb_byte *info_ptr,
			       struct die_info *comp_unit_die,
			       int has_children,
			       void *data)
{
  struct dwarf2_cu *cu = reader->cu;
  struct dwarf2_read_addr_index_data *aidata =
    (struct dwarf2_read_addr_index_data *) data;

  aidata->addr_base = cu->addr_base;
  aidata->addr_size = cu->header.addr_size;
}

/* Given an index in .debug_addr, fetch the value.
   NOTE: This can be called during dwarf expression evaluation,
   long after the debug information has been read, and thus per_cu->cu
   may no longer exist.  */

CORE_ADDR
dwarf2_read_addr_index (struct dwarf2_per_cu_data *per_cu,
			unsigned int addr_index)
{
  struct objfile *objfile = per_cu->objfile;
  struct dwarf2_cu *cu = per_cu->cu;
  ULONGEST addr_base;
  int addr_size;

  /* This is intended to be called from outside this file.  */
  dw2_setup (objfile);

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
      struct dwarf2_read_addr_index_data aidata;

      /* Note: We can't use init_cutu_and_read_dies_simple here,
	 we need addr_base.  */
      init_cutu_and_read_dies (per_cu, NULL, 0, 0,
			       dwarf2_read_addr_index_reader, &aidata);
      addr_base = aidata.addr_base;
      addr_size = aidata.addr_size;
    }

  return read_addr_index_1 (addr_index, addr_base, addr_size);
}

/* Given a DW_FORM_GNU_str_index, fetch the string.
   This is only used by the Fission support.  */

static const char *
read_str_index (const struct die_reader_specs *reader, ULONGEST str_index)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  const char *objf_name = objfile_name (objfile);
  bfd *abfd = objfile->obfd;
  struct dwarf2_cu *cu = reader->cu;
  struct dwarf2_section_info *str_section = &reader->dwo_file->sections.str;
  struct dwarf2_section_info *str_offsets_section =
    &reader->dwo_file->sections.str_offsets;
  const gdb_byte *info_ptr;
  ULONGEST str_offset;
  static const char form_name[] = "DW_FORM_GNU_str_index";

  dwarf2_read_section (objfile, str_section);
  dwarf2_read_section (objfile, str_offsets_section);
  if (str_section->buffer == NULL)
    error (_("%s used without .debug_str.dwo section"
	     " in CU at offset 0x%x [in module %s]"),
	   form_name, to_underlying (cu->header.sect_off), objf_name);
  if (str_offsets_section->buffer == NULL)
    error (_("%s used without .debug_str_offsets.dwo section"
	     " in CU at offset 0x%x [in module %s]"),
	   form_name, to_underlying (cu->header.sect_off), objf_name);
  if (str_index * cu->header.offset_size >= str_offsets_section->size)
    error (_("%s pointing outside of .debug_str_offsets.dwo"
	     " section in CU at offset 0x%x [in module %s]"),
	   form_name, to_underlying (cu->header.sect_off), objf_name);
  info_ptr = (str_offsets_section->buffer
	      + str_index * cu->header.offset_size);
  if (cu->header.offset_size == 4)
    str_offset = bfd_get_32 (abfd, info_ptr);
  else
    str_offset = bfd_get_64 (abfd, info_ptr);
  if (str_offset >= str_section->size)
    error (_("Offset from %s pointing outside of"
	     " .debug_str.dwo section in CU at offset 0x%x [in module %s]"),
	   form_name, to_underlying (cu->header.sect_off), objf_name);
  return (const char *) (str_section->buffer + str_offset);
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

static void
set_cu_language (unsigned int lang, struct dwarf2_cu *cu)
{
  switch (lang)
    {
    case DW_LANG_C89:
    case DW_LANG_C99:
    case DW_LANG_C11:
    case DW_LANG_C:
    case DW_LANG_UPC:
      cu->language = language_c;
      break;
    case DW_LANG_Java:
    case DW_LANG_C_plus_plus:
    case DW_LANG_C_plus_plus_11:
    case DW_LANG_C_plus_plus_14:
      cu->language = language_cplus;
      break;
    case DW_LANG_D:
      cu->language = language_d;
      break;
    case DW_LANG_Fortran77:
    case DW_LANG_Fortran90:
    case DW_LANG_Fortran95:
    case DW_LANG_Fortran03:
    case DW_LANG_Fortran08:
      cu->language = language_fortran;
      break;
    case DW_LANG_Go:
      cu->language = language_go;
      break;
    case DW_LANG_Mips_Assembler:
      cu->language = language_asm;
      break;
    case DW_LANG_Ada83:
    case DW_LANG_Ada95:
      cu->language = language_ada;
      break;
    case DW_LANG_Modula2:
      cu->language = language_m2;
      break;
    case DW_LANG_Pascal83:
      cu->language = language_pascal;
      break;
    case DW_LANG_ObjC:
      cu->language = language_objc;
      break;
    case DW_LANG_Rust:
    case DW_LANG_Rust_old:
      cu->language = language_rust;
      break;
    case DW_LANG_Cobol74:
    case DW_LANG_Cobol85:
    default:
      cu->language = language_minimal;
      break;
    }
  cu->language_defn = language_def (cu->language);
}

/* Return the named attribute or NULL if not there.  */

static struct attribute *
dwarf2_attr (struct die_info *die, unsigned int name, struct dwarf2_cu *cu)
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

      die = follow_die_ref (die, spec, &cu);
    }

  return NULL;
}

/* Return the named attribute or NULL if not there,
   but do not follow DW_AT_specification, etc.
   This is for use in contexts where we're reading .debug_types dies.
   Following DW_AT_specification, DW_AT_abstract_origin will take us
   back up the chain, and we want to go down.  */

static struct attribute *
dwarf2_attr_no_follow (struct die_info *die, unsigned int name)
{
  unsigned int i;

  for (i = 0; i < die->num_attrs; ++i)
    if (die->attrs[i].name == name)
      return &die->attrs[i];

  return NULL;
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
      if (attr->form == DW_FORM_strp || attr->form == DW_FORM_line_strp
	  || attr->form == DW_FORM_string || attr->form == DW_FORM_GNU_strp_alt)
	str = DW_STRING (attr);
      else
        complaint (&symfile_complaints,
	           _("string type expected for attribute %s for "
		     "DIE at 0x%x in module %s"),
		   dwarf_attr_name (name), to_underlying (die->sect_off),
		   objfile_name (cu->objfile));
    }

  return str;
}

/* Return non-zero iff the attribute NAME is defined for the given DIE,
   and holds a non-zero value.  This function should only be used for
   DW_FORM_flag or DW_FORM_flag_present attributes.  */

static int
dwarf2_flag_true_p (struct die_info *die, unsigned name, struct dwarf2_cu *cu)
{
  struct attribute *attr = dwarf2_attr (die, name, cu);

  return (attr && DW_UNSND (attr));
}

static int
die_is_declaration (struct die_info *die, struct dwarf2_cu *cu)
{
  /* A DIE is a declaration if it has a DW_AT_declaration attribute
     which value is non-zero.  However, we have to be careful with
     DIEs having a DW_AT_specification attribute, because dwarf2_attr()
     (via dwarf2_flag_true_p) follows this attribute.  So we may
     end up accidently finding a declaration attribute that belongs
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

/* Stub for free_line_header to match void * callback types.  */

static void
free_line_header_voidp (void *arg)
{
  struct line_header *lh = (struct line_header *) arg;

  delete lh;
}

void
line_header::add_include_dir (const char *include_dir)
{
  if (dwarf_line_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "Adding dir %zu: %s\n",
			include_dirs.size () + 1, include_dir);

  include_dirs.push_back (include_dir);
}

void
line_header::add_file_name (const char *name,
			    dir_index d_index,
			    unsigned int mod_time,
			    unsigned int length)
{
  if (dwarf_line_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "Adding file %u: %s\n",
			(unsigned) file_names.size () + 1, name);

  file_names.emplace_back (name, d_index, mod_time, length);
}

/* A convenience function to find the proper .debug_line section for a CU.  */

static struct dwarf2_section_info *
get_debug_line_section (struct dwarf2_cu *cu)
{
  struct dwarf2_section_info *section;

  /* For TUs in DWO files, the DW_AT_stmt_list attribute lives in the
     DWO file.  */
  if (cu->dwo_unit && cu->per_cu->is_debug_types)
    section = &cu->dwo_unit->dwo_file->sections.line;
  else if (cu->per_cu->is_dwz)
    {
      struct dwz_file *dwz = dwarf2_get_dwz_file ();

      section = &dwz->line;
    }
  else
    section = &dwarf2_per_objfile->line;

  return section;
}

/* Read directory or file name entry format, starting with byte of
   format count entries, ULEB128 pairs of entry formats, ULEB128 of
   entries count and the entries themselves in the described entry
   format.  */

static void
read_formatted_entries (bfd *abfd, const gdb_byte **bufp,
			struct line_header *lh,
			const struct comp_unit_head *cu_header,
			void (*callback) (struct line_header *lh,
					  const char *name,
					  dir_index d_index,
					  unsigned int mod_time,
					  unsigned int length))
{
  gdb_byte format_count, formati;
  ULONGEST data_count, datai;
  const gdb_byte *buf = *bufp;
  const gdb_byte *format_header_data;
  int i;
  unsigned int bytes_read;

  format_count = read_1_byte (abfd, buf);
  buf += 1;
  format_header_data = buf;
  for (formati = 0; formati < format_count; formati++)
    {
      read_unsigned_leb128 (abfd, buf, &bytes_read);
      buf += bytes_read;
      read_unsigned_leb128 (abfd, buf, &bytes_read);
      buf += bytes_read;
    }

  data_count = read_unsigned_leb128 (abfd, buf, &bytes_read);
  buf += bytes_read;
  for (datai = 0; datai < data_count; datai++)
    {
      const gdb_byte *format = format_header_data;
      struct file_entry fe;

      for (formati = 0; formati < format_count; formati++)
	{
	  ULONGEST content_type = read_unsigned_leb128 (abfd, format, &bytes_read);
	  format += bytes_read;

	  ULONGEST form  = read_unsigned_leb128 (abfd, format, &bytes_read);
	  format += bytes_read;

	  gdb::optional<const char *> string;
	  gdb::optional<unsigned int> uint;

	  switch (form)
	    {
	    case DW_FORM_string:
	      string.emplace (read_direct_string (abfd, buf, &bytes_read));
	      buf += bytes_read;
	      break;

	    case DW_FORM_line_strp:
	      string.emplace (read_indirect_line_string (abfd, buf,
							 cu_header,
							 &bytes_read));
	      buf += bytes_read;
	      break;

	    case DW_FORM_data1:
	      uint.emplace (read_1_byte (abfd, buf));
	      buf += 1;
	      break;

	    case DW_FORM_data2:
	      uint.emplace (read_2_bytes (abfd, buf));
	      buf += 2;
	      break;

	    case DW_FORM_data4:
	      uint.emplace (read_4_bytes (abfd, buf));
	      buf += 4;
	      break;

	    case DW_FORM_data8:
	      uint.emplace (read_8_bytes (abfd, buf));
	      buf += 8;
	      break;

	    case DW_FORM_udata:
	      uint.emplace (read_unsigned_leb128 (abfd, buf, &bytes_read));
	      buf += bytes_read;
	      break;

	    case DW_FORM_block:
	      /* It is valid only for DW_LNCT_timestamp which is ignored by
		 current GDB.  */
	      break;
	    }

	  switch (content_type)
	    {
	    case DW_LNCT_path:
	      if (string.has_value ())
		fe.name = *string;
	      break;
	    case DW_LNCT_directory_index:
	      if (uint.has_value ())
		fe.d_index = (dir_index) *uint;
	      break;
	    case DW_LNCT_timestamp:
	      if (uint.has_value ())
		fe.mod_time = *uint;
	      break;
	    case DW_LNCT_size:
	      if (uint.has_value ())
		fe.length = *uint;
	      break;
	    case DW_LNCT_MD5:
	      break;
	    default:
	      complaint (&symfile_complaints,
			 _("Unknown format content type %s"),
			 pulongest (content_type));
	    }
	}

      callback (lh, fe.name, fe.d_index, fe.mod_time, fe.length);
    }

  *bufp = buf;
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
dwarf_decode_line_header (sect_offset sect_off, struct dwarf2_cu *cu)
{
  const gdb_byte *line_ptr;
  unsigned int bytes_read, offset_size;
  int i;
  const char *cur_dir, *cur_file;
  struct dwarf2_section_info *section;
  bfd *abfd;

  section = get_debug_line_section (cu);
  dwarf2_read_section (dwarf2_per_objfile->objfile, section);
  if (section->buffer == NULL)
    {
      if (cu->dwo_unit && cu->per_cu->is_debug_types)
	complaint (&symfile_complaints, _("missing .debug_line.dwo section"));
      else
	complaint (&symfile_complaints, _("missing .debug_line section"));
      return 0;
    }

  /* We can't do this until we know the section is non-empty.
     Only then do we know we have such a section.  */
  abfd = get_section_bfd_owner (section);

  /* Make sure that at least there's room for the total_length field.
     That could be 12 bytes long, but we're just going to fudge that.  */
  if (to_underlying (sect_off) + 4 >= section->size)
    {
      dwarf2_statement_list_fits_in_line_number_section_complaint ();
      return 0;
    }

  line_header_up lh (new line_header ());

  lh->sect_off = sect_off;
  lh->offset_in_dwz = cu->per_cu->is_dwz;

  line_ptr = section->buffer + to_underlying (sect_off);

  /* Read in the header.  */
  lh->total_length =
    read_checked_initial_length_and_offset (abfd, line_ptr, &cu->header,
					    &bytes_read, &offset_size);
  line_ptr += bytes_read;
  if (line_ptr + lh->total_length > (section->buffer + section->size))
    {
      dwarf2_statement_list_fits_in_line_number_section_complaint ();
      return 0;
    }
  lh->statement_program_end = line_ptr + lh->total_length;
  lh->version = read_2_bytes (abfd, line_ptr);
  line_ptr += 2;
  if (lh->version > 5)
    {
      /* This is a version we don't understand.  The format could have
	 changed in ways we don't handle properly so just punt.  */
      complaint (&symfile_complaints,
		 _("unsupported version in .debug_line section"));
      return NULL;
    }
  if (lh->version >= 5)
    {
      gdb_byte segment_selector_size;

      /* Skip address size.  */
      read_1_byte (abfd, line_ptr);
      line_ptr += 1;

      segment_selector_size = read_1_byte (abfd, line_ptr);
      line_ptr += 1;
      if (segment_selector_size != 0)
	{
	  complaint (&symfile_complaints,
		     _("unsupported segment selector size %u "
		       "in .debug_line section"),
		     segment_selector_size);
	  return NULL;
	}
    }
  lh->header_length = read_offset_1 (abfd, line_ptr, offset_size);
  line_ptr += offset_size;
  lh->minimum_instruction_length = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  if (lh->version >= 4)
    {
      lh->maximum_ops_per_instruction = read_1_byte (abfd, line_ptr);
      line_ptr += 1;
    }
  else
    lh->maximum_ops_per_instruction = 1;

  if (lh->maximum_ops_per_instruction == 0)
    {
      lh->maximum_ops_per_instruction = 1;
      complaint (&symfile_complaints,
		 _("invalid maximum_ops_per_instruction "
		   "in `.debug_line' section"));
    }

  lh->default_is_stmt = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  lh->line_base = read_1_signed_byte (abfd, line_ptr);
  line_ptr += 1;
  lh->line_range = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  lh->opcode_base = read_1_byte (abfd, line_ptr);
  line_ptr += 1;
  lh->standard_opcode_lengths.reset (new unsigned char[lh->opcode_base]);

  lh->standard_opcode_lengths[0] = 1;  /* This should never be used anyway.  */
  for (i = 1; i < lh->opcode_base; ++i)
    {
      lh->standard_opcode_lengths[i] = read_1_byte (abfd, line_ptr);
      line_ptr += 1;
    }

  if (lh->version >= 5)
    {
      /* Read directory table.  */
      read_formatted_entries (abfd, &line_ptr, lh.get (), &cu->header,
			      [] (struct line_header *lh, const char *name,
				  dir_index d_index, unsigned int mod_time,
				  unsigned int length)
	{
	  lh->add_include_dir (name);
	});

      /* Read file name table.  */
      read_formatted_entries (abfd, &line_ptr, lh.get (), &cu->header,
			      [] (struct line_header *lh, const char *name,
				  dir_index d_index, unsigned int mod_time,
				  unsigned int length)
	{
	  lh->add_file_name (name, d_index, mod_time, length);
	});
    }
  else
    {
      /* Read directory table.  */
      while ((cur_dir = read_direct_string (abfd, line_ptr, &bytes_read)) != NULL)
	{
	  line_ptr += bytes_read;
	  lh->add_include_dir (cur_dir);
	}
      line_ptr += bytes_read;

      /* Read file name table.  */
      while ((cur_file = read_direct_string (abfd, line_ptr, &bytes_read)) != NULL)
	{
	  unsigned int mod_time, length;
	  dir_index d_index;

	  line_ptr += bytes_read;
	  d_index = (dir_index) read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	  line_ptr += bytes_read;
	  mod_time = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	  line_ptr += bytes_read;
	  length = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	  line_ptr += bytes_read;

	  lh->add_file_name (cur_file, d_index, mod_time, length);
	}
      line_ptr += bytes_read;
    }
  lh->statement_program_start = line_ptr;

  if (line_ptr > (section->buffer + section->size))
    complaint (&symfile_complaints,
	       _("line number info header doesn't "
		 "fit in `.debug_line' section"));

  return lh;
}

/* Subroutine of dwarf_decode_lines to simplify it.
   Return the file name of the psymtab for included file FILE_INDEX
   in line header LH of PST.
   COMP_DIR is the compilation directory (DW_AT_comp_dir) or NULL if unknown.
   If space for the result is malloc'd, it will be freed by a cleanup.
   Returns NULL if FILE_INDEX should be ignored, i.e., it is pst->filename.

   The function creates dangling cleanup registration.  */

static const char *
psymtab_include_file_name (const struct line_header *lh, int file_index,
			   const struct partial_symtab *pst,
			   const char *comp_dir)
{
  const file_entry &fe = lh->file_names[file_index];
  const char *include_name = fe.name;
  const char *include_name_to_compare = include_name;
  const char *pst_filename;
  char *copied_name = NULL;
  int file_is_pst;

  const char *dir_name = fe.include_dir (lh);

  if (!IS_ABSOLUTE_PATH (include_name)
      && (dir_name != NULL || comp_dir != NULL))
    {
      /* Avoid creating a duplicate psymtab for PST.
	 We do this by comparing INCLUDE_NAME and PST_FILENAME.
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
	  char *tem = concat (dir_name, SLASH_STRING,
			      include_name, (char *)NULL);

	  make_cleanup (xfree, tem);
	  include_name = tem;
	  include_name_to_compare = include_name;
	}
      if (!IS_ABSOLUTE_PATH (include_name) && comp_dir != NULL)
	{
	  char *tem = concat (comp_dir, SLASH_STRING,
			      include_name, (char *)NULL);

	  make_cleanup (xfree, tem);
	  include_name_to_compare = tem;
	}
    }

  pst_filename = pst->filename;
  if (!IS_ABSOLUTE_PATH (pst_filename) && pst->dirname != NULL)
    {
      copied_name = concat (pst->dirname, SLASH_STRING,
			    pst_filename, (char *)NULL);
      pst_filename = copied_name;
    }

  file_is_pst = FILENAME_CMP (include_name_to_compare, pst_filename) == 0;

  if (copied_name != NULL)
    xfree (copied_name);

  if (file_is_pst)
    return NULL;
  return include_name;
}

/* State machine to track the state of the line number program.  */

class lnp_state_machine
{
public:
  /* Initialize a machine state for the start of a line number
     program.  */
  lnp_state_machine (gdbarch *arch, line_header *lh, bool record_lines_p);

  file_entry *current_file ()
  {
    /* lh->file_names is 0-based, but the file name numbers in the
       statement program are 1-based.  */
    return m_line_header->file_name_at (m_file);
  }

  /* Record the line in the state machine.  END_SEQUENCE is true if
     we're processing the end of a sequence.  */
  void record_line (bool end_sequence);

  /* Check address and if invalid nop-out the rest of the lines in this
     sequence.  */
  void check_line_address (struct dwarf2_cu *cu,
			   const gdb_byte *line_ptr,
			   CORE_ADDR lowpc, CORE_ADDR address);

  void handle_set_discriminator (unsigned int discriminator)
  {
    m_discriminator = discriminator;
    m_line_has_non_zero_discriminator |= discriminator != 0;
  }

  /* Handle DW_LNE_set_address.  */
  void handle_set_address (CORE_ADDR baseaddr, CORE_ADDR address)
  {
    m_op_index = 0;
    address += baseaddr;
    m_address = gdbarch_adjust_dwarf2_line (m_gdbarch, address, false);
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
    m_is_stmt = !m_is_stmt;
  }

  /* Handle DW_LNS_const_add_pc.  */
  void handle_const_add_pc ();

  /* Handle DW_LNS_fixed_advance_pc.  */
  void handle_fixed_advance_pc (CORE_ADDR addr_adj)
  {
    m_address += gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
    m_op_index = 0;
  }

  /* Handle DW_LNS_copy.  */
  void handle_copy ()
  {
    record_line (false);
    m_discriminator = 0;
  }

  /* Handle DW_LNE_end_sequence.  */
  void handle_end_sequence ()
  {
    m_record_line_callback = ::record_line;
  }

private:
  /* Advance the line by LINE_DELTA.  */
  void advance_line (int line_delta)
  {
    m_line += line_delta;

    if (line_delta != 0)
      m_line_has_non_zero_discriminator = m_discriminator != 0;
  }

  gdbarch *m_gdbarch;

  /* True if we're recording lines.
     Otherwise we're building partial symtabs and are just interested in
     finding include files mentioned by the line number program.  */
  bool m_record_lines_p;

  /* The line number header.  */
  line_header *m_line_header;

  /* These are part of the standard DWARF line number state machine,
     and initialized according to the DWARF spec.  */

  unsigned char m_op_index = 0;
  /* The line table index (1-based) of the current file.  */
  file_name_index m_file = (file_name_index) 1;
  unsigned int m_line = 1;

  /* These are initialized in the constructor.  */

  CORE_ADDR m_address;
  bool m_is_stmt;
  unsigned int m_discriminator;

  /* Additional bits of state we need to track.  */

  /* The last file that we called dwarf2_start_subfile for.
     This is only used for TLLs.  */
  unsigned int m_last_file = 0;
  /* The last file a line number was recorded for.  */
  struct subfile *m_last_subfile = NULL;

  /* The function to call to record a line.  */
  record_line_ftype *m_record_line_callback = NULL;

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
  m_address += gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
  m_op_index = ((m_op_index + adjust)
		% m_line_header->maximum_ops_per_instruction);
}

void
lnp_state_machine::handle_special_opcode (unsigned char op_code)
{
  unsigned char adj_opcode = op_code - m_line_header->opcode_base;
  CORE_ADDR addr_adj = (((m_op_index
			  + (adj_opcode / m_line_header->line_range))
			 / m_line_header->maximum_ops_per_instruction)
			* m_line_header->minimum_instruction_length);
  m_address += gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
  m_op_index = ((m_op_index + (adj_opcode / m_line_header->line_range))
		% m_line_header->maximum_ops_per_instruction);

  int line_delta = (m_line_header->line_base
		    + (adj_opcode % m_line_header->line_range));
  advance_line (line_delta);
  record_line (false);
  m_discriminator = 0;
}

void
lnp_state_machine::handle_set_file (file_name_index file)
{
  m_file = file;

  const file_entry *fe = current_file ();
  if (fe == NULL)
    dwarf2_debug_line_missing_file_complaint ();
  else if (m_record_lines_p)
    {
      const char *dir = fe->include_dir (m_line_header);

      m_last_subfile = current_subfile;
      m_line_has_non_zero_discriminator = m_discriminator != 0;
      dwarf2_start_subfile (fe->name, dir);
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

  m_address += gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
  m_op_index = ((m_op_index + adjust)
		% m_line_header->maximum_ops_per_instruction);
}

/* Ignore this record_line request.  */

static void
noop_record_line (struct subfile *subfile, int line, CORE_ADDR pc)
{
  return;
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
dwarf_record_line_p (unsigned int line, unsigned int last_line,
		     int line_has_non_zero_discriminator,
		     struct subfile *last_subfile)
{
  if (current_subfile != last_subfile)
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

/* Use P_RECORD_LINE to record line number LINE beginning at address ADDRESS
   in the line table of subfile SUBFILE.  */

static void
dwarf_record_line_1 (struct gdbarch *gdbarch, struct subfile *subfile,
		     unsigned int line, CORE_ADDR address,
		     record_line_ftype p_record_line)
{
  CORE_ADDR addr = gdbarch_addr_bits_remove (gdbarch, address);

  if (dwarf_line_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "Recording line %u, file %s, address %s\n",
			  line, lbasename (subfile->name),
			  paddress (gdbarch, address));
    }

  (*p_record_line) (subfile, line, addr);
}

/* Subroutine of dwarf_decode_lines_1 to simplify it.
   Mark the end of a set of line number records.
   The arguments are the same as for dwarf_record_line_1.
   If SUBFILE is NULL the request is ignored.  */

static void
dwarf_finish_line (struct gdbarch *gdbarch, struct subfile *subfile,
		   CORE_ADDR address, record_line_ftype p_record_line)
{
  if (subfile == NULL)
    return;

  if (dwarf_line_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "Finishing current line, file %s, address %s\n",
			  lbasename (subfile->name),
			  paddress (gdbarch, address));
    }

  dwarf_record_line_1 (gdbarch, subfile, 0, address, p_record_line);
}

void
lnp_state_machine::record_line (bool end_sequence)
{
  if (dwarf_line_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "Processing actual line %u: file %u,"
			  " address %s, is_stmt %u, discrim %u\n",
			  m_line, to_underlying (m_file),
			  paddress (m_gdbarch, m_address),
			  m_is_stmt, m_discriminator);
    }

  file_entry *fe = current_file ();

  if (fe == NULL)
    dwarf2_debug_line_missing_file_complaint ();
  /* For now we ignore lines not starting on an instruction boundary.
     But not when processing end_sequence for compatibility with the
     previous version of the code.  */
  else if (m_op_index == 0 || end_sequence)
    {
      fe->included_p = 1;
      if (m_record_lines_p && m_is_stmt)
	{
	  if (m_last_subfile != current_subfile || end_sequence)
	    {
	      dwarf_finish_line (m_gdbarch, m_last_subfile,
				 m_address, m_record_line_callback);
	    }

	  if (!end_sequence)
	    {
	      if (dwarf_record_line_p (m_line, m_last_line,
				       m_line_has_non_zero_discriminator,
				       m_last_subfile))
		{
		  dwarf_record_line_1 (m_gdbarch, current_subfile,
				       m_line, m_address,
				       m_record_line_callback);
		}
	      m_last_subfile = current_subfile;
	      m_last_line = m_line;
	    }
	}
    }
}

lnp_state_machine::lnp_state_machine (gdbarch *arch, line_header *lh,
				      bool record_lines_p)
{
  m_gdbarch = arch;
  m_record_lines_p = record_lines_p;
  m_line_header = lh;

  m_record_line_callback = ::record_line;

  /* Call `gdbarch_adjust_dwarf2_line' on the initial 0 address as if there
     was a line entry for it so that the backend has a chance to adjust it
     and also record it in case it needs it.  This is currently used by MIPS
     code, cf. `mips_adjust_dwarf2_line'.  */
  m_address = gdbarch_adjust_dwarf2_line (arch, 0, 0);
  m_is_stmt = lh->default_is_stmt;
  m_discriminator = 0;
}

void
lnp_state_machine::check_line_address (struct dwarf2_cu *cu,
				       const gdb_byte *line_ptr,
				       CORE_ADDR lowpc, CORE_ADDR address)
{
  /* If address < lowpc then it's not a usable value, it's outside the
     pc range of the CU.  However, we restrict the test to only address
     values of zero to preserve GDB's previous behaviour which is to
     handle the specific case of a function being GC'd by the linker.  */

  if (address == 0 && address < lowpc)
    {
      /* This line table is for a function which has been
	 GCd by the linker.  Ignore it.  PR gdb/12528 */

      struct objfile *objfile = cu->objfile;
      long line_offset = line_ptr - get_debug_line_section (cu)->buffer;

      complaint (&symfile_complaints,
		 _(".debug_line address at offset 0x%lx is 0 [in module %s]"),
		 line_offset, objfile_name (objfile));
      m_record_line_callback = noop_record_line;
      /* Note: record_line_callback is left as noop_record_line until
	 we see DW_LNE_end_sequence.  */
    }
}

/* Subroutine of dwarf_decode_lines to simplify it.
   Process the line number information in LH.
   If DECODE_FOR_PST_P is non-zero, all we do is process the line number
   program in order to set included_p for every referenced header.  */

static void
dwarf_decode_lines_1 (struct line_header *lh, struct dwarf2_cu *cu,
		      const int decode_for_pst_p, CORE_ADDR lowpc)
{
  const gdb_byte *line_ptr, *extended_end;
  const gdb_byte *line_end;
  unsigned int bytes_read, extended_len;
  unsigned char op_code, extended_op;
  CORE_ADDR baseaddr;
  struct objfile *objfile = cu->objfile;
  bfd *abfd = objfile->obfd;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  /* True if we're recording line info (as opposed to building partial
     symtabs and just interested in finding include files mentioned by
     the line number program).  */
  bool record_lines_p = !decode_for_pst_p;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  line_ptr = lh->statement_program_start;
  line_end = lh->statement_program_end;

  /* Read the statement sequences until there's nothing left.  */
  while (line_ptr < line_end)
    {
      /* The DWARF line number program state machine.  Reset the state
	 machine at the start of each sequence.  */
      lnp_state_machine state_machine (gdbarch, lh, record_lines_p);
      bool end_sequence = false;

      if (record_lines_p)
	{
	  /* Start a subfile for the current file of the state
	     machine.  */
	  const file_entry *fe = state_machine.current_file ();

	  if (fe != NULL)
	    dwarf2_start_subfile (fe->name, fe->include_dir (lh));
	}

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
	      switch (extended_op)
		{
		case DW_LNE_end_sequence:
		  state_machine.handle_end_sequence ();
		  end_sequence = true;
		  break;
		case DW_LNE_set_address:
		  {
		    CORE_ADDR address
		      = read_address (abfd, line_ptr, cu, &bytes_read);
		    line_ptr += bytes_read;

		    state_machine.check_line_address (cu, line_ptr,
						      lowpc, address);
		    state_machine.handle_set_address (baseaddr, address);
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
		  complaint (&symfile_complaints,
			     _("mangled .debug_line section"));
		  return;
		}
	      /* Make sure that we parsed the extended op correctly.  If e.g.
		 we expected a different address size than the producer used,
		 we may have read the wrong number of bytes.  */
	      if (line_ptr != extended_end)
		{
		  complaint (&symfile_complaints,
			     _("mangled .debug_line section"));
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

   1. If PST is NULL, then this procedure uses the data from the program
      to create all necessary symbol tables, and their linetables.

   2. If PST is not NULL, this procedure reads the program to determine
      the list of files included by the unit represented by PST, and
      builds all the associated partial symbol tables.

   COMP_DIR is the compilation directory (DW_AT_comp_dir) or NULL if unknown.
   It is used for relative paths in the line table.
   NOTE: When processing partial symtabs (pst != NULL),
   comp_dir == pst->dirname.

   NOTE: It is important that psymtabs have the same file name (via strcmp)
   as the corresponding symtab.  Since COMP_DIR is not used in the name of the
   symtab we don't use it in the name of the psymtabs we create.
   E.g. expand_line_sal requires this when finding psymtabs to expand.
   A good testcase for this is mb-inline.exp.

   LOWPC is the lowest address in CU (or 0 if not known).

   Boolean DECODE_MAPPING specifies we need to fully decode .debug_line
   for its PC<->lines mapping information.  Otherwise only the filename
   table is read in.  */

static void
dwarf_decode_lines (struct line_header *lh, const char *comp_dir,
		    struct dwarf2_cu *cu, struct partial_symtab *pst,
		    CORE_ADDR lowpc, int decode_mapping)
{
  struct objfile *objfile = cu->objfile;
  const int decode_for_pst_p = (pst != NULL);

  if (decode_mapping)
    dwarf_decode_lines_1 (lh, cu, decode_for_pst_p, lowpc);

  if (decode_for_pst_p)
    {
      int file_index;

      /* Now that we're done scanning the Line Header Program, we can
         create the psymtab of each included file.  */
      for (file_index = 0; file_index < lh->file_names.size (); file_index++)
        if (lh->file_names[file_index].included_p == 1)
          {
	    const char *include_name =
	      psymtab_include_file_name (lh, file_index, pst, comp_dir);
	    if (include_name != NULL)
              dwarf2_create_include_psymtab (include_name, pst, objfile);
          }
    }
  else
    {
      /* Make sure a symtab is created for every file, even files
	 which contain only variables (i.e. no code with associated
	 line numbers).  */
      struct compunit_symtab *cust = buildsym_compunit_symtab ();
      int i;

      for (i = 0; i < lh->file_names.size (); i++)
	{
	  file_entry &fe = lh->file_names[i];

	  dwarf2_start_subfile (fe.name, fe.include_dir (lh));

	  if (current_subfile->symtab == NULL)
	    {
	      current_subfile->symtab
		= allocate_symtab (cust, current_subfile->name);
	    }
	  fe.symtab = current_subfile->symtab;
	}
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
dwarf2_start_subfile (const char *filename, const char *dirname)
{
  char *copy = NULL;

  /* In order not to lose the line information directory,
     we concatenate it to the filename when it makes sense.
     Note that the Dwarf3 standard says (speaking of filenames in line
     information): ``The directory index is ignored for file names
     that represent full path names''.  Thus ignoring dirname in the
     `else' branch below isn't an issue.  */

  if (!IS_ABSOLUTE_PATH (filename) && dirname != NULL)
    {
      copy = concat (dirname, SLASH_STRING, filename, (char *)NULL);
      filename = copy;
    }

  start_subfile (filename);

  if (copy != NULL)
    xfree (copy);
}

/* Start a symtab for DWARF.
   NAME, COMP_DIR, LOW_PC are passed to start_symtab.  */

static struct compunit_symtab *
dwarf2_start_symtab (struct dwarf2_cu *cu,
		     const char *name, const char *comp_dir, CORE_ADDR low_pc)
{
  struct compunit_symtab *cust
    = start_symtab (cu->objfile, name, comp_dir, low_pc);

  record_debugformat ("DWARF 2");
  record_producer (cu->producer);

  /* We assume that we're processing GCC output.  */
  processing_gcc_compilation = 2;

  cu->processing_has_namespace_info = 0;

  return cust;
}

static void
var_decode_location (struct attribute *attr, struct symbol *sym,
		     struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct comp_unit_head *cu_header = &cu->header;

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
  if (attr_form_is_block (attr) && DW_BLOCK (attr)->size == 0)
    {
      SYMBOL_ACLASS_INDEX (sym) = LOC_OPTIMIZED_OUT;
      return;
    }

  /* Handle one degenerate form of location expression specially, to
     preserve GDB's previous behavior when section offsets are
     specified.  If this is just a DW_OP_addr or DW_OP_GNU_addr_index
     then mark this symbol as LOC_STATIC.  */

  if (attr_form_is_block (attr)
      && ((DW_BLOCK (attr)->data[0] == DW_OP_addr
	   && DW_BLOCK (attr)->size == 1 + cu_header->addr_size)
	  || (DW_BLOCK (attr)->data[0] == DW_OP_GNU_addr_index
	      && (DW_BLOCK (attr)->size
		  == 1 + leb128_size (&DW_BLOCK (attr)->data[1])))))
    {
      unsigned int dummy;

      if (DW_BLOCK (attr)->data[0] == DW_OP_addr)
	SYMBOL_VALUE_ADDRESS (sym) =
	  read_address (objfile->obfd, DW_BLOCK (attr)->data + 1, cu, &dummy);
      else
	SYMBOL_VALUE_ADDRESS (sym) =
	  read_addr_index_from_leb128 (cu, DW_BLOCK (attr)->data + 1, &dummy);
      SYMBOL_ACLASS_INDEX (sym) = LOC_STATIC;
      fixup_symbol_section (sym, objfile);
      SYMBOL_VALUE_ADDRESS (sym) += ANOFFSET (objfile->section_offsets,
					      SYMBOL_SECTION (sym));
      return;
    }

  /* NOTE drow/2002-01-30: It might be worthwhile to have a static
     expression evaluator, and use LOC_COMPUTED only when necessary
     (i.e. when the value of a register or memory location is
     referenced, or a thread-local block, etc.).  Then again, it might
     not be worthwhile.  I'm assuming that it isn't unless performance
     or memory numbers show me otherwise.  */

  dwarf2_symbol_mark_computed (attr, sym, cu, 0);

  if (SYMBOL_COMPUTED_OPS (sym)->location_has_loclist)
    cu->has_loclist = 1;
}

/* Given a pointer to a DWARF information entry, figure out if we need
   to make a symbol table entry for it, and if so, create a new entry
   and return a pointer to it.
   If TYPE is NULL, determine symbol type from the die, otherwise
   used the passed type.
   If SPACE is not NULL, use it to hold the new symbol.  If it is
   NULL, allocate a new symbol on the objfile's obstack.  */

static struct symbol *
new_symbol_full (struct die_info *die, struct type *type, struct dwarf2_cu *cu,
		 struct symbol *space)
{
  struct objfile *objfile = cu->objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct symbol *sym = NULL;
  const char *name;
  struct attribute *attr = NULL;
  struct attribute *attr2 = NULL;
  CORE_ADDR baseaddr;
  struct pending **list_to_add = NULL;

  int inlined_func = (die->tag == DW_TAG_inlined_subroutine);

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  name = dwarf2_name (die, cu);
  if (name)
    {
      const char *linkagename;
      int suppress_add = 0;

      if (space)
	sym = space;
      else
	sym = allocate_symbol (objfile);
      OBJSTAT (objfile, n_syms++);

      /* Cache this symbol's name and the name's demangled form (if any).  */
      SYMBOL_SET_LANGUAGE (sym, cu->language, &objfile->objfile_obstack);
      linkagename = dwarf2_physname (name, die, cu);
      SYMBOL_SET_NAMES (sym, linkagename, strlen (linkagename), 0, objfile);

      /* Fortran does not have mangling standard and the mangling does differ
	 between gfortran, iFort etc.  */
      if (cu->language == language_fortran
          && symbol_get_demangled_name (&(sym->ginfo)) == NULL)
	symbol_set_demangled_name (&(sym->ginfo),
				   dwarf2_full_name (name, die, cu),
	                           NULL);

      /* Default assumptions.
         Use the passed type or decode it from the die.  */
      SYMBOL_DOMAIN (sym) = VAR_DOMAIN;
      SYMBOL_ACLASS_INDEX (sym) = LOC_OPTIMIZED_OUT;
      if (type != NULL)
	SYMBOL_TYPE (sym) = type;
      else
	SYMBOL_TYPE (sym) = die_type (die, cu);
      attr = dwarf2_attr (die,
			  inlined_func ? DW_AT_call_line : DW_AT_decl_line,
			  cu);
      if (attr)
	{
	  SYMBOL_LINE (sym) = DW_UNSND (attr);
	}

      attr = dwarf2_attr (die,
			  inlined_func ? DW_AT_call_file : DW_AT_decl_file,
			  cu);
      if (attr)
	{
	  file_name_index file_index = (file_name_index) DW_UNSND (attr);
	  struct file_entry *fe;

	  if (cu->line_header != NULL)
	    fe = cu->line_header->file_name_at (file_index);
	  else
	    fe = NULL;

	  if (fe == NULL)
	    complaint (&symfile_complaints,
		       _("file index out of range"));
	  else
	    symbol_set_symtab (sym, fe->symtab);
	}

      switch (die->tag)
	{
	case DW_TAG_label:
	  attr = dwarf2_attr (die, DW_AT_low_pc, cu);
	  if (attr)
	    {
	      CORE_ADDR addr;

	      addr = attr_value_as_address (attr);
	      addr = gdbarch_adjust_dwarf2_addr (gdbarch, addr + baseaddr);
	      SYMBOL_VALUE_ADDRESS (sym) = addr;
	    }
	  SYMBOL_TYPE (sym) = objfile_type (objfile)->builtin_core_addr;
	  SYMBOL_DOMAIN (sym) = LABEL_DOMAIN;
	  SYMBOL_ACLASS_INDEX (sym) = LOC_LABEL;
	  add_symbol_to_list (sym, cu->list_in_scope);
	  break;
	case DW_TAG_subprogram:
	  /* SYMBOL_BLOCK_VALUE (sym) will be filled in later by
	     finish_block.  */
	  SYMBOL_ACLASS_INDEX (sym) = LOC_BLOCK;
	  attr2 = dwarf2_attr (die, DW_AT_external, cu);
	  if ((attr2 && (DW_UNSND (attr2) != 0))
              || cu->language == language_ada)
	    {
              /* Subprograms marked external are stored as a global symbol.
                 Ada subprograms, whether marked external or not, are always
                 stored as a global symbol, because we want to be able to
                 access them globally.  For instance, we want to be able
                 to break on a nested subprogram without having to
                 specify the context.  */
	      list_to_add = &global_symbols;
	    }
	  else
	    {
	      list_to_add = cu->list_in_scope;
	    }
	  break;
	case DW_TAG_inlined_subroutine:
	  /* SYMBOL_BLOCK_VALUE (sym) will be filled in later by
	     finish_block.  */
	  SYMBOL_ACLASS_INDEX (sym) = LOC_BLOCK;
	  SYMBOL_INLINED (sym) = 1;
	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_template_value_param:
	  suppress_add = 1;
	  /* Fall through.  */
	case DW_TAG_constant:
	case DW_TAG_variable:
	case DW_TAG_member:
	  /* Compilation with minimal debug info may result in
	     variables with missing type entries.  Change the
	     misleading `void' type to something sensible.  */
	  if (TYPE_CODE (SYMBOL_TYPE (sym)) == TYPE_CODE_VOID)
	    SYMBOL_TYPE (sym)
	      = objfile_type (objfile)->nodebug_data_symbol;

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
	  if (attr)
	    {
	      dwarf2_const_value (attr, sym, cu);
	      attr2 = dwarf2_attr (die, DW_AT_external, cu);
	      if (!suppress_add)
		{
		  if (attr2 && (DW_UNSND (attr2) != 0))
		    list_to_add = &global_symbols;
		  else
		    list_to_add = cu->list_in_scope;
		}
	      break;
	    }
	  attr = dwarf2_attr (die, DW_AT_location, cu);
	  if (attr)
	    {
	      var_decode_location (attr, sym, cu);
	      attr2 = dwarf2_attr (die, DW_AT_external, cu);

	      /* Fortran explicitly imports any global symbols to the local
		 scope by DW_TAG_common_block.  */
	      if (cu->language == language_fortran && die->parent
		  && die->parent->tag == DW_TAG_common_block)
		attr2 = NULL;

	      if (SYMBOL_CLASS (sym) == LOC_STATIC
		  && SYMBOL_VALUE_ADDRESS (sym) == 0
		  && !dwarf2_per_objfile->has_section_at_zero)
		{
		  /* When a static variable is eliminated by the linker,
		     the corresponding debug information is not stripped
		     out, but the variable address is set to null;
		     do not add such variables into symbol table.  */
		}
	      else if (attr2 && (DW_UNSND (attr2) != 0))
		{
		  /* Workaround gfortran PR debug/40040 - it uses
		     DW_AT_location for variables in -fPIC libraries which may
		     get overriden by other libraries/executable and get
		     a different address.  Resolve it by the minimal symbol
		     which may come from inferior's executable using copy
		     relocation.  Make this workaround only for gfortran as for
		     other compilers GDB cannot guess the minimal symbol
		     Fortran mangling kind.  */
		  if (cu->language == language_fortran && die->parent
		      && die->parent->tag == DW_TAG_module
		      && cu->producer
		      && startswith (cu->producer, "GNU Fortran"))
		    SYMBOL_ACLASS_INDEX (sym) = LOC_UNRESOLVED;

		  /* A variable with DW_AT_external is never static,
		     but it may be block-scoped.  */
		  list_to_add = (cu->list_in_scope == &file_symbols
				 ? &global_symbols : cu->list_in_scope);
		}
	      else
		list_to_add = cu->list_in_scope;
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
	      if (cu->language == language_fortran && die->parent
		  && die->parent->tag == DW_TAG_common_block)
		{
		  /* SYMBOL_CLASS doesn't matter here because
		     read_common_block is going to reset it.  */
		  if (!suppress_add)
		    list_to_add = cu->list_in_scope;
		}
	      else if (attr2 && (DW_UNSND (attr2) != 0)
		       && dwarf2_attr (die, DW_AT_type, cu) != NULL)
		{
		  /* A variable with DW_AT_external is never static, but it
		     may be block-scoped.  */
		  list_to_add = (cu->list_in_scope == &file_symbols
				 ? &global_symbols : cu->list_in_scope);

		  SYMBOL_ACLASS_INDEX (sym) = LOC_UNRESOLVED;
		}
	      else if (!die_is_declaration (die, cu))
		{
		  /* Use the default LOC_OPTIMIZED_OUT class.  */
		  gdb_assert (SYMBOL_CLASS (sym) == LOC_OPTIMIZED_OUT);
		  if (!suppress_add)
		    list_to_add = cu->list_in_scope;
		}
	    }
	  break;
	case DW_TAG_formal_parameter:
	  /* If we are inside a function, mark this as an argument.  If
	     not, we might be looking at an argument to an inlined function
	     when we do not have enough information to show inlined frames;
	     pretend it's a local variable in that case so that the user can
	     still see it.  */
	  if (context_stack_depth > 0
	      && context_stack[context_stack_depth - 1].name != NULL)
	    SYMBOL_IS_ARGUMENT (sym) = 1;
	  attr = dwarf2_attr (die, DW_AT_location, cu);
	  if (attr)
	    {
	      var_decode_location (attr, sym, cu);
	    }
	  attr = dwarf2_attr (die, DW_AT_const_value, cu);
	  if (attr)
	    {
	      dwarf2_const_value (attr, sym, cu);
	    }

	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_unspecified_parameters:
	  /* From varargs functions; gdb doesn't seem to have any
	     interest in this information, so just ignore it for now.
	     (FIXME?) */
	  break;
	case DW_TAG_template_type_param:
	  suppress_add = 1;
	  /* Fall through.  */
	case DW_TAG_class_type:
	case DW_TAG_interface_type:
	case DW_TAG_structure_type:
	case DW_TAG_union_type:
	case DW_TAG_set_type:
	case DW_TAG_enumeration_type:
	  SYMBOL_ACLASS_INDEX (sym) = LOC_TYPEDEF;
	  SYMBOL_DOMAIN (sym) = STRUCT_DOMAIN;

	  {
	    /* NOTE: carlton/2003-11-10: C++ class symbols shouldn't
	       really ever be static objects: otherwise, if you try
	       to, say, break of a class's method and you're in a file
	       which doesn't mention that class, it won't work unless
	       the check for all static symbols in lookup_symbol_aux
	       saves you.  See the OtherFileClass tests in
	       gdb.c++/namespace.exp.  */

	    if (!suppress_add)
	      {
		list_to_add = (cu->list_in_scope == &file_symbols
			       && cu->language == language_cplus
			       ? &global_symbols : cu->list_in_scope);

		/* The semantics of C++ state that "struct foo {
		   ... }" also defines a typedef for "foo".  */
		if (cu->language == language_cplus
		    || cu->language == language_ada
		    || cu->language == language_d
		    || cu->language == language_rust)
		  {
		    /* The symbol's name is already allocated along
		       with this objfile, so we don't need to
		       duplicate it for the type.  */
		    if (TYPE_NAME (SYMBOL_TYPE (sym)) == 0)
		      TYPE_NAME (SYMBOL_TYPE (sym)) = SYMBOL_SEARCH_NAME (sym);
		  }
	      }
	  }
	  break;
	case DW_TAG_typedef:
	  SYMBOL_ACLASS_INDEX (sym) = LOC_TYPEDEF;
	  SYMBOL_DOMAIN (sym) = VAR_DOMAIN;
	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_base_type:
        case DW_TAG_subrange_type:
	  SYMBOL_ACLASS_INDEX (sym) = LOC_TYPEDEF;
	  SYMBOL_DOMAIN (sym) = VAR_DOMAIN;
	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_enumerator:
	  attr = dwarf2_attr (die, DW_AT_const_value, cu);
	  if (attr)
	    {
	      dwarf2_const_value (attr, sym, cu);
	    }
	  {
	    /* NOTE: carlton/2003-11-10: See comment above in the
	       DW_TAG_class_type, etc. block.  */

	    list_to_add = (cu->list_in_scope == &file_symbols
			   && cu->language == language_cplus
			   ? &global_symbols : cu->list_in_scope);
	  }
	  break;
	case DW_TAG_imported_declaration:
	case DW_TAG_namespace:
	  SYMBOL_ACLASS_INDEX (sym) = LOC_TYPEDEF;
	  list_to_add = &global_symbols;
	  break;
	case DW_TAG_module:
	  SYMBOL_ACLASS_INDEX (sym) = LOC_TYPEDEF;
	  SYMBOL_DOMAIN (sym) = MODULE_DOMAIN;
	  list_to_add = &global_symbols;
	  break;
	case DW_TAG_common_block:
	  SYMBOL_ACLASS_INDEX (sym) = LOC_COMMON_BLOCK;
	  SYMBOL_DOMAIN (sym) = COMMON_BLOCK_DOMAIN;
	  add_symbol_to_list (sym, cu->list_in_scope);
	  break;
	default:
	  /* Not a tag we recognize.  Hopefully we aren't processing
	     trash data, but since we must specifically ignore things
	     we don't recognize, there is nothing else we should do at
	     this point.  */
	  complaint (&symfile_complaints, _("unsupported tag: '%s'"),
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
	  && cu->language == language_cplus)
	cp_scan_for_anonymous_namespaces (sym, objfile);
    }
  return (sym);
}

/* A wrapper for new_symbol_full that always allocates a new symbol.  */

static struct symbol *
new_symbol (struct die_info *die, struct type *type, struct dwarf2_cu *cu)
{
  return new_symbol_full (die, type, cu, NULL);
}

/* Given an attr with a DW_FORM_dataN value in host byte order,
   zero-extend it as appropriate for the symbol's type.  The DWARF
   standard (v4) is not entirely clear about the meaning of using
   DW_FORM_dataN for a constant with a signed type, where the type is
   wider than the data.  The conclusion of a discussion on the DWARF
   list was that this is unspecified.  We choose to always zero-extend
   because that is the interpretation long in use by GCC.  */

static gdb_byte *
dwarf2_const_value_data (const struct attribute *attr, struct obstack *obstack,
			 struct dwarf2_cu *cu, LONGEST *value, int bits)
{
  struct objfile *objfile = cu->objfile;
  enum bfd_endian byte_order = bfd_big_endian (objfile->obfd) ?
				BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;
  LONGEST l = DW_UNSND (attr);

  if (bits < sizeof (*value) * 8)
    {
      l &= ((LONGEST) 1 << bits) - 1;
      *value = l;
    }
  else if (bits == sizeof (*value) * 8)
    *value = l;
  else
    {
      gdb_byte *bytes = (gdb_byte *) obstack_alloc (obstack, bits / 8);
      store_unsigned_integer (bytes, bits / 8, byte_order, l);
      return bytes;
    }

  return NULL;
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
  struct objfile *objfile = cu->objfile;
  struct comp_unit_head *cu_header = &cu->header;
  struct dwarf_block *blk;
  enum bfd_endian byte_order = (bfd_big_endian (objfile->obfd) ?
				BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE);

  *value = 0;
  *bytes = NULL;
  *baton = NULL;

  switch (attr->form)
    {
    case DW_FORM_addr:
    case DW_FORM_GNU_addr_index:
      {
	gdb_byte *data;

	if (TYPE_LENGTH (type) != cu_header->addr_size)
	  dwarf2_const_value_length_mismatch_complaint (name,
							cu_header->addr_size,
							TYPE_LENGTH (type));
	/* Symbols of this form are reasonably rare, so we just
	   piggyback on the existing location code rather than writing
	   a new implementation of symbol_computed_ops.  */
	*baton = XOBNEW (obstack, struct dwarf2_locexpr_baton);
	(*baton)->per_cu = cu->per_cu;
	gdb_assert ((*baton)->per_cu);

	(*baton)->size = 2 + cu_header->addr_size;
	data = (gdb_byte *) obstack_alloc (obstack, (*baton)->size);
	(*baton)->data = data;

	data[0] = DW_OP_addr;
	store_unsigned_integer (&data[1], cu_header->addr_size,
				byte_order, DW_ADDR (attr));
	data[cu_header->addr_size + 1] = DW_OP_stack_value;
      }
      break;
    case DW_FORM_string:
    case DW_FORM_strp:
    case DW_FORM_GNU_str_index:
    case DW_FORM_GNU_strp_alt:
      /* DW_STRING is already allocated on the objfile obstack, point
	 directly to it.  */
      *bytes = (const gdb_byte *) DW_STRING (attr);
      break;
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
    case DW_FORM_exprloc:
    case DW_FORM_data16:
      blk = DW_BLOCK (attr);
      if (TYPE_LENGTH (type) != blk->size)
	dwarf2_const_value_length_mismatch_complaint (name, blk->size,
						      TYPE_LENGTH (type));
      *bytes = blk->data;
      break;

      /* The DW_AT_const_value attributes are supposed to carry the
	 symbol's value "represented as it would be on the target
	 architecture."  By the time we get here, it's already been
	 converted to host endianness, so we just need to sign- or
	 zero-extend it as appropriate.  */
    case DW_FORM_data1:
      *bytes = dwarf2_const_value_data (attr, obstack, cu, value, 8);
      break;
    case DW_FORM_data2:
      *bytes = dwarf2_const_value_data (attr, obstack, cu, value, 16);
      break;
    case DW_FORM_data4:
      *bytes = dwarf2_const_value_data (attr, obstack, cu, value, 32);
      break;
    case DW_FORM_data8:
      *bytes = dwarf2_const_value_data (attr, obstack, cu, value, 64);
      break;

    case DW_FORM_sdata:
    case DW_FORM_implicit_const:
      *value = DW_SND (attr);
      break;

    case DW_FORM_udata:
      *value = DW_UNSND (attr);
      break;

    default:
      complaint (&symfile_complaints,
		 _("unsupported const value attribute form: '%s'"),
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
  struct objfile *objfile = cu->objfile;
  LONGEST value;
  const gdb_byte *bytes;
  struct dwarf2_locexpr_baton *baton;

  dwarf2_const_value_attr (attr, SYMBOL_TYPE (sym),
			   SYMBOL_PRINT_NAME (sym),
			   &objfile->objfile_obstack, cu,
			   &value, &bytes, &baton);

  if (baton != NULL)
    {
      SYMBOL_LOCATION_BATON (sym) = baton;
      SYMBOL_ACLASS_INDEX (sym) = dwarf2_locexpr_index;
    }
  else if (bytes != NULL)
     {
      SYMBOL_VALUE_BYTES (sym) = bytes;
      SYMBOL_ACLASS_INDEX (sym) = LOC_CONST_BYTES;
    }
  else
    {
      SYMBOL_VALUE (sym) = value;
      SYMBOL_ACLASS_INDEX (sym) = LOC_CONST;
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
      /* A missing DW_AT_type represents a void type.  */
      return objfile_type (cu->objfile)->builtin_void;
    }

  return lookup_die_type (die, type_attr, cu);
}

/* True iff CU's producer generates GNAT Ada auxiliary information
   that allows to find parallel types through that information instead
   of having to do expensive parallel lookups by type name.  */

static int
need_gnat_info (struct dwarf2_cu *cu)
{
  /* FIXME: brobecker/2010-10-12: As of now, only the AdaCore version
     of GNAT produces this auxiliary information, without any indication
     that it is produced.  Part of enhancing the FSF version of GNAT
     to produce that information will be to put in place an indicator
     that we can use in order to determine whether the descriptive type
     info is available or not.  One suggestion that has been made is
     to use a new attribute, attached to the CU die.  For now, assume
     that the descriptive type info is not available.  */
  return 0;
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

  type_attr = dwarf2_attr (die, DW_AT_containing_type, cu);
  if (!type_attr)
    error (_("Dwarf Error: Problem turning containing type into gdb type "
	     "[in module %s]"), objfile_name (cu->objfile));

  return lookup_die_type (die, type_attr, cu);
}

/* Return an error marker type to use for the ill formed type in DIE/CU.  */

static struct type *
build_error_marker_type (struct dwarf2_cu *cu, struct die_info *die)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  char *message, *saved;

  message = xstrprintf (_("<unknown type in %s, CU 0x%x, DIE 0x%x>"),
			objfile_name (objfile),
			to_underlying (cu->header.sect_off),
			to_underlying (die->sect_off));
  saved = (char *) obstack_copy0 (&objfile->objfile_obstack,
				  message, strlen (message));
  xfree (message);

  return init_type (objfile, TYPE_CODE_ERROR, 0, saved);
}

/* Look up the type of DIE in CU using its type attribute ATTR.
   ATTR must be one of: DW_AT_type, DW_AT_GNAT_descriptive_type,
   DW_AT_containing_type.
   If there is no type substitute an error marker.  */

static struct type *
lookup_die_type (struct die_info *die, const struct attribute *attr,
		 struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  struct type *this_type;

  gdb_assert (attr->name == DW_AT_type
	      || attr->name == DW_AT_GNAT_descriptive_type
	      || attr->name == DW_AT_containing_type);

  /* First see if we have it cached.  */

  if (attr->form == DW_FORM_GNU_ref_alt)
    {
      struct dwarf2_per_cu_data *per_cu;
      sect_offset sect_off = dwarf2_get_ref_die_offset (attr);

      per_cu = dwarf2_find_containing_comp_unit (sect_off, 1, cu->objfile);
      this_type = get_die_type_at_offset (sect_off, per_cu);
    }
  else if (attr_form_is_ref (attr))
    {
      sect_offset sect_off = dwarf2_get_ref_die_offset (attr);

      this_type = get_die_type_at_offset (sect_off, cu->per_cu);
    }
  else if (attr->form == DW_FORM_ref_sig8)
    {
      ULONGEST signature = DW_SIGNATURE (attr);

      return get_signatured_type (die, signature, cu);
    }
  else
    {
      complaint (&symfile_complaints,
		 _("Dwarf Error: Bad type attribute %s in DIE"
		   " at 0x%x [in module %s]"),
		 dwarf_attr_name (attr->name), to_underlying (die->sect_off),
		 objfile_name (objfile));
      return build_error_marker_type (cu, die);
    }

  /* If not cached we need to read it in.  */

  if (this_type == NULL)
    {
      struct die_info *type_die = NULL;
      struct dwarf2_cu *type_cu = cu;

      if (attr_form_is_ref (attr))
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
      complaint (&symfile_complaints,
		 _("unexpected tag in read_type_die: '%s'"),
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

static char *
guess_full_die_structure_name (struct die_info *die, struct dwarf2_cu *cu)
{
  struct die_info *spec_die;
  struct dwarf2_cu *spec_cu;
  struct die_info *child;

  spec_cu = cu;
  spec_die = die_specification (die, &spec_cu);
  if (spec_die != NULL)
    {
      die = spec_die;
      cu = spec_cu;
    }

  for (child = die->child;
       child != NULL;
       child = child->sibling)
    {
      if (child->tag == DW_TAG_subprogram)
	{
	  const char *linkage_name;

	  linkage_name = dwarf2_string_attr (child, DW_AT_linkage_name, cu);
	  if (linkage_name == NULL)
	    linkage_name = dwarf2_string_attr (child, DW_AT_MIPS_linkage_name,
	                                       cu);
	  if (linkage_name != NULL)
	    {
	      char *actual_name
		= language_class_name_from_physname (cu->language_defn,
						     linkage_name);
	      char *name = NULL;

	      if (actual_name != NULL)
		{
		  const char *die_name = dwarf2_name (die, cu);

		  if (die_name != NULL
		      && strcmp (die_name, actual_name) != 0)
		    {
		      /* Strip off the class name from the full name.
			 We want the prefix.  */
		      int die_name_len = strlen (die_name);
		      int actual_name_len = strlen (actual_name);

		      /* Test for '::' as a sanity check.  */
		      if (actual_name_len > die_name_len + 2
			  && actual_name[actual_name_len
					 - die_name_len - 1] == ':')
			name = (char *) obstack_copy0 (
			  &cu->objfile->per_bfd->storage_obstack,
			  actual_name, actual_name_len - die_name_len - 2);
		    }
		}
	      xfree (actual_name);
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

  attr = dwarf2_attr (die, DW_AT_linkage_name, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_MIPS_linkage_name, cu);
  if (attr == NULL || DW_STRING (attr) == NULL)
    return NULL;

  /* dwarf2_name had to be already called.  */
  gdb_assert (DW_STRING_IS_CANONICAL (attr));

  /* Strip the base name, keep any leading namespaces/classes.  */
  base = strrchr (DW_STRING (attr), ':');
  if (base == NULL || base == DW_STRING (attr) || base[-1] != ':')
    return "";

  return (char *) obstack_copy0 (&cu->objfile->per_bfd->storage_obstack,
				 DW_STRING (attr),
				 &base[-1] - DW_STRING (attr));
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
  struct die_info *parent, *spec_die;
  struct dwarf2_cu *spec_cu;
  struct type *parent_type;
  const char *retval;

  if (cu->language != language_cplus
      && cu->language != language_fortran && cu->language != language_d
      && cu->language != language_rust)
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
     the context of its DW_AT_specification, namely the contex of die
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
      complaint (&symfile_complaints,
		 _("template param type '%s' defined within parent '%s'"),
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
	if (cu->language == language_cplus
	    && strcmp (TYPE_TAG_NAME (parent_type), "::") == 0)
	  return "";
	/* We give a name to even anonymous namespaces.  */
	return TYPE_TAG_NAME (parent_type);
      case DW_TAG_class_type:
      case DW_TAG_interface_type:
      case DW_TAG_structure_type:
      case DW_TAG_union_type:
      case DW_TAG_module:
	parent_type = read_type_die (parent, cu);
	if (TYPE_TAG_NAME (parent_type) != NULL)
	  return TYPE_TAG_NAME (parent_type);
	else
	  /* An anonymous structure is only allowed non-static data
	     members; no typedefs, no member functions, et cetera.
	     So it does not need a prefix.  */
	  return "";
      case DW_TAG_compile_unit:
      case DW_TAG_partial_unit:
	/* gcc-4.5 -gdwarf-4 can drop the enclosing namespace.  Cope.  */
	if (cu->language == language_cplus
	    && !VEC_empty (dwarf2_section_info_def, dwarf2_per_objfile->types)
	    && die->child != NULL
	    && (die->tag == DW_TAG_class_type
		|| die->tag == DW_TAG_structure_type
		|| die->tag == DW_TAG_union_type))
	  {
	    char *name = guess_full_die_structure_name (die, cu);
	    if (name != NULL)
	      return name;
	  }
	return "";
      case DW_TAG_enumeration_type:
	parent_type = read_type_die (parent, cu);
	if (TYPE_DECLARED_CLASS (parent_type))
	  {
	    if (TYPE_TAG_NAME (parent_type) != NULL)
	      return TYPE_TAG_NAME (parent_type);
	    return "";
	  }
	/* Fall through.  */
      default:
	return determine_prefix (parent, cu);
      }
}

/* Return a newly-allocated string formed by concatenating PREFIX and SUFFIX
   with appropriate separator.  If PREFIX or SUFFIX is NULL or empty, then
   simply copy the SUFFIX or PREFIX, respectively.  If OBS is non-null, perform
   an obconcat, otherwise allocate storage for the result.  The CU argument is
   used to determine the language and hence, the appropriate separator.  */

#define MAX_SEP_LEN 7  /* strlen ("__") + strlen ("_MOD_")  */

static char *
typename_concat (struct obstack *obs, const char *prefix, const char *suffix,
                 int physname, struct dwarf2_cu *cu)
{
  const char *lead = "";
  const char *sep;

  if (suffix == NULL || suffix[0] == '\0'
      || prefix == NULL || prefix[0] == '\0')
    sep = "";
  else if (cu->language == language_d)
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
  else if (cu->language == language_fortran && physname)
    {
      /* This is gfortran specific mangling.  Normally DW_AT_linkage_name or
	 DW_AT_MIPS_linkage_name is preferred and used instead.  */

      lead = "__";
      sep = "_MOD_";
    }
  else
    sep = "::";

  if (prefix == NULL)
    prefix = "";
  if (suffix == NULL)
    suffix = "";

  if (obs == NULL)
    {
      char *retval
	= ((char *)
	   xmalloc (strlen (prefix) + MAX_SEP_LEN + strlen (suffix) + 1));

      strcpy (retval, lead);
      strcat (retval, prefix);
      strcat (retval, sep);
      strcat (retval, suffix);
      return retval;
    }
  else
    {
      /* We have an obstack.  */
      return obconcat (obs, lead, prefix, sep, suffix, (char *) NULL);
    }
}

/* Return sibling of die, NULL if no sibling.  */

static struct die_info *
sibling_die (struct die_info *die)
{
  return die->sibling;
}

/* Get name of a die, return NULL if not found.  */

static const char *
dwarf2_canonicalize_name (const char *name, struct dwarf2_cu *cu,
			  struct obstack *obstack)
{
  if (name && cu->language == language_cplus)
    {
      std::string canon_name = cp_canonicalize_string (name);

      if (!canon_name.empty ())
	{
	  if (canon_name != name)
	    name = (const char *) obstack_copy0 (obstack,
						 canon_name.c_str (),
						 canon_name.length ());
	}
    }

  return name;
}

/* Get name of a die, return NULL if not found.
   Anonymous namespaces are converted to their magic string.  */

static const char *
dwarf2_name (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_name, cu);
  if ((!attr || !DW_STRING (attr))
      && die->tag != DW_TAG_namespace
      && die->tag != DW_TAG_class_type
      && die->tag != DW_TAG_interface_type
      && die->tag != DW_TAG_structure_type
      && die->tag != DW_TAG_union_type)
    return NULL;

  switch (die->tag)
    {
    case DW_TAG_compile_unit:
    case DW_TAG_partial_unit:
      /* Compilation units have a DW_AT_name that is a filename, not
	 a source language identifier.  */
    case DW_TAG_enumeration_type:
    case DW_TAG_enumerator:
      /* These tags always have simple identifiers already; no need
	 to canonicalize them.  */
      return DW_STRING (attr);

    case DW_TAG_namespace:
      if (attr != NULL && DW_STRING (attr) != NULL)
	return DW_STRING (attr);
      return CP_ANONYMOUS_NAMESPACE_STR;

    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
      /* Some GCC versions emit spurious DW_AT_name attributes for unnamed
	 structures or unions.  These were of the form "._%d" in GCC 4.1,
	 or simply "<anonymous struct>" or "<anonymous union>" in GCC 4.3
	 and GCC 4.4.  We work around this problem by ignoring these.  */
      if (attr && DW_STRING (attr)
	  && (startswith (DW_STRING (attr), "._")
	      || startswith (DW_STRING (attr), "<anonymous")))
	return NULL;

      /* GCC might emit a nameless typedef that has a linkage name.  See
	 http://gcc.gnu.org/bugzilla/show_bug.cgi?id=47510.  */
      if (!attr || DW_STRING (attr) == NULL)
	{
	  char *demangled = NULL;

	  attr = dwarf2_attr (die, DW_AT_linkage_name, cu);
	  if (attr == NULL)
	    attr = dwarf2_attr (die, DW_AT_MIPS_linkage_name, cu);

	  if (attr == NULL || DW_STRING (attr) == NULL)
	    return NULL;

	  /* Avoid demangling DW_STRING (attr) the second time on a second
	     call for the same DIE.  */
	  if (!DW_STRING_IS_CANONICAL (attr))
	    demangled = gdb_demangle (DW_STRING (attr), DMGL_TYPES);

	  if (demangled)
	    {
	      const char *base;

	      /* FIXME: we already did this for the partial symbol... */
	      DW_STRING (attr)
		= ((const char *)
		   obstack_copy0 (&cu->objfile->per_bfd->storage_obstack,
				  demangled, strlen (demangled)));
	      DW_STRING_IS_CANONICAL (attr) = 1;
	      xfree (demangled);

	      /* Strip any leading namespaces/classes, keep only the base name.
		 DW_AT_name for named DIEs does not contain the prefixes.  */
	      base = strrchr (DW_STRING (attr), ':');
	      if (base && base > DW_STRING (attr) && base[-1] == ':')
		return &base[1];
	      else
		return DW_STRING (attr);
	    }
	}
      break;

    default:
      break;
    }

  if (!DW_STRING_IS_CANONICAL (attr))
    {
      DW_STRING (attr)
	= dwarf2_canonicalize_name (DW_STRING (attr), cu,
				    &cu->objfile->per_bfd->storage_obstack);
      DW_STRING_IS_CANONICAL (attr) = 1;
    }
  return DW_STRING (attr);
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

/* Convert a DIE tag into its string name.  */

static const char *
dwarf_tag_name (unsigned tag)
{
  const char *name = get_DW_TAG_name (tag);

  if (name == NULL)
    return "DW_TAG_<unknown>";

  return name;
}

/* Convert a DWARF attribute code into its string name.  */

static const char *
dwarf_attr_name (unsigned attr)
{
  const char *name;

#ifdef MIPS /* collides with DW_AT_HP_block_index */
  if (attr == DW_AT_MIPS_fde)
    return "DW_AT_MIPS_fde";
#else
  if (attr == DW_AT_HP_block_index)
    return "DW_AT_HP_block_index";
#endif

  name = get_DW_AT_name (attr);

  if (name == NULL)
    return "DW_AT_<unknown>";

  return name;
}

/* Convert a DWARF value form code into its string name.  */

static const char *
dwarf_form_name (unsigned form)
{
  const char *name = get_DW_FORM_name (form);

  if (name == NULL)
    return "DW_FORM_<unknown>";

  return name;
}

static const char *
dwarf_bool_name (unsigned mybool)
{
  if (mybool)
    return "TRUE";
  else
    return "FALSE";
}

/* Convert a DWARF type code into its string name.  */

static const char *
dwarf_type_encoding_name (unsigned enc)
{
  const char *name = get_DW_ATE_name (enc);

  if (name == NULL)
    return "DW_ATE_<unknown>";

  return name;
}

static void
dump_die_shallow (struct ui_file *f, int indent, struct die_info *die)
{
  unsigned int i;

  print_spaces (indent, f);
  fprintf_unfiltered (f, "Die: %s (abbrev %d, offset 0x%x)\n",
		      dwarf_tag_name (die->tag), die->abbrev,
		      to_underlying (die->sect_off));

  if (die->parent != NULL)
    {
      print_spaces (indent, f);
      fprintf_unfiltered (f, "  parent at offset: 0x%x\n",
			  to_underlying (die->parent->sect_off));
    }

  print_spaces (indent, f);
  fprintf_unfiltered (f, "  has children: %s\n",
	   dwarf_bool_name (die->child != NULL));

  print_spaces (indent, f);
  fprintf_unfiltered (f, "  attributes:\n");

  for (i = 0; i < die->num_attrs; ++i)
    {
      print_spaces (indent, f);
      fprintf_unfiltered (f, "    %s (%s) ",
	       dwarf_attr_name (die->attrs[i].name),
	       dwarf_form_name (die->attrs[i].form));

      switch (die->attrs[i].form)
	{
	case DW_FORM_addr:
	case DW_FORM_GNU_addr_index:
	  fprintf_unfiltered (f, "address: ");
	  fputs_filtered (hex_string (DW_ADDR (&die->attrs[i])), f);
	  break;
	case DW_FORM_block2:
	case DW_FORM_block4:
	case DW_FORM_block:
	case DW_FORM_block1:
	  fprintf_unfiltered (f, "block: size %s",
			      pulongest (DW_BLOCK (&die->attrs[i])->size));
	  break;
	case DW_FORM_exprloc:
	  fprintf_unfiltered (f, "expression: size %s",
			      pulongest (DW_BLOCK (&die->attrs[i])->size));
	  break;
	case DW_FORM_data16:
	  fprintf_unfiltered (f, "constant of 16 bytes");
	  break;
	case DW_FORM_ref_addr:
	  fprintf_unfiltered (f, "ref address: ");
	  fputs_filtered (hex_string (DW_UNSND (&die->attrs[i])), f);
	  break;
	case DW_FORM_GNU_ref_alt:
	  fprintf_unfiltered (f, "alt ref address: ");
	  fputs_filtered (hex_string (DW_UNSND (&die->attrs[i])), f);
	  break;
	case DW_FORM_ref1:
	case DW_FORM_ref2:
	case DW_FORM_ref4:
	case DW_FORM_ref8:
	case DW_FORM_ref_udata:
	  fprintf_unfiltered (f, "constant ref: 0x%lx (adjusted)",
			      (long) (DW_UNSND (&die->attrs[i])));
	  break;
	case DW_FORM_data1:
	case DW_FORM_data2:
	case DW_FORM_data4:
	case DW_FORM_data8:
	case DW_FORM_udata:
	case DW_FORM_sdata:
	  fprintf_unfiltered (f, "constant: %s",
			      pulongest (DW_UNSND (&die->attrs[i])));
	  break;
	case DW_FORM_sec_offset:
	  fprintf_unfiltered (f, "section offset: %s",
			      pulongest (DW_UNSND (&die->attrs[i])));
	  break;
	case DW_FORM_ref_sig8:
	  fprintf_unfiltered (f, "signature: %s",
			      hex_string (DW_SIGNATURE (&die->attrs[i])));
	  break;
	case DW_FORM_string:
	case DW_FORM_strp:
	case DW_FORM_line_strp:
	case DW_FORM_GNU_str_index:
	case DW_FORM_GNU_strp_alt:
	  fprintf_unfiltered (f, "string: \"%s\" (%s canonicalized)",
		   DW_STRING (&die->attrs[i])
		   ? DW_STRING (&die->attrs[i]) : "",
		   DW_STRING_IS_CANONICAL (&die->attrs[i]) ? "is" : "not");
	  break;
	case DW_FORM_flag:
	  if (DW_UNSND (&die->attrs[i]))
	    fprintf_unfiltered (f, "flag: TRUE");
	  else
	    fprintf_unfiltered (f, "flag: FALSE");
	  break;
	case DW_FORM_flag_present:
	  fprintf_unfiltered (f, "flag: TRUE");
	  break;
	case DW_FORM_indirect:
	  /* The reader will have reduced the indirect form to
	     the "base form" so this form should not occur.  */
	  fprintf_unfiltered (f, 
			      "unexpected attribute form: DW_FORM_indirect");
	  break;
	case DW_FORM_implicit_const:
	  fprintf_unfiltered (f, "constant: %s",
			      plongest (DW_SND (&die->attrs[i])));
	  break;
	default:
	  fprintf_unfiltered (f, "unsupported attribute form: %d.",
		   die->attrs[i].form);
	  break;
	}
      fprintf_unfiltered (f, "\n");
    }
}

static void
dump_die_for_error (struct die_info *die)
{
  dump_die_shallow (gdb_stderr, 0, die);
}

static void
dump_die_1 (struct ui_file *f, int level, int max_level, struct die_info *die)
{
  int indent = level * 4;

  gdb_assert (die != NULL);

  if (level >= max_level)
    return;

  dump_die_shallow (f, indent, die);

  if (die->child != NULL)
    {
      print_spaces (indent, f);
      fprintf_unfiltered (f, "  Children:");
      if (level + 1 < max_level)
	{
	  fprintf_unfiltered (f, "\n");
	  dump_die_1 (f, level + 1, max_level, die->child);
	}
      else
	{
	  fprintf_unfiltered (f,
			      " [not printed, max nesting level reached]\n");
	}
    }

  if (die->sibling != NULL && level > 0)
    {
      dump_die_1 (f, level, max_level, die->sibling);
    }
}

/* This is called from the pdie macro in gdbinit.in.
   It's not static so gcc will keep a copy callable from gdb.  */

void
dump_die (struct die_info *die, int max_level)
{
  dump_die_1 (gdb_stdlog, 0, max_level, die);
}

static void
store_in_ref_table (struct die_info *die, struct dwarf2_cu *cu)
{
  void **slot;

  slot = htab_find_slot_with_hash (cu->die_hash, die,
				   to_underlying (die->sect_off),
				   INSERT);

  *slot = die;
}

/* Return DIE offset of ATTR.  Return 0 with complaint if ATTR is not of the
   required kind.  */

static sect_offset
dwarf2_get_ref_die_offset (const struct attribute *attr)
{
  if (attr_form_is_ref (attr))
    return (sect_offset) DW_UNSND (attr);

  complaint (&symfile_complaints,
	     _("unsupported die ref attribute form: '%s'"),
	     dwarf_form_name (attr->form));
  return {};
}

/* Return the constant value held by ATTR.  Return DEFAULT_VALUE if
 * the value held by the attribute is not constant.  */

static LONGEST
dwarf2_get_attr_constant_value (const struct attribute *attr, int default_value)
{
  if (attr->form == DW_FORM_sdata || attr->form == DW_FORM_implicit_const)
    return DW_SND (attr);
  else if (attr->form == DW_FORM_udata
           || attr->form == DW_FORM_data1
           || attr->form == DW_FORM_data2
           || attr->form == DW_FORM_data4
           || attr->form == DW_FORM_data8)
    return DW_UNSND (attr);
  else
    {
      /* For DW_FORM_data16 see attr_form_is_constant.  */
      complaint (&symfile_complaints,
		 _("Attribute value is not a constant (%s)"),
                 dwarf_form_name (attr->form));
      return default_value;
    }
}

/* Follow reference or signature attribute ATTR of SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.  */

static struct die_info *
follow_die_ref_or_sig (struct die_info *src_die, const struct attribute *attr,
		       struct dwarf2_cu **ref_cu)
{
  struct die_info *die;

  if (attr_form_is_ref (attr))
    die = follow_die_ref (src_die, attr, ref_cu);
  else if (attr->form == DW_FORM_ref_sig8)
    die = follow_die_sig (src_die, attr, ref_cu);
  else
    {
      dump_die_for_error (src_die);
      error (_("Dwarf Error: Expected reference attribute [in module %s]"),
	     objfile_name ((*ref_cu)->objfile));
    }

  return die;
}

/* Follow reference OFFSET.
   On entry *REF_CU is the CU of the source die referencing OFFSET.
   On exit *REF_CU is the CU of the result.
   Returns NULL if OFFSET is invalid.  */

static struct die_info *
follow_die_offset (sect_offset sect_off, int offset_in_dwz,
		   struct dwarf2_cu **ref_cu)
{
  struct die_info temp_die;
  struct dwarf2_cu *target_cu, *cu = *ref_cu;

  gdb_assert (cu->per_cu != NULL);

  target_cu = cu;

  if (cu->per_cu->is_debug_types)
    {
      /* .debug_types CUs cannot reference anything outside their CU.
	 If they need to, they have to reference a signatured type via
	 DW_FORM_ref_sig8.  */
      if (!offset_in_cu_p (&cu->header, sect_off))
	return NULL;
    }
  else if (offset_in_dwz != cu->per_cu->is_dwz
	   || !offset_in_cu_p (&cu->header, sect_off))
    {
      struct dwarf2_per_cu_data *per_cu;

      per_cu = dwarf2_find_containing_comp_unit (sect_off, offset_in_dwz,
						 cu->objfile);

      /* If necessary, add it to the queue and load its DIEs.  */
      if (maybe_queue_comp_unit (cu, per_cu, cu->language))
	load_full_comp_unit (per_cu, cu->language);

      target_cu = per_cu->cu;
    }
  else if (cu->dies == NULL)
    {
      /* We're loading full DIEs during partial symbol reading.  */
      gdb_assert (dwarf2_per_objfile->reading_partial_symbols);
      load_full_comp_unit (cu->per_cu, language_minimal);
    }

  *ref_cu = target_cu;
  temp_die.sect_off = sect_off;
  return (struct die_info *) htab_find_with_hash (target_cu->die_hash,
						  &temp_die,
						  to_underlying (sect_off));
}

/* Follow reference attribute ATTR of SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.  */

static struct die_info *
follow_die_ref (struct die_info *src_die, const struct attribute *attr,
		struct dwarf2_cu **ref_cu)
{
  sect_offset sect_off = dwarf2_get_ref_die_offset (attr);
  struct dwarf2_cu *cu = *ref_cu;
  struct die_info *die;

  die = follow_die_offset (sect_off,
			   (attr->form == DW_FORM_GNU_ref_alt
			    || cu->per_cu->is_dwz),
			   ref_cu);
  if (!die)
    error (_("Dwarf Error: Cannot find DIE at 0x%x referenced from DIE "
	   "at 0x%x [in module %s]"),
	   to_underlying (sect_off), to_underlying (src_die->sect_off),
	   objfile_name (cu->objfile));

  return die;
}

/* Return DWARF block referenced by DW_AT_location of DIE at SECT_OFF at PER_CU.
   Returned value is intended for DW_OP_call*.  Returned
   dwarf2_locexpr_baton->data has lifetime of PER_CU->OBJFILE.  */

struct dwarf2_locexpr_baton
dwarf2_fetch_die_loc_sect_off (sect_offset sect_off,
			       struct dwarf2_per_cu_data *per_cu,
			       CORE_ADDR (*get_frame_pc) (void *baton),
			       void *baton)
{
  struct dwarf2_cu *cu;
  struct die_info *die;
  struct attribute *attr;
  struct dwarf2_locexpr_baton retval;

  dw2_setup (per_cu->objfile);

  if (per_cu->cu == NULL)
    load_cu (per_cu);
  cu = per_cu->cu;
  if (cu == NULL)
    {
      /* We shouldn't get here for a dummy CU, but don't crash on the user.
	 Instead just throw an error, not much else we can do.  */
      error (_("Dwarf Error: Dummy CU at 0x%x referenced in module %s"),
	     to_underlying (sect_off), objfile_name (per_cu->objfile));
    }

  die = follow_die_offset (sect_off, per_cu->is_dwz, &cu);
  if (!die)
    error (_("Dwarf Error: Cannot find DIE at 0x%x referenced in module %s"),
	   to_underlying (sect_off), objfile_name (per_cu->objfile));

  attr = dwarf2_attr (die, DW_AT_location, cu);
  if (!attr)
    {
      /* DWARF: "If there is no such attribute, then there is no effect.".
	 DATA is ignored if SIZE is 0.  */

      retval.data = NULL;
      retval.size = 0;
    }
  else if (attr_form_is_section_offset (attr))
    {
      struct dwarf2_loclist_baton loclist_baton;
      CORE_ADDR pc = (*get_frame_pc) (baton);
      size_t size;

      fill_in_loclist_baton (cu, &loclist_baton, attr);

      retval.data = dwarf2_find_location_expression (&loclist_baton,
						     &size, pc);
      retval.size = size;
    }
  else
    {
      if (!attr_form_is_block (attr))
	error (_("Dwarf Error: DIE at 0x%x referenced in module %s "
		 "is neither DW_FORM_block* nor DW_FORM_exprloc"),
	       to_underlying (sect_off), objfile_name (per_cu->objfile));

      retval.data = DW_BLOCK (attr)->data;
      retval.size = DW_BLOCK (attr)->size;
    }
  retval.per_cu = cu->per_cu;

  age_cached_comp_units ();

  return retval;
}

/* Like dwarf2_fetch_die_loc_sect_off, but take a CU
   offset.  */

struct dwarf2_locexpr_baton
dwarf2_fetch_die_loc_cu_off (cu_offset offset_in_cu,
			     struct dwarf2_per_cu_data *per_cu,
			     CORE_ADDR (*get_frame_pc) (void *baton),
			     void *baton)
{
  sect_offset sect_off = per_cu->sect_off + to_underlying (offset_in_cu);

  return dwarf2_fetch_die_loc_sect_off (sect_off, per_cu, get_frame_pc, baton);
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

  *len = TYPE_LENGTH (type);
  result = (gdb_byte *) obstack_alloc (obstack, *len);
  store_unsigned_integer (result, *len, byte_order, value);

  return result;
}

/* If the DIE at OFFSET in PER_CU has a DW_AT_const_value, return a
   pointer to the constant bytes and set LEN to the length of the
   data.  If memory is needed, allocate it on OBSTACK.  If the DIE
   does not have a DW_AT_const_value, return NULL.  */

const gdb_byte *
dwarf2_fetch_constant_bytes (sect_offset sect_off,
			     struct dwarf2_per_cu_data *per_cu,
			     struct obstack *obstack,
			     LONGEST *len)
{
  struct dwarf2_cu *cu;
  struct die_info *die;
  struct attribute *attr;
  const gdb_byte *result = NULL;
  struct type *type;
  LONGEST value;
  enum bfd_endian byte_order;

  dw2_setup (per_cu->objfile);

  if (per_cu->cu == NULL)
    load_cu (per_cu);
  cu = per_cu->cu;
  if (cu == NULL)
    {
      /* We shouldn't get here for a dummy CU, but don't crash on the user.
	 Instead just throw an error, not much else we can do.  */
      error (_("Dwarf Error: Dummy CU at 0x%x referenced in module %s"),
	     to_underlying (sect_off), objfile_name (per_cu->objfile));
    }

  die = follow_die_offset (sect_off, per_cu->is_dwz, &cu);
  if (!die)
    error (_("Dwarf Error: Cannot find DIE at 0x%x referenced in module %s"),
	   to_underlying (sect_off), objfile_name (per_cu->objfile));


  attr = dwarf2_attr (die, DW_AT_const_value, cu);
  if (attr == NULL)
    return NULL;

  byte_order = (bfd_big_endian (per_cu->objfile->obfd)
		? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE);

  switch (attr->form)
    {
    case DW_FORM_addr:
    case DW_FORM_GNU_addr_index:
      {
	gdb_byte *tem;

	*len = cu->header.addr_size;
	tem = (gdb_byte *) obstack_alloc (obstack, *len);
	store_unsigned_integer (tem, *len, byte_order, DW_ADDR (attr));
	result = tem;
      }
      break;
    case DW_FORM_string:
    case DW_FORM_strp:
    case DW_FORM_GNU_str_index:
    case DW_FORM_GNU_strp_alt:
      /* DW_STRING is already allocated on the objfile obstack, point
	 directly to it.  */
      result = (const gdb_byte *) DW_STRING (attr);
      *len = strlen (DW_STRING (attr));
      break;
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
    case DW_FORM_exprloc:
    case DW_FORM_data16:
      result = DW_BLOCK (attr)->data;
      *len = DW_BLOCK (attr)->size;
      break;

      /* The DW_AT_const_value attributes are supposed to carry the
	 symbol's value "represented as it would be on the target
	 architecture."  By the time we get here, it's already been
	 converted to host endianness, so we just need to sign- or
	 zero-extend it as appropriate.  */
    case DW_FORM_data1:
      type = die_type (die, cu);
      result = dwarf2_const_value_data (attr, obstack, cu, &value, 8);
      if (result == NULL)
	result = write_constant_as_bytes (obstack, byte_order,
					  type, value, len);
      break;
    case DW_FORM_data2:
      type = die_type (die, cu);
      result = dwarf2_const_value_data (attr, obstack, cu, &value, 16);
      if (result == NULL)
	result = write_constant_as_bytes (obstack, byte_order,
					  type, value, len);
      break;
    case DW_FORM_data4:
      type = die_type (die, cu);
      result = dwarf2_const_value_data (attr, obstack, cu, &value, 32);
      if (result == NULL)
	result = write_constant_as_bytes (obstack, byte_order,
					  type, value, len);
      break;
    case DW_FORM_data8:
      type = die_type (die, cu);
      result = dwarf2_const_value_data (attr, obstack, cu, &value, 64);
      if (result == NULL)
	result = write_constant_as_bytes (obstack, byte_order,
					  type, value, len);
      break;

    case DW_FORM_sdata:
    case DW_FORM_implicit_const:
      type = die_type (die, cu);
      result = write_constant_as_bytes (obstack, byte_order,
					type, DW_SND (attr), len);
      break;

    case DW_FORM_udata:
      type = die_type (die, cu);
      result = write_constant_as_bytes (obstack, byte_order,
					type, DW_UNSND (attr), len);
      break;

    default:
      complaint (&symfile_complaints,
		 _("unsupported const value attribute form: '%s'"),
		 dwarf_form_name (attr->form));
      break;
    }

  return result;
}

/* Return the type of the die at OFFSET in PER_CU.  Return NULL if no
   valid type for this die is found.  */

struct type *
dwarf2_fetch_die_type_sect_off (sect_offset sect_off,
				struct dwarf2_per_cu_data *per_cu)
{
  struct dwarf2_cu *cu;
  struct die_info *die;

  dw2_setup (per_cu->objfile);

  if (per_cu->cu == NULL)
    load_cu (per_cu);
  cu = per_cu->cu;
  if (!cu)
    return NULL;

  die = follow_die_offset (sect_off, per_cu->is_dwz, &cu);
  if (!die)
    return NULL;

  return die_type (die, cu);
}

/* Return the type of the DIE at DIE_OFFSET in the CU named by
   PER_CU.  */

struct type *
dwarf2_get_die_type (cu_offset die_offset,
		     struct dwarf2_per_cu_data *per_cu)
{
  dw2_setup (per_cu->objfile);

  sect_offset die_offset_sect = per_cu->sect_off + to_underlying (die_offset);
  return get_die_type_at_offset (die_offset_sect, per_cu);
}

/* Follow type unit SIG_TYPE referenced by SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.
   Returns NULL if the referenced DIE isn't found.  */

static struct die_info *
follow_die_sig_1 (struct die_info *src_die, struct signatured_type *sig_type,
		  struct dwarf2_cu **ref_cu)
{
  struct die_info temp_die;
  struct dwarf2_cu *sig_cu;
  struct die_info *die;

  /* While it might be nice to assert sig_type->type == NULL here,
     we can get here for DW_AT_imported_declaration where we need
     the DIE not the type.  */

  /* If necessary, add it to the queue and load its DIEs.  */

  if (maybe_queue_comp_unit (*ref_cu, &sig_type->per_cu, language_minimal))
    read_signatured_type (sig_type);

  sig_cu = sig_type->per_cu.cu;
  gdb_assert (sig_cu != NULL);
  gdb_assert (to_underlying (sig_type->type_offset_in_section) != 0);
  temp_die.sect_off = sig_type->type_offset_in_section;
  die = (struct die_info *) htab_find_with_hash (sig_cu->die_hash, &temp_die,
						 to_underlying (temp_die.sect_off));
  if (die)
    {
      /* For .gdb_index version 7 keep track of included TUs.
	 http://sourceware.org/bugzilla/show_bug.cgi?id=15021.  */
      if (dwarf2_per_objfile->index_table != NULL
	  && dwarf2_per_objfile->index_table->version <= 7)
	{
	  VEC_safe_push (dwarf2_per_cu_ptr,
			 (*ref_cu)->per_cu->imported_symtabs,
			 sig_cu->per_cu);
	}

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
  ULONGEST signature = DW_SIGNATURE (attr);
  struct signatured_type *sig_type;
  struct die_info *die;

  gdb_assert (attr->form == DW_FORM_ref_sig8);

  sig_type = lookup_signatured_type (*ref_cu, signature);
  /* sig_type will be NULL if the signatured type is missing from
     the debug info.  */
  if (sig_type == NULL)
    {
      error (_("Dwarf Error: Cannot find signatured DIE %s referenced"
               " from DIE at 0x%x [in module %s]"),
             hex_string (signature), to_underlying (src_die->sect_off),
	     objfile_name ((*ref_cu)->objfile));
    }

  die = follow_die_sig_1 (src_die, sig_type, ref_cu);
  if (die == NULL)
    {
      dump_die_for_error (src_die);
      error (_("Dwarf Error: Problem reading signatured DIE %s referenced"
	       " from DIE at 0x%x [in module %s]"),
	     hex_string (signature), to_underlying (src_die->sect_off),
	     objfile_name ((*ref_cu)->objfile));
    }

  return die;
}

/* Get the type specified by SIGNATURE referenced in DIE/CU,
   reading in and processing the type unit if necessary.  */

static struct type *
get_signatured_type (struct die_info *die, ULONGEST signature,
		     struct dwarf2_cu *cu)
{
  struct signatured_type *sig_type;
  struct dwarf2_cu *type_cu;
  struct die_info *type_die;
  struct type *type;

  sig_type = lookup_signatured_type (cu, signature);
  /* sig_type will be NULL if the signatured type is missing from
     the debug info.  */
  if (sig_type == NULL)
    {
      complaint (&symfile_complaints,
		 _("Dwarf Error: Cannot find signatured DIE %s referenced"
		   " from DIE at 0x%x [in module %s]"),
		 hex_string (signature), to_underlying (die->sect_off),
		 objfile_name (dwarf2_per_objfile->objfile));
      return build_error_marker_type (cu, die);
    }

  /* If we already know the type we're done.  */
  if (sig_type->type != NULL)
    return sig_type->type;

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
	  complaint (&symfile_complaints,
		     _("Dwarf Error: Cannot build signatured type %s"
		       " referenced from DIE at 0x%x [in module %s]"),
		     hex_string (signature), to_underlying (die->sect_off),
		     objfile_name (dwarf2_per_objfile->objfile));
	  type = build_error_marker_type (cu, die);
	}
    }
  else
    {
      complaint (&symfile_complaints,
		 _("Dwarf Error: Problem reading signatured DIE %s referenced"
		   " from DIE at 0x%x [in module %s]"),
		 hex_string (signature), to_underlying (die->sect_off),
		 objfile_name (dwarf2_per_objfile->objfile));
      type = build_error_marker_type (cu, die);
    }
  sig_type->type = type;

  return type;
}

/* Get the type specified by the DW_AT_signature ATTR in DIE/CU,
   reading in and processing the type unit if necessary.  */

static struct type *
get_DW_AT_signature_type (struct die_info *die, const struct attribute *attr,
			  struct dwarf2_cu *cu) /* ARI: editCase function */
{
  /* Yes, DW_AT_signature can use a non-ref_sig8 reference.  */
  if (attr_form_is_ref (attr))
    {
      struct dwarf2_cu *type_cu = cu;
      struct die_info *type_die = follow_die_ref (die, attr, &type_cu);

      return read_type_die (type_die, type_cu);
    }
  else if (attr->form == DW_FORM_ref_sig8)
    {
      return get_signatured_type (die, DW_SIGNATURE (attr), cu);
    }
  else
    {
      complaint (&symfile_complaints,
		 _("Dwarf Error: DW_AT_signature has bad form %s in DIE"
		   " at 0x%x [in module %s]"),
		 dwarf_form_name (attr->form), to_underlying (die->sect_off),
		 objfile_name (dwarf2_per_objfile->objfile));
      return build_error_marker_type (cu, die);
    }
}

/* Load the DIEs associated with type unit PER_CU into memory.  */

static void
load_full_type_unit (struct dwarf2_per_cu_data *per_cu)
{
  struct signatured_type *sig_type;

  /* Caller is responsible for ensuring type_unit_groups don't get here.  */
  gdb_assert (! IS_TYPE_UNIT_GROUP (per_cu));

  /* We have the per_cu, but we need the signatured_type.
     Fortunately this is an easy translation.  */
  gdb_assert (per_cu->is_debug_types);
  sig_type = (struct signatured_type *) per_cu;

  gdb_assert (per_cu->cu == NULL);

  read_signatured_type (sig_type);

  gdb_assert (per_cu->cu != NULL);
}

/* die_reader_func for read_signatured_type.
   This is identical to load_full_comp_unit_reader,
   but is kept separate for now.  */

static void
read_signatured_type_reader (const struct die_reader_specs *reader,
			     const gdb_byte *info_ptr,
			     struct die_info *comp_unit_die,
			     int has_children,
			     void *data)
{
  struct dwarf2_cu *cu = reader->cu;

  gdb_assert (cu->die_hash == NULL);
  cu->die_hash =
    htab_create_alloc_ex (cu->header.length / 12,
			  die_hash,
			  die_eq,
			  NULL,
			  &cu->comp_unit_obstack,
			  hashtab_obstack_allocate,
			  dummy_obstack_deallocate);

  if (has_children)
    comp_unit_die->child = read_die_and_siblings (reader, info_ptr,
						  &info_ptr, comp_unit_die);
  cu->dies = comp_unit_die;
  /* comp_unit_die is not stored in die_hash, no need.  */

  /* We try not to read any attributes in this function, because not
     all CUs needed for references have been loaded yet, and symbol
     table processing isn't initialized.  But we have to set the CU language,
     or we won't be able to build types correctly.
     Similarly, if we do not read the producer, we can not apply
     producer-specific interpretation.  */
  prepare_one_comp_unit (cu, cu->dies, language_minimal);
}

/* Read in a signatured type and build its CU and DIEs.
   If the type is a stub for the real type in a DWO file,
   read in the real type from the DWO file as well.  */

static void
read_signatured_type (struct signatured_type *sig_type)
{
  struct dwarf2_per_cu_data *per_cu = &sig_type->per_cu;

  gdb_assert (per_cu->is_debug_types);
  gdb_assert (per_cu->cu == NULL);

  init_cutu_and_read_dies (per_cu, NULL, 0, 1,
			   read_signatured_type_reader, NULL);
  sig_type->per_cu.tu_read = 1;
}

/* Decode simple location descriptions.
   Given a pointer to a dwarf block that defines a location, compute
   the location and return the value.

   NOTE drow/2003-11-18: This function is called in two situations
   now: for the address of static or global variables (partial symbols
   only) and for offsets into structures which are expected to be
   (more or less) constant.  The partial symbol case should go away,
   and only the constant case should remain.  That will let this
   function complain more accurately.  A few special modes are allowed
   without complaint for global variables (for instance, global
   register values and thread-local values).

   A location description containing no operations indicates that the
   object is optimized out.  The return value is 0 for that case.
   FIXME drow/2003-11-16: No callers check for this case any more; soon all
   callers will only want a very basic result and this can become a
   complaint.

   Note that stack[0] is unused except as a default error return.  */

static CORE_ADDR
decode_locdesc (struct dwarf_block *blk, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->objfile;
  size_t i;
  size_t size = blk->size;
  const gdb_byte *data = blk->data;
  CORE_ADDR stack[64];
  int stacki;
  unsigned int bytes_read, unsnd;
  gdb_byte op;

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

	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	  stack[++stacki] = op - DW_OP_reg0;
	  if (i < size)
	    dwarf2_complex_location_expr_complaint ();
	  break;

	case DW_OP_regx:
	  unsnd = read_unsigned_leb128 (NULL, (data + i), &bytes_read);
	  i += bytes_read;
	  stack[++stacki] = unsnd;
	  if (i < size)
	    dwarf2_complex_location_expr_complaint ();
	  break;

	case DW_OP_addr:
	  stack[++stacki] = read_address (objfile->obfd, &data[i],
					  cu, &bytes_read);
	  i += bytes_read;
	  break;

	case DW_OP_const1u:
	  stack[++stacki] = read_1_byte (objfile->obfd, &data[i]);
	  i += 1;
	  break;

	case DW_OP_const1s:
	  stack[++stacki] = read_1_signed_byte (objfile->obfd, &data[i]);
	  i += 1;
	  break;

	case DW_OP_const2u:
	  stack[++stacki] = read_2_bytes (objfile->obfd, &data[i]);
	  i += 2;
	  break;

	case DW_OP_const2s:
	  stack[++stacki] = read_2_signed_bytes (objfile->obfd, &data[i]);
	  i += 2;
	  break;

	case DW_OP_const4u:
	  stack[++stacki] = read_4_bytes (objfile->obfd, &data[i]);
	  i += 4;
	  break;

	case DW_OP_const4s:
	  stack[++stacki] = read_4_signed_bytes (objfile->obfd, &data[i]);
	  i += 4;
	  break;

	case DW_OP_const8u:
	  stack[++stacki] = read_8_bytes (objfile->obfd, &data[i]);
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
	     this using GDB's address_class enum.  This is valid for partial
	     global symbols, although the variable's address will be bogus
	     in the psymtab.  */
	  if (i < size)
	    dwarf2_complex_location_expr_complaint ();
	  break;

        case DW_OP_GNU_push_tls_address:
	case DW_OP_form_tls_address:
	  /* The top of the stack has the offset from the beginning
	     of the thread control block at which the variable is located.  */
	  /* Nothing should follow this operator, so the top of stack would
	     be returned.  */
	  /* This is valid for partial global symbols, but the variable's
	     address will be bogus in the psymtab.  Make it always at least
	     non-zero to not look as a variable garbage collected by linker
	     which have DW_OP_addr 0.  */
	  if (i < size)
	    dwarf2_complex_location_expr_complaint ();
	  stack[stacki]++;
          break;

	case DW_OP_GNU_uninit:
	  break;

	case DW_OP_GNU_addr_index:
	case DW_OP_GNU_const_index:
	  stack[++stacki] = read_addr_index_from_leb128 (cu, &data[i],
							 &bytes_read);
	  i += bytes_read;
	  break;

	default:
	  {
	    const char *name = get_DW_OP_name (op);

	    if (name)
	      complaint (&symfile_complaints, _("unsupported stack op: '%s'"),
			 name);
	    else
	      complaint (&symfile_complaints, _("unsupported stack op: '%02x'"),
			 op);
	  }

	  return (stack[stacki]);
	}

      /* Enforce maximum stack depth of SIZE-1 to avoid writing
         outside of the allocated space.  Also enforce minimum>0.  */
      if (stacki >= ARRAY_SIZE (stack) - 1)
	{
	  complaint (&symfile_complaints,
		     _("location description stack overflow"));
	  return 0;
	}

      if (stacki <= 0)
	{
	  complaint (&symfile_complaints,
		     _("location description stack underflow"));
	  return 0;
	}
    }
  return (stack[stacki]);
}

/* memory allocation interface */

static struct dwarf_block *
dwarf_alloc_block (struct dwarf2_cu *cu)
{
  return XOBNEW (&cu->comp_unit_obstack, struct dwarf_block);
}

static struct die_info *
dwarf_alloc_die (struct dwarf2_cu *cu, int num_attrs)
{
  struct die_info *die;
  size_t size = sizeof (struct die_info);

  if (num_attrs > 1)
    size += (num_attrs - 1) * sizeof (struct attribute);

  die = (struct die_info *) obstack_alloc (&cu->comp_unit_obstack, size);
  memset (die, 0, sizeof (struct die_info));
  return (die);
}


/* Macro support.  */

/* Return file name relative to the compilation directory of file number I in
   *LH's file name table.  The result is allocated using xmalloc; the caller is
   responsible for freeing it.  */

static char *
file_file_name (int file, struct line_header *lh)
{
  /* Is the file number a valid index into the line header's file name
     table?  Remember that file numbers start with one, not zero.  */
  if (1 <= file && file <= lh->file_names.size ())
    {
      const file_entry &fe = lh->file_names[file - 1];

      if (!IS_ABSOLUTE_PATH (fe.name))
	{
	  const char *dir = fe.include_dir (lh);
	  if (dir != NULL)
	    return concat (dir, SLASH_STRING, fe.name, (char *) NULL);
	}
      return xstrdup (fe.name);
    }
  else
    {
      /* The compiler produced a bogus file number.  We can at least
         record the macro definitions made in the file, even if we
         won't be able to find the file by name.  */
      char fake_name[80];

      xsnprintf (fake_name, sizeof (fake_name),
		 "<bad macro file number %d>", file);

      complaint (&symfile_complaints,
                 _("bad file number in macro information (%d)"),
                 file);

      return xstrdup (fake_name);
    }
}

/* Return the full name of file number I in *LH's file name table.
   Use COMP_DIR as the name of the current directory of the
   compilation.  The result is allocated using xmalloc; the caller is
   responsible for freeing it.  */
static char *
file_full_name (int file, struct line_header *lh, const char *comp_dir)
{
  /* Is the file number a valid index into the line header's file name
     table?  Remember that file numbers start with one, not zero.  */
  if (1 <= file && file <= lh->file_names.size ())
    {
      char *relative = file_file_name (file, lh);

      if (IS_ABSOLUTE_PATH (relative) || comp_dir == NULL)
	return relative;
      return reconcat (relative, comp_dir, SLASH_STRING,
		       relative, (char *) NULL);
    }
  else
    return file_file_name (file, lh);
}


static struct macro_source_file *
macro_start_file (int file, int line,
                  struct macro_source_file *current_file,
                  struct line_header *lh)
{
  /* File name relative to the compilation directory of this source file.  */
  char *file_name = file_file_name (file, lh);

  if (! current_file)
    {
      /* Note: We don't create a macro table for this compilation unit
	 at all until we actually get a filename.  */
      struct macro_table *macro_table = get_macro_table ();

      /* If we have no current file, then this must be the start_file
	 directive for the compilation unit's main source file.  */
      current_file = macro_set_main (macro_table, file_name);
      macro_define_special (macro_table);
    }
  else
    current_file = macro_include (current_file, line, file_name);

  xfree (file_name);

  return current_file;
}


/* Copy the LEN characters at BUF to a xmalloc'ed block of memory,
   followed by a null byte.  */
static char *
copy_string (const char *buf, int len)
{
  char *s = (char *) xmalloc (len + 1);

  memcpy (s, buf, len);
  s[len] = '\0';
  return s;
}


static const char *
consume_improper_spaces (const char *p, const char *body)
{
  if (*p == ' ')
    {
      complaint (&symfile_complaints,
		 _("macro definition contains spaces "
		   "in formal argument list:\n`%s'"),
		 body);

      while (*p == ' ')
        p++;
    }

  return p;
}


static void
parse_macro_definition (struct macro_source_file *file, int line,
                        const char *body)
{
  const char *p;

  /* The body string takes one of two forms.  For object-like macro
     definitions, it should be:

        <macro name> " " <definition>

     For function-like macro definitions, it should be:

        <macro name> "() " <definition>
     or
        <macro name> "(" <arg name> ( "," <arg name> ) * ") " <definition>

     Spaces may appear only where explicitly indicated, and in the
     <definition>.

     The Dwarf 2 spec says that an object-like macro's name is always
     followed by a space, but versions of GCC around March 2002 omit
     the space when the macro's definition is the empty string.

     The Dwarf 2 spec says that there should be no spaces between the
     formal arguments in a function-like macro's formal argument list,
     but versions of GCC around March 2002 include spaces after the
     commas.  */


  /* Find the extent of the macro name.  The macro name is terminated
     by either a space or null character (for an object-like macro) or
     an opening paren (for a function-like macro).  */
  for (p = body; *p; p++)
    if (*p == ' ' || *p == '(')
      break;

  if (*p == ' ' || *p == '\0')
    {
      /* It's an object-like macro.  */
      int name_len = p - body;
      char *name = copy_string (body, name_len);
      const char *replacement;

      if (*p == ' ')
        replacement = body + name_len + 1;
      else
        {
	  dwarf2_macro_malformed_definition_complaint (body);
          replacement = body + name_len;
        }

      macro_define_object (file, line, name, replacement);

      xfree (name);
    }
  else if (*p == '(')
    {
      /* It's a function-like macro.  */
      char *name = copy_string (body, p - body);
      int argc = 0;
      int argv_size = 1;
      char **argv = XNEWVEC (char *, argv_size);

      p++;

      p = consume_improper_spaces (p, body);

      /* Parse the formal argument list.  */
      while (*p && *p != ')')
        {
          /* Find the extent of the current argument name.  */
          const char *arg_start = p;

          while (*p && *p != ',' && *p != ')' && *p != ' ')
            p++;

          if (! *p || p == arg_start)
	    dwarf2_macro_malformed_definition_complaint (body);
          else
            {
              /* Make sure argv has room for the new argument.  */
              if (argc >= argv_size)
                {
                  argv_size *= 2;
                  argv = XRESIZEVEC (char *, argv, argv_size);
                }

              argv[argc++] = copy_string (arg_start, p - arg_start);
            }

          p = consume_improper_spaces (p, body);

          /* Consume the comma, if present.  */
          if (*p == ',')
            {
              p++;

              p = consume_improper_spaces (p, body);
            }
        }

      if (*p == ')')
        {
          p++;

          if (*p == ' ')
            /* Perfectly formed definition, no complaints.  */
            macro_define_function (file, line, name,
                                   argc, (const char **) argv,
                                   p + 1);
          else if (*p == '\0')
            {
              /* Complain, but do define it.  */
	      dwarf2_macro_malformed_definition_complaint (body);
              macro_define_function (file, line, name,
                                     argc, (const char **) argv,
                                     p);
            }
          else
            /* Just complain.  */
	    dwarf2_macro_malformed_definition_complaint (body);
        }
      else
        /* Just complain.  */
	dwarf2_macro_malformed_definition_complaint (body);

      xfree (name);
      {
        int i;

        for (i = 0; i < argc; i++)
          xfree (argv[i]);
      }
      xfree (argv);
    }
  else
    dwarf2_macro_malformed_definition_complaint (body);
}

/* Skip some bytes from BYTES according to the form given in FORM.
   Returns the new pointer.  */

static const gdb_byte *
skip_form_bytes (bfd *abfd, const gdb_byte *bytes, const gdb_byte *buffer_end,
		 enum dwarf_form form,
		 unsigned int offset_size,
		 struct dwarf2_section_info *section)
{
  unsigned int bytes_read;

  switch (form)
    {
    case DW_FORM_data1:
    case DW_FORM_flag:
      ++bytes;
      break;

    case DW_FORM_data2:
      bytes += 2;
      break;

    case DW_FORM_data4:
      bytes += 4;
      break;

    case DW_FORM_data8:
      bytes += 8;
      break;

    case DW_FORM_data16:
      bytes += 16;
      break;

    case DW_FORM_string:
      read_direct_string (abfd, bytes, &bytes_read);
      bytes += bytes_read;
      break;

    case DW_FORM_sec_offset:
    case DW_FORM_strp:
    case DW_FORM_GNU_strp_alt:
      bytes += offset_size;
      break;

    case DW_FORM_block:
      bytes += read_unsigned_leb128 (abfd, bytes, &bytes_read);
      bytes += bytes_read;
      break;

    case DW_FORM_block1:
      bytes += 1 + read_1_byte (abfd, bytes);
      break;
    case DW_FORM_block2:
      bytes += 2 + read_2_bytes (abfd, bytes);
      break;
    case DW_FORM_block4:
      bytes += 4 + read_4_bytes (abfd, bytes);
      break;

    case DW_FORM_sdata:
    case DW_FORM_udata:
    case DW_FORM_GNU_addr_index:
    case DW_FORM_GNU_str_index:
      bytes = gdb_skip_leb128 (bytes, buffer_end);
      if (bytes == NULL)
	{
	  dwarf2_section_buffer_overflow_complaint (section);
	  return NULL;
	}
      break;

    case DW_FORM_implicit_const:
      break;

    default:
      {
      complain:
	complaint (&symfile_complaints,
		   _("invalid form 0x%x in `%s'"),
		   form, get_section_name (section));
	return NULL;
      }
    }

  return bytes;
}

/* A helper for dwarf_decode_macros that handles skipping an unknown
   opcode.  Returns an updated pointer to the macro data buffer; or,
   on error, issues a complaint and returns NULL.  */

static const gdb_byte *
skip_unknown_opcode (unsigned int opcode,
		     const gdb_byte **opcode_definitions,
		     const gdb_byte *mac_ptr, const gdb_byte *mac_end,
		     bfd *abfd,
		     unsigned int offset_size,
		     struct dwarf2_section_info *section)
{
  unsigned int bytes_read, i;
  unsigned long arg;
  const gdb_byte *defn;

  if (opcode_definitions[opcode] == NULL)
    {
      complaint (&symfile_complaints,
		 _("unrecognized DW_MACFINO opcode 0x%x"),
		 opcode);
      return NULL;
    }

  defn = opcode_definitions[opcode];
  arg = read_unsigned_leb128 (abfd, defn, &bytes_read);
  defn += bytes_read;

  for (i = 0; i < arg; ++i)
    {
      mac_ptr = skip_form_bytes (abfd, mac_ptr, mac_end,
				 (enum dwarf_form) defn[i], offset_size,
				 section);
      if (mac_ptr == NULL)
	{
	  /* skip_form_bytes already issued the complaint.  */
	  return NULL;
	}
    }

  return mac_ptr;
}

/* A helper function which parses the header of a macro section.
   If the macro section is the extended (for now called "GNU") type,
   then this updates *OFFSET_SIZE.  Returns a pointer to just after
   the header, or issues a complaint and returns NULL on error.  */

static const gdb_byte *
dwarf_parse_macro_header (const gdb_byte **opcode_definitions,
			  bfd *abfd,
			  const gdb_byte *mac_ptr,
			  unsigned int *offset_size,
			  int section_is_gnu)
{
  memset (opcode_definitions, 0, 256 * sizeof (gdb_byte *));

  if (section_is_gnu)
    {
      unsigned int version, flags;

      version = read_2_bytes (abfd, mac_ptr);
      if (version != 4 && version != 5)
	{
	  complaint (&symfile_complaints,
		     _("unrecognized version `%d' in .debug_macro section"),
		     version);
	  return NULL;
	}
      mac_ptr += 2;

      flags = read_1_byte (abfd, mac_ptr);
      ++mac_ptr;
      *offset_size = (flags & 1) ? 8 : 4;

      if ((flags & 2) != 0)
	/* We don't need the line table offset.  */
	mac_ptr += *offset_size;

      /* Vendor opcode descriptions.  */
      if ((flags & 4) != 0)
	{
	  unsigned int i, count;

	  count = read_1_byte (abfd, mac_ptr);
	  ++mac_ptr;
	  for (i = 0; i < count; ++i)
	    {
	      unsigned int opcode, bytes_read;
	      unsigned long arg;

	      opcode = read_1_byte (abfd, mac_ptr);
	      ++mac_ptr;
	      opcode_definitions[opcode] = mac_ptr;
	      arg = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;
	      mac_ptr += arg;
	    }
	}
    }

  return mac_ptr;
}

/* A helper for dwarf_decode_macros that handles the GNU extensions,
   including DW_MACRO_import.  */

static void
dwarf_decode_macro_bytes (bfd *abfd,
			  const gdb_byte *mac_ptr, const gdb_byte *mac_end,
			  struct macro_source_file *current_file,
			  struct line_header *lh,
			  struct dwarf2_section_info *section,
			  int section_is_gnu, int section_is_dwz,
			  unsigned int offset_size,
			  htab_t include_hash)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  enum dwarf_macro_record_type macinfo_type;
  int at_commandline;
  const gdb_byte *opcode_definitions[256];

  mac_ptr = dwarf_parse_macro_header (opcode_definitions, abfd, mac_ptr,
				      &offset_size, section_is_gnu);
  if (mac_ptr == NULL)
    {
      /* We already issued a complaint.  */
      return;
    }

  /* Determines if GDB is still before first DW_MACINFO_start_file.  If true
     GDB is still reading the definitions from command line.  First
     DW_MACINFO_start_file will need to be ignored as it was already executed
     to create CURRENT_FILE for the main source holding also the command line
     definitions.  On first met DW_MACINFO_start_file this flag is reset to
     normally execute all the remaining DW_MACINFO_start_file macinfos.  */

  at_commandline = 1;

  do
    {
      /* Do we at least have room for a macinfo type byte?  */
      if (mac_ptr >= mac_end)
	{
	  dwarf2_section_buffer_overflow_complaint (section);
	  break;
	}

      macinfo_type = (enum dwarf_macro_record_type) read_1_byte (abfd, mac_ptr);
      mac_ptr++;

      /* Note that we rely on the fact that the corresponding GNU and
	 DWARF constants are the same.  */
      switch (macinfo_type)
	{
	  /* A zero macinfo type indicates the end of the macro
	     information.  */
	case 0:
	  break;

        case DW_MACRO_define:
        case DW_MACRO_undef:
	case DW_MACRO_define_strp:
	case DW_MACRO_undef_strp:
	case DW_MACRO_define_sup:
	case DW_MACRO_undef_sup:
          {
            unsigned int bytes_read;
            int line;
            const char *body;
	    int is_define;

	    line = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;

	    if (macinfo_type == DW_MACRO_define
		|| macinfo_type == DW_MACRO_undef)
	      {
		body = read_direct_string (abfd, mac_ptr, &bytes_read);
		mac_ptr += bytes_read;
	      }
	    else
	      {
		LONGEST str_offset;

		str_offset = read_offset_1 (abfd, mac_ptr, offset_size);
		mac_ptr += offset_size;

		if (macinfo_type == DW_MACRO_define_sup
		    || macinfo_type == DW_MACRO_undef_sup
		    || section_is_dwz)
		  {
		    struct dwz_file *dwz = dwarf2_get_dwz_file ();

		    body = read_indirect_string_from_dwz (dwz, str_offset);
		  }
		else
		  body = read_indirect_string_at_offset (abfd, str_offset);
	      }

	    is_define = (macinfo_type == DW_MACRO_define
			 || macinfo_type == DW_MACRO_define_strp
			 || macinfo_type == DW_MACRO_define_sup);
            if (! current_file)
	      {
		/* DWARF violation as no main source is present.  */
		complaint (&symfile_complaints,
			   _("debug info with no main source gives macro %s "
			     "on line %d: %s"),
			   is_define ? _("definition") : _("undefinition"),
			   line, body);
		break;
	      }
	    if ((line == 0 && !at_commandline)
		|| (line != 0 && at_commandline))
	      complaint (&symfile_complaints,
			 _("debug info gives %s macro %s with %s line %d: %s"),
			 at_commandline ? _("command-line") : _("in-file"),
			 is_define ? _("definition") : _("undefinition"),
			 line == 0 ? _("zero") : _("non-zero"), line, body);

	    if (is_define)
	      parse_macro_definition (current_file, line, body);
	    else
	      {
		gdb_assert (macinfo_type == DW_MACRO_undef
			    || macinfo_type == DW_MACRO_undef_strp
			    || macinfo_type == DW_MACRO_undef_sup);
		macro_undef (current_file, line, body);
	      }
          }
          break;

        case DW_MACRO_start_file:
          {
            unsigned int bytes_read;
            int line, file;

            line = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
            mac_ptr += bytes_read;
            file = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
            mac_ptr += bytes_read;

	    if ((line == 0 && !at_commandline)
		|| (line != 0 && at_commandline))
	      complaint (&symfile_complaints,
			 _("debug info gives source %d included "
			   "from %s at %s line %d"),
			 file, at_commandline ? _("command-line") : _("file"),
			 line == 0 ? _("zero") : _("non-zero"), line);

	    if (at_commandline)
	      {
		/* This DW_MACRO_start_file was executed in the
		   pass one.  */
		at_commandline = 0;
	      }
	    else
	      current_file = macro_start_file (file, line, current_file, lh);
          }
          break;

        case DW_MACRO_end_file:
          if (! current_file)
	    complaint (&symfile_complaints,
		       _("macro debug info has an unmatched "
			 "`close_file' directive"));
          else
            {
              current_file = current_file->included_by;
              if (! current_file)
                {
                  enum dwarf_macro_record_type next_type;

                  /* GCC circa March 2002 doesn't produce the zero
                     type byte marking the end of the compilation
                     unit.  Complain if it's not there, but exit no
                     matter what.  */

                  /* Do we at least have room for a macinfo type byte?  */
                  if (mac_ptr >= mac_end)
                    {
		      dwarf2_section_buffer_overflow_complaint (section);
                      return;
                    }

                  /* We don't increment mac_ptr here, so this is just
                     a look-ahead.  */
                  next_type
		    = (enum dwarf_macro_record_type) read_1_byte (abfd,
								  mac_ptr);
                  if (next_type != 0)
		    complaint (&symfile_complaints,
			       _("no terminating 0-type entry for "
				 "macros in `.debug_macinfo' section"));

                  return;
                }
            }
          break;

	case DW_MACRO_import:
	case DW_MACRO_import_sup:
	  {
	    LONGEST offset;
	    void **slot;
	    bfd *include_bfd = abfd;
	    struct dwarf2_section_info *include_section = section;
	    const gdb_byte *include_mac_end = mac_end;
	    int is_dwz = section_is_dwz;
	    const gdb_byte *new_mac_ptr;

	    offset = read_offset_1 (abfd, mac_ptr, offset_size);
	    mac_ptr += offset_size;

	    if (macinfo_type == DW_MACRO_import_sup)
	      {
		struct dwz_file *dwz = dwarf2_get_dwz_file ();

		dwarf2_read_section (objfile, &dwz->macro);

		include_section = &dwz->macro;
		include_bfd = get_section_bfd_owner (include_section);
		include_mac_end = dwz->macro.buffer + dwz->macro.size;
		is_dwz = 1;
	      }

	    new_mac_ptr = include_section->buffer + offset;
	    slot = htab_find_slot (include_hash, new_mac_ptr, INSERT);

	    if (*slot != NULL)
	      {
		/* This has actually happened; see
		   http://sourceware.org/bugzilla/show_bug.cgi?id=13568.  */
		complaint (&symfile_complaints,
			   _("recursive DW_MACRO_import in "
			     ".debug_macro section"));
	      }
	    else
	      {
		*slot = (void *) new_mac_ptr;

		dwarf_decode_macro_bytes (include_bfd, new_mac_ptr,
					  include_mac_end, current_file, lh,
					  section, section_is_gnu, is_dwz,
					  offset_size, include_hash);

		htab_remove_elt (include_hash, (void *) new_mac_ptr);
	      }
	  }
	  break;

        case (int)DW_MACINFO_vendor_ext:
	  if (!section_is_gnu)
	    {
	      unsigned int bytes_read;

	      /* This reads the constant, but since we don't recognize
		 any vendor extensions, we ignore it.  */
	      read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;
	      read_direct_string (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;

	      /* We don't recognize any vendor extensions.  */
	      break;
	    }
	  /* FALLTHROUGH */

	default:
	  mac_ptr = skip_unknown_opcode (macinfo_type, opcode_definitions,
					 mac_ptr, mac_end, abfd, offset_size,
					 section);
	  if (mac_ptr == NULL)
	    return;
	  break;
        }
    } while (macinfo_type != 0);
}

static void
dwarf_decode_macros (struct dwarf2_cu *cu, unsigned int offset,
                     int section_is_gnu)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct line_header *lh = cu->line_header;
  bfd *abfd;
  const gdb_byte *mac_ptr, *mac_end;
  struct macro_source_file *current_file = 0;
  enum dwarf_macro_record_type macinfo_type;
  unsigned int offset_size = cu->header.offset_size;
  const gdb_byte *opcode_definitions[256];
  struct cleanup *cleanup;
  void **slot;
  struct dwarf2_section_info *section;
  const char *section_name;

  if (cu->dwo_unit != NULL)
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
	  section = &dwarf2_per_objfile->macro;
	  section_name = ".debug_macro";
	}
      else
	{
	  section = &dwarf2_per_objfile->macinfo;
	  section_name = ".debug_macinfo";
	}
    }

  dwarf2_read_section (objfile, section);
  if (section->buffer == NULL)
    {
      complaint (&symfile_complaints, _("missing %s section"), section_name);
      return;
    }
  abfd = get_section_bfd_owner (section);

  /* First pass: Find the name of the base filename.
     This filename is needed in order to process all macros whose definition
     (or undefinition) comes from the command line.  These macros are defined
     before the first DW_MACINFO_start_file entry, and yet still need to be
     associated to the base file.

     To determine the base file name, we scan the macro definitions until we
     reach the first DW_MACINFO_start_file entry.  We then initialize
     CURRENT_FILE accordingly so that any macro definition found before the
     first DW_MACINFO_start_file can still be associated to the base file.  */

  mac_ptr = section->buffer + offset;
  mac_end = section->buffer + section->size;

  mac_ptr = dwarf_parse_macro_header (opcode_definitions, abfd, mac_ptr,
				      &offset_size, section_is_gnu);
  if (mac_ptr == NULL)
    {
      /* We already issued a complaint.  */
      return;
    }

  do
    {
      /* Do we at least have room for a macinfo type byte?  */
      if (mac_ptr >= mac_end)
        {
	  /* Complaint is printed during the second pass as GDB will probably
	     stop the first pass earlier upon finding
	     DW_MACINFO_start_file.  */
	  break;
        }

      macinfo_type = (enum dwarf_macro_record_type) read_1_byte (abfd, mac_ptr);
      mac_ptr++;

      /* Note that we rely on the fact that the corresponding GNU and
	 DWARF constants are the same.  */
      switch (macinfo_type)
        {
          /* A zero macinfo type indicates the end of the macro
             information.  */
        case 0:
	  break;

	case DW_MACRO_define:
	case DW_MACRO_undef:
	  /* Only skip the data by MAC_PTR.  */
	  {
	    unsigned int bytes_read;

	    read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	    read_direct_string (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	  }
	  break;

	case DW_MACRO_start_file:
	  {
	    unsigned int bytes_read;
	    int line, file;

	    line = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	    file = read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;

	    current_file = macro_start_file (file, line, current_file, lh);
	  }
	  break;

	case DW_MACRO_end_file:
	  /* No data to skip by MAC_PTR.  */
	  break;

	case DW_MACRO_define_strp:
	case DW_MACRO_undef_strp:
	case DW_MACRO_define_sup:
	case DW_MACRO_undef_sup:
	  {
	    unsigned int bytes_read;

	    read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	    mac_ptr += bytes_read;
	    mac_ptr += offset_size;
	  }
	  break;

	case DW_MACRO_import:
	case DW_MACRO_import_sup:
	  /* Note that, according to the spec, a transparent include
	     chain cannot call DW_MACRO_start_file.  So, we can just
	     skip this opcode.  */
	  mac_ptr += offset_size;
	  break;

	case (int)DW_MACINFO_vendor_ext:
	  /* Only skip the data by MAC_PTR.  */
	  if (!section_is_gnu)
	    {
	      unsigned int bytes_read;

	      read_unsigned_leb128 (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;
	      read_direct_string (abfd, mac_ptr, &bytes_read);
	      mac_ptr += bytes_read;
	    }
	  /* FALLTHROUGH */

	default:
	  mac_ptr = skip_unknown_opcode (macinfo_type, opcode_definitions,
					 mac_ptr, mac_end, abfd, offset_size,
					 section);
	  if (mac_ptr == NULL)
	    return;
	  break;
	}
    } while (macinfo_type != 0 && current_file == NULL);

  /* Second pass: Process all entries.

     Use the AT_COMMAND_LINE flag to determine whether we are still processing
     command-line macro definitions/undefinitions.  This flag is unset when we
     reach the first DW_MACINFO_start_file entry.  */

  htab_up include_hash (htab_create_alloc (1, htab_hash_pointer,
					   htab_eq_pointer,
					   NULL, xcalloc, xfree));
  mac_ptr = section->buffer + offset;
  slot = htab_find_slot (include_hash.get (), mac_ptr, INSERT);
  *slot = (void *) mac_ptr;
  dwarf_decode_macro_bytes (abfd, mac_ptr, mac_end,
			    current_file, lh, section,
			    section_is_gnu, 0, offset_size,
			    include_hash.get ());
}

/* Check if the attribute's form is a DW_FORM_block*
   if so return true else false.  */

static int
attr_form_is_block (const struct attribute *attr)
{
  return (attr == NULL ? 0 :
      attr->form == DW_FORM_block1
      || attr->form == DW_FORM_block2
      || attr->form == DW_FORM_block4
      || attr->form == DW_FORM_block
      || attr->form == DW_FORM_exprloc);
}

/* Return non-zero if ATTR's value is a section offset --- classes
   lineptr, loclistptr, macptr or rangelistptr --- or zero, otherwise.
   You may use DW_UNSND (attr) to retrieve such offsets.

   Section 7.5.4, "Attribute Encodings", explains that no attribute
   may have a value that belongs to more than one of these classes; it
   would be ambiguous if we did, because we use the same forms for all
   of them.  */

static int
attr_form_is_section_offset (const struct attribute *attr)
{
  return (attr->form == DW_FORM_data4
          || attr->form == DW_FORM_data8
	  || attr->form == DW_FORM_sec_offset);
}

/* Return non-zero if ATTR's value falls in the 'constant' class, or
   zero otherwise.  When this function returns true, you can apply
   dwarf2_get_attr_constant_value to it.

   However, note that for some attributes you must check
   attr_form_is_section_offset before using this test.  DW_FORM_data4
   and DW_FORM_data8 are members of both the constant class, and of
   the classes that contain offsets into other debug sections
   (lineptr, loclistptr, macptr or rangelistptr).  The DWARF spec says
   that, if an attribute's can be either a constant or one of the
   section offset classes, DW_FORM_data4 and DW_FORM_data8 should be
   taken as section offsets, not constants.

   DW_FORM_data16 is not considered as dwarf2_get_attr_constant_value
   cannot handle that.  */

static int
attr_form_is_constant (const struct attribute *attr)
{
  switch (attr->form)
    {
    case DW_FORM_sdata:
    case DW_FORM_udata:
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_implicit_const:
      return 1;
    default:
      return 0;
    }
}


/* DW_ADDR is always stored already as sect_offset; despite for the forms
   besides DW_FORM_ref_addr it is stored as cu_offset in the DWARF file.  */

static int
attr_form_is_ref (const struct attribute *attr)
{
  switch (attr->form)
    {
    case DW_FORM_ref_addr:
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref8:
    case DW_FORM_ref_udata:
    case DW_FORM_GNU_ref_alt:
      return 1;
    default:
      return 0;
    }
}

/* Return the .debug_loc section to use for CU.
   For DWO files use .debug_loc.dwo.  */

static struct dwarf2_section_info *
cu_debug_loc_section (struct dwarf2_cu *cu)
{
  if (cu->dwo_unit)
    {
      struct dwo_sections *sections = &cu->dwo_unit->dwo_file->sections;
      
      return cu->header.version >= 5 ? &sections->loclists : &sections->loc;
    }
  return (cu->header.version >= 5 ? &dwarf2_per_objfile->loclists
				  : &dwarf2_per_objfile->loc);
}

/* A helper function that fills in a dwarf2_loclist_baton.  */

static void
fill_in_loclist_baton (struct dwarf2_cu *cu,
		       struct dwarf2_loclist_baton *baton,
		       const struct attribute *attr)
{
  struct dwarf2_section_info *section = cu_debug_loc_section (cu);

  dwarf2_read_section (dwarf2_per_objfile->objfile, section);

  baton->per_cu = cu->per_cu;
  gdb_assert (baton->per_cu);
  /* We don't know how long the location list is, but make sure we
     don't run off the edge of the section.  */
  baton->size = section->size - DW_UNSND (attr);
  baton->data = section->buffer + DW_UNSND (attr);
  baton->base_address = cu->base_address;
  baton->from_dwo = cu->dwo_unit != NULL;
}

static void
dwarf2_symbol_mark_computed (const struct attribute *attr, struct symbol *sym,
			     struct dwarf2_cu *cu, int is_block)
{
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  struct dwarf2_section_info *section = cu_debug_loc_section (cu);

  if (attr_form_is_section_offset (attr)
      /* .debug_loc{,.dwo} may not exist at all, or the offset may be outside
	 the section.  If so, fall through to the complaint in the
	 other branch.  */
      && DW_UNSND (attr) < dwarf2_section_size (objfile, section))
    {
      struct dwarf2_loclist_baton *baton;

      baton = XOBNEW (&objfile->objfile_obstack, struct dwarf2_loclist_baton);

      fill_in_loclist_baton (cu, baton, attr);

      if (cu->base_known == 0)
	complaint (&symfile_complaints,
		   _("Location list used without "
		     "specifying the CU base address."));

      SYMBOL_ACLASS_INDEX (sym) = (is_block
				   ? dwarf2_loclist_block_index
				   : dwarf2_loclist_index);
      SYMBOL_LOCATION_BATON (sym) = baton;
    }
  else
    {
      struct dwarf2_locexpr_baton *baton;

      baton = XOBNEW (&objfile->objfile_obstack, struct dwarf2_locexpr_baton);
      baton->per_cu = cu->per_cu;
      gdb_assert (baton->per_cu);

      if (attr_form_is_block (attr))
	{
	  /* Note that we're just copying the block's data pointer
	     here, not the actual data.  We're still pointing into the
	     info_buffer for SYM's objfile; right now we never release
	     that buffer, but when we do clean up properly this may
	     need to change.  */
	  baton->size = DW_BLOCK (attr)->size;
	  baton->data = DW_BLOCK (attr)->data;
	}
      else
	{
	  dwarf2_invalid_attrib_class_complaint ("location description",
						 SYMBOL_NATURAL_NAME (sym));
	  baton->size = 0;
	}

      SYMBOL_ACLASS_INDEX (sym) = (is_block
				   ? dwarf2_locexpr_block_index
				   : dwarf2_locexpr_index);
      SYMBOL_LOCATION_BATON (sym) = baton;
    }
}

/* Return the OBJFILE associated with the compilation unit CU.  If CU
   came from a separate debuginfo file, then the master objfile is
   returned.  */

struct objfile *
dwarf2_per_cu_objfile (struct dwarf2_per_cu_data *per_cu)
{
  struct objfile *objfile = per_cu->objfile;

  /* Return the master objfile, so that we can report and look up the
     correct file containing this variable.  */
  if (objfile->separate_debug_objfile_backlink)
    objfile = objfile->separate_debug_objfile_backlink;

  return objfile;
}

/* Return comp_unit_head for PER_CU, either already available in PER_CU->CU
   (CU_HEADERP is unused in such case) or prepare a temporary copy at
   CU_HEADERP first.  */

static const struct comp_unit_head *
per_cu_header_read_in (struct comp_unit_head *cu_headerp,
		       struct dwarf2_per_cu_data *per_cu)
{
  const gdb_byte *info_ptr;

  if (per_cu->cu)
    return &per_cu->cu->header;

  info_ptr = per_cu->section->buffer + to_underlying (per_cu->sect_off);

  memset (cu_headerp, 0, sizeof (*cu_headerp));
  read_comp_unit_head (cu_headerp, info_ptr, per_cu->section,
		       rcuh_kind::COMPILE);

  return cu_headerp;
}

/* Return the address size given in the compilation unit header for CU.  */

int
dwarf2_per_cu_addr_size (struct dwarf2_per_cu_data *per_cu)
{
  struct comp_unit_head cu_header_local;
  const struct comp_unit_head *cu_headerp;

  cu_headerp = per_cu_header_read_in (&cu_header_local, per_cu);

  return cu_headerp->addr_size;
}

/* Return the offset size given in the compilation unit header for CU.  */

int
dwarf2_per_cu_offset_size (struct dwarf2_per_cu_data *per_cu)
{
  struct comp_unit_head cu_header_local;
  const struct comp_unit_head *cu_headerp;

  cu_headerp = per_cu_header_read_in (&cu_header_local, per_cu);

  return cu_headerp->offset_size;
}

/* See its dwarf2loc.h declaration.  */

int
dwarf2_per_cu_ref_addr_size (struct dwarf2_per_cu_data *per_cu)
{
  struct comp_unit_head cu_header_local;
  const struct comp_unit_head *cu_headerp;

  cu_headerp = per_cu_header_read_in (&cu_header_local, per_cu);

  if (cu_headerp->version == 2)
    return cu_headerp->addr_size;
  else
    return cu_headerp->offset_size;
}

/* Return the text offset of the CU.  The returned offset comes from
   this CU's objfile.  If this objfile came from a separate debuginfo
   file, then the offset may be different from the corresponding
   offset in the parent objfile.  */

CORE_ADDR
dwarf2_per_cu_text_offset (struct dwarf2_per_cu_data *per_cu)
{
  struct objfile *objfile = per_cu->objfile;

  return ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));
}

/* Return DWARF version number of PER_CU.  */

short
dwarf2_version (struct dwarf2_per_cu_data *per_cu)
{
  return per_cu->dwarf_version;
}

/* Locate the .debug_info compilation unit from CU's objfile which contains
   the DIE at OFFSET.  Raises an error on failure.  */

static struct dwarf2_per_cu_data *
dwarf2_find_containing_comp_unit (sect_offset sect_off,
				  unsigned int offset_in_dwz,
				  struct objfile *objfile)
{
  struct dwarf2_per_cu_data *this_cu;
  int low, high;
  const sect_offset *cu_off;

  low = 0;
  high = dwarf2_per_objfile->n_comp_units - 1;
  while (high > low)
    {
      struct dwarf2_per_cu_data *mid_cu;
      int mid = low + (high - low) / 2;

      mid_cu = dwarf2_per_objfile->all_comp_units[mid];
      cu_off = &mid_cu->sect_off;
      if (mid_cu->is_dwz > offset_in_dwz
	  || (mid_cu->is_dwz == offset_in_dwz && *cu_off >= sect_off))
	high = mid;
      else
	low = mid + 1;
    }
  gdb_assert (low == high);
  this_cu = dwarf2_per_objfile->all_comp_units[low];
  cu_off = &this_cu->sect_off;
  if (this_cu->is_dwz != offset_in_dwz || *cu_off > sect_off)
    {
      if (low == 0 || this_cu->is_dwz != offset_in_dwz)
	error (_("Dwarf Error: could not find partial DIE containing "
	       "offset 0x%x [in module %s]"),
	       to_underlying (sect_off), bfd_get_filename (objfile->obfd));

      gdb_assert (dwarf2_per_objfile->all_comp_units[low-1]->sect_off
		  <= sect_off);
      return dwarf2_per_objfile->all_comp_units[low-1];
    }
  else
    {
      this_cu = dwarf2_per_objfile->all_comp_units[low];
      if (low == dwarf2_per_objfile->n_comp_units - 1
	  && sect_off >= this_cu->sect_off + this_cu->length)
	error (_("invalid dwarf2 offset %u"), to_underlying (sect_off));
      gdb_assert (sect_off < this_cu->sect_off + this_cu->length);
      return this_cu;
    }
}

/* Initialize dwarf2_cu CU, owned by PER_CU.  */

static void
init_one_comp_unit (struct dwarf2_cu *cu, struct dwarf2_per_cu_data *per_cu)
{
  memset (cu, 0, sizeof (*cu));
  per_cu->cu = cu;
  cu->per_cu = per_cu;
  cu->objfile = per_cu->objfile;
  obstack_init (&cu->comp_unit_obstack);
}

/* Initialize basic fields of dwarf_cu CU according to DIE COMP_UNIT_DIE.  */

static void
prepare_one_comp_unit (struct dwarf2_cu *cu, struct die_info *comp_unit_die,
		       enum language pretend_language)
{
  struct attribute *attr;

  /* Set the language we're debugging.  */
  attr = dwarf2_attr (comp_unit_die, DW_AT_language, cu);
  if (attr)
    set_cu_language (DW_UNSND (attr), cu);
  else
    {
      cu->language = pretend_language;
      cu->language_defn = language_def (cu->language);
    }

  cu->producer = dwarf2_string_attr (comp_unit_die, DW_AT_producer, cu);
}

/* Release one cached compilation unit, CU.  We unlink it from the tree
   of compilation units, but we don't remove it from the read_in_chain;
   the caller is responsible for that.
   NOTE: DATA is a void * because this function is also used as a
   cleanup routine.  */

static void
free_heap_comp_unit (void *data)
{
  struct dwarf2_cu *cu = (struct dwarf2_cu *) data;

  gdb_assert (cu->per_cu != NULL);
  cu->per_cu->cu = NULL;
  cu->per_cu = NULL;

  obstack_free (&cu->comp_unit_obstack, NULL);

  xfree (cu);
}

/* This cleanup function is passed the address of a dwarf2_cu on the stack
   when we're finished with it.  We can't free the pointer itself, but be
   sure to unlink it from the cache.  Also release any associated storage.  */

static void
free_stack_comp_unit (void *data)
{
  struct dwarf2_cu *cu = (struct dwarf2_cu *) data;

  gdb_assert (cu->per_cu != NULL);
  cu->per_cu->cu = NULL;
  cu->per_cu = NULL;

  obstack_free (&cu->comp_unit_obstack, NULL);
  cu->partial_dies = NULL;
}

/* Free all cached compilation units.  */

static void
free_cached_comp_units (void *data)
{
  struct dwarf2_per_cu_data *per_cu, **last_chain;

  per_cu = dwarf2_per_objfile->read_in_chain;
  last_chain = &dwarf2_per_objfile->read_in_chain;
  while (per_cu != NULL)
    {
      struct dwarf2_per_cu_data *next_cu;

      next_cu = per_cu->cu->read_in_chain;

      free_heap_comp_unit (per_cu->cu);
      *last_chain = next_cu;

      per_cu = next_cu;
    }
}

/* Increase the age counter on each cached compilation unit, and free
   any that are too old.  */

static void
age_cached_comp_units (void)
{
  struct dwarf2_per_cu_data *per_cu, **last_chain;

  dwarf2_clear_marks (dwarf2_per_objfile->read_in_chain);
  per_cu = dwarf2_per_objfile->read_in_chain;
  while (per_cu != NULL)
    {
      per_cu->cu->last_used ++;
      if (per_cu->cu->last_used <= dwarf_max_cache_age)
	dwarf2_mark (per_cu->cu);
      per_cu = per_cu->cu->read_in_chain;
    }

  per_cu = dwarf2_per_objfile->read_in_chain;
  last_chain = &dwarf2_per_objfile->read_in_chain;
  while (per_cu != NULL)
    {
      struct dwarf2_per_cu_data *next_cu;

      next_cu = per_cu->cu->read_in_chain;

      if (!per_cu->cu->mark)
	{
	  free_heap_comp_unit (per_cu->cu);
	  *last_chain = next_cu;
	}
      else
	last_chain = &per_cu->cu->read_in_chain;

      per_cu = next_cu;
    }
}

/* Remove a single compilation unit from the cache.  */

static void
free_one_cached_comp_unit (struct dwarf2_per_cu_data *target_per_cu)
{
  struct dwarf2_per_cu_data *per_cu, **last_chain;

  per_cu = dwarf2_per_objfile->read_in_chain;
  last_chain = &dwarf2_per_objfile->read_in_chain;
  while (per_cu != NULL)
    {
      struct dwarf2_per_cu_data *next_cu;

      next_cu = per_cu->cu->read_in_chain;

      if (per_cu == target_per_cu)
	{
	  free_heap_comp_unit (per_cu->cu);
	  per_cu->cu = NULL;
	  *last_chain = next_cu;
	  break;
	}
      else
	last_chain = &per_cu->cu->read_in_chain;

      per_cu = next_cu;
    }
}

/* Release all extra memory associated with OBJFILE.  */

void
dwarf2_free_objfile (struct objfile *objfile)
{
  dwarf2_per_objfile
    = (struct dwarf2_per_objfile *) objfile_data (objfile,
						  dwarf2_objfile_data_key);

  if (dwarf2_per_objfile == NULL)
    return;

  /* Cached DIE trees use xmalloc and the comp_unit_obstack.  */
  free_cached_comp_units (NULL);

  if (dwarf2_per_objfile->quick_file_names_table)
    htab_delete (dwarf2_per_objfile->quick_file_names_table);

  if (dwarf2_per_objfile->line_header_hash)
    htab_delete (dwarf2_per_objfile->line_header_hash);

  /* Everything else should be on the objfile obstack.  */
}

/* A set of CU "per_cu" pointer, DIE offset, and GDB type pointer.
   We store these in a hash table separate from the DIEs, and preserve them
   when the DIEs are flushed out of cache.

   The CU "per_cu" pointer is needed because offset alone is not enough to
   uniquely identify the type.  A file may have multiple .debug_types sections,
   or the type may come from a DWO file.  Furthermore, while it's more logical
   to use per_cu->section+offset, with Fission the section with the data is in
   the DWO file but we don't know that section at the point we need it.
   We have to use something in dwarf2_per_cu_data (or the pointer to it)
   because we can enter the lookup routine, get_die_type_at_offset, from
   outside this file, and thus won't necessarily have PER_CU->cu.
   Fortunately, PER_CU is stable for the life of the objfile.  */

struct dwarf2_per_cu_offset_and_type
{
  const struct dwarf2_per_cu_data *per_cu;
  sect_offset sect_off;
  struct type *type;
};

/* Hash function for a dwarf2_per_cu_offset_and_type.  */

static hashval_t
per_cu_offset_and_type_hash (const void *item)
{
  const struct dwarf2_per_cu_offset_and_type *ofs
    = (const struct dwarf2_per_cu_offset_and_type *) item;

  return (uintptr_t) ofs->per_cu + to_underlying (ofs->sect_off);
}

/* Equality function for a dwarf2_per_cu_offset_and_type.  */

static int
per_cu_offset_and_type_eq (const void *item_lhs, const void *item_rhs)
{
  const struct dwarf2_per_cu_offset_and_type *ofs_lhs
    = (const struct dwarf2_per_cu_offset_and_type *) item_lhs;
  const struct dwarf2_per_cu_offset_and_type *ofs_rhs
    = (const struct dwarf2_per_cu_offset_and_type *) item_rhs;

  return (ofs_lhs->per_cu == ofs_rhs->per_cu
	  && ofs_lhs->sect_off == ofs_rhs->sect_off);
}

/* Set the type associated with DIE to TYPE.  Save it in CU's hash
   table if necessary.  For convenience, return TYPE.

   The DIEs reading must have careful ordering to:
    * Not cause infite loops trying to read in DIEs as a prerequisite for
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
set_die_type (struct die_info *die, struct type *type, struct dwarf2_cu *cu)
{
  struct dwarf2_per_cu_offset_and_type **slot, ofs;
  struct objfile *objfile = cu->objfile;
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
      && TYPE_CODE (type) != TYPE_CODE_FUNC
      && TYPE_CODE (type) != TYPE_CODE_FLT
      && TYPE_CODE (type) != TYPE_CODE_METHODPTR
      && TYPE_CODE (type) != TYPE_CODE_MEMBERPTR
      && TYPE_CODE (type) != TYPE_CODE_METHOD
      && !HAVE_GNAT_AUX_INFO (type))
    INIT_GNAT_SPECIFIC (type);

  /* Read DW_AT_allocated and set in type.  */
  attr = dwarf2_attr (die, DW_AT_allocated, cu);
  if (attr_form_is_block (attr))
    {
      if (attr_to_dynamic_prop (attr, die, cu, &prop))
        add_dyn_prop (DYN_PROP_ALLOCATED, prop, type, objfile);
    }
  else if (attr != NULL)
    {
      complaint (&symfile_complaints,
		 _("DW_AT_allocated has the wrong form (%s) at DIE 0x%x"),
		 (attr != NULL ? dwarf_form_name (attr->form) : "n/a"),
		 to_underlying (die->sect_off));
    }

  /* Read DW_AT_associated and set in type.  */
  attr = dwarf2_attr (die, DW_AT_associated, cu);
  if (attr_form_is_block (attr))
    {
      if (attr_to_dynamic_prop (attr, die, cu, &prop))
        add_dyn_prop (DYN_PROP_ASSOCIATED, prop, type, objfile);
    }
  else if (attr != NULL)
    {
      complaint (&symfile_complaints,
		 _("DW_AT_associated has the wrong form (%s) at DIE 0x%x"),
		 (attr != NULL ? dwarf_form_name (attr->form) : "n/a"),
		 to_underlying (die->sect_off));
    }

  /* Read DW_AT_data_location and set in type.  */
  attr = dwarf2_attr (die, DW_AT_data_location, cu);
  if (attr_to_dynamic_prop (attr, die, cu, &prop))
    add_dyn_prop (DYN_PROP_DATA_LOCATION, prop, type, objfile);

  if (dwarf2_per_objfile->die_type_hash == NULL)
    {
      dwarf2_per_objfile->die_type_hash =
	htab_create_alloc_ex (127,
			      per_cu_offset_and_type_hash,
			      per_cu_offset_and_type_eq,
			      NULL,
			      &objfile->objfile_obstack,
			      hashtab_obstack_allocate,
			      dummy_obstack_deallocate);
    }

  ofs.per_cu = cu->per_cu;
  ofs.sect_off = die->sect_off;
  ofs.type = type;
  slot = (struct dwarf2_per_cu_offset_and_type **)
    htab_find_slot (dwarf2_per_objfile->die_type_hash, &ofs, INSERT);
  if (*slot)
    complaint (&symfile_complaints,
	       _("A problem internal to GDB: DIE 0x%x has type already set"),
	       to_underlying (die->sect_off));
  *slot = XOBNEW (&objfile->objfile_obstack,
		  struct dwarf2_per_cu_offset_and_type);
  **slot = ofs;
  return type;
}

/* Look up the type for the die at SECT_OFF in PER_CU in die_type_hash,
   or return NULL if the die does not have a saved type.  */

static struct type *
get_die_type_at_offset (sect_offset sect_off,
			struct dwarf2_per_cu_data *per_cu)
{
  struct dwarf2_per_cu_offset_and_type *slot, ofs;

  if (dwarf2_per_objfile->die_type_hash == NULL)
    return NULL;

  ofs.per_cu = per_cu;
  ofs.sect_off = sect_off;
  slot = ((struct dwarf2_per_cu_offset_and_type *)
	  htab_find (dwarf2_per_objfile->die_type_hash, &ofs));
  if (slot)
    return slot->type;
  else
    return NULL;
}

/* Look up the type for DIE in CU in die_type_hash,
   or return NULL if DIE does not have a saved type.  */

static struct type *
get_die_type (struct die_info *die, struct dwarf2_cu *cu)
{
  return get_die_type_at_offset (die->sect_off, cu->per_cu);
}

/* Add a dependence relationship from CU to REF_PER_CU.  */

static void
dwarf2_add_dependence (struct dwarf2_cu *cu,
		       struct dwarf2_per_cu_data *ref_per_cu)
{
  void **slot;

  if (cu->dependencies == NULL)
    cu->dependencies
      = htab_create_alloc_ex (5, htab_hash_pointer, htab_eq_pointer,
			      NULL, &cu->comp_unit_obstack,
			      hashtab_obstack_allocate,
			      dummy_obstack_deallocate);

  slot = htab_find_slot (cu->dependencies, ref_per_cu, INSERT);
  if (*slot == NULL)
    *slot = ref_per_cu;
}

/* Subroutine of dwarf2_mark to pass to htab_traverse.
   Set the mark field in every compilation unit in the
   cache that we must keep because we are keeping CU.  */

static int
dwarf2_mark_helper (void **slot, void *data)
{
  struct dwarf2_per_cu_data *per_cu;

  per_cu = (struct dwarf2_per_cu_data *) *slot;

  /* cu->dependencies references may not yet have been ever read if QUIT aborts
     reading of the chain.  As such dependencies remain valid it is not much
     useful to track and undo them during QUIT cleanups.  */
  if (per_cu->cu == NULL)
    return 1;

  if (per_cu->cu->mark)
    return 1;
  per_cu->cu->mark = 1;

  if (per_cu->cu->dependencies != NULL)
    htab_traverse (per_cu->cu->dependencies, dwarf2_mark_helper, NULL);

  return 1;
}

/* Set the mark field in CU and in every other compilation unit in the
   cache that we must keep because we are keeping CU.  */

static void
dwarf2_mark (struct dwarf2_cu *cu)
{
  if (cu->mark)
    return;
  cu->mark = 1;
  if (cu->dependencies != NULL)
    htab_traverse (cu->dependencies, dwarf2_mark_helper, NULL);
}

static void
dwarf2_clear_marks (struct dwarf2_per_cu_data *per_cu)
{
  while (per_cu)
    {
      per_cu->cu->mark = 0;
      per_cu = per_cu->cu->read_in_chain;
    }
}

/* Trivial hash function for partial_die_info: the hash value of a DIE
   is its offset in .debug_info for this objfile.  */

static hashval_t
partial_die_hash (const void *item)
{
  const struct partial_die_info *part_die
    = (const struct partial_die_info *) item;

  return to_underlying (part_die->sect_off);
}

/* Trivial comparison function for partial_die_info structures: two DIEs
   are equal if they have the same offset.  */

static int
partial_die_eq (const void *item_lhs, const void *item_rhs)
{
  const struct partial_die_info *part_die_lhs
    = (const struct partial_die_info *) item_lhs;
  const struct partial_die_info *part_die_rhs
    = (const struct partial_die_info *) item_rhs;

  return part_die_lhs->sect_off == part_die_rhs->sect_off;
}

static struct cmd_list_element *set_dwarf_cmdlist;
static struct cmd_list_element *show_dwarf_cmdlist;

static void
set_dwarf_cmd (char *args, int from_tty)
{
  help_list (set_dwarf_cmdlist, "maintenance set dwarf ", all_commands,
	     gdb_stdout);
}

static void
show_dwarf_cmd (char *args, int from_tty)
{
  cmd_show_list (show_dwarf_cmdlist, from_tty, "");
}

/* Free data associated with OBJFILE, if necessary.  */

static void
dwarf2_per_objfile_free (struct objfile *objfile, void *d)
{
  struct dwarf2_per_objfile *data = (struct dwarf2_per_objfile *) d;
  int ix;

  /* Make sure we don't accidentally use dwarf2_per_objfile while
     cleaning up.  */
  dwarf2_per_objfile = NULL;

  for (ix = 0; ix < data->n_comp_units; ++ix)
   VEC_free (dwarf2_per_cu_ptr, data->all_comp_units[ix]->imported_symtabs);

  for (ix = 0; ix < data->n_type_units; ++ix)
    VEC_free (dwarf2_per_cu_ptr,
	      data->all_type_units[ix]->per_cu.imported_symtabs);
  xfree (data->all_type_units);

  VEC_free (dwarf2_section_info_def, data->types);

  if (data->dwo_files)
    free_dwo_files (data->dwo_files, objfile);
  if (data->dwp_file)
    gdb_bfd_unref (data->dwp_file->dbfd);

  if (data->dwz_file && data->dwz_file->dwz_bfd)
    gdb_bfd_unref (data->dwz_file->dwz_bfd);
}


/* The "save gdb-index" command.  */

/* The contents of the hash table we create when building the string
   table.  */
struct strtab_entry
{
  offset_type offset;
  const char *str;
};

/* Hash function for a strtab_entry.

   Function is used only during write_hash_table so no index format backward
   compatibility is needed.  */

static hashval_t
hash_strtab_entry (const void *e)
{
  const struct strtab_entry *entry = (const struct strtab_entry *) e;
  return mapped_index_string_hash (INT_MAX, entry->str);
}

/* Equality function for a strtab_entry.  */

static int
eq_strtab_entry (const void *a, const void *b)
{
  const struct strtab_entry *ea = (const struct strtab_entry *) a;
  const struct strtab_entry *eb = (const struct strtab_entry *) b;
  return !strcmp (ea->str, eb->str);
}

/* Create a strtab_entry hash table.  */

static htab_t
create_strtab (void)
{
  return htab_create_alloc (100, hash_strtab_entry, eq_strtab_entry,
			    xfree, xcalloc, xfree);
}

/* Add a string to the constant pool.  Return the string's offset in
   host order.  */

static offset_type
add_string (htab_t table, struct obstack *cpool, const char *str)
{
  void **slot;
  struct strtab_entry entry;
  struct strtab_entry *result;

  entry.str = str;
  slot = htab_find_slot (table, &entry, INSERT);
  if (*slot)
    result = (struct strtab_entry *) *slot;
  else
    {
      result = XNEW (struct strtab_entry);
      result->offset = obstack_object_size (cpool);
      result->str = str;
      obstack_grow_str0 (cpool, str);
      *slot = result;
    }
  return result->offset;
}

/* An entry in the symbol table.  */
struct symtab_index_entry
{
  /* The name of the symbol.  */
  const char *name;
  /* The offset of the name in the constant pool.  */
  offset_type index_offset;
  /* A sorted vector of the indices of all the CUs that hold an object
     of this name.  */
  VEC (offset_type) *cu_indices;
};

/* The symbol table.  This is a power-of-2-sized hash table.  */
struct mapped_symtab
{
  offset_type n_elements;
  offset_type size;
  struct symtab_index_entry **data;
};

/* Hash function for a symtab_index_entry.  */

static hashval_t
hash_symtab_entry (const void *e)
{
  const struct symtab_index_entry *entry
    = (const struct symtab_index_entry *) e;
  return iterative_hash (VEC_address (offset_type, entry->cu_indices),
			 sizeof (offset_type) * VEC_length (offset_type,
							    entry->cu_indices),
			 0);
}

/* Equality function for a symtab_index_entry.  */

static int
eq_symtab_entry (const void *a, const void *b)
{
  const struct symtab_index_entry *ea = (const struct symtab_index_entry *) a;
  const struct symtab_index_entry *eb = (const struct symtab_index_entry *) b;
  int len = VEC_length (offset_type, ea->cu_indices);
  if (len != VEC_length (offset_type, eb->cu_indices))
    return 0;
  return !memcmp (VEC_address (offset_type, ea->cu_indices),
		  VEC_address (offset_type, eb->cu_indices),
		  sizeof (offset_type) * len);
}

/* Destroy a symtab_index_entry.  */

static void
delete_symtab_entry (void *p)
{
  struct symtab_index_entry *entry = (struct symtab_index_entry *) p;
  VEC_free (offset_type, entry->cu_indices);
  xfree (entry);
}

/* Create a hash table holding symtab_index_entry objects.  */

static htab_t
create_symbol_hash_table (void)
{
  return htab_create_alloc (100, hash_symtab_entry, eq_symtab_entry,
			    delete_symtab_entry, xcalloc, xfree);
}

/* Create a new mapped symtab object.  */

static struct mapped_symtab *
create_mapped_symtab (void)
{
  struct mapped_symtab *symtab = XNEW (struct mapped_symtab);
  symtab->n_elements = 0;
  symtab->size = 1024;
  symtab->data = XCNEWVEC (struct symtab_index_entry *, symtab->size);
  return symtab;
}

/* Destroy a mapped_symtab.  */

static void
cleanup_mapped_symtab (void *p)
{
  struct mapped_symtab *symtab = (struct mapped_symtab *) p;
  /* The contents of the array are freed when the other hash table is
     destroyed.  */
  xfree (symtab->data);
  xfree (symtab);
}

/* Find a slot in SYMTAB for the symbol NAME.  Returns a pointer to
   the slot.
   
   Function is used only during write_hash_table so no index format backward
   compatibility is needed.  */

static struct symtab_index_entry **
find_slot (struct mapped_symtab *symtab, const char *name)
{
  offset_type index, step, hash = mapped_index_string_hash (INT_MAX, name);

  index = hash & (symtab->size - 1);
  step = ((hash * 17) & (symtab->size - 1)) | 1;

  for (;;)
    {
      if (!symtab->data[index] || !strcmp (name, symtab->data[index]->name))
	return &symtab->data[index];
      index = (index + step) & (symtab->size - 1);
    }
}

/* Expand SYMTAB's hash table.  */

static void
hash_expand (struct mapped_symtab *symtab)
{
  offset_type old_size = symtab->size;
  offset_type i;
  struct symtab_index_entry **old_entries = symtab->data;

  symtab->size *= 2;
  symtab->data = XCNEWVEC (struct symtab_index_entry *, symtab->size);

  for (i = 0; i < old_size; ++i)
    {
      if (old_entries[i])
	{
	  struct symtab_index_entry **slot = find_slot (symtab,
							old_entries[i]->name);
	  *slot = old_entries[i];
	}
    }

  xfree (old_entries);
}

/* Add an entry to SYMTAB.  NAME is the name of the symbol.
   CU_INDEX is the index of the CU in which the symbol appears.
   IS_STATIC is one if the symbol is static, otherwise zero (global).  */

static void
add_index_entry (struct mapped_symtab *symtab, const char *name,
		 int is_static, gdb_index_symbol_kind kind,
		 offset_type cu_index)
{
  struct symtab_index_entry **slot;
  offset_type cu_index_and_attrs;

  ++symtab->n_elements;
  if (4 * symtab->n_elements / 3 >= symtab->size)
    hash_expand (symtab);

  slot = find_slot (symtab, name);
  if (!*slot)
    {
      *slot = XNEW (struct symtab_index_entry);
      (*slot)->name = name;
      /* index_offset is set later.  */
      (*slot)->cu_indices = NULL;
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
     sort and uniqufy the list after we've processed all symbols.  */
  VEC_safe_push (offset_type, (*slot)->cu_indices, cu_index_and_attrs);
}

/* qsort helper routine for uniquify_cu_indices.  */

static int
offset_type_compare (const void *ap, const void *bp)
{
  offset_type a = *(offset_type *) ap;
  offset_type b = *(offset_type *) bp;

  return (a > b) - (b > a);
}

/* Sort and remove duplicates of all symbols' cu_indices lists.  */

static void
uniquify_cu_indices (struct mapped_symtab *symtab)
{
  int i;

  for (i = 0; i < symtab->size; ++i)
    {
      struct symtab_index_entry *entry = symtab->data[i];

      if (entry
	  && entry->cu_indices != NULL)
	{
	  unsigned int next_to_insert, next_to_check;
	  offset_type last_value;

	  qsort (VEC_address (offset_type, entry->cu_indices),
		 VEC_length (offset_type, entry->cu_indices),
		 sizeof (offset_type), offset_type_compare);

	  last_value = VEC_index (offset_type, entry->cu_indices, 0);
	  next_to_insert = 1;
	  for (next_to_check = 1;
	       next_to_check < VEC_length (offset_type, entry->cu_indices);
	       ++next_to_check)
	    {
	      if (VEC_index (offset_type, entry->cu_indices, next_to_check)
		  != last_value)
		{
		  last_value = VEC_index (offset_type, entry->cu_indices,
					  next_to_check);
		  VEC_replace (offset_type, entry->cu_indices, next_to_insert,
			       last_value);
		  ++next_to_insert;
		}
	    }
	  VEC_truncate (offset_type, entry->cu_indices, next_to_insert);
	}
    }
}

/* Add a vector of indices to the constant pool.  */

static offset_type
add_indices_to_cpool (htab_t symbol_hash_table, struct obstack *cpool,
		      struct symtab_index_entry *entry)
{
  void **slot;

  slot = htab_find_slot (symbol_hash_table, entry, INSERT);
  if (!*slot)
    {
      offset_type len = VEC_length (offset_type, entry->cu_indices);
      offset_type val = MAYBE_SWAP (len);
      offset_type iter;
      int i;

      *slot = entry;
      entry->index_offset = obstack_object_size (cpool);

      obstack_grow (cpool, &val, sizeof (val));
      for (i = 0;
	   VEC_iterate (offset_type, entry->cu_indices, i, iter);
	   ++i)
	{
	  val = MAYBE_SWAP (iter);
	  obstack_grow (cpool, &val, sizeof (val));
	}
    }
  else
    {
      struct symtab_index_entry *old_entry
	= (struct symtab_index_entry *) *slot;
      entry->index_offset = old_entry->index_offset;
      entry = old_entry;
    }
  return entry->index_offset;
}

/* Write the mapped hash table SYMTAB to the obstack OUTPUT, with
   constant pool entries going into the obstack CPOOL.  */

static void
write_hash_table (struct mapped_symtab *symtab,
		  struct obstack *output, struct obstack *cpool)
{
  offset_type i;
  htab_t symbol_hash_table;
  htab_t str_table;

  symbol_hash_table = create_symbol_hash_table ();
  str_table = create_strtab ();

  /* We add all the index vectors to the constant pool first, to
     ensure alignment is ok.  */
  for (i = 0; i < symtab->size; ++i)
    {
      if (symtab->data[i])
	add_indices_to_cpool (symbol_hash_table, cpool, symtab->data[i]);
    }

  /* Now write out the hash table.  */
  for (i = 0; i < symtab->size; ++i)
    {
      offset_type str_off, vec_off;

      if (symtab->data[i])
	{
	  str_off = add_string (str_table, cpool, symtab->data[i]->name);
	  vec_off = symtab->data[i]->index_offset;
	}
      else
	{
	  /* While 0 is a valid constant pool index, it is not valid
	     to have 0 for both offsets.  */
	  str_off = 0;
	  vec_off = 0;
	}

      str_off = MAYBE_SWAP (str_off);
      vec_off = MAYBE_SWAP (vec_off);

      obstack_grow (output, &str_off, sizeof (str_off));
      obstack_grow (output, &vec_off, sizeof (vec_off));
    }

  htab_delete (str_table);
  htab_delete (symbol_hash_table);
}

/* Struct to map psymtab to CU index in the index file.  */
struct psymtab_cu_index_map
{
  struct partial_symtab *psymtab;
  unsigned int cu_index;
};

static hashval_t
hash_psymtab_cu_index (const void *item)
{
  const struct psymtab_cu_index_map *map
    = (const struct psymtab_cu_index_map *) item;

  return htab_hash_pointer (map->psymtab);
}

static int
eq_psymtab_cu_index (const void *item_lhs, const void *item_rhs)
{
  const struct psymtab_cu_index_map *lhs
    = (const struct psymtab_cu_index_map *) item_lhs;
  const struct psymtab_cu_index_map *rhs
    = (const struct psymtab_cu_index_map *) item_rhs;

  return lhs->psymtab == rhs->psymtab;
}

/* Helper struct for building the address table.  */
struct addrmap_index_data
{
  struct objfile *objfile;
  struct obstack *addr_obstack;
  htab_t cu_index_htab;

  /* Non-zero if the previous_* fields are valid.
     We can't write an entry until we see the next entry (since it is only then
     that we know the end of the entry).  */
  int previous_valid;
  /* Index of the CU in the table of all CUs in the index file.  */
  unsigned int previous_cu_index;
  /* Start address of the CU.  */
  CORE_ADDR previous_cu_start;
};

/* Write an address entry to OBSTACK.  */

static void
add_address_entry (struct objfile *objfile, struct obstack *obstack,
		   CORE_ADDR start, CORE_ADDR end, unsigned int cu_index)
{
  offset_type cu_index_to_write;
  gdb_byte addr[8];
  CORE_ADDR baseaddr;

  baseaddr = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

  store_unsigned_integer (addr, 8, BFD_ENDIAN_LITTLE, start - baseaddr);
  obstack_grow (obstack, addr, 8);
  store_unsigned_integer (addr, 8, BFD_ENDIAN_LITTLE, end - baseaddr);
  obstack_grow (obstack, addr, 8);
  cu_index_to_write = MAYBE_SWAP (cu_index);
  obstack_grow (obstack, &cu_index_to_write, sizeof (offset_type));
}

/* Worker function for traversing an addrmap to build the address table.  */

static int
add_address_entry_worker (void *datap, CORE_ADDR start_addr, void *obj)
{
  struct addrmap_index_data *data = (struct addrmap_index_data *) datap;
  struct partial_symtab *pst = (struct partial_symtab *) obj;

  if (data->previous_valid)
    add_address_entry (data->objfile, data->addr_obstack,
		       data->previous_cu_start, start_addr,
		       data->previous_cu_index);

  data->previous_cu_start = start_addr;
  if (pst != NULL)
    {
      struct psymtab_cu_index_map find_map, *map;
      find_map.psymtab = pst;
      map = ((struct psymtab_cu_index_map *)
	     htab_find (data->cu_index_htab, &find_map));
      gdb_assert (map != NULL);
      data->previous_cu_index = map->cu_index;
      data->previous_valid = 1;
    }
  else
      data->previous_valid = 0;

  return 0;
}

/* Write OBJFILE's address map to OBSTACK.
   CU_INDEX_HTAB is used to map addrmap entries to their CU indices
   in the index file.  */

static void
write_address_map (struct objfile *objfile, struct obstack *obstack,
		   htab_t cu_index_htab)
{
  struct addrmap_index_data addrmap_index_data;

  /* When writing the address table, we have to cope with the fact that
     the addrmap iterator only provides the start of a region; we have to
     wait until the next invocation to get the start of the next region.  */

  addrmap_index_data.objfile = objfile;
  addrmap_index_data.addr_obstack = obstack;
  addrmap_index_data.cu_index_htab = cu_index_htab;
  addrmap_index_data.previous_valid = 0;

  addrmap_foreach (objfile->psymtabs_addrmap, add_address_entry_worker,
		   &addrmap_index_data);

  /* It's highly unlikely the last entry (end address = 0xff...ff)
     is valid, but we should still handle it.
     The end address is recorded as the start of the next region, but that
     doesn't work here.  To cope we pass 0xff...ff, this is a rare situation
     anyway.  */
  if (addrmap_index_data.previous_valid)
    add_address_entry (objfile, obstack,
		       addrmap_index_data.previous_cu_start, (CORE_ADDR) -1,
		       addrmap_index_data.previous_cu_index);
}

/* Return the symbol kind of PSYM.  */

static gdb_index_symbol_kind
symbol_kind (struct partial_symbol *psym)
{
  domain_enum domain = PSYMBOL_DOMAIN (psym);
  enum address_class aclass = PSYMBOL_CLASS (psym);

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
		htab_t psyms_seen,
		struct partial_symbol **psymp,
		int count,
		offset_type cu_index,
		int is_static)
{
  for (; count-- > 0; ++psymp)
    {
      struct partial_symbol *psym = *psymp;
      void **slot;

      if (SYMBOL_LANGUAGE (psym) == language_ada)
	error (_("Ada is not currently supported by the index"));

      /* Only add a given psymbol once.  */
      slot = htab_find_slot (psyms_seen, psym, INSERT);
      if (!*slot)
	{
	  gdb_index_symbol_kind kind = symbol_kind (psym);

	  *slot = psym;
	  add_index_entry (symtab, SYMBOL_SEARCH_NAME (psym),
			   is_static, kind, cu_index);
	}
    }
}

/* Write the contents of an ("unfinished") obstack to FILE.  Throw an
   exception if there is an error.  */

static void
write_obstack (FILE *file, struct obstack *obstack)
{
  if (fwrite (obstack_base (obstack), 1, obstack_object_size (obstack),
	      file)
      != obstack_object_size (obstack))
    error (_("couldn't data write to file"));
}

/* A helper struct used when iterating over debug_types.  */
struct signatured_type_index_data
{
  struct objfile *objfile;
  struct mapped_symtab *symtab;
  struct obstack *types_list;
  htab_t psyms_seen;
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
  struct partial_symtab *psymtab = entry->per_cu.v.psymtab;
  gdb_byte val[8];

  write_psymbols (info->symtab,
		  info->psyms_seen,
		  info->objfile->global_psymbols.list
		  + psymtab->globals_offset,
		  psymtab->n_global_syms, info->cu_index,
		  0);
  write_psymbols (info->symtab,
		  info->psyms_seen,
		  info->objfile->static_psymbols.list
		  + psymtab->statics_offset,
		  psymtab->n_static_syms, info->cu_index,
		  1);

  store_unsigned_integer (val, 8, BFD_ENDIAN_LITTLE,
			  to_underlying (entry->per_cu.sect_off));
  obstack_grow (info->types_list, val, 8);
  store_unsigned_integer (val, 8, BFD_ENDIAN_LITTLE,
			  to_underlying (entry->type_offset_in_tu));
  obstack_grow (info->types_list, val, 8);
  store_unsigned_integer (val, 8, BFD_ENDIAN_LITTLE, entry->signature);
  obstack_grow (info->types_list, val, 8);

  ++info->cu_index;

  return 1;
}

/* Recurse into all "included" dependencies and write their symbols as
   if they appeared in this psymtab.  */

static void
recursively_write_psymbols (struct objfile *objfile,
			    struct partial_symtab *psymtab,
			    struct mapped_symtab *symtab,
			    htab_t psyms_seen,
			    offset_type cu_index)
{
  int i;

  for (i = 0; i < psymtab->number_of_dependencies; ++i)
    if (psymtab->dependencies[i]->user != NULL)
      recursively_write_psymbols (objfile, psymtab->dependencies[i],
				  symtab, psyms_seen, cu_index);

  write_psymbols (symtab,
		  psyms_seen,
		  objfile->global_psymbols.list + psymtab->globals_offset,
		  psymtab->n_global_syms, cu_index,
		  0);
  write_psymbols (symtab,
		  psyms_seen,
		  objfile->static_psymbols.list + psymtab->statics_offset,
		  psymtab->n_static_syms, cu_index,
		  1);
}

/* Create an index file for OBJFILE in the directory DIR.  */

static void
write_psymtabs_to_index (struct objfile *objfile, const char *dir)
{
  struct cleanup *cleanup;
  char *filename;
  struct obstack contents, addr_obstack, constant_pool, symtab_obstack;
  struct obstack cu_list, types_cu_list;
  int i;
  FILE *out_file;
  struct mapped_symtab *symtab;
  offset_type val, size_of_contents, total_len;
  struct stat st;
  struct psymtab_cu_index_map *psymtab_cu_index_map;

  if (dwarf2_per_objfile->using_index)
    error (_("Cannot use an index to create the index"));

  if (VEC_length (dwarf2_section_info_def, dwarf2_per_objfile->types) > 1)
    error (_("Cannot make an index when the file has multiple .debug_types sections"));

  if (!objfile->psymtabs || !objfile->psymtabs_addrmap)
    return;

  if (stat (objfile_name (objfile), &st) < 0)
    perror_with_name (objfile_name (objfile));

  filename = concat (dir, SLASH_STRING, lbasename (objfile_name (objfile)),
		     INDEX_SUFFIX, (char *) NULL);
  cleanup = make_cleanup (xfree, filename);

  out_file = gdb_fopen_cloexec (filename, "wb");
  if (!out_file)
    error (_("Can't open `%s' for writing"), filename);

  gdb::unlinker unlink_file (filename);

  symtab = create_mapped_symtab ();
  make_cleanup (cleanup_mapped_symtab, symtab);

  obstack_init (&addr_obstack);
  make_cleanup_obstack_free (&addr_obstack);

  obstack_init (&cu_list);
  make_cleanup_obstack_free (&cu_list);

  obstack_init (&types_cu_list);
  make_cleanup_obstack_free (&types_cu_list);

  htab_up psyms_seen (htab_create_alloc (100, htab_hash_pointer,
					 htab_eq_pointer,
					 NULL, xcalloc, xfree));

  /* While we're scanning CU's create a table that maps a psymtab pointer
     (which is what addrmap records) to its index (which is what is recorded
     in the index file).  This will later be needed to write the address
     table.  */
  htab_up cu_index_htab (htab_create_alloc (100,
					    hash_psymtab_cu_index,
					    eq_psymtab_cu_index,
					    NULL, xcalloc, xfree));
  psymtab_cu_index_map = XNEWVEC (struct psymtab_cu_index_map,
				  dwarf2_per_objfile->n_comp_units);
  make_cleanup (xfree, psymtab_cu_index_map);

  /* The CU list is already sorted, so we don't need to do additional
     work here.  Also, the debug_types entries do not appear in
     all_comp_units, but only in their own hash table.  */
  for (i = 0; i < dwarf2_per_objfile->n_comp_units; ++i)
    {
      struct dwarf2_per_cu_data *per_cu
	= dwarf2_per_objfile->all_comp_units[i];
      struct partial_symtab *psymtab = per_cu->v.psymtab;
      gdb_byte val[8];
      struct psymtab_cu_index_map *map;
      void **slot;

      /* CU of a shared file from 'dwz -m' may be unused by this main file.
	 It may be referenced from a local scope but in such case it does not
	 need to be present in .gdb_index.  */
      if (psymtab == NULL)
	continue;

      if (psymtab->user == NULL)
	recursively_write_psymbols (objfile, psymtab, symtab,
				    psyms_seen.get (), i);

      map = &psymtab_cu_index_map[i];
      map->psymtab = psymtab;
      map->cu_index = i;
      slot = htab_find_slot (cu_index_htab.get (), map, INSERT);
      gdb_assert (slot != NULL);
      gdb_assert (*slot == NULL);
      *slot = map;

      store_unsigned_integer (val, 8, BFD_ENDIAN_LITTLE,
			      to_underlying (per_cu->sect_off));
      obstack_grow (&cu_list, val, 8);
      store_unsigned_integer (val, 8, BFD_ENDIAN_LITTLE, per_cu->length);
      obstack_grow (&cu_list, val, 8);
    }

  /* Dump the address map.  */
  write_address_map (objfile, &addr_obstack, cu_index_htab.get ());

  /* Write out the .debug_type entries, if any.  */
  if (dwarf2_per_objfile->signatured_types)
    {
      struct signatured_type_index_data sig_data;

      sig_data.objfile = objfile;
      sig_data.symtab = symtab;
      sig_data.types_list = &types_cu_list;
      sig_data.psyms_seen = psyms_seen.get ();
      sig_data.cu_index = dwarf2_per_objfile->n_comp_units;
      htab_traverse_noresize (dwarf2_per_objfile->signatured_types,
			      write_one_signatured_type, &sig_data);
    }

  /* Now that we've processed all symbols we can shrink their cu_indices
     lists.  */
  uniquify_cu_indices (symtab);

  obstack_init (&constant_pool);
  make_cleanup_obstack_free (&constant_pool);
  obstack_init (&symtab_obstack);
  make_cleanup_obstack_free (&symtab_obstack);
  write_hash_table (symtab, &symtab_obstack, &constant_pool);

  obstack_init (&contents);
  make_cleanup_obstack_free (&contents);
  size_of_contents = 6 * sizeof (offset_type);
  total_len = size_of_contents;

  /* The version number.  */
  val = MAYBE_SWAP (8);
  obstack_grow (&contents, &val, sizeof (val));

  /* The offset of the CU list from the start of the file.  */
  val = MAYBE_SWAP (total_len);
  obstack_grow (&contents, &val, sizeof (val));
  total_len += obstack_object_size (&cu_list);

  /* The offset of the types CU list from the start of the file.  */
  val = MAYBE_SWAP (total_len);
  obstack_grow (&contents, &val, sizeof (val));
  total_len += obstack_object_size (&types_cu_list);

  /* The offset of the address table from the start of the file.  */
  val = MAYBE_SWAP (total_len);
  obstack_grow (&contents, &val, sizeof (val));
  total_len += obstack_object_size (&addr_obstack);

  /* The offset of the symbol table from the start of the file.  */
  val = MAYBE_SWAP (total_len);
  obstack_grow (&contents, &val, sizeof (val));
  total_len += obstack_object_size (&symtab_obstack);

  /* The offset of the constant pool from the start of the file.  */
  val = MAYBE_SWAP (total_len);
  obstack_grow (&contents, &val, sizeof (val));
  total_len += obstack_object_size (&constant_pool);

  gdb_assert (obstack_object_size (&contents) == size_of_contents);

  write_obstack (out_file, &contents);
  write_obstack (out_file, &cu_list);
  write_obstack (out_file, &types_cu_list);
  write_obstack (out_file, &addr_obstack);
  write_obstack (out_file, &symtab_obstack);
  write_obstack (out_file, &constant_pool);

  fclose (out_file);

  /* We want to keep the file.  */
  unlink_file.keep ();

  do_cleanups (cleanup);
}

/* Implementation of the `save gdb-index' command.
   
   Note that the file format used by this command is documented in the
   GDB manual.  Any changes here must be documented there.  */

static void
save_gdb_index_command (char *arg, int from_tty)
{
  struct objfile *objfile;

  if (!arg || !*arg)
    error (_("usage: save gdb-index DIRECTORY"));

  ALL_OBJFILES (objfile)
  {
    struct stat st;

    /* If the objfile does not correspond to an actual file, skip it.  */
    if (stat (objfile_name (objfile), &st) < 0)
      continue;

    dwarf2_per_objfile
      = (struct dwarf2_per_objfile *) objfile_data (objfile,
						    dwarf2_objfile_data_key);
    if (dwarf2_per_objfile)
      {

	TRY
	  {
	    write_psymtabs_to_index (objfile, arg);
	  }
	CATCH (except, RETURN_MASK_ERROR)
	  {
	    exception_fprintf (gdb_stderr, except,
			       _("Error while writing index for `%s': "),
			       objfile_name (objfile));
	  }
	END_CATCH
      }
  }
}



int dwarf_always_disassemble;

static void
show_dwarf_always_disassemble (struct ui_file *file, int from_tty,
			       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Whether to always disassemble "
		      "DWARF expressions is %s.\n"),
		    value);
}

static void
show_check_physname (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Whether to check \"physname\" is %s.\n"),
		    value);
}

void _initialize_dwarf2_read (void);

void
_initialize_dwarf2_read (void)
{
  struct cmd_list_element *c;

  dwarf2_objfile_data_key
    = register_objfile_data_with_cleanup (NULL, dwarf2_per_objfile_free);

  add_prefix_cmd ("dwarf", class_maintenance, set_dwarf_cmd, _("\
Set DWARF specific variables.\n\
Configure DWARF variables such as the cache size"),
                  &set_dwarf_cmdlist, "maintenance set dwarf ",
                  0/*allow-unknown*/, &maintenance_set_cmdlist);

  add_prefix_cmd ("dwarf", class_maintenance, show_dwarf_cmd, _("\
Show DWARF specific variables\n\
Show DWARF variables such as the cache size"),
                  &show_dwarf_cmdlist, "maintenance show dwarf ",
                  0/*allow-unknown*/, &maintenance_show_cmdlist);

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

  add_setshow_boolean_cmd ("always-disassemble", class_obscure,
			   &dwarf_always_disassemble, _("\
Set whether `info address' always disassembles DWARF expressions."), _("\
Show whether `info address' always disassembles DWARF expressions."), _("\
When enabled, DWARF expressions are always printed in an assembly-like\n\
syntax.  When disabled, expressions will be printed in a more\n\
conversational style, when possible."),
			   NULL,
			   show_dwarf_always_disassemble,
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

  c = add_cmd ("gdb-index", class_files, save_gdb_index_command,
	       _("\
Save a gdb-index file.\n\
Usage: save gdb-index DIRECTORY"),
	       &save_cmdlist);
  set_cmd_completer (c, filename_completer);

  dwarf2_locexpr_index = register_symbol_computed_impl (LOC_COMPUTED,
							&dwarf2_locexpr_funcs);
  dwarf2_loclist_index = register_symbol_computed_impl (LOC_COMPUTED,
							&dwarf2_loclist_funcs);

  dwarf2_locexpr_block_index = register_symbol_block_impl (LOC_BLOCK,
					&dwarf2_block_frame_base_locexpr_funcs);
  dwarf2_loclist_block_index = register_symbol_block_impl (LOC_BLOCK,
					&dwarf2_block_frame_base_loclist_funcs);
}
