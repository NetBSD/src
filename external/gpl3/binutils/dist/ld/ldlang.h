/* ldlang.h - linker command language support
   Copyright (C) 1991-2016 Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

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

#ifndef LDLANG_H
#define LDLANG_H

#define DEFAULT_MEMORY_REGION   "*default*"

typedef enum
{
  lang_input_file_is_l_enum,
  lang_input_file_is_symbols_only_enum,
  lang_input_file_is_marker_enum,
  lang_input_file_is_fake_enum,
  lang_input_file_is_search_file_enum,
  lang_input_file_is_file_enum
} lang_input_file_enum_type;

struct _fill_type
{
  size_t size;
  unsigned char data[1];
};

typedef struct statement_list
{
  union lang_statement_union *  head;
  union lang_statement_union ** tail;
} lang_statement_list_type;

typedef struct memory_region_name_struct
{
  const char * name;
  struct memory_region_name_struct * next;
} lang_memory_region_name;

typedef struct memory_region_struct
{
  lang_memory_region_name name_list;
  struct memory_region_struct *next;
  union etree_union *origin_exp;
  bfd_vma origin;
  bfd_size_type length;
  union etree_union *length_exp;
  bfd_vma current;
  union lang_statement_union *last_os;
  flagword flags;
  flagword not_flags;
  bfd_boolean had_full_message;
} lang_memory_region_type;

enum statement_enum
{
  lang_output_section_statement_enum,
  lang_assignment_statement_enum,
  lang_input_statement_enum,
  lang_address_statement_enum,
  lang_wild_statement_enum,
  lang_input_section_enum,
  lang_object_symbols_statement_enum,
  lang_fill_statement_enum,
  lang_data_statement_enum,
  lang_reloc_statement_enum,
  lang_target_statement_enum,
  lang_output_statement_enum,
  lang_padding_statement_enum,
  lang_group_statement_enum,
  lang_insert_statement_enum,
  lang_constructors_statement_enum
};

typedef struct lang_statement_header_struct
{
  union lang_statement_union *next;
  enum statement_enum type;
} lang_statement_header_type;

typedef struct
{
  lang_statement_header_type header;
  union etree_union *exp;
} lang_assignment_statement_type;

typedef struct lang_target_statement_struct
{
  lang_statement_header_type header;
  const char *target;
} lang_target_statement_type;

typedef struct lang_output_statement_struct
{
  lang_statement_header_type header;
  const char *name;
} lang_output_statement_type;

/* Section types specified in a linker script.  */

enum section_type
{
  normal_section,
  overlay_section,
  noload_section,
  noalloc_section
};

/* This structure holds a list of program headers describing
   segments in which this section should be placed.  */

typedef struct lang_output_section_phdr_list
{
  struct lang_output_section_phdr_list *next;
  const char *name;
  bfd_boolean used;
} lang_output_section_phdr_list;

typedef struct lang_output_section_statement_struct
{
  lang_statement_header_type header;
  lang_statement_list_type children;
  struct lang_output_section_statement_struct *next;
  struct lang_output_section_statement_struct *prev;
  const char *name;
  asection *bfd_section;
  lang_memory_region_type *region;
  lang_memory_region_type *lma_region;
  fill_type *fill;
  union etree_union *addr_tree;
  union etree_union *load_base;

  /* If non-null, an expression to evaluate after setting the section's
     size.  The expression is evaluated inside REGION (above) with '.'
     set to the end of the section.  Used in the last overlay section
     to move '.' past all the overlaid sections.  */
  union etree_union *update_dot_tree;

  lang_output_section_phdr_list *phdrs;

  unsigned int block_value;
  int subsection_alignment;	/* Alignment of components.  */
  int section_alignment;	/* Alignment of start of section.  */
  int constraint;
  flagword flags;
  enum section_type sectype;
  unsigned int processed_vma : 1;
  unsigned int processed_lma : 1;
  unsigned int all_input_readonly : 1;
  /* If this section should be ignored.  */
  unsigned int ignored : 1;
  /* If this section should update "dot".  Prevents section being ignored.  */
  unsigned int update_dot : 1;
  /* If this section is after assignment to _end.  */
  unsigned int after_end : 1;
  /* If this section uses the alignment of its input sections.  */
  unsigned int align_lma_with_input : 1;
} lang_output_section_statement_type;

