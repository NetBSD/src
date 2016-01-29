/* Generic target-file-type support for the BFD library.
   Copyright (C) 1990-2015 Free Software Foundation, Inc.
   Written by Cygnus Support.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "fnmatch.h"

/*
   It's okay to see some:
#if 0
   directives in this source file, as targets.c uses them to exclude
   certain BFD vectors.  This comment is specially formatted to catch
   users who grep for ^#if 0, so please keep it this way!
*/

/*
SECTION
	Targets

DESCRIPTION
	Each port of BFD to a different machine requires the creation
	of a target back end. All the back end provides to the root
	part of BFD is a structure containing pointers to functions
	which perform certain low level operations on files. BFD
	translates the applications's requests through a pointer into
	calls to the back end routines.

	When a file is opened with <<bfd_openr>>, its format and
	target are unknown. BFD uses various mechanisms to determine
	how to interpret the file. The operations performed are:

	o Create a BFD by calling the internal routine
	<<_bfd_new_bfd>>, then call <<bfd_find_target>> with the
	target string supplied to <<bfd_openr>> and the new BFD pointer.

	o If a null target string was provided to <<bfd_find_target>>,
	look up the environment variable <<GNUTARGET>> and use
	that as the target string.

	o If the target string is still <<NULL>>, or the target string is
	<<default>>, then use the first item in the target vector
	as the target type, and set <<target_defaulted>> in the BFD to
	cause <<bfd_check_format>> to loop through all the targets.
	@xref{bfd_target}.  @xref{Formats}.

	o Otherwise, inspect the elements in the target vector
	one by one, until a match on target name is found. When found,
	use it.

	o Otherwise return the error <<bfd_error_invalid_target>> to
	<<bfd_openr>>.

	o <<bfd_openr>> attempts to open the file using
	<<bfd_open_file>>, and returns the BFD.

	Once the BFD has been opened and the target selected, the file
	format may be determined. This is done by calling
	<<bfd_check_format>> on the BFD with a suggested format.
	If <<target_defaulted>> has been set, each possible target
	type is tried to see if it recognizes the specified format.
	<<bfd_check_format>> returns <<TRUE>> when the caller guesses right.
@menu
@* bfd_target::
@end menu
*/

