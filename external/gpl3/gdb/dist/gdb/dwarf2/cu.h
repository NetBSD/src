/* DWARF CU data structure

   Copyright (C) 2021-2025 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_CU_H
#define GDB_DWARF2_CU_H

#include "buildsym.h"
#include "dwarf2/unit-head.h"
#include <optional>
#include "language.h"
#include "gdbsupport/unordered_set.h"
#include "dwarf2/die.h"

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

/* Internal state when decoding a particular compilation unit.  */
struct dwarf2_cu
{
  explicit dwarf2_cu (dwarf2_per_cu *per_cu, dwarf2_per_objfile *per_objfile);

  DISABLE_COPY_AND_ASSIGN (dwarf2_cu);

  /* The section the DIEs were effectively read from.  This could be
     .debug_info, .debug_types, or with split DWARF, their .dwo
     variants.  */
  const dwarf2_section_info &section () const;

  /* TU version of handle_DW_AT_stmt_list for read_type_unit_scope.
     Create the set of symtabs used by this TU, or if this TU is sharing
     symtabs with another TU and the symtabs have already been created
     then restore those symtabs in the line header.
     We don't need the pc/line-number mapping for type units.  */
  void setup_type_unit_groups (struct die_info *die);

  /* Start a compunit_symtab for DWARF.  NAME, COMP_DIR, LOW_PC are passed to
     the buildsym_compunit constructor.  */
  struct compunit_symtab *start_compunit_symtab (const char *name,
						 const char *comp_dir,
						 CORE_ADDR low_pc);

  /* Reset the builder.  */
  void reset_builder () { m_builder.reset (); }

  /* Return a type that is a generic pointer type, the size of which
     matches the address size given in the compilation unit header for
     this CU.  */
  struct type *addr_type () const;

  /* Find an integer type the same size as the address size given in
     the compilation unit header for this CU.  UNSIGNED_P controls if
     the integer is unsigned or not.  */
  struct type *addr_sized_int_type (bool unsigned_p) const;

  /* Mark this CU as used.  */
  void mark ();

  /* Clear the mark on this CU.  */
  void clear_mark ()
  {
    m_mark = false;
  }

  /* True if this CU has been marked.  */
  bool is_marked () const
  {
    return m_mark;
  }

  /* Add a dependence relationship from this cu to REF_PER_CU.  */
  void add_dependence (dwarf2_per_cu *ref_per_cu)
  { m_dependencies.emplace (ref_per_cu); }

  /* Find the DIE at section offset SECT_OFF.

     Return nullptr if not found.  */
  die_info *find_die (sect_offset sect_off) const
  {
    auto it = die_hash.find (sect_off);
    return it != die_hash.end () ? *it : nullptr;
  }

  /* The header of the compilation unit.  */
  struct unit_head header;

  /* Base address of this compilation unit.  */
  std::optional<unrelocated_addr> base_address;

  /* The language we are debugging.  */
  const struct language_defn *language_defn = nullptr;

  enum language lang () const
  {
    gdb_assert (language_defn != language_def (language_unknown));
    return language_defn->la_language;
  }

  /* Set the producer string and check various properties of it.  */
  void set_producer (const char *producer);

