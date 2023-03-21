/* Definitions for reading symbol files into GDB.

   Copyright (C) 1990-2020 Free Software Foundation, Inc.

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

#if !defined (SYMFILE_H)
#define SYMFILE_H

/* This file requires that you first include "bfd.h".  */
#include "symtab.h"
#include "probe.h"
#include "symfile-add-flags.h"
#include "objfile-flags.h"
#include "gdb_bfd.h"
#include "gdbsupport/function-view.h"

/* Opaque declarations.  */
struct target_section;
struct objfile;
struct obj_section;
struct obstack;
struct block;
struct value;
struct frame_info;
struct agent_expr;
struct axs_value;
class probe;

/* Comparison function for symbol look ups.  */

typedef int (symbol_compare_ftype) (const char *string1,
				    const char *string2);

struct other_sections
{
  other_sections (CORE_ADDR addr_, std::string &&name_, int sectindex_)
    : addr (addr_),
      name (std::move (name_)),
      sectindex (sectindex_)
  {
  }

  other_sections (other_sections &&other) = default;

  DISABLE_COPY_AND_ASSIGN (other_sections);

  CORE_ADDR addr;
  std::string name;

  /* SECTINDEX must be valid for associated BFD or set to -1.
     See syms_from_objfile_1 for an exception to this rule.
   */
  int sectindex;
};

/* Define an array of addresses to accommodate non-contiguous dynamic
   loading of modules.  This is for use when entering commands, so we
   can keep track of the section names until we read the file and can
   map them to bfd sections.  This structure is also used by solib.c
   to communicate the section addresses in shared objects to
   symbol_file_add ().  */

typedef std::vector<other_sections> section_addr_info;

/* A table listing the load segments in a symfile, and which segment
   each BFD section belongs to.  */
struct symfile_segment_data
{
  struct segment
  {
    segment (CORE_ADDR base, CORE_ADDR size)
      : base (base), size (size)
    {}

    /* The original base address the segment.  */
    CORE_ADDR base;

    /* The memory size of the segment.  */
    CORE_ADDR size;
  };

  /* The segments present in this file.  If there are
     two, the text segment is the first one and the data segment
     is the second one.  */
  std::vector<segment> segments;

  /* This is an array of entries recording which segment contains each BFD
     section.  SEGMENT_INFO[I] is S+1 if the I'th BFD section belongs to segment
     S, or zero if it is not in any segment.  */
  std::vector<int> segment_info;
};

using symfile_segment_data_up = std::unique_ptr<symfile_segment_data>;

/* Callback for quick_symbol_functions->map_symbol_filenames.  */

typedef void (symbol_filename_ftype) (const char *filename,
				      const char *fullname, void *data);

/* Callback for quick_symbol_functions->expand_symtabs_matching
   to match a file name.  */

typedef bool (expand_symtabs_file_matcher_ftype) (const char *filename,
						  bool basenames);

/* Callback for quick_symbol_functions->expand_symtabs_matching
   to match a symbol name.  */

typedef bool (expand_symtabs_symbol_matcher_ftype) (const char *name);

/* Callback for quick_symbol_functions->expand_symtabs_matching
   to be called after a symtab has been expanded.  */

typedef void (expand_symtabs_exp_notify_ftype) (compunit_symtab *symtab);

/* The "quick" symbol functions exist so that symbol readers can
   avoiding an initial read of all the symbols.  For example, symbol
   readers might choose to use the "partial symbol table" utilities,
   which is one implementation of the quick symbol functions.

   The quick symbol functions are generally opaque: the underlying
   representation is hidden from the caller.

   In general, these functions should only look at whatever special
   index the symbol reader creates -- looking through the symbol
   tables themselves is handled by generic code.  If a function is
   defined as returning a "symbol table", this means that the function
   should only return a newly-created symbol table; it should not
   examine pre-existing ones.

   The exact list of functions here was determined in an ad hoc way
   based on gdb's history.  */

struct quick_symbol_functions
{
  /* Return true if this objfile has any "partial" symbols
     available.  */
  int (*has_symbols) (struct objfile *objfile);

  /* Return the symbol table for the "last" file appearing in
     OBJFILE.  */
  struct symtab *(*find_last_source_symtab) (struct objfile *objfile);

  /* Forget all cached full file names for OBJFILE.  */
  void (*forget_cached_source_info) (struct objfile *objfile);