/*

INODE
	bfd_target,  , Targets, Targets
DOCDD
SUBSECTION
	bfd_target

DESCRIPTION
	This structure contains everything that BFD knows about a
	target. It includes things like its byte order, name, and which
	routines to call to do various operations.

	Every BFD points to a target structure with its <<xvec>>
	member.

	The macros below are used to dispatch to functions through the
	<<bfd_target>> vector. They are used in a number of macros further
	down in @file{bfd.h}, and are also used when calling various
	routines by hand inside the BFD implementation.  The @var{arglist}
	argument must be parenthesized; it contains all the arguments
	to the called function.

	They make the documentation (more) unpleasant to read, so if
	someone wants to fix this and not break the above, please do.

.#define BFD_SEND(bfd, message, arglist) \
.  ((*((bfd)->xvec->message)) arglist)
.
.#ifdef DEBUG_BFD_SEND
.#undef BFD_SEND
.#define BFD_SEND(bfd, message, arglist) \
.  (((bfd) && (bfd)->xvec && (bfd)->xvec->message) ? \
.    ((*((bfd)->xvec->message)) arglist) : \
.    (bfd_assert (__FILE__,__LINE__), NULL))
.#endif

	For operations which index on the BFD format:

.#define BFD_SEND_FMT(bfd, message, arglist) \
.  (((bfd)->xvec->message[(int) ((bfd)->format)]) arglist)
.
.#ifdef DEBUG_BFD_SEND
.#undef BFD_SEND_FMT
.#define BFD_SEND_FMT(bfd, message, arglist) \
.  (((bfd) && (bfd)->xvec && (bfd)->xvec->message) ? \
.   (((bfd)->xvec->message[(int) ((bfd)->format)]) arglist) : \
.   (bfd_assert (__FILE__,__LINE__), NULL))
.#endif
.
	This is the structure which defines the type of BFD this is.  The
	<<xvec>> member of the struct <<bfd>> itself points here.  Each
	module that implements access to a different target under BFD,
	defines one of these.

	FIXME, these names should be rationalised with the names of
	the entry points which call them. Too bad we can't have one
	macro to define them both!

.enum bfd_flavour
.{
.  {* N.B. Update bfd_flavour_name if you change this.  *}
.  bfd_target_unknown_flavour,
.  bfd_target_aout_flavour,
.  bfd_target_coff_flavour,
.  bfd_target_ecoff_flavour,
.  bfd_target_xcoff_flavour,
.  bfd_target_elf_flavour,
.  bfd_target_ieee_flavour,
.  bfd_target_nlm_flavour,
.  bfd_target_oasys_flavour,
.  bfd_target_tekhex_flavour,
.  bfd_target_srec_flavour,
.  bfd_target_verilog_flavour,
.  bfd_target_ihex_flavour,
.  bfd_target_som_flavour,
.  bfd_target_os9k_flavour,
.  bfd_target_versados_flavour,
.  bfd_target_msdos_flavour,
.  bfd_target_ovax_flavour,
.  bfd_target_evax_flavour,
.  bfd_target_mmo_flavour,
.  bfd_target_mach_o_flavour,
.  bfd_target_pef_flavour,
.  bfd_target_pef_xlib_flavour,
.  bfd_target_sym_flavour
.};
.
.enum bfd_endian { BFD_ENDIAN_BIG, BFD_ENDIAN_LITTLE, BFD_ENDIAN_UNKNOWN };
.
.{* Forward declaration.  *}
.typedef struct bfd_link_info _bfd_link_info;
.
.{* Forward declaration.  *}
.typedef struct flag_info flag_info;
.
.typedef struct bfd_target
.{
.  {* Identifies the kind of target, e.g., SunOS4, Ultrix, etc.  *}
.  char *name;
.
. {* The "flavour" of a back end is a general indication about
.    the contents of a file.  *}
.  enum bfd_flavour flavour;
.
.  {* The order of bytes within the data area of a file.  *}
.  enum bfd_endian byteorder;
.
. {* The order of bytes within the header parts of a file.  *}
.  enum bfd_endian header_byteorder;
.
.  {* A mask of all the flags which an executable may have set -
.     from the set <<BFD_NO_FLAGS>>, <<HAS_RELOC>>, ...<<D_PAGED>>.  *}
.  flagword object_flags;
.
. {* A mask of all the flags which a section may have set - from
.    the set <<SEC_NO_FLAGS>>, <<SEC_ALLOC>>, ...<<SET_NEVER_LOAD>>.  *}
.  flagword section_flags;
.
. {* The character normally found at the front of a symbol.
.    (if any), perhaps `_'.  *}
.  char symbol_leading_char;
.
. {* The pad character for file names within an archive header.  *}
.  char ar_pad_char;
.
.  {* The maximum number of characters in an archive header.  *}
.  unsigned char ar_max_namelen;
.
.  {* How well this target matches, used to select between various
.     possible targets when more than one target matches.  *}
.  unsigned char match_priority;
.
.  {* Entries for byte swapping for data. These are different from the
.     other entry points, since they don't take a BFD as the first argument.
.     Certain other handlers could do the same.  *}
.  bfd_uint64_t   (*bfd_getx64) (const void *);
.  bfd_int64_t    (*bfd_getx_signed_64) (const void *);
.  void           (*bfd_putx64) (bfd_uint64_t, void *);
.  bfd_vma        (*bfd_getx32) (const void *);
.  bfd_signed_vma (*bfd_getx_signed_32) (const void *);
.  void           (*bfd_putx32) (bfd_vma, void *);
.  bfd_vma        (*bfd_getx16) (const void *);
.  bfd_signed_vma (*bfd_getx_signed_16) (const void *);
.  void           (*bfd_putx16) (bfd_vma, void *);
.
.  {* Byte swapping for the headers.  *}
.  bfd_uint64_t   (*bfd_h_getx64) (const void *);
.  bfd_int64_t    (*bfd_h_getx_signed_64) (const void *);
.  void           (*bfd_h_putx64) (bfd_uint64_t, void *);
.  bfd_vma        (*bfd_h_getx32) (const void *);
.  bfd_signed_vma (*bfd_h_getx_signed_32) (const void *);
.  void           (*bfd_h_putx32) (bfd_vma, void *);
.  bfd_vma        (*bfd_h_getx16) (const void *);
.  bfd_signed_vma (*bfd_h_getx_signed_16) (const void *);
.  void           (*bfd_h_putx16) (bfd_vma, void *);
.
.  {* Format dependent routines: these are vectors of entry points
.     within the target vector structure, one for each format to check.  *}
.
.  {* Check the format of a file being read.  Return a <<bfd_target *>> or zero.  *}
.  const struct bfd_target *(*_bfd_check_format[bfd_type_end]) (bfd *);
.
.  {* Set the format of a file being written.  *}
.  bfd_boolean (*_bfd_set_format[bfd_type_end]) (bfd *);
.
.  {* Write cached information into a file being written, at <<bfd_close>>.  *}
.  bfd_boolean (*_bfd_write_contents[bfd_type_end]) (bfd *);
.
The general target vector.  These vectors are initialized using the
BFD_JUMP_TABLE macros.
.
.  {* Generic entry points.  *}
.#define BFD_JUMP_TABLE_GENERIC(NAME) \
.  NAME##_close_and_cleanup, \
.  NAME##_bfd_free_cached_info, \
.  NAME##_new_section_hook, \
.  NAME##_get_section_contents, \
.  NAME##_get_section_contents_in_window
.
.  {* Called when the BFD is being closed to do any necessary cleanup.  *}
.  bfd_boolean (*_close_and_cleanup) (bfd *);
.  {* Ask the BFD to free all cached information.  *}
.  bfd_boolean (*_bfd_free_cached_info) (bfd *);
.  {* Called when a new section is created.  *}
.  bfd_boolean (*_new_section_hook) (bfd *, sec_ptr);
.  {* Read the contents of a section.  *}
.  bfd_boolean (*_bfd_get_section_contents)
.    (bfd *, sec_ptr, void *, file_ptr, bfd_size_type);
.  bfd_boolean (*_bfd_get_section_contents_in_window)
.    (bfd *, sec_ptr, bfd_window *, file_ptr, bfd_size_type);
.
.  {* Entry points to copy private data.  *}
.#define BFD_JUMP_TABLE_COPY(NAME) \
.  NAME##_bfd_copy_private_bfd_data, \
.  NAME##_bfd_merge_private_bfd_data, \
.  _bfd_generic_init_private_section_data, \
.  NAME##_bfd_copy_private_section_data, \
.  NAME##_bfd_copy_private_symbol_data, \
.  NAME##_bfd_copy_private_header_data, \
.  NAME##_bfd_set_private_flags, \
.  NAME##_bfd_print_private_bfd_data
.
.  {* Called to copy BFD general private data from one object file
.     to another.  *}
.  bfd_boolean (*_bfd_copy_private_bfd_data) (bfd *, bfd *);
.  {* Called to merge BFD general private data from one object file
.     to a common output file when linking.  *}
.  bfd_boolean (*_bfd_merge_private_bfd_data) (bfd *, bfd *);
.  {* Called to initialize BFD private section data from one object file
.     to another.  *}
.#define bfd_init_private_section_data(ibfd, isec, obfd, osec, link_info) \
.  BFD_SEND (obfd, _bfd_init_private_section_data, (ibfd, isec, obfd, osec, link_info))
.  bfd_boolean (*_bfd_init_private_section_data)
.    (bfd *, sec_ptr, bfd *, sec_ptr, struct bfd_link_info *);
.  {* Called to copy BFD private section data from one object file
.     to another.  *}
.  bfd_boolean (*_bfd_copy_private_section_data)
.    (bfd *, sec_ptr, bfd *, sec_ptr);
.  {* Called to copy BFD private symbol data from one symbol
.     to another.  *}
.  bfd_boolean (*_bfd_copy_private_symbol_data)
.    (bfd *, asymbol *, bfd *, asymbol *);
.  {* Called to copy BFD private header data from one object file
.     to another.  *}
.  bfd_boolean (*_bfd_copy_private_header_data)
.    (bfd *, bfd *);
.  {* Called to set private backend flags.  *}
.  bfd_boolean (*_bfd_set_private_flags) (bfd *, flagword);
.
.  {* Called to print private BFD data.  *}
.  bfd_boolean (*_bfd_print_private_bfd_data) (bfd *, void *);
.
.  {* Core file entry points.  *}
.#define BFD_JUMP_TABLE_CORE(NAME) \
.  NAME##_core_file_failing_command, \
.  NAME##_core_file_failing_signal, \
.  NAME##_core_file_matches_executable_p, \
.  NAME##_core_file_pid
.
.  char *      (*_core_file_failing_command) (bfd *);
.  int         (*_core_file_failing_signal) (bfd *);
.  bfd_boolean (*_core_file_matches_executable_p) (bfd *, bfd *);
.  int         (*_core_file_pid) (bfd *);
.
.  {* Archive entry points.  *}
.#define BFD_JUMP_TABLE_ARCHIVE(NAME) \
.  NAME##_slurp_armap, \
.  NAME##_slurp_extended_name_table, \
.  NAME##_construct_extended_name_table, \
.  NAME##_truncate_arname, \
.  NAME##_write_armap, \
.  NAME##_read_ar_hdr, \
.  NAME##_write_ar_hdr, \
.  NAME##_openr_next_archived_file, \
.  NAME##_get_elt_at_index, \
.  NAME##_generic_stat_arch_elt, \
.  NAME##_update_armap_timestamp
.
.  bfd_boolean (*_bfd_slurp_armap) (bfd *);
.  bfd_boolean (*_bfd_slurp_extended_name_table) (bfd *);
.  bfd_boolean (*_bfd_construct_extended_name_table)
.    (bfd *, char **, bfd_size_type *, const char **);
.  void        (*_bfd_truncate_arname) (bfd *, const char *, char *);
.  bfd_boolean (*write_armap)
.    (bfd *, unsigned int, struct orl *, unsigned int, int);
.  void *      (*_bfd_read_ar_hdr_fn) (bfd *);
.  bfd_boolean (*_bfd_write_ar_hdr_fn) (bfd *, bfd *);
.  bfd *       (*openr_next_archived_file) (bfd *, bfd *);
.#define bfd_get_elt_at_index(b,i) BFD_SEND (b, _bfd_get_elt_at_index, (b,i))
.  bfd *       (*_bfd_get_elt_at_index) (bfd *, symindex);
.  int         (*_bfd_stat_arch_elt) (bfd *, struct stat *);
.  bfd_boolean (*_bfd_update_armap_timestamp) (bfd *);
.
.  {* Entry points used for symbols.  *}
.#define BFD_JUMP_TABLE_SYMBOLS(NAME) \
.  NAME##_get_symtab_upper_bound, \
.  NAME##_canonicalize_symtab, \
.  NAME##_make_empty_symbol, \
.  NAME##_print_symbol, \
.  NAME##_get_symbol_info, \
.  NAME##_get_symbol_version_string, \
.  NAME##_bfd_is_local_label_name, \
.  NAME##_bfd_is_target_special_symbol, \
.  NAME##_get_lineno, \
.  NAME##_find_nearest_line, \
.  NAME##_find_line, \
.  NAME##_find_inliner_info, \
.  NAME##_bfd_make_debug_symbol, \
.  NAME##_read_minisymbols, \
.  NAME##_minisymbol_to_symbol
.
.  long        (*_bfd_get_symtab_upper_bound) (bfd *);
.  long        (*_bfd_canonicalize_symtab)
.    (bfd *, struct bfd_symbol **);
.  struct bfd_symbol *
.              (*_bfd_make_empty_symbol) (bfd *);
.  void        (*_bfd_print_symbol)
.    (bfd *, void *, struct bfd_symbol *, bfd_print_symbol_type);
.#define bfd_print_symbol(b,p,s,e) BFD_SEND (b, _bfd_print_symbol, (b,p,s,e))
.  void        (*_bfd_get_symbol_info)
.    (bfd *, struct bfd_symbol *, symbol_info *);
.#define bfd_get_symbol_info(b,p,e) BFD_SEND (b, _bfd_get_symbol_info, (b,p,e))
.  const char *(*_bfd_get_symbol_version_string)
.    (bfd *, struct bfd_symbol *, bfd_boolean *);
.#define bfd_get_symbol_version_string(b,s,h) BFD_SEND (b, _bfd_get_symbol_version_string, (b,s,h))
.  bfd_boolean (*_bfd_is_local_label_name) (bfd *, const char *);
.  bfd_boolean (*_bfd_is_target_special_symbol) (bfd *, asymbol *);
.  alent *     (*_get_lineno) (bfd *, struct bfd_symbol *);
.  bfd_boolean (*_bfd_find_nearest_line)
.    (bfd *, struct bfd_symbol **, struct bfd_section *, bfd_vma,
.     const char **, const char **, unsigned int *, unsigned int *);
.  bfd_boolean (*_bfd_find_line)
.    (bfd *, struct bfd_symbol **, struct bfd_symbol *,
.     const char **, unsigned int *);
.  bfd_boolean (*_bfd_find_inliner_info)
.    (bfd *, const char **, const char **, unsigned int *);
. {* Back-door to allow format-aware applications to create debug symbols
.    while using BFD for everything else.  Currently used by the assembler
.    when creating COFF files.  *}
.  asymbol *   (*_bfd_make_debug_symbol)
.    (bfd *, void *, unsigned long size);
.#define bfd_read_minisymbols(b, d, m, s) \
.  BFD_SEND (b, _read_minisymbols, (b, d, m, s))
.  long        (*_read_minisymbols)
.    (bfd *, bfd_boolean, void **, unsigned int *);
.#define bfd_minisymbol_to_symbol(b, d, m, f) \
.  BFD_SEND (b, _minisymbol_to_symbol, (b, d, m, f))
.  asymbol *   (*_minisymbol_to_symbol)
.    (bfd *, bfd_boolean, const void *, asymbol *);
.
.  {* Routines for relocs.  *}
.#define BFD_JUMP_TABLE_RELOCS(NAME) \
.  NAME##_get_reloc_upper_bound, \
.  NAME##_canonicalize_reloc, \
.  NAME##_bfd_reloc_type_lookup, \
.  NAME##_bfd_reloc_name_lookup
.
.  long        (*_get_reloc_upper_bound) (bfd *, sec_ptr);
.  long        (*_bfd_canonicalize_reloc)
.    (bfd *, sec_ptr, arelent **, struct bfd_symbol **);
.  {* See documentation on reloc types.  *}
.  reloc_howto_type *
.              (*reloc_type_lookup) (bfd *, bfd_reloc_code_real_type);
.  reloc_howto_type *
.              (*reloc_name_lookup) (bfd *, const char *);
.
.
.  {* Routines used when writing an object file.  *}
.#define BFD_JUMP_TABLE_WRITE(NAME) \
.  NAME##_set_arch_mach, \
.  NAME##_set_section_contents
.
.  bfd_boolean (*_bfd_set_arch_mach)
.    (bfd *, enum bfd_architecture, unsigned long);
.  bfd_boolean (*_bfd_set_section_contents)
.    (bfd *, sec_ptr, const void *, file_ptr, bfd_size_type);
.
.  {* Routines used by the linker.  *}
.#define BFD_JUMP_TABLE_LINK(NAME) \
.  NAME##_sizeof_headers, \
.  NAME##_bfd_get_relocated_section_contents, \
.  NAME##_bfd_relax_section, \
.  NAME##_bfd_link_hash_table_create, \
.  NAME##_bfd_link_add_symbols, \
.  NAME##_bfd_link_just_syms, \
.  NAME##_bfd_copy_link_hash_symbol_type, \
.  NAME##_bfd_final_link, \
.  NAME##_bfd_link_split_section, \
.  NAME##_bfd_gc_sections, \
.  NAME##_bfd_lookup_section_flags, \
.  NAME##_bfd_merge_sections, \
.  NAME##_bfd_is_group_section, \
.  NAME##_bfd_discard_group, \
.  NAME##_section_already_linked, \
.  NAME##_bfd_define_common_symbol
.
.  int         (*_bfd_sizeof_headers) (bfd *, struct bfd_link_info *);
.  bfd_byte *  (*_bfd_get_relocated_section_contents)
.    (bfd *, struct bfd_link_info *, struct bfd_link_order *,
.     bfd_byte *, bfd_boolean, struct bfd_symbol **);
.
.  bfd_boolean (*_bfd_relax_section)
.    (bfd *, struct bfd_section *, struct bfd_link_info *, bfd_boolean *);
.
.  {* Create a hash table for the linker.  Different backends store
.     different information in this table.  *}
.  struct bfd_link_hash_table *
.              (*_bfd_link_hash_table_create) (bfd *);
.
.  {* Add symbols from this object file into the hash table.  *}
.  bfd_boolean (*_bfd_link_add_symbols) (bfd *, struct bfd_link_info *);
.
.  {* Indicate that we are only retrieving symbol values from this section.  *}
.  void        (*_bfd_link_just_syms) (asection *, struct bfd_link_info *);
.
.  {* Copy the symbol type and other attributes for a linker script
.     assignment of one symbol to another.  *}
.#define bfd_copy_link_hash_symbol_type(b, t, f) \
.  BFD_SEND (b, _bfd_copy_link_hash_symbol_type, (b, t, f))
.  void (*_bfd_copy_link_hash_symbol_type)
.    (bfd *, struct bfd_link_hash_entry *, struct bfd_link_hash_entry *);
.
.  {* Do a link based on the link_order structures attached to each
.     section of the BFD.  *}
.  bfd_boolean (*_bfd_final_link) (bfd *, struct bfd_link_info *);
.
.  {* Should this section be split up into smaller pieces during linking.  *}
.  bfd_boolean (*_bfd_link_split_section) (bfd *, struct bfd_section *);
.
.  {* Remove sections that are not referenced from the output.  *}
.  bfd_boolean (*_bfd_gc_sections) (bfd *, struct bfd_link_info *);
.
.  {* Sets the bitmask of allowed and disallowed section flags.  *}
.  bfd_boolean (*_bfd_lookup_section_flags) (struct bfd_link_info *,
.					     struct flag_info *,
.					     asection *);
.
.  {* Attempt to merge SEC_MERGE sections.  *}
.  bfd_boolean (*_bfd_merge_sections) (bfd *, struct bfd_link_info *);
.
.  {* Is this section a member of a group?  *}
.  bfd_boolean (*_bfd_is_group_section) (bfd *, const struct bfd_section *);
.
.  {* Discard members of a group.  *}
.  bfd_boolean (*_bfd_discard_group) (bfd *, struct bfd_section *);
.
.  {* Check if SEC has been already linked during a reloceatable or
.     final link.  *}
.  bfd_boolean (*_section_already_linked) (bfd *, asection *,
.					   struct bfd_link_info *);
.
.  {* Define a common symbol.  *}
.  bfd_boolean (*_bfd_define_common_symbol) (bfd *, struct bfd_link_info *,
.					     struct bfd_link_hash_entry *);
.
.  {* Routines to handle dynamic symbols and relocs.  *}
.#define BFD_JUMP_TABLE_DYNAMIC(NAME) \
.  NAME##_get_dynamic_symtab_upper_bound, \
.  NAME##_canonicalize_dynamic_symtab, \
.  NAME##_get_synthetic_symtab, \
.  NAME##_get_dynamic_reloc_upper_bound, \
.  NAME##_canonicalize_dynamic_reloc
.
.  {* Get the amount of memory required to hold the dynamic symbols.  *}
.  long        (*_bfd_get_dynamic_symtab_upper_bound) (bfd *);
.  {* Read in the dynamic symbols.  *}
.  long        (*_bfd_canonicalize_dynamic_symtab)
.    (bfd *, struct bfd_symbol **);
.  {* Create synthetized symbols.  *}
.  long        (*_bfd_get_synthetic_symtab)
.    (bfd *, long, struct bfd_symbol **, long, struct bfd_symbol **,
.     struct bfd_symbol **);
.  {* Get the amount of memory required to hold the dynamic relocs.  *}
.  long        (*_bfd_get_dynamic_reloc_upper_bound) (bfd *);
.  {* Read in the dynamic relocs.  *}
.  long        (*_bfd_canonicalize_dynamic_reloc)
.    (bfd *, arelent **, struct bfd_symbol **);
.

A pointer to an alternative bfd_target in case the current one is not
satisfactory.  This can happen when the target cpu supports both big
and little endian code, and target chosen by the linker has the wrong
endianness.  The function open_output() in ld/ldlang.c uses this field
to find an alternative output format that is suitable.

.  {* Opposite endian version of this target.  *}
.  const struct bfd_target * alternative_target;
.

.  {* Data for use by back-end routines, which isn't
.     generic enough to belong in this structure.  *}
.  const void *backend_data;
.
.} bfd_target;
.
*/