typedef struct
{
  lang_statement_header_type header;
} lang_common_statement_type;

typedef struct
{
  lang_statement_header_type header;
} lang_object_symbols_statement_type;

typedef struct
{
  lang_statement_header_type header;
  fill_type *fill;
  int size;
  asection *output_section;
} lang_fill_statement_type;

typedef struct
{
  lang_statement_header_type header;
  unsigned int type;
  union etree_union *exp;
  bfd_vma value;
  asection *output_section;
  bfd_vma output_offset;
} lang_data_statement_type;

/* Generate a reloc in the output file.  */

typedef struct
{
  lang_statement_header_type header;

  /* Reloc to generate.  */
  bfd_reloc_code_real_type reloc;

  /* Reloc howto structure.  */
  reloc_howto_type *howto;

  /* Section to generate reloc against.
     Exactly one of section and name must be NULL.  */
  asection *section;

  /* Name of symbol to generate reloc against.
     Exactly one of section and name must be NULL.  */
  const char *name;

  /* Expression for addend.  */
  union etree_union *addend_exp;

  /* Resolved addend.  */
  bfd_vma addend_value;

  /* Output section where reloc should be performed.  */
  asection *output_section;

  /* Offset within output section.  */
  bfd_vma output_offset;
} lang_reloc_statement_type;

struct lang_input_statement_flags
{
  /* 1 means this file was specified in a -l option.  */
  unsigned int maybe_archive : 1;

  /* 1 means this file was specified in a -l:namespec option.  */
  unsigned int full_name_provided : 1;

  /* 1 means search a set of directories for this file.  */
  unsigned int search_dirs : 1;

  /* 1 means this was found when processing a script in the sysroot.  */
  unsigned int sysrooted : 1;

  /* 1 means this is base file of incremental load.
     Do not load this file's text or data.
     Also default text_start to after this file's bss.  */
  unsigned int just_syms : 1;

  /* Whether to search for this entry as a dynamic archive.  */
  unsigned int dynamic : 1;

  /* Set if a DT_NEEDED tag should be added not just for the dynamic library
     explicitly given by this entry but also for any dynamic libraries in
     this entry's needed list.  */
  unsigned int add_DT_NEEDED_for_dynamic : 1;

  /* Set if this entry should cause a DT_NEEDED tag only when some
     regular file references its symbols (ie. --as-needed is in effect).  */
  unsigned int add_DT_NEEDED_for_regular : 1;

  /* Whether to include the entire contents of an archive.  */
  unsigned int whole_archive : 1;

  /* Set when bfd opening is successful.  */
  unsigned int loaded : 1;

  unsigned int real : 1;

  /* Set if the file does not exist.  */
  unsigned int missing_file : 1;

  /* Set if reloading an archive or --as-needed lib.  */
  unsigned int reload : 1;

#ifdef ENABLE_PLUGINS
  /* Set if the file was claimed by a plugin.  */
  unsigned int claimed : 1;

  /* Set if the file was claimed from an archive.  */
  unsigned int claim_archive : 1;

  /* Set if added by the lto plugin add_input_file callback.  */
  unsigned int lto_output : 1;
#endif /* ENABLE_PLUGINS */

  /* Head of list of pushed flags.  */
  struct lang_input_statement_flags *pushed;
};

typedef struct lang_input_statement_struct
{
  lang_statement_header_type header;
  /* Name of this file.  */
  const char *filename;
  /* Name to use for the symbol giving address of text start.
     Usually the same as filename, but for a file spec'd with
     -l this is the -l switch itself rather than the filename.  */
  const char *local_sym_name;

  bfd *the_bfd;

  struct flag_info *section_flag_list;

  /* Point to the next file - whatever it is, wanders up and down
     archives */
  union lang_statement_union *next;

  /* Point to the next file, but skips archive contents.  */
  union lang_statement_union *next_real_file;

  const char *target;

  struct lang_input_statement_flags flags;
} lang_input_statement_type;