  /* Expand and iterate over each "partial" symbol table in OBJFILE
     where the source file is named NAME.

     If NAME is not absolute, a match after a '/' in the symbol table's
     file name will also work, REAL_PATH is NULL then.  If NAME is
     absolute then REAL_PATH is non-NULL absolute file name as resolved
     via gdb_realpath from NAME.

     If a match is found, the "partial" symbol table is expanded.
     Then, this calls iterate_over_some_symtabs (or equivalent) over
     all newly-created symbol tables, passing CALLBACK to it.
     The result of this call is returned.  */
  bool (*map_symtabs_matching_filename)
    (struct objfile *objfile, const char *name, const char *real_path,
     gdb::function_view<bool (symtab *)> callback);

  /* Check to see if the symbol is defined in a "partial" symbol table
     of OBJFILE.  BLOCK_INDEX should be either GLOBAL_BLOCK or STATIC_BLOCK,
     depending on whether we want to search global symbols or static
     symbols.  NAME is the name of the symbol to look for.  DOMAIN
     indicates what sort of symbol to search for.

     Returns the newly-expanded compunit in which the symbol is
     defined, or NULL if no such symbol table exists.  If OBJFILE
     contains !TYPE_OPAQUE symbol prefer its compunit.  If it contains
     only TYPE_OPAQUE symbol(s), return at least that compunit.  */
  struct compunit_symtab *(*lookup_symbol) (struct objfile *objfile,
					    block_enum block_index,
					    const char *name,
					    domain_enum domain);

  /* Check to see if the global symbol is defined in a "partial" symbol table
     of OBJFILE. NAME is the name of the symbol to look for.  DOMAIN
     indicates what sort of symbol to search for.

     If found, sets *symbol_found_p to true and returns the symbol language.
     defined, or NULL if no such symbol table exists.  */
  enum language (*lookup_global_symbol_language) (struct objfile *objfile,
						  const char *name,
						  domain_enum domain,
						  bool *symbol_found_p);

  /* Print statistics about any indices loaded for OBJFILE.  The
     statistics should be printed to gdb_stdout.  This is used for
     "maint print statistics".  */
  void (*print_stats) (struct objfile *objfile);

  /* Dump any indices loaded for OBJFILE.  The dump should go to
     gdb_stdout.  This is used for "maint print objfiles".  */
  void (*dump) (struct objfile *objfile);

  /* Find all the symbols in OBJFILE named FUNC_NAME, and ensure that
     the corresponding symbol tables are loaded.  */
  void (*expand_symtabs_for_function) (struct objfile *objfile,
				       const char *func_name);

  /* Read all symbol tables associated with OBJFILE.  */
  void (*expand_all_symtabs) (struct objfile *objfile);

  /* Read all symbol tables associated with OBJFILE which have
     symtab_to_fullname equal to FULLNAME.
     This is for the purposes of examining code only, e.g., expand_line_sal.
     The routine may ignore debug info that is known to not be useful with
     code, e.g., DW_TAG_type_unit for dwarf debug info.  */
  void (*expand_symtabs_with_fullname) (struct objfile *objfile,
					const char *fullname);

  /* Find global or static symbols in all tables that are in DOMAIN
     and for which MATCH (symbol name, NAME) == 0, passing each to 
     CALLBACK, reading in partial symbol tables as needed.  Look
     through global symbols if GLOBAL and otherwise static symbols.
     Passes NAME and NAMESPACE to CALLBACK with each symbol
     found.  After each block is processed, passes NULL to CALLBACK.
     MATCH must be weaker than strcmp_iw_ordered in the sense that
     strcmp_iw_ordered(x,y) == 0 --> MATCH(x,y) == 0.  ORDERED_COMPARE,
     if non-null, must be an ordering relation compatible with
     strcmp_iw_ordered in the sense that
            strcmp_iw_ordered(x,y) == 0 --> ORDERED_COMPARE(x,y) == 0
     and 
            strcmp_iw_ordered(x,y) <= 0 --> ORDERED_COMPARE(x,y) <= 0
     (allowing strcmp_iw_ordered(x,y) < 0 while ORDERED_COMPARE(x, y) == 0).
     CALLBACK returns true to indicate that the scan should continue, or
     false to indicate that the scan should be terminated.  */

  void (*map_matching_symbols)
    (struct objfile *,
     const lookup_name_info &lookup_name,
     domain_enum domain,
     int global,
     gdb::function_view<symbol_found_callback_ftype> callback,
     symbol_compare_ftype *ordered_compare);