/* All known xvecs (even those that don't compile on all systems).
   Alphabetized for easy reference.
   They are listed a second time below, since
   we can't intermix extern's and initializers.  */
extern const bfd_target aarch64_elf32_be_vec;
extern const bfd_target aarch64_elf32_le_vec;
extern const bfd_target aarch64_elf64_be_vec;
extern const bfd_target aarch64_elf64_be_cloudabi_vec;
extern const bfd_target aarch64_elf64_le_vec;
extern const bfd_target aarch64_elf64_le_cloudabi_vec;
extern const bfd_target alpha_ecoff_le_vec;
extern const bfd_target alpha_elf64_vec;
extern const bfd_target alpha_elf64_fbsd_vec;
extern const bfd_target alpha_nlm32_vec;
extern const bfd_target alpha_vms_vec;
extern const bfd_target alpha_vms_lib_txt_vec;
extern const bfd_target am33_elf32_linux_vec;
extern const bfd_target aout0_be_vec;
extern const bfd_target aout64_vec;
extern const bfd_target aout_vec;
extern const bfd_target aout_adobe_vec;
extern const bfd_target arc_elf32_be_vec;
extern const bfd_target arc_elf32_le_vec;
extern const bfd_target arm_aout_be_vec;
extern const bfd_target arm_aout_le_vec;
extern const bfd_target arm_aout_nbsd_vec;
extern const bfd_target arm_aout_riscix_vec;
extern const bfd_target arm_coff_be_vec;
extern const bfd_target arm_coff_le_vec;
extern const bfd_target arm_elf32_be_vec;
extern const bfd_target arm_elf32_le_vec;
extern const bfd_target arm_elf32_nacl_be_vec;
extern const bfd_target arm_elf32_nacl_le_vec;
extern const bfd_target arm_elf32_symbian_be_vec;
extern const bfd_target arm_elf32_symbian_le_vec;
extern const bfd_target arm_elf32_vxworks_be_vec;
extern const bfd_target arm_elf32_vxworks_le_vec;
extern const bfd_target arm_pe_be_vec;
extern const bfd_target arm_pe_le_vec;
extern const bfd_target arm_pe_epoc_be_vec;
extern const bfd_target arm_pe_epoc_le_vec;
extern const bfd_target arm_pe_wince_be_vec;
extern const bfd_target arm_pe_wince_le_vec;
extern const bfd_target arm_pei_be_vec;
extern const bfd_target arm_pei_le_vec;
extern const bfd_target arm_pei_epoc_be_vec;
extern const bfd_target arm_pei_epoc_le_vec;
extern const bfd_target arm_pei_wince_be_vec;
extern const bfd_target arm_pei_wince_le_vec;
extern const bfd_target avr_elf32_vec;
extern const bfd_target bfin_elf32_vec;
extern const bfd_target bfin_elf32_fdpic_vec;
extern const bfd_target bout_be_vec;
extern const bfd_target bout_le_vec;
extern const bfd_target cr16_elf32_vec;
extern const bfd_target cr16c_elf32_vec;
extern const bfd_target cris_aout_vec;
extern const bfd_target cris_elf32_vec;
extern const bfd_target cris_elf32_us_vec;
extern const bfd_target crx_elf32_vec;
extern const bfd_target d10v_elf32_vec;
extern const bfd_target d30v_elf32_vec;
extern const bfd_target dlx_elf32_be_vec;
extern const bfd_target elf32_be_vec;
extern const bfd_target elf32_le_vec;
extern const bfd_target elf64_be_vec;
extern const bfd_target elf64_le_vec;
extern const bfd_target epiphany_elf32_vec;
extern const bfd_target fr30_elf32_vec;
extern const bfd_target frv_elf32_vec;
extern const bfd_target frv_elf32_fdpic_vec;
extern const bfd_target h8300_coff_vec;
extern const bfd_target h8300_elf32_vec;
extern const bfd_target h8300_elf32_linux_vec;
extern const bfd_target h8500_coff_vec;
extern const bfd_target hppa_elf32_vec;
extern const bfd_target hppa_elf32_linux_vec;
extern const bfd_target hppa_elf32_nbsd_vec;
extern const bfd_target hppa_elf64_vec;
extern const bfd_target hppa_elf64_linux_vec;
extern const bfd_target hppa_som_vec;
extern const bfd_target i370_elf32_vec;
extern const bfd_target i386_aout_vec;
extern const bfd_target i386_aout_bsd_vec;
extern const bfd_target i386_aout_dynix_vec;
extern const bfd_target i386_aout_fbsd_vec;
extern const bfd_target i386_aout_linux_vec;
extern const bfd_target i386_aout_lynx_vec;
extern const bfd_target i386_aout_mach3_vec;
extern const bfd_target i386_aout_nbsd_vec;
extern const bfd_target i386_aout_os9k_vec;
extern const bfd_target i386_coff_vec;
extern const bfd_target i386_coff_go32_vec;
extern const bfd_target i386_coff_go32stubbed_vec;
extern const bfd_target i386_coff_lynx_vec;
extern const bfd_target i386_elf32_vec;
extern const bfd_target i386_elf32_fbsd_vec;
extern const bfd_target i386_elf32_nacl_vec;
extern const bfd_target i386_elf32_sol2_vec;
extern const bfd_target i386_elf32_vxworks_vec;
extern const bfd_target i386_mach_o_vec;
extern const bfd_target i386_msdos_vec;
extern const bfd_target i386_nlm32_vec;
extern const bfd_target i386_pe_vec;
extern const bfd_target i386_pei_vec;
extern const bfd_target iamcu_elf32_vec;
extern const bfd_target i860_coff_vec;
extern const bfd_target i860_elf32_vec;
extern const bfd_target i860_elf32_le_vec;
extern const bfd_target i960_elf32_vec;
extern const bfd_target ia64_elf32_be_vec;
extern const bfd_target ia64_elf32_hpux_be_vec;
extern const bfd_target ia64_elf64_be_vec;
extern const bfd_target ia64_elf64_le_vec;
extern const bfd_target ia64_elf64_hpux_be_vec;
extern const bfd_target ia64_elf64_vms_vec;
extern const bfd_target ia64_pei_vec;
extern const bfd_target icoff_be_vec;
extern const bfd_target icoff_le_vec;
extern const bfd_target ieee_vec;
extern const bfd_target ip2k_elf32_vec;
extern const bfd_target iq2000_elf32_vec;
extern const bfd_target k1om_elf64_vec;
extern const bfd_target k1om_elf64_fbsd_vec;
extern const bfd_target l1om_elf64_vec;
extern const bfd_target l1om_elf64_fbsd_vec;
extern const bfd_target lm32_elf32_vec;
extern const bfd_target lm32_elf32_fdpic_vec;
extern const bfd_target m32c_elf32_vec;
extern const bfd_target m32r_elf32_vec;
extern const bfd_target m32r_elf32_le_vec;
extern const bfd_target m32r_elf32_linux_vec;
extern const bfd_target m32r_elf32_linux_le_vec;
extern const bfd_target m68hc11_elf32_vec;
extern const bfd_target m68hc12_elf32_vec;
extern const bfd_target m68k_aout_4knbsd_vec;
extern const bfd_target m68k_aout_hp300bsd_vec;
extern const bfd_target m68k_aout_hp300hpux_vec;
extern const bfd_target m68k_aout_linux_vec;
extern const bfd_target m68k_aout_nbsd_vec;
extern const bfd_target m68k_aout_newsos3_vec;
extern const bfd_target m68k_coff_vec;
extern const bfd_target m68k_coff_apollo_vec;
extern const bfd_target m68k_coff_aux_vec;
extern const bfd_target m68k_coff_sysv_vec;
extern const bfd_target m68k_coff_un_vec;
extern const bfd_target m68k_elf32_vec;
extern const bfd_target m68k_versados_vec;
extern const bfd_target m88k_aout_mach3_vec;
extern const bfd_target m88k_aout_obsd_vec;
extern const bfd_target m88k_coff_bcs_vec;
extern const bfd_target m88k_elf32_vec;
extern const bfd_target mach_o_be_vec;
extern const bfd_target mach_o_le_vec;
extern const bfd_target mach_o_fat_vec;
extern const bfd_target mcore_elf32_be_vec;
extern const bfd_target mcore_elf32_le_vec;
extern const bfd_target mcore_pe_be_vec;
extern const bfd_target mcore_pe_le_vec;
extern const bfd_target mcore_pei_be_vec;
extern const bfd_target mcore_pei_le_vec;
extern const bfd_target mep_elf32_vec;
extern const bfd_target mep_elf32_le_vec;
extern const bfd_target metag_elf32_vec;
extern const bfd_target microblaze_elf32_vec;
extern const bfd_target microblaze_elf32_le_vec;
extern const bfd_target mips_aout_be_vec;
extern const bfd_target mips_aout_le_vec;
extern const bfd_target mips_ecoff_be_vec;
extern const bfd_target mips_ecoff_le_vec;
extern const bfd_target mips_ecoff_bele_vec;
extern const bfd_target mips_elf32_be_vec;
extern const bfd_target mips_elf32_le_vec;
extern const bfd_target mips_elf32_n_be_vec;
extern const bfd_target mips_elf32_n_le_vec;
extern const bfd_target mips_elf32_ntrad_be_vec;
extern const bfd_target mips_elf32_ntrad_le_vec;
extern const bfd_target mips_elf32_ntradfbsd_be_vec;
extern const bfd_target mips_elf32_ntradfbsd_le_vec;
extern const bfd_target mips_elf32_trad_be_vec;
extern const bfd_target mips_elf32_trad_le_vec;
extern const bfd_target mips_elf32_tradfbsd_be_vec;
extern const bfd_target mips_elf32_tradfbsd_le_vec;
extern const bfd_target mips_elf32_vxworks_be_vec;
extern const bfd_target mips_elf32_vxworks_le_vec;
extern const bfd_target mips_elf64_be_vec;
extern const bfd_target mips_elf64_le_vec;
extern const bfd_target mips_elf64_trad_be_vec;
extern const bfd_target mips_elf64_trad_le_vec;
extern const bfd_target mips_elf64_tradfbsd_be_vec;
extern const bfd_target mips_elf64_tradfbsd_le_vec;
extern const bfd_target mips_pe_le_vec;
extern const bfd_target mips_pei_le_vec;
extern const bfd_target mmix_elf64_vec;
extern const bfd_target mmix_mmo_vec;
extern const bfd_target mn10200_elf32_vec;
extern const bfd_target mn10300_elf32_vec;
extern const bfd_target moxie_elf32_be_vec;
extern const bfd_target moxie_elf32_le_vec;
extern const bfd_target msp430_elf32_vec;
extern const bfd_target msp430_elf32_ti_vec;
extern const bfd_target mt_elf32_vec;
extern const bfd_target nds32_elf32_be_vec;
extern const bfd_target nds32_elf32_le_vec;
extern const bfd_target nds32_elf32_linux_be_vec;
extern const bfd_target nds32_elf32_linux_le_vec;
extern const bfd_target nios2_elf32_be_vec;
extern const bfd_target nios2_elf32_le_vec;
extern const bfd_target ns32k_aout_pc532mach_vec;
extern const bfd_target ns32k_aout_pc532nbsd_vec;
extern const bfd_target oasys_vec;
extern const bfd_target or1k_elf32_vec;
extern const bfd_target pdp11_aout_vec;
extern const bfd_target pef_vec;
extern const bfd_target pef_xlib_vec;
extern const bfd_target pj_elf32_vec;
extern const bfd_target pj_elf32_le_vec;
extern const bfd_target plugin_vec;
extern const bfd_target powerpc_boot_vec;
extern const bfd_target powerpc_elf32_vec;
extern const bfd_target powerpc_elf32_le_vec;
extern const bfd_target powerpc_elf32_fbsd_vec;
extern const bfd_target powerpc_elf32_vxworks_vec;
extern const bfd_target powerpc_elf64_vec;
extern const bfd_target powerpc_elf64_le_vec;
extern const bfd_target powerpc_elf64_fbsd_vec;
extern const bfd_target powerpc_nlm32_vec;
extern const bfd_target powerpc_pe_vec;
extern const bfd_target powerpc_pe_le_vec;
extern const bfd_target powerpc_pei_vec;
extern const bfd_target powerpc_pei_le_vec;
extern const bfd_target powerpc_xcoff_vec;
extern const bfd_target rl78_elf32_vec;
extern const bfd_target rs6000_xcoff64_vec;
extern const bfd_target rs6000_xcoff64_aix_vec;
extern const bfd_target rs6000_xcoff_vec;
extern const bfd_target rx_elf32_be_vec;
extern const bfd_target rx_elf32_be_ns_vec;
extern const bfd_target rx_elf32_le_vec;
extern const bfd_target s390_elf32_vec;
extern const bfd_target s390_elf64_vec;
extern const bfd_target score_elf32_be_vec;
extern const bfd_target score_elf32_le_vec;
extern const bfd_target sh64_elf32_vec;
extern const bfd_target sh64_elf32_le_vec;
extern const bfd_target sh64_elf32_linux_vec;
extern const bfd_target sh64_elf32_linux_be_vec;
extern const bfd_target sh64_elf32_nbsd_vec;
extern const bfd_target sh64_elf32_nbsd_le_vec;
extern const bfd_target sh64_elf64_vec;
extern const bfd_target sh64_elf64_le_vec;
extern const bfd_target sh64_elf64_linux_vec;
extern const bfd_target sh64_elf64_linux_be_vec;
extern const bfd_target sh64_elf64_nbsd_vec;
extern const bfd_target sh64_elf64_nbsd_le_vec;
extern const bfd_target sh_coff_vec;
extern const bfd_target sh_coff_le_vec;
extern const bfd_target sh_coff_small_vec;
extern const bfd_target sh_coff_small_le_vec;
extern const bfd_target sh_elf32_vec;
extern const bfd_target sh_elf32_le_vec;
extern const bfd_target sh_elf32_fdpic_be_vec;
extern const bfd_target sh_elf32_fdpic_le_vec;
extern const bfd_target sh_elf32_linux_vec;
extern const bfd_target sh_elf32_linux_be_vec;
extern const bfd_target sh_elf32_nbsd_vec;
extern const bfd_target sh_elf32_nbsd_le_vec;
extern const bfd_target sh_elf32_symbian_le_vec;
extern const bfd_target sh_elf32_vxworks_vec;
extern const bfd_target sh_elf32_vxworks_le_vec;
extern const bfd_target sh_pe_le_vec;
extern const bfd_target sh_pei_le_vec;
extern const bfd_target sparc_aout_le_vec;
extern const bfd_target sparc_aout_linux_vec;
extern const bfd_target sparc_aout_lynx_vec;
extern const bfd_target sparc_aout_nbsd_vec;
extern const bfd_target sparc_aout_sunos_be_vec;
extern const bfd_target sparc_coff_vec;
extern const bfd_target sparc_coff_lynx_vec;
extern const bfd_target sparc_elf32_vec;
extern const bfd_target sparc_elf32_sol2_vec;
extern const bfd_target sparc_elf32_vxworks_vec;
extern const bfd_target sparc_elf64_vec;
extern const bfd_target sparc_elf64_fbsd_vec;
extern const bfd_target sparc_elf64_sol2_vec;
extern const bfd_target sparc_nlm32_vec;
extern const bfd_target spu_elf32_vec;
extern const bfd_target sym_vec;
extern const bfd_target tic30_aout_vec;
extern const bfd_target tic30_coff_vec;
extern const bfd_target tic4x_coff0_vec;
extern const bfd_target tic4x_coff0_beh_vec;
extern const bfd_target tic4x_coff1_vec;
extern const bfd_target tic4x_coff1_beh_vec;
extern const bfd_target tic4x_coff2_vec;
extern const bfd_target tic4x_coff2_beh_vec;
extern const bfd_target tic54x_coff0_vec;
extern const bfd_target tic54x_coff0_beh_vec;
extern const bfd_target tic54x_coff1_vec;
extern const bfd_target tic54x_coff1_beh_vec;
extern const bfd_target tic54x_coff2_vec;
extern const bfd_target tic54x_coff2_beh_vec;
extern const bfd_target tic6x_elf32_be_vec;
extern const bfd_target tic6x_elf32_le_vec;
extern const bfd_target tic6x_elf32_c6000_be_vec;
extern const bfd_target tic6x_elf32_c6000_le_vec;
extern const bfd_target tic6x_elf32_linux_be_vec;
extern const bfd_target tic6x_elf32_linux_le_vec;
extern const bfd_target tic80_coff_vec;
extern const bfd_target tilegx_elf32_be_vec;
extern const bfd_target tilegx_elf32_le_vec;
extern const bfd_target tilegx_elf64_be_vec;
extern const bfd_target tilegx_elf64_le_vec;
extern const bfd_target tilepro_elf32_vec;
extern const bfd_target v800_elf32_vec;
extern const bfd_target v850_elf32_vec;
extern const bfd_target ft32_elf32_vec;
extern const bfd_target vax_aout_1knbsd_vec;
extern const bfd_target vax_aout_bsd_vec;
extern const bfd_target vax_aout_nbsd_vec;
extern const bfd_target vax_elf32_vec;
extern const bfd_target visium_elf32_vec;
extern const bfd_target w65_coff_vec;
extern const bfd_target we32k_coff_vec;
extern const bfd_target x86_64_coff_vec;
extern const bfd_target x86_64_elf32_vec;
extern const bfd_target x86_64_elf32_nacl_vec;
extern const bfd_target x86_64_elf64_vec;
extern const bfd_target x86_64_elf64_cloudabi_vec;
extern const bfd_target x86_64_elf64_fbsd_vec;
extern const bfd_target x86_64_elf64_nacl_vec;
extern const bfd_target x86_64_elf64_sol2_vec;
extern const bfd_target x86_64_mach_o_vec;
extern const bfd_target x86_64_pe_vec;
extern const bfd_target x86_64_pe_be_vec;
extern const bfd_target x86_64_pei_vec;
extern const bfd_target xc16x_elf32_vec;
extern const bfd_target xgate_elf32_vec;
extern const bfd_target xstormy16_elf32_vec;
extern const bfd_target xtensa_elf32_be_vec;
extern const bfd_target xtensa_elf32_le_vec;
extern const bfd_target z80_coff_vec;
extern const bfd_target z8k_coff_vec;