  /* Check for GCC PR debug/45124 fix which is not present in any G++
     version up to 4.5.any while it is present already in G++ 4.6.0 -
     the PR has been fixed during 4.6.0 experimental.  */
  bool producer_is_gxx_lt_4_6 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gxx_lt_4_6;
  }

  /* Check for GCC < 4.5.x.  */
  bool producer_is_gcc_lt_4_5 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gcc_lt_4_5;
  }

  /* Codewarrior (at least as of version 5.0.40) generates dwarf line
     information with incorrect is_stmt attributes.  */
  bool producer_is_codewarrior () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_codewarrior;
  }

  bool producer_is_gas_lt_2_38 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gas_lt_2_38;
  }

  bool producer_is_gas_2_39 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gas_2_39;
  }

  /* Return true if CU is produced by GAS 2.39 or later.  */
  bool producer_is_gas_ge_2_39 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gas_2_39 || m_producer_is_gas_ge_2_40;
  }

  /* ICC<14 does not output the required DW_AT_declaration on
     incomplete types, but gives them a size of zero.  Starting with
     version 14, ICC is compatible with GCC.  */
  bool producer_is_icc_lt_14 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_icc_lt_14;
  }

  /* ICC generates a DW_AT_type for C void functions.  This was
     observed on ICC 14.0.5.212, and appears to be against the DWARF
     spec (V5 3.3.2) which says that void functions should not have a
     DW_AT_type.  */
  bool producer_is_icc () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_icc;
  }

  /* Check for possibly missing DW_AT_comp_dir with relative
     .debug_line directory paths.  GCC SVN r127613 (new option
     -fdebug-prefix-map) fixed this, it was first present in GCC
     release 4.3.0.  */
  bool producer_is_gcc_lt_4_3 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gcc_lt_4_3;
  }

  /* Check for GCC 4 or later.  */
  bool producer_is_gcc_ge_4 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gcc_ge_4;
  }

  /* Check for GCC 11 exactly.  */
  bool producer_is_gcc_11 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gcc_11;
  }

  /* Check for any version of GCC.  */
  bool producer_is_gcc () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gcc;
  }

  /* Return true if the producer of the inferior is clang.  */
  bool producer_is_clang () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_clang;
  }

  /* Return true if the producer is RealView.  */
  bool producer_is_realview () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_realview;
  }

  /* Return true if the producer is IBM XLC.  */
  bool producer_is_xlc () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_xlc;
  }

  /* Return true if the producer is IBM XLC for OpenCL.  */
  bool producer_is_xlc_opencl () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_xlc_opencl;
  }

  /* Return true if the producer is GNU F77.  */
  bool producer_is_gf77 () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_gf77;
  }

  /* Return true if the producer is GNU Go.  */
  bool producer_is_ggo () const
  {
    gdb_assert (m_checked_producer);
    return m_producer_is_ggo;
  }

  /* Return the producer.  In general this should not be used and
     instead new checks should be added to set_producer.  */
  const char *get_producer () const
  {
    gdb_assert (m_checked_producer);
    return m_producer;
  }

private:
  const char *m_producer = nullptr;

  /* The symtab builder for this CU.  This is only non-NULL when full
     symbols are being read.  */
  buildsym_compunit_up m_builder;

  /* A set of pointers to dwarf2_per_cu objects for compilation units referenced
     by this one.  Only used during full symbol processing; partial symbol
     tables do not have dependencies.  */
  gdb::unordered_set<dwarf2_per_cu *> m_dependencies;