typedef struct
{
  lang_statement_header_type header;
  asection *section;
} lang_input_section_type;

struct map_symbol_def {
  struct bfd_link_hash_entry *entry;
  struct map_symbol_def *next;
};

/* For input sections, when writing a map file: head / tail of a linked
   list of hash table entries for symbols defined in this section.  */
typedef struct input_section_userdata_struct
{
  struct map_symbol_def *map_symbol_def_head;
  struct map_symbol_def **map_symbol_def_tail;
  unsigned long map_symbol_def_count;
} input_section_userdata_type;

#define get_userdata(x) ((x)->userdata)


typedef struct lang_wild_statement_struct lang_wild_statement_type;

typedef void (*callback_t) (lang_wild_statement_type *, struct wildcard_list *,
			    asection *, struct flag_info *,
			    lang_input_statement_type *, void *);

typedef void (*walk_wild_section_handler_t) (lang_wild_statement_type *,
					     lang_input_statement_type *,
					     callback_t callback,
					     void *data);

typedef bfd_boolean (*lang_match_sec_type_func) (bfd *, const asection *,
						 bfd *, const asection *);

/* Binary search tree structure to efficiently sort sections by
   name.  */
typedef struct lang_section_bst
{
  asection *section;
  struct lang_section_bst *left;
  struct lang_section_bst *right;
} lang_section_bst_type;

struct lang_wild_statement_struct
{
  lang_statement_header_type header;
  const char *filename;
  bfd_boolean filenames_sorted;
  struct wildcard_list *section_list;
  bfd_boolean keep_sections;
  lang_statement_list_type children;

  walk_wild_section_handler_t walk_wild_section_handler;
  struct wildcard_list *handler_data[4];
  lang_section_bst_type *tree;
  struct flag_info *section_flag_list;
};

typedef struct lang_address_statement_struct
{
  lang_statement_header_type header;
  const char *section_name;
  union etree_union *address;
  const segment_type *segment;
} lang_address_statement_type;

typedef struct
{
  lang_statement_header_type header;
  bfd_vma output_offset;
  bfd_size_type size;
  asection *output_section;
  fill_type *fill;
} lang_padding_statement_type;

/* A group statement collects a set of libraries together.  The
   libraries are searched multiple times, until no new undefined
   symbols are found.  The effect is to search a group of libraries as
   though they were a single library.  */

typedef struct
{
  lang_statement_header_type header;
  lang_statement_list_type children;
} lang_group_statement_type;

typedef struct
{
  lang_statement_header_type header;
  const char *where;
  bfd_boolean is_before;
} lang_insert_statement_type;

typedef union lang_statement_union
{
  lang_statement_header_type header;
  lang_wild_statement_type wild_statement;
  lang_data_statement_type data_statement;
  lang_reloc_statement_type reloc_statement;
  lang_address_statement_type address_statement;
  lang_output_section_statement_type output_section_statement;
  lang_assignment_statement_type assignment_statement;
  lang_input_statement_type input_statement;
  lang_target_statement_type target_statement;
  lang_output_statement_type output_statement;
  lang_input_section_type input_section;
  lang_common_statement_type common_statement;
  lang_object_symbols_statement_type object_symbols_statement;
  lang_fill_statement_type fill_statement;
  lang_padding_statement_type padding_statement;
  lang_group_statement_type group_statement;
  lang_insert_statement_type insert_statement;
} lang_statement_union_type;

/* This structure holds information about a program header, from the
   PHDRS command in the linker script.  */

struct lang_phdr
{
  struct lang_phdr *next;
  const char *name;
  unsigned long type;
  bfd_boolean filehdr;
  bfd_boolean phdrs;
  etree_type *at;
  etree_type *flags;
};

/* This structure is used to hold a list of sections which may not
   cross reference each other.  */

typedef struct lang_nocrossref
{
  struct lang_nocrossref *next;
  const char *name;
} lang_nocrossref_type;

/* The list of nocrossref lists.  */