/* These are always included.  */
extern const bfd_target srec_vec;
extern const bfd_target symbolsrec_vec;
extern const bfd_target verilog_vec;
extern const bfd_target tekhex_vec;
extern const bfd_target binary_vec;
extern const bfd_target ihex_vec;

/* All of the xvecs for core files.  */
extern const bfd_target core_aix386_vec;
extern const bfd_target core_cisco_be_vec;
extern const bfd_target core_cisco_le_vec;
extern const bfd_target core_hppabsd_vec;
extern const bfd_target core_hpux_vec;
extern const bfd_target core_irix_vec;
extern const bfd_target core_netbsd_vec;
extern const bfd_target core_osf_vec;
extern const bfd_target core_ptrace_vec;
extern const bfd_target core_sco5_vec;
extern const bfd_target core_trad_vec;

static const bfd_target * const _bfd_target_vector[] =
{
#ifdef SELECT_VECS

	SELECT_VECS,

#else /* not SELECT_VECS */

#ifdef DEFAULT_VECTOR
	&DEFAULT_VECTOR,
#endif
	/* This list is alphabetized to make it easy to compare
	   with other vector lists -- the decls above and
	   the case statement in configure.ac.
	   Try to keep it in order when adding new targets, and
	   use a name of the form <cpu>_<format>_<other>_<endian>_vec.
	   Note that sorting is done as if _<endian>_vec wasn't present.
	   Vectors that don't compile on all systems, or aren't finished,
	   should have an entry here with #if 0 around it, to show that
	   it wasn't omitted by mistake.  */
#ifdef BFD64
	&aarch64_elf32_be_vec,
	&aarch64_elf32_le_vec,
	&aarch64_elf64_be_vec,
	&aarch64_elf64_be_cloudabi_vec,
	&aarch64_elf64_le_vec,
	&aarch64_elf64_le_cloudabi_vec,
#endif

#ifdef BFD64
	&alpha_ecoff_le_vec,
	&alpha_elf64_vec,
	&alpha_elf64_fbsd_vec,
	&alpha_nlm32_vec,
	&alpha_vms_vec,
#endif
	&alpha_vms_lib_txt_vec,

	&am33_elf32_linux_vec,

	&aout0_be_vec,
#ifdef BFD64
	&aout64_vec,	/* Only compiled if host has long-long support.  */
#endif
#if 0
	/* Since a.out files lack decent magic numbers, no way to recognize
	   which kind of a.out file it is.  */
	&aout_vec,
#endif
	&aout_adobe_vec,

	&arc_elf32_be_vec,
	&arc_elf32_le_vec,

#if 0
	/* We have no way of distinguishing these from other a.out variants.  */
	&arm_aout_be_vec,
	&arm_aout_le_vec,
#endif
	&arm_aout_nbsd_vec,
#if 0
	/* We have no way of distinguishing these from other a.out variants.  */
	&arm_aout_riscix_vec,
#endif
	&arm_coff_be_vec,
	&arm_coff_le_vec,
	&arm_elf32_be_vec,
	&arm_elf32_le_vec,
	&arm_elf32_symbian_be_vec,
	&arm_elf32_symbian_le_vec,
	&arm_elf32_vxworks_be_vec,
	&arm_elf32_vxworks_le_vec,
	&arm_pe_be_vec,
	&arm_pe_le_vec,
	&arm_pe_epoc_be_vec,
	&arm_pe_epoc_le_vec,
	&arm_pe_wince_be_vec,
	&arm_pe_wince_le_vec,
	&arm_pei_be_vec,
	&arm_pei_le_vec,
	&arm_pei_epoc_be_vec,
	&arm_pei_epoc_le_vec,
	&arm_pei_wince_be_vec,
	&arm_pei_wince_le_vec,

	&avr_elf32_vec,

	&bfin_elf32_vec,
	&bfin_elf32_fdpic_vec,

	&bout_be_vec,
	&bout_le_vec,

	&cr16_elf32_vec,
	&cr16c_elf32_vec,

	&cris_aout_vec,
	&cris_elf32_vec,
	&cris_elf32_us_vec,

	&crx_elf32_vec,

	&d10v_elf32_vec,
	&d30v_elf32_vec,

	&dlx_elf32_be_vec,

	/* This, and other vectors, may not be used in any *.mt configuration.
	   But that does not mean they are unnecessary.  If configured with
	   --enable-targets=all, objdump or gdb should be able to examine
	   the file even if we don't recognize the machine type.  */
	&elf32_be_vec,
	&elf32_le_vec,
#ifdef BFD64
	&elf64_be_vec,
	&elf64_le_vec,
#endif

	&epiphany_elf32_vec,

	&fr30_elf32_vec,

	&frv_elf32_vec,
	&frv_elf32_fdpic_vec,

	&h8300_coff_vec,
	&h8300_elf32_vec,
	&h8300_elf32_linux_vec,
	&h8500_coff_vec,

	&hppa_elf32_vec,
	&hppa_elf32_linux_vec,
	&hppa_elf32_nbsd_vec,
#ifdef BFD64
	&hppa_elf64_vec,
	&hppa_elf64_linux_vec,
#endif
	&hppa_som_vec,

	&i370_elf32_vec,

	&i386_aout_vec,
	&i386_aout_bsd_vec,
#if 0
	&i386_aout_dynix_vec,
#endif
	&i386_aout_fbsd_vec,
#if 0
	/* Since a.out files lack decent magic numbers, no way to recognize
	   which kind of a.out file it is.  */
	&i386_aout_linux_vec,
#endif
	&i386_aout_lynx_vec,
#if 0
	/* No distinguishing features for Mach 3 executables.  */
	&i386_aout_mach3_vec,
#endif
	&i386_aout_nbsd_vec,
	&i386_aout_os9k_vec,
	&i386_coff_vec,
	&i386_coff_go32_vec,
	&i386_coff_go32stubbed_vec,
	&i386_coff_lynx_vec,
	&i386_elf32_vec,
	&i386_elf32_fbsd_vec,
	&i386_elf32_nacl_vec,
	&i386_elf32_sol2_vec,
	&i386_elf32_vxworks_vec,
	&i386_mach_o_vec,
	&i386_msdos_vec,
	&i386_nlm32_vec,
	&i386_pe_vec,
	&i386_pei_vec,

	&iamcu_elf32_vec,

	&i860_coff_vec,
	&i860_elf32_vec,
	&i860_elf32_le_vec,

	&i960_elf32_vec,

#ifdef BFD64
#if 0
	&ia64_elf32_be_vec,
#endif
	&ia64_elf32_hpux_be_vec,
	&ia64_elf64_be_vec,
	&ia64_elf64_le_vec,
	&ia64_elf64_hpux_be_vec,
	&ia64_elf64_vms_vec,
	&ia64_pei_vec,
#endif

	&icoff_be_vec,
	&icoff_le_vec,

	&ieee_vec,

	&ip2k_elf32_vec,
	&iq2000_elf32_vec,

#ifdef BFD64
	&k1om_elf64_vec,
	&k1om_elf64_fbsd_vec,
	&l1om_elf64_vec,
	&l1om_elf64_fbsd_vec,
#endif

	&lm32_elf32_vec,

	&m32c_elf32_vec,

	&m32r_elf32_vec,
	&m32r_elf32_le_vec,
	&m32r_elf32_linux_vec,
	&m32r_elf32_linux_le_vec,

	&m68hc11_elf32_vec,
	&m68hc12_elf32_vec,

#if 0
	&m68k_aout_4knbsd_vec,
	/* Clashes with sparc_aout_sunos_be_vec magic no.  */
	&m68k_aout_hp300bsd_vec,
#endif
	&m68k_aout_hp300hpux_vec,
#if 0
	/* Since a.out files lack decent magic numbers, no way to recognize
	   which kind of a.out file it is.  */
	&m68k_aout_linux_vec,
#endif
	&m68k_aout_nbsd_vec,
	&m68k_aout_newsos3_vec,
	&m68k_coff_vec,
#if 0
	&m68k_coff_apollo_vec,
	&m68k_coff_aux_vec,
#endif
	&m68k_coff_sysv_vec,
	&m68k_coff_un_vec,
	&m68k_elf32_vec,
	&m68k_versados_vec,

	&m88k_aout_mach3_vec,
	&m88k_aout_obsd_vec,
	&m88k_coff_bcs_vec,
	&m88k_elf32_vec,

	&mach_o_be_vec,
	&mach_o_le_vec,
	&mach_o_fat_vec,

	&mcore_elf32_be_vec,
	&mcore_elf32_le_vec,
	&mcore_pe_be_vec,
	&mcore_pe_le_vec,
	&mcore_pei_be_vec,
	&mcore_pei_le_vec,

	&mep_elf32_vec,

	&metag_elf32_vec,

	&microblaze_elf32_vec,

#if 0
	/* No one seems to use this.  */
	&mips_aout_be_vec,
#endif
	&mips_aout_le_vec,
	&mips_ecoff_be_vec,
	&mips_ecoff_le_vec,
	&mips_ecoff_bele_vec,
#ifdef BFD64
	&mips_elf32_be_vec,
	&mips_elf32_le_vec,
	&mips_elf32_n_be_vec,
	&mips_elf32_n_le_vec,
	&mips_elf32_ntrad_be_vec,
	&mips_elf32_ntrad_le_vec,
	&mips_elf32_ntradfbsd_be_vec,
	&mips_elf32_ntradfbsd_le_vec,
	&mips_elf32_trad_be_vec,
	&mips_elf32_trad_le_vec,
	&mips_elf32_tradfbsd_be_vec,
	&mips_elf32_tradfbsd_le_vec,
	&mips_elf32_vxworks_be_vec,
	&mips_elf32_vxworks_le_vec,
	&mips_elf64_be_vec,
	&mips_elf64_le_vec,
	&mips_elf64_trad_be_vec,
	&mips_elf64_trad_le_vec,
	&mips_elf64_tradfbsd_be_vec,
	&mips_elf64_tradfbsd_le_vec,
#endif
	&mips_pe_le_vec,
	&mips_pei_le_vec,

#ifdef BFD64
	&mmix_elf64_vec,
	&mmix_mmo_vec,
#endif

	&mn10200_elf32_vec,
	&mn10300_elf32_vec,

	&moxie_elf32_be_vec,
	&moxie_elf32_le_vec,

	&msp430_elf32_vec,
	&msp430_elf32_ti_vec,

	&mt_elf32_vec,

	&nds32_elf32_be_vec,
	&nds32_elf32_le_vec,
	&nds32_elf32_linux_be_vec,
	&nds32_elf32_linux_le_vec,

	&nios2_elf32_be_vec,
	&nios2_elf32_le_vec,

	&ns32k_aout_pc532mach_vec,
	&ns32k_aout_pc532nbsd_vec,

#if 0
	/* We have no oasys tools anymore, so we can't test any of this
	   anymore. If you want to test the stuff yourself, go ahead...
	   steve@cygnus.com
	   Worse, since there is no magic number for archives, there
	   can be annoying target mis-matches.  */
	&oasys_vec,
#endif

	&or1k_elf32_vec,

	&pdp11_aout_vec,

	&pef_vec,
	&pef_xlib_vec,

	&pj_elf32_vec,
	&pj_elf32_le_vec,

#if BFD_SUPPORTS_PLUGINS
	&plugin_vec,
#endif

	&powerpc_boot_vec,
	&powerpc_elf32_vec,
	&powerpc_elf32_le_vec,
	&powerpc_elf32_fbsd_vec,
	&powerpc_elf32_vxworks_vec,
#ifdef BFD64
	&powerpc_elf64_vec,
	&powerpc_elf64_le_vec,
	&powerpc_elf64_fbsd_vec,
#endif
	&powerpc_nlm32_vec,
	&powerpc_pe_vec,
	&powerpc_pe_le_vec,
	&powerpc_pei_vec,
	&powerpc_pei_le_vec,
#if 0
	/* This has the same magic number as RS/6000.  */
	&powerpc_xcoff_vec,
#endif

	&rl78_elf32_vec,

#ifdef BFD64
	&rs6000_xcoff64_vec,
	&rs6000_xcoff64_aix_vec,
#endif
	&rs6000_xcoff_vec,

	&rx_elf32_be_vec,
	&rx_elf32_be_ns_vec,
	&rx_elf32_le_vec,

	&s390_elf32_vec,
#ifdef BFD64
	&s390_elf64_vec,
#endif

#ifdef BFD64
	&score_elf32_be_vec,
	&score_elf32_le_vec,
#endif

#ifdef BFD64
	&sh64_elf32_vec,
	&sh64_elf32_le_vec,
	&sh64_elf32_linux_vec,
	&sh64_elf32_linux_be_vec,
	&sh64_elf32_nbsd_vec,
	&sh64_elf32_nbsd_le_vec,
	&sh64_elf64_vec,
	&sh64_elf64_le_vec,
	&sh64_elf64_linux_vec,
	&sh64_elf64_linux_be_vec,
	&sh64_elf64_nbsd_vec,
	&sh64_elf64_nbsd_le_vec,
#endif
	&sh_coff_vec,
	&sh_coff_le_vec,
	&sh_coff_small_vec,
	&sh_coff_small_le_vec,
	&sh_elf32_vec,
	&sh_elf32_le_vec,
	&sh_elf32_fdpic_be_vec,
	&sh_elf32_fdpic_le_vec,
	&sh_elf32_linux_vec,
	&sh_elf32_linux_be_vec,
	&sh_elf32_nbsd_vec,
	&sh_elf32_nbsd_le_vec,
	&sh_elf32_symbian_le_vec,
	&sh_elf32_vxworks_vec,
	&sh_elf32_vxworks_le_vec,
	&sh_pe_le_vec,
	&sh_pei_le_vec,

	&sparc_aout_le_vec,
	&sparc_aout_linux_vec,
	&sparc_aout_lynx_vec,
	&sparc_aout_nbsd_vec,
	&sparc_aout_sunos_be_vec,
	&sparc_coff_vec,
	&sparc_coff_lynx_vec,
	&sparc_elf32_vec,
	&sparc_elf32_sol2_vec,
	&sparc_elf32_vxworks_vec,
#ifdef BFD64
	&sparc_elf64_vec,
	&sparc_elf64_fbsd_vec,
	&sparc_elf64_sol2_vec,
#endif
	&sparc_nlm32_vec,

	&spu_elf32_vec,

	&sym_vec,

	&tic30_aout_vec,
	&tic30_coff_vec,
	&tic54x_coff0_beh_vec,
	&tic54x_coff0_vec,
	&tic54x_coff1_beh_vec,
	&tic54x_coff1_vec,
	&tic54x_coff2_beh_vec,
	&tic54x_coff2_vec,
	&tic6x_elf32_be_vec,
	&tic6x_elf32_le_vec,
	&tic80_coff_vec,

	&tilegx_elf32_be_vec,
	&tilegx_elf32_le_vec,
#ifdef BFD64
	&tilegx_elf64_be_vec,
	&tilegx_elf64_le_vec,
#endif
	&tilepro_elf32_vec,

	&ft32_elf32_vec,

	&v800_elf32_vec,
	&v850_elf32_vec,

	&vax_aout_1knbsd_vec,
	&vax_aout_bsd_vec,
	&vax_aout_nbsd_vec,
	&vax_elf32_vec,

	&visium_elf32_vec,

	&w65_coff_vec,

	&we32k_coff_vec,

#ifdef BFD64
	&x86_64_coff_vec,
	&x86_64_elf32_vec,
	&x86_64_elf32_nacl_vec,
	&x86_64_elf64_vec,
	&x86_64_elf64_cloudabi_vec,
	&x86_64_elf64_fbsd_vec,
	&x86_64_elf64_nacl_vec,
	&x86_64_elf64_sol2_vec,
	&x86_64_mach_o_vec,
	&x86_64_pe_vec,
	&x86_64_pe_be_vec,
	&x86_64_pei_vec,
#endif

	&xc16x_elf32_vec,

	&xgate_elf32_vec,

	&xstormy16_elf32_vec,

	&xtensa_elf32_be_vec,
	&xtensa_elf32_le_vec,

	&z80_coff_vec,

	&z8k_coff_vec,
#endif /* not SELECT_VECS */

/* Always support S-records, for convenience.  */
	&srec_vec,
	&symbolsrec_vec,
/* And verilog.  */
	&verilog_vec,
/* And tekhex */
	&tekhex_vec,
/* Likewise for binary output.  */
	&binary_vec,
/* Likewise for ihex.  */
	&ihex_vec,

/* Add any required traditional-core-file-handler.  */

#ifdef AIX386_CORE
	&core_aix386_vec,
#endif
#if 0
	/* We don't include cisco_core_*_vec.  Although it has a magic number,
	   the magic number isn't at the beginning of the file, and thus
	   might spuriously match other kinds of files.  */
	&core_cisco_be_vec,
	&core_cisco_le_vec,
#endif
#ifdef HPPABSD_CORE
	&core_hppabsd_vec,
#endif
#ifdef HPUX_CORE
	&core_hpux_vec,
#endif
#ifdef IRIX_CORE
	&core_irix_vec,
#endif
#ifdef NETBSD_CORE
	&core_netbsd_vec,
#endif
#ifdef OSF_CORE
	&core_osf_vec,
#endif
#ifdef PTRACE_CORE
	&core_ptrace_vec,
#endif
#ifdef SCO5_CORE
	&core_sco5_vec,
#endif
#ifdef TRAD_CORE
	&core_trad_vec,
#endif

	NULL /* end of list marker */
};
const bfd_target * const *bfd_target_vector = _bfd_target_vector;