public:
  /* The generic symbol table building routines have separate lists for
     file scope symbols and all all other scopes (local scopes).  So
     we need to select the right one to pass to add_symbol_to_list().
     We do it by keeping a pointer to the correct list in list_in_scope.

     FIXME: The original dwarf code just treated the file scope as the
     first local scope, and all other local scopes as nested local
     scopes, and worked fine.  Check to see if we really need to
     distinguish these in buildsym.c.  */
  struct pending **list_in_scope = nullptr;

  /* Storage for things with the same lifetime as this read-in
     compilation unit. */
  auto_obstack comp_unit_obstack;

  /* Backlink to our per_cu entry.  */
  dwarf2_per_cu *per_cu;

  /* The dwarf2_per_objfile that owns this.  */
  dwarf2_per_objfile *per_objfile;

  /* How many compilation units ago was this CU last referenced?  */
  int last_used = 0;

  /* A hash table of DIE cu_offset for following references with
     die_info->offset.sect_off as hash.  */
  using die_hash_t = gdb::unordered_set<die_info *, die_info_hash_sect_off,
					die_info_eq_sect_off>;
  die_hash_t die_hash;

  /* Full DIEs if read in.  */
  struct die_info *dies = nullptr;

  /* Header data from the line table, during full symbol processing.  */
  struct line_header *line_header = nullptr;
  /* Non-NULL if LINE_HEADER is owned by this DWARF_CU.  Otherwise,
     it's owned by dwarf2_per_bfd::line_header_hash.  If non-NULL,
     this is the DW_TAG_compile_unit die for this CU.  We'll hold on
     to the line header as long as this DIE is being processed.  See
     process_die_scope.  */
  die_info *line_header_die_owner = nullptr;

  /* A list of methods which need to have physnames computed
     after all type information has been read.  */
  std::vector<delayed_method_info> method_list;

  /* To be copied to symtab->call_site_htab.  */
  call_site_htab_t call_site_htab;

  /* Non-NULL if this CU came from a DWO file.
     There is an invariant here that is important to remember:
     Except for attributes copied from the top level DIE in the "main"
     (or "stub") file in preparation for reading the DWO file
     (e.g., DW_AT_addr_base), we KISS: there is only *one* CU.
     Either there isn't a DWO file (in which case this is NULL and the point
     is moot), or there is and either we're not going to read it (in which
     case this is NULL) or there is and we are reading it (in which case this
     is non-NULL).  */
  struct dwo_unit *dwo_unit = nullptr;

  /* The DW_AT_addr_base (DW_AT_GNU_addr_base) attribute if present.
     Note this value comes from the Fission stub CU/TU's DIE.  */
  std::optional<ULONGEST> addr_base;

  /* The DW_AT_GNU_ranges_base attribute, if present.

     This is only relevant in the context of pre-DWARF 5 split units.  In this
     context, there is a .debug_ranges section in the linked executable,
     containing all the ranges data for all the compilation units.  Each
     skeleton/stub unit has (if needed) a DW_AT_GNU_ranges_base attribute that
     indicates the base of its contribution to that section.  The DW_AT_ranges
     attributes in the split-unit are of the form DW_FORM_sec_offset and point
     into the .debug_ranges section of the linked file.  However, they are not
     "true" DW_FORM_sec_offset, because they are relative to the base of their
     compilation unit's contribution, rather than relative to the beginning of
     the section.  The DW_AT_GNU_ranges_base value must be added to it to make
     it relative to the beginning of the section.

     Note that the value is zero when we are not in a pre-DWARF 5 split-unit
     case, so this value can be added without needing to know whether we are in
     this case or not.

     N.B. If a DW_AT_ranges attribute is found on the DW_TAG_compile_unit in the
     skeleton/stub, it must not have the base added, as it already points to the
     right place.  And since the DW_TAG_compile_unit DIE in the split-unit can't
     have a DW_AT_ranges attribute, we can use the

       die->tag != DW_TAG_compile_unit

     to determine whether the base should be added or not.  */
  ULONGEST gnu_ranges_base = 0;

  /* The DW_AT_rnglists_base attribute, if present.

     This is used when processing attributes of form DW_FORM_rnglistx in
     non-split units.  Attributes of this form found in a split unit don't
     use it, as split-unit files have their own non-shared .debug_rnglists.dwo
     section.  */
  ULONGEST rnglists_base = 0;

  /* The DW_AT_loclists_base attribute if present.  */
  ULONGEST loclist_base = 0;

  /* When reading debug info generated by older versions of rustc, we
     have to rewrite some union types to be struct types with a
     variant part.  This rewriting must be done after the CU is fully
     read in, because otherwise at the point of rewriting some struct
     type might not have been fully processed.  So, we keep a list of
     all such types here and process them after expansion.  */
  std::vector<struct type *> rust_unions;

  /* The DW_AT_str_offsets_base attribute if present.  For DWARF 4 version DWO
     files, the value is implicitly zero.  For DWARF 5 version DWO files, the
     value is often implicit and is the size of the header of
     .debug_str_offsets section (8 or 4, depending on the address size).  */
  std::optional<ULONGEST> str_offsets_base;

  /* Mark used when releasing cached dies.  */
  bool m_mark : 1;

  /* This CU references .debug_loc.  See the symtab->locations_valid field.
     This test is imperfect as there may exist optimized debug code not using
     any location list and still facing inlining issues if handled as
     unoptimized code.  For a future better test see GCC PR other/32998.  */
  bool has_loclist : 1;

  /* These cache the results for producer_is_* fields.  CHECKED_PRODUCER is true
     if all the producer_is_* fields are valid.  This information is cached
     because profiling CU expansion showed excessive time spent in
     producer_is_gxx_lt_4_6.  */
  bool m_checked_producer : 1;
  bool m_producer_is_gxx_lt_4_6 : 1;
  bool m_producer_is_gcc_lt_4_5 : 1;
  bool m_producer_is_gcc_lt_4_3 : 1;
  bool m_producer_is_gcc_ge_4 : 1;
  bool m_producer_is_gcc_11 : 1;
  bool m_producer_is_gcc : 1;
  bool m_producer_is_icc : 1;
  bool m_producer_is_icc_lt_14 : 1;
  bool m_producer_is_codewarrior : 1;
  bool m_producer_is_clang : 1;
  bool m_producer_is_gas_lt_2_38 : 1;
  bool m_producer_is_gas_2_39 : 1;
  bool m_producer_is_gas_ge_2_40 : 1;
  bool m_producer_is_realview : 1;
  bool m_producer_is_xlc : 1;
  bool m_producer_is_xlc_opencl : 1;
  bool m_producer_is_gf77 : 1;
  bool m_producer_is_ggo : 1;

  /* When true, the file that we're processing is known to have
     debugging info for C++ namespaces.  GCC 3.3.x did not produce
     this information, but later versions do.  */

  bool processing_has_namespace_info : 1;

  /* Get the buildsym_compunit for this CU.  */
  buildsym_compunit *get_builder ();
};

using dwarf2_cu_up = std::unique_ptr<dwarf2_cu>;

#endif /* GDB_DWARF2_CU_H */