  /* Expand all symbol tables in OBJFILE matching some criteria.

     FILE_MATCHER is called for each file in OBJFILE.  The file name
     is passed to it.  If the matcher returns false, the file is
     skipped.  If FILE_MATCHER is NULL the file is not skipped.  If
     BASENAMES is true the matcher should consider only file base
     names (the passed file name is already only the lbasename'd
     part).

     If the file is not skipped, and SYMBOL_MATCHER and LOOKUP_NAME are NULL,
     the symbol table is expanded.

     Otherwise, individual symbols are considered.

     If KIND does not match, the symbol is skipped.

     If the symbol name does not match LOOKUP_NAME, the symbol is skipped.

     If SYMBOL_MATCHER returns false, then the symbol is skipped.

     Otherwise, the symbol's symbol table is expanded.  */
  void (*expand_symtabs_matching)
    (struct objfile *objfile,
     gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
     const lookup_name_info *lookup_name,
     gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
     gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
     enum search_domain kind);

  /* Return the comp unit from OBJFILE that contains PC and
     SECTION.  Return NULL if there is no such compunit.  This
     should return the compunit that contains a symbol whose
     address exactly matches PC, or, if there is no exact match, the
     compunit that contains a symbol whose address is closest to
     PC.  */
  struct compunit_symtab *(*find_pc_sect_compunit_symtab)
    (struct objfile *objfile, struct bound_minimal_symbol msymbol,
     CORE_ADDR pc, struct obj_section *section, int warn_if_readin);

  /* Return the comp unit from OBJFILE that contains a symbol at
     ADDRESS.  Return NULL if there is no such comp unit.  Unlike
     find_pc_sect_compunit_symtab, any sort of symbol (not just text
     symbols) can be considered, and only exact address matches are
     considered.  This pointer may be NULL.  */
  struct compunit_symtab *(*find_compunit_symtab_by_address)
    (struct objfile *objfile, CORE_ADDR address);

  /* Call a callback for every file defined in OBJFILE whose symtab is
     not already read in.  FUN is the callback.  It is passed the file's
     FILENAME, the file's FULLNAME (if need_fullname is non-zero), and
     the DATA passed to this function.  */
  void (*map_symbol_filenames) (struct objfile *objfile,
				symbol_filename_ftype *fun, void *data,
				int need_fullname);
};

/* Structure of functions used for probe support.  If one of these functions
   is provided, all must be.  */

struct sym_probe_fns
{
  /* If non-NULL, return a reference to vector of probe objects.  */
  const std::vector<std::unique_ptr<probe>> &(*sym_get_probes)
    (struct objfile *);
};

/* Structure to keep track of symbol reading functions for various
   object file types.  */

struct sym_fns
{
  /* Initializes anything that is global to the entire symbol table.
     It is called during symbol_file_add, when we begin debugging an
     entirely new program.  */

  void (*sym_new_init) (struct objfile *);

  /* Reads any initial information from a symbol file, and initializes
     the struct sym_fns SF in preparation for sym_read().  It is
     called every time we read a symbol file for any reason.  */

  void (*sym_init) (struct objfile *);

  /* sym_read (objfile, symfile_flags) Reads a symbol file into a psymtab
     (or possibly a symtab).  OBJFILE is the objfile struct for the
     file we are reading.  SYMFILE_FLAGS are the flags passed to
     symbol_file_add & co.  */

  void (*sym_read) (struct objfile *, symfile_add_flags);

  /* Read the partial symbols for an objfile.  This may be NULL, in which case
     gdb has to check other ways if this objfile has any symbols.  This may
     only be non-NULL if the objfile actually does have debuginfo available.
     */

  void (*sym_read_psymbols) (struct objfile *);

  /* Called when we are finished with an objfile.  Should do all
     cleanup that is specific to the object file format for the
     particular objfile.  */

  void (*sym_finish) (struct objfile *);


  /* This function produces a file-dependent section_offsets
     structure, allocated in the objfile's storage.

     The section_addr_info structure contains the offset of loadable and
     allocated sections, relative to the absolute offsets found in the BFD.  */

  void (*sym_offsets) (struct objfile *, const section_addr_info &);

  /* This function produces a format-independent description of
     the segments of ABFD.  Each segment is a unit of the file
     which may be relocated independently.  */

  symfile_segment_data_up (*sym_segments) (bfd *abfd);

  /* This function should read the linetable from the objfile when
     the line table cannot be read while processing the debugging
     information.  */

  void (*sym_read_linetable) (struct objfile *);

  /* Relocate the contents of a debug section SECTP.  The
     contents are stored in BUF if it is non-NULL, or returned in a
     malloc'd buffer otherwise.  */

  bfd_byte *(*sym_relocate) (struct objfile *, asection *sectp, bfd_byte *buf);

  /* If non-NULL, this objfile has probe support, and all the probe
     functions referred to here will be non-NULL.  */
  const struct sym_probe_fns *sym_probe_fns;