/* bfd_default_vector[0] contains either the address of the default vector,
   if there is one, or zero if there isn't.  */

const bfd_target *bfd_default_vector[] = {
#ifdef DEFAULT_VECTOR
	&DEFAULT_VECTOR,
#endif
	NULL
};

/* bfd_associated_vector[] contains the associated target vectors used
   to reduce the ambiguity in bfd_check_format_matches.  */

static const bfd_target *_bfd_associated_vector[] = {
#ifdef ASSOCIATED_VECS
	ASSOCIATED_VECS,
#endif
	NULL
};
const bfd_target * const *bfd_associated_vector = _bfd_associated_vector;

/* When there is an ambiguous match, bfd_check_format_matches puts the
   names of the matching targets in an array.  This variable is the maximum
   number of entries that the array could possibly need.  */
const size_t _bfd_target_vector_entries = sizeof (_bfd_target_vector)/sizeof (*_bfd_target_vector);

/* This array maps configuration triplets onto BFD vectors.  */

struct targmatch
{
  /* The configuration triplet.  */
  const char *triplet;
  /* The BFD vector.  If this is NULL, then the vector is found by
     searching forward for the next structure with a non NULL vector
     field.  */
  const bfd_target *vector;
};

/* targmatch.h is built by Makefile out of config.bfd.  */
static const struct targmatch bfd_target_match[] = {
#include "targmatch.h"
  { NULL, NULL }
};