struct lang_nocrossrefs
{
  struct lang_nocrossrefs *next;
  lang_nocrossref_type *list;
  bfd_boolean onlyfirst;
};

/* This structure is used to hold a list of input section names which
   will not match an output section in the linker script.  */

struct unique_sections
{
  struct unique_sections *next;
  const char *name;
};

/* Used by place_orphan to keep track of orphan sections and statements.  */

struct orphan_save
{
  const char *name;
  flagword flags;
  lang_output_section_statement_type *os;
  asection **section;
  lang_statement_union_type **stmt;
  lang_output_section_statement_type **os_tail;
};

struct asneeded_minfo
{
  struct asneeded_minfo *next;
  const char *soname;
  bfd *ref;
  const char *name;
};

extern struct lang_phdr *lang_phdr_list;
extern struct lang_nocrossrefs *nocrossref_list;
extern const char *output_target;
extern lang_output_section_statement_type *abs_output_section;
extern lang_statement_list_type lang_output_section_statement;
extern struct lang_input_statement_flags input_flags;
extern bfd_boolean lang_has_input_file;
extern lang_statement_list_type *stat_ptr;
extern bfd_boolean delete_output_file_on_failure;

extern struct bfd_sym_chain entry_symbol;
extern const char *entry_section;
extern bfd_boolean entry_from_cmdline;
extern lang_statement_list_type file_chain;
extern lang_statement_list_type input_file_chain;

extern int lang_statement_iteration;
extern struct asneeded_minfo **asneeded_list_tail;

extern void (*output_bfd_hash_table_free_fn) (struct bfd_link_hash_table *);

extern void lang_init
  (void);
extern void lang_finish
  (void);
extern lang_memory_region_type * lang_memory_region_lookup
  (const char * const, bfd_boolean);
extern void lang_memory_region_alias
  (const char *, const char *);
extern void lang_map
  (void);
extern void lang_set_flags
  (lang_memory_region_type *, const char *, int);
extern void lang_add_output
  (const char *, int from_script);
extern lang_output_section_statement_type *lang_enter_output_section_statement
  (const char *, etree_type *, enum section_type, etree_type *, etree_type *,
   etree_type *, int, int);
extern void lang_final
  (void);
extern void lang_relax_sections
  (bfd_boolean);
extern void lang_process
  (void);
extern void lang_section_start
  (const char *, union etree_union *, const segment_type *);
extern void lang_add_entry
  (const char *, bfd_boolean);
extern void lang_default_entry
  (const char *);
extern void lang_add_target
  (const char *);
extern void lang_add_wild
  (struct wildcard_spec *, struct wildcard_list *, bfd_boolean);
extern void lang_add_map
  (const char *);
extern void lang_add_fill
  (fill_type *);
extern lang_assignment_statement_type *lang_add_assignment
  (union etree_union *);
extern void lang_add_attribute
  (enum statement_enum);
extern void lang_startup
  (const char *);
extern void lang_float
  (bfd_boolean);
extern void lang_leave_output_section_statement
  (fill_type *, const char *, lang_output_section_phdr_list *,
   const char *);
extern void lang_statement_append
  (lang_statement_list_type *, lang_statement_union_type *,
   lang_statement_union_type **);
extern void lang_for_each_input_file
  (void (*dothis) (lang_input_statement_type *));
extern void lang_for_each_file
  (void (*dothis) (lang_input_statement_type *));
extern void lang_reset_memory_regions
  (void);
extern void lang_do_assignments
  (lang_phase_type);
extern asection *section_for_dot
  (void);

#define LANG_FOR_EACH_INPUT_STATEMENT(statement)			\
  lang_input_statement_type *statement;					\
  for (statement = (lang_input_statement_type *) file_chain.head;	\
       statement != (lang_input_statement_type *) NULL;			\
       statement = (lang_input_statement_type *) statement->next)	\

#define lang_output_section_find(NAME) \
  lang_output_section_statement_lookup (NAME, 0, FALSE)

extern void lang_process
  (void);
extern void ldlang_add_file
  (lang_input_statement_type *);
extern lang_output_section_statement_type *lang_output_section_find_by_flags
  (const asection *, flagword, lang_output_section_statement_type **,
   lang_match_sec_type_func);