  /* The "quick" (aka partial) symbol functions for this symbol
     reader.  */
  const struct quick_symbol_functions *qf;
};

extern section_addr_info
  build_section_addr_info_from_objfile (const struct objfile *objfile);

extern void relative_addr_info_to_section_offsets
  (section_offsets &section_offsets, const section_addr_info &addrs);

extern void addr_info_make_relative (section_addr_info *addrs,
				     bfd *abfd);

/* The default version of sym_fns.sym_offsets for readers that don't
   do anything special.  */

extern void default_symfile_offsets (struct objfile *objfile,
				     const section_addr_info &);

/* The default version of sym_fns.sym_segments for readers that don't
   do anything special.  */

extern symfile_segment_data_up default_symfile_segments (bfd *abfd);

/* The default version of sym_fns.sym_relocate for readers that don't
   do anything special.  */

extern bfd_byte *default_symfile_relocate (struct objfile *objfile,
                                           asection *sectp, bfd_byte *buf);

extern struct symtab *allocate_symtab (struct compunit_symtab *, const char *)
  ATTRIBUTE_NONNULL (1);

extern struct compunit_symtab *allocate_compunit_symtab (struct objfile *,
							 const char *)
  ATTRIBUTE_NONNULL (1);

extern void add_compunit_symtab_to_objfile (struct compunit_symtab *cu);

extern void add_symtab_fns (enum bfd_flavour flavour, const struct sym_fns *);

extern void clear_symtab_users (symfile_add_flags add_flags);

extern enum language deduce_language_from_filename (const char *);

/* Map the filename extension EXT to the language LANG.  Any previous
   association of EXT will be removed.  EXT will be copied by this
   function.  */
extern void add_filename_language (const char *ext, enum language lang);

extern struct objfile *symbol_file_add (const char *, symfile_add_flags,
					section_addr_info *, objfile_flags);

extern struct objfile *symbol_file_add_from_bfd (bfd *, const char *, symfile_add_flags,
						 section_addr_info *,
                                                 objfile_flags, struct objfile *parent);

extern void symbol_file_add_separate (bfd *, const char *, symfile_add_flags,
				      struct objfile *);

extern std::string find_separate_debug_file_by_debuglink (struct objfile *);

/* Build (allocate and populate) a section_addr_info struct from an
   existing section table.  */

extern section_addr_info
   build_section_addr_info_from_section_table (const struct target_section
					       *start,
					       const struct target_section
					       *end);

			/*   Variables   */

/* If true, shared library symbols will be added automatically
   when the inferior is created, new libraries are loaded, or when
   attaching to the inferior.  This is almost always what users will
   want to have happen; but for very large programs, the startup time
   will be excessive, and so if this is a problem, the user can clear
   this flag and then add the shared library symbols as needed.  Note
   that there is a potential for confusion, since if the shared
   library symbols are not loaded, commands like "info fun" will *not*
   report all the functions that are actually present.  */

extern bool auto_solib_add;

/* From symfile.c */

extern void set_initial_language (void);

extern void find_lowest_section (bfd *, asection *, void *);

extern gdb_bfd_ref_ptr symfile_bfd_open (const char *);

extern int get_section_index (struct objfile *, const char *);

extern int print_symbol_loading_p (int from_tty, int mainline, int full);

/* Utility functions for overlay sections: */
extern enum overlay_debugging_state
{
  ovly_off,
  ovly_on,
  ovly_auto
} overlay_debugging;
extern int overlay_cache_invalid;

/* Return the "mapped" overlay section containing the PC.  */
extern struct obj_section *find_pc_mapped_section (CORE_ADDR);

/* Return any overlay section containing the PC (even in its LMA
   region).  */
extern struct obj_section *find_pc_overlay (CORE_ADDR);

/* Return true if the section is an overlay.  */
extern int section_is_overlay (struct obj_section *);

/* Return true if the overlay section is currently "mapped".  */
extern int section_is_mapped (struct obj_section *);

/* Return true if pc belongs to section's VMA.  */
extern CORE_ADDR pc_in_mapped_range (CORE_ADDR, struct obj_section *);

/* Return true if pc belongs to section's LMA.  */
extern CORE_ADDR pc_in_unmapped_range (CORE_ADDR, struct obj_section *);

/* Map an address from a section's LMA to its VMA.  */
extern CORE_ADDR overlay_mapped_address (CORE_ADDR, struct obj_section *);

/* Map an address from a section's VMA to its LMA.  */
extern CORE_ADDR overlay_unmapped_address (CORE_ADDR, struct obj_section *);