/* Find a target vector, given a name or configuration triplet.  */

static const bfd_target *
find_target (const char *name)
{
  const bfd_target * const *target;
  const struct targmatch *match;

  for (target = &bfd_target_vector[0]; *target != NULL; target++)
    if (strcmp (name, (*target)->name) == 0)
      return *target;

  /* If we couldn't match on the exact name, try matching on the
     configuration triplet.  FIXME: We should run the triplet through
     config.sub first, but that is hard.  */
  for (match = &bfd_target_match[0]; match->triplet != NULL; match++)
    {
      if (fnmatch (match->triplet, name, 0) == 0)
	{
	  while (match->vector == NULL)
	    ++match;
	  return match->vector;
	}
    }

  bfd_set_error (bfd_error_invalid_target);
  return NULL;
}

/*
FUNCTION
	bfd_set_default_target

SYNOPSIS
	bfd_boolean bfd_set_default_target (const char *name);

DESCRIPTION
	Set the default target vector to use when recognizing a BFD.
	This takes the name of the target, which may be a BFD target
	name or a configuration triplet.
*/

bfd_boolean
bfd_set_default_target (const char *name)
{
  const bfd_target *target;

  if (bfd_default_vector[0] != NULL
      && strcmp (name, bfd_default_vector[0]->name) == 0)
    return TRUE;

  target = find_target (name);
  if (target == NULL)
    return FALSE;

  bfd_default_vector[0] = target;
  return TRUE;
}