extern lang_output_section_statement_type *lang_insert_orphan
  (asection *, const char *, int, lang_output_section_statement_type *,
   struct orphan_save *, etree_type *, lang_statement_list_type *);
extern lang_input_statement_type *lang_add_input_file
  (const char *, lang_input_file_enum_type, const char *);
extern void lang_add_keepsyms_file
  (const char *);
extern lang_output_section_statement_type *lang_output_section_get
  (const asection *);
extern lang_output_section_statement_type *lang_output_section_statement_lookup
  (const char *, int, bfd_boolean);
extern lang_output_section_statement_type *next_matching_output_section_statement
  (lang_output_section_statement_type *, int);
extern void ldlang_add_undef
  (const char *const, bfd_boolean);
extern void ldlang_add_require_defined
  (const char *const);
extern void lang_add_output_format
  (const char *, const char *, const char *, int);
extern void lang_list_init
  (lang_statement_list_type *);
extern void push_stat_ptr
  (lang_statement_list_type *);
extern void pop_stat_ptr
  (void);
extern void lang_add_data
  (int type, union etree_union *);
extern void lang_add_reloc
  (bfd_reloc_code_real_type, reloc_howto_type *, asection *, const char *,
   union etree_union *);
extern void lang_for_each_statement
  (void (*) (lang_statement_union_type *));
extern void lang_for_each_statement_worker
  (void (*) (lang_statement_union_type *), lang_statement_union_type *);
extern void *stat_alloc
  (size_t);
extern void strip_excluded_output_sections
  (void);
extern void lang_clear_os_map
  (void);
extern void dprint_statement
  (lang_statement_union_type *, int);
extern void lang_size_sections
  (bfd_boolean *, bfd_boolean);
extern void one_lang_size_sections_pass
  (bfd_boolean *, bfd_boolean);
extern void lang_add_insert
  (const char *, int);
extern void lang_enter_group
  (void);
extern void lang_leave_group
  (void);
extern void lang_add_section
  (lang_statement_list_type *, asection *,
   struct flag_info *, lang_output_section_statement_type *);
extern void lang_new_phdr
  (const char *, etree_type *, bfd_boolean, bfd_boolean, etree_type *,
   etree_type *);
extern void lang_add_nocrossref
  (lang_nocrossref_type *);
extern void lang_add_nocrossref_to
  (lang_nocrossref_type *);
extern void lang_enter_overlay
  (etree_type *, etree_type *);
extern void lang_enter_overlay_section
  (const char *);
extern void lang_leave_overlay_section
  (fill_type *, lang_output_section_phdr_list *);
extern void lang_leave_overlay
  (etree_type *, int, fill_type *, const char *,
   lang_output_section_phdr_list *, const char *);

extern struct bfd_elf_version_expr *lang_new_vers_pattern
  (struct bfd_elf_version_expr *, const char *, const char *, bfd_boolean);
extern struct bfd_elf_version_tree *lang_new_vers_node
  (struct bfd_elf_version_expr *, struct bfd_elf_version_expr *);
extern struct bfd_elf_version_deps *lang_add_vers_depend
  (struct bfd_elf_version_deps *, const char *);
extern void lang_register_vers_node
  (const char *, struct bfd_elf_version_tree *, struct bfd_elf_version_deps *);
extern void lang_append_dynamic_list (struct bfd_elf_version_expr *);
extern void lang_append_dynamic_list_cpp_typeinfo (void);
extern void lang_append_dynamic_list_cpp_new (void);
extern void lang_add_unique
  (const char *);
extern const char *lang_get_output_target
  (void);
extern void add_excluded_libs (const char *);
extern bfd_boolean load_symbols
  (lang_input_statement_type *, lang_statement_list_type *);

extern bfd_boolean
ldlang_override_segment_assignment
  (struct bfd_link_info *, bfd *, asection *, asection *, bfd_boolean);

extern void
lang_ld_feature (char *);

extern void
lang_print_memory_usage (void);

extern void
lang_add_gc_name (const char *);

#endif