/* Convert an address in an overlay section (force into VMA range).  */
extern CORE_ADDR symbol_overlayed_address (CORE_ADDR, struct obj_section *);

/* Load symbols from a file.  */
extern void symbol_file_add_main (const char *args,
				  symfile_add_flags add_flags);

/* Clear GDB symbol tables.  */
extern void symbol_file_clear (int from_tty);

/* Default overlay update function.  */
extern void simple_overlay_update (struct obj_section *);

extern bfd_byte *symfile_relocate_debug_section (struct objfile *, asection *,
						 bfd_byte *);

extern int symfile_map_offsets_to_segments (bfd *,
					    const struct symfile_segment_data *,
					    section_offsets &,
					    int, const CORE_ADDR *);
symfile_segment_data_up get_symfile_segment_data (bfd *abfd);

extern scoped_restore_tmpl<int> increment_reading_symtab (void);

void expand_symtabs_matching
  (gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   const lookup_name_info &lookup_name,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
   enum search_domain kind);

void map_symbol_filenames (symbol_filename_ftype *fun, void *data,
			   int need_fullname);

/* Target-agnostic function to load the sections of an executable into memory.

   ARGS should be in the form "EXECUTABLE [OFFSET]", where OFFSET is an
   optional offset to apply to each section.  */
extern void generic_load (const char *args, int from_tty);

/* From dwarf2read.c */

/* Names for a dwarf2 debugging section.  The field NORMAL is the normal
   section name (usually from the DWARF standard), while the field COMPRESSED
   is the name of compressed sections.  If your object file format doesn't
   support compressed sections, the field COMPRESSED can be NULL.  Likewise,
   the debugging section is not supported, the field NORMAL can be NULL too.
   It doesn't make sense to have a NULL NORMAL field but a non-NULL COMPRESSED
   field.  */

struct dwarf2_section_names {
  const char *normal;
  const char *compressed;
};

/* List of names for dward2 debugging sections.  Also most object file formats
   use the standardized (ie ELF) names, some (eg XCOFF) have customized names
   due to restrictions.
   The table for the standard names is defined in dwarf2read.c.  Please
   update all instances of dwarf2_debug_sections if you add a field to this
   structure.  It is always safe to use { NULL, NULL } in this case.  */

struct dwarf2_debug_sections {
  struct dwarf2_section_names info;
  struct dwarf2_section_names abbrev;
  struct dwarf2_section_names line;
  struct dwarf2_section_names loc;
  struct dwarf2_section_names loclists;
  struct dwarf2_section_names macinfo;
  struct dwarf2_section_names macro;
  struct dwarf2_section_names str;
  struct dwarf2_section_names str_offsets;
  struct dwarf2_section_names line_str;
  struct dwarf2_section_names ranges;
  struct dwarf2_section_names rnglists;
  struct dwarf2_section_names types;
  struct dwarf2_section_names addr;
  struct dwarf2_section_names frame;
  struct dwarf2_section_names eh_frame;
  struct dwarf2_section_names gdb_index;
  struct dwarf2_section_names debug_names;
  struct dwarf2_section_names debug_aranges;
  /* This field has no meaning, but exists solely to catch changes to
     this structure which are not reflected in some instance.  */
  int sentinel;
};

extern int dwarf2_has_info (struct objfile *,
                            const struct dwarf2_debug_sections *,
			    bool = false);

/* Dwarf2 sections that can be accessed by dwarf2_get_section_info.  */
enum dwarf2_section_enum {
  DWARF2_DEBUG_FRAME,
  DWARF2_EH_FRAME
};

extern void dwarf2_get_section_info (struct objfile *,
                                     enum dwarf2_section_enum,
				     asection **, const gdb_byte **,
				     bfd_size_type *);

/* A DWARF names index variant.  */
enum class dw_index_kind
{
  /* GDB's own .gdb_index format.   */
  GDB_INDEX,

  /* DWARF5 .debug_names.  */
  DEBUG_NAMES,
};

/* Initialize for reading DWARF for OBJFILE.  Return false if this
   file will use psymtabs, or true if using an index, in which case
   *INDEX_KIND is set to the index variant in use.  */
extern bool dwarf2_initialize_objfile (struct objfile *objfile,
				       dw_index_kind *index_kind);

extern void dwarf2_build_psymtabs (struct objfile *);
extern void dwarf2_build_frame_info (struct objfile *);

/* From minidebug.c.  */

extern gdb_bfd_ref_ptr find_separate_debug_file_in_section (struct objfile *);

/* True if we are printing debug output about separate debug info files.  */

extern bool separate_debug_file_debug;

#endif /* !defined(SYMFILE_H) */