/*
FUNCTION
	bfd_find_target

SYNOPSIS
	const bfd_target *bfd_find_target (const char *target_name, bfd *abfd);

DESCRIPTION
	Return a pointer to the transfer vector for the object target
	named @var{target_name}.  If @var{target_name} is <<NULL>>,
	choose the one in the environment variable <<GNUTARGET>>; if
	that is null or not defined, then choose the first entry in the
	target list.  Passing in the string "default" or setting the
	environment variable to "default" will cause the first entry in
	the target list to be returned, and "target_defaulted" will be
	set in the BFD if @var{abfd} isn't <<NULL>>.  This causes
	<<bfd_check_format>> to loop over all the targets to find the
	one that matches the file being read.
*/

const bfd_target *
bfd_find_target (const char *target_name, bfd *abfd)
{
  const char *targname;
  const bfd_target *target;

  if (target_name != NULL)
    targname = target_name;
  else
    targname = getenv ("GNUTARGET");

  /* This is safe; the vector cannot be null.  */
  if (targname == NULL || strcmp (targname, "default") == 0)
    {
      if (bfd_default_vector[0] != NULL)
	target = bfd_default_vector[0];
      else
	target = bfd_target_vector[0];
      if (abfd)
	{
	  abfd->xvec = target;
	  abfd->target_defaulted = TRUE;
	}
      return target;
    }

  if (abfd)
    abfd->target_defaulted = FALSE;

  target = find_target (targname);
  if (target == NULL)
    return NULL;

  if (abfd)
    abfd->xvec = target;
  return target;
}

/* Helper function for bfd_get_target_info to determine the target's
   architecture.  This method handles bfd internal target names as
   tuples and triplets.  */
static bfd_boolean
_bfd_find_arch_match (const char *tname, const char **arch,
		      const char **def_target_arch)
{
  if (!arch)
    return FALSE;

  while (*arch != NULL)
    {
      const char *in_a = strstr (*arch, tname);
      char end_ch = (in_a ? in_a[strlen (tname)] : 0);

      if (in_a && (in_a == *arch || in_a[-1] == ':')
          && end_ch == 0)
        {
          *def_target_arch = *arch;
          return TRUE;
        }
      arch++;
    }
  return FALSE;
}

/*
FUNCTION
	bfd_get_target_info
SYNOPSIS
	const bfd_target *bfd_get_target_info (const char *target_name,
					       bfd *abfd,
					       bfd_boolean *is_bigendian,
					       int *underscoring,
					       const char **def_target_arch);
DESCRIPTION
        Return a pointer to the transfer vector for the object target
        named @var{target_name}.  If @var{target_name} is <<NULL>>,
        choose the one in the environment variable <<GNUTARGET>>; if
        that is null or not defined, then choose the first entry in the
        target list.  Passing in the string "default" or setting the
        environment variable to "default" will cause the first entry in
        the target list to be returned, and "target_defaulted" will be
        set in the BFD if @var{abfd} isn't <<NULL>>.  This causes
        <<bfd_check_format>> to loop over all the targets to find the
        one that matches the file being read.
	If @var{is_bigendian} is not <<NULL>>, then set this value to target's
	endian mode. True for big-endian, FALSE for little-endian or for
	invalid target.
	If @var{underscoring} is not <<NULL>>, then set this value to target's
	underscoring mode. Zero for none-underscoring, -1 for invalid target,
	else the value of target vector's symbol underscoring.
	If @var{def_target_arch} is not <<NULL>>, then set it to the architecture
	string specified by the target_name.
*/
const bfd_target *
bfd_get_target_info (const char *target_name, bfd *abfd,
		     bfd_boolean *is_bigendian,
		     int *underscoring, const char **def_target_arch)
{
  const bfd_target *target_vec;

  if (is_bigendian)
    *is_bigendian = FALSE;
  if (underscoring)
    *underscoring = -1;
  if (def_target_arch)
    *def_target_arch = NULL;
  target_vec = bfd_find_target (target_name, abfd);
  if (! target_vec)
    return NULL;
  if (is_bigendian)
    *is_bigendian = ((target_vec->byteorder == BFD_ENDIAN_BIG) ? TRUE
							       : FALSE);
  if (underscoring)
    *underscoring = ((int) target_vec->symbol_leading_char) & 0xff;

  if (def_target_arch)
    {
      const char *tname = target_vec->name;
      const char **arches = bfd_arch_list ();

      if (arches && tname)
        {
          char *hyp = strchr (tname, '-');

          if (hyp != NULL)
            {
              tname = ++hyp;

	      /* Make sure we detect architecture names
		 for triplets like "pe-arm-wince-little".  */
	      if (!_bfd_find_arch_match (tname, arches, def_target_arch))
		{
		  char new_tname[50];

		  strcpy (new_tname, hyp);
		  while ((hyp = strrchr (new_tname, '-')) != NULL)
		    {
		      *hyp = 0;
		      if (_bfd_find_arch_match (new_tname, arches,
						def_target_arch))
			break;
		    }
		}
	    }
	  else
	    _bfd_find_arch_match (tname, arches, def_target_arch);
	}

      if (arches)
        free (arches);
    }
  return target_vec;
}

/*
FUNCTION
	bfd_target_list

SYNOPSIS
	const char ** bfd_target_list (void);

DESCRIPTION
	Return a freshly malloced NULL-terminated
	vector of the names of all the valid BFD targets. Do not
	modify the names.

*/

const char **
bfd_target_list (void)
{
  int vec_length = 0;
  bfd_size_type amt;
  const bfd_target * const *target;
  const  char **name_list, **name_ptr;

  for (target = &bfd_target_vector[0]; *target != NULL; target++)
    vec_length++;

  amt = (vec_length + 1) * sizeof (char **);
  name_ptr = name_list = (const  char **) bfd_malloc (amt);

  if (name_list == NULL)
    return NULL;

  for (target = &bfd_target_vector[0]; *target != NULL; target++)
    if (target == &bfd_target_vector[0]
	|| *target != bfd_target_vector[0])
      *name_ptr++ = (*target)->name;

  *name_ptr = NULL;
  return name_list;
}

/*
FUNCTION
	bfd_seach_for_target

SYNOPSIS
	const bfd_target *bfd_search_for_target
	  (int (*search_func) (const bfd_target *, void *),
	   void *);

DESCRIPTION
	Return a pointer to the first transfer vector in the list of
	transfer vectors maintained by BFD that produces a non-zero
	result when passed to the function @var{search_func}.  The
	parameter @var{data} is passed, unexamined, to the search
	function.
*/

const bfd_target *
bfd_search_for_target (int (*search_func) (const bfd_target *, void *),
		       void *data)
{
  const bfd_target * const *target;

  for (target = bfd_target_vector; *target != NULL; target ++)
    if (search_func (*target, data))
      return *target;

  return NULL;
}

/*
FUNCTION
	bfd_flavour_name

SYNOPSIS
	const char *bfd_flavour_name (enum bfd_flavour flavour);

DESCRIPTION
	Return the string form of @var{flavour}.
*/

const char *
bfd_flavour_name (enum bfd_flavour flavour)
{
  switch (flavour)
    {
    case bfd_target_unknown_flavour: return "unknown file format";
    case bfd_target_aout_flavour: return "a.out";
    case bfd_target_coff_flavour: return "COFF";
    case bfd_target_ecoff_flavour: return "ECOFF";
    case bfd_target_xcoff_flavour: return "XCOFF";
    case bfd_target_elf_flavour: return "ELF";
    case bfd_target_ieee_flavour: return "IEEE";
    case bfd_target_nlm_flavour: return "NLM";
    case bfd_target_oasys_flavour: return "Oasys";
    case bfd_target_tekhex_flavour: return "Tekhex";
    case bfd_target_srec_flavour: return "Srec";
    case bfd_target_verilog_flavour: return "Verilog";
    case bfd_target_ihex_flavour: return "Ihex";
    case bfd_target_som_flavour: return "SOM";
    case bfd_target_os9k_flavour: return "OS9K";
    case bfd_target_versados_flavour: return "Versados";
    case bfd_target_msdos_flavour: return "MSDOS";
    case bfd_target_ovax_flavour: return "Ovax";
    case bfd_target_evax_flavour: return "Evax";
    case bfd_target_mmo_flavour: return "mmo";
    case bfd_target_mach_o_flavour: return "MACH_O";
    case bfd_target_pef_flavour: return "PEF";
    case bfd_target_pef_xlib_flavour: return "PEF_XLIB";
    case bfd_target_sym_flavour: return "SYM";
    /* There is no "default" case here so that -Wswitch (part of -Wall)
       catches missing entries.  */
    }

  abort ();
}
