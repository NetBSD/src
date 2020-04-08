/* ld.h -- general linker header file
   Copyright (C) 1991-2018 Free Software Foundation, Inc.

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

#ifndef LD_H
#define LD_H

#ifdef HAVE_LOCALE_H
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifdef HAVE_LOCALE_H
# ifndef ENABLE_NLS
   /* The Solaris version of locale.h always includes libintl.h.  If we have
      been configured with --disable-nls then ENABLE_NLS will not be defined
      and the dummy definitions of bindtextdomain (et al) below will conflict
      with the defintions in libintl.h.  So we define these values to prevent
      the bogus inclusion of libintl.h.  */
#  define _LIBINTL_H
#  define _LIBGETTEXT_H
# endif
# include <locale.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(String) gettext (String)
# ifdef gettext_noop
#  define N_(String) gettext_noop (String)
# else
#  define N_(String) (String)
# endif
#else
# define gettext(Msgid) (Msgid)
# define dgettext(Domainname, Msgid) (Msgid)
# define dcgettext(Domainname, Msgid, Category) (Msgid)
# define ngettext(Msgid1, Msgid2, n) \
  (n == 1 ? Msgid1 : Msgid2)
# define dngettext(Domainname, Msgid1, Msgid2, n) \
  (n == 1 ? Msgid1 : Msgid2)
# define dcngettext(Domainname, Msgid1, Msgid2, n, Category) \
  (n == 1 ? Msgid1 : Msgid2)
# define textdomain(Domainname) do {} while (0)
# define bindtextdomain(Domainname, Dirname) do {} while (0)
# define _(String) (String)
# define N_(String) (String)
#endif

/* Look in this environment name for the linker to pretend to be */
#define EMULATION_ENVIRON "LDEMULATION"
/* If in there look for the strings: */

/* Look in this variable for a target format */
#define TARGET_ENVIRON "GNUTARGET"

/* Input sections which are put in a section of this name are actually
   discarded.  */
#define DISCARD_SECTION_NAME "/DISCARD/"

/* A file name list.  */
typedef struct name_list
{
  const char *name;
  struct name_list *next;
}
name_list;

typedef enum {sort_none, sort_ascending, sort_descending} sort_order;

/* A wildcard specification.  */

typedef enum
{
  none, by_name, by_alignment, by_name_alignment, by_alignment_name,
  by_none, by_init_priority
} sort_type;

extern sort_type sort_section;

struct wildcard_spec
{
  const char *name;
  struct name_list *exclude_name_list;
  sort_type sorted;
  struct flag_info *section_flag_list;
};

struct wildcard_list
{
  struct wildcard_list *next;
  struct wildcard_spec spec;
};

#define BYTE_SIZE	(1)
#define SHORT_SIZE	(2)
#define LONG_SIZE	(4)
#define QUAD_SIZE	(8)

enum endian_enum { ENDIAN_UNSET = 0, ENDIAN_BIG, ENDIAN_LITTLE };

enum symbolic_enum
{
  symbolic_unset = 0,
  symbolic,
  symbolic_functions,
};

enum dynamic_list_enum
{
  dynamic_list_unset = 0,
  dynamic_list_data,
  dynamic_list
};

typedef struct
{
  /* 1 => assign space to common symbols even if `relocatable_output'.  */
  bfd_boolean force_common_definition;

  /* If TRUE, build MIPS embedded PIC relocation tables in the output
     file.  */
  bfd_boolean embedded_relocs;

  /* If TRUE, force generation of a file with a .exe file.  */
  bfd_boolean force_exe_suffix;

  /* If TRUE, generate a cross reference report.  */
  bfd_boolean cref;

  /* If TRUE (which is the default), warn about mismatched input
     files.  */
  bfd_boolean warn_mismatch;

  /* Warn on attempting to open an incompatible library during a library
     search.  */
  bfd_boolean warn_search_mismatch;

  /* If non-zero check section addresses, once computed,
     for overlaps.  Relocatable links only check when this is > 0.  */
  signed char check_section_addresses;

  /* If TRUE allow the linking of input files in an unknown architecture
     assuming that the user knows what they are doing.  This was the old
     behaviour of the linker.  The new default behaviour is to reject such
     input files.  */
  bfd_boolean accept_unknown_input_arch;

  /* Name of the import library to generate.  */
  char *out_implib_filename;

  /* If TRUE we'll just print the default output on stdout.  */
  bfd_boolean print_output_format;

  /* If set, display the target memory usage (per memory region).  */
  bfd_boolean print_memory_usage;

  /* Should we force section groups to be resolved?  Controlled with
     --force-group-allocation on the command line or FORCE_GROUP_ALLOCATION
     in the linker script.  */
  bfd_boolean force_group_allocation;

  /* Big or little endian as set on command line.  */
  enum endian_enum endian;

  /* -Bsymbolic and -Bsymbolic-functions, as set on command line.  */
  enum symbolic_enum symbolic;

  /* --dynamic-list, --dynamic-list-cpp-new, --dynamic-list-cpp-typeinfo
     and --dynamic-list FILE, as set on command line.  */
  enum dynamic_list_enum dynamic_list;

  /* Name of runtime interpreter to invoke.  */
  char *interpreter;

  /* Name to give runtime library from the -soname argument.  */
  char *soname;

  /* Runtime library search path from the -rpath argument.  */
  char *rpath;

  /* Link time runtime library search path from the -rpath-link
     argument.  */
  char *rpath_link;

  /* Name of shared object whose symbol table should be filtered with
     this shared object.  From the --filter option.  */
  char *filter_shlib;

  /* Name of shared object for whose symbol table this shared object
     is an auxiliary filter.  From the --auxiliary option.  */
  char **auxiliary_filters;

  /* A version symbol to be applied to the symbol names found in the
     .exports sections.  */
  char *version_exports_section;

  /* Default linker script.  */
  char *default_script;
} args_type;

extern args_type command_line;

typedef int token_code_type;

/* Different ways we can handle orphan sections.  */

enum orphan_handling_enum
{
  /* The classic strategy, find a suitable section to place the orphan
     into.  */
  orphan_handling_place = 0,

  /* Discard any orphan sections as though they were assign to the section
     /DISCARD/.  */
  orphan_handling_discard,

  /* Find somewhere to place the orphan section, as with
     ORPHAN_HANDLING_PLACE, but also issue a warning.  */
  orphan_handling_warn,

  /* Issue a fatal error if any orphan sections are found.  */
  orphan_handling_error,
};

typedef struct
{
  bfd_boolean magic_demand_paged;
  bfd_boolean make_executable;

  /* If TRUE, -shared is supported.  */
  /* ??? A better way to do this is perhaps to define this in the
     ld_emulation_xfer_struct since this is really a target dependent
     parameter.  */
  bfd_boolean has_shared;

  /* If TRUE, build constructors.  */
  bfd_boolean build_constructors;

  /* If TRUE, warn about any constructors.  */
  bfd_boolean warn_constructors;

  /* If TRUE, warn about merging common symbols with others.  */
  bfd_boolean warn_common;

  /* If TRUE, only warn once about a particular undefined symbol.  */
  bfd_boolean warn_once;

  /* How should we deal with orphan sections.  */
  enum orphan_handling_enum orphan_handling;

  /* If TRUE, warn if multiple global-pointers are needed (Alpha
     only).  */
  bfd_boolean warn_multiple_gp;

  /* If TRUE, warn if the starting address of an output section
     changes due to the alignment of an input section.  */
  bfd_boolean warn_section_align;

  /* If TRUE, warning messages are fatal */
  bfd_boolean fatal_warnings;

  sort_order sort_common;

  bfd_boolean text_read_only;

  bfd_boolean stats;

  /* If set, orphan input sections will be mapped to separate output
     sections.  */
  bfd_boolean unique_orphan_sections;

  /* If set, only search library directories explicitly selected
     on the command line.  */
  bfd_boolean only_cmd_line_lib_dirs;

  /* If set, numbers and absolute symbols are simply treated as
     numbers everywhere.  */
  bfd_boolean sane_expr;

  /* If set, code and non-code sections should never be in one segment.  */
  bfd_boolean separate_code;

  /* The rpath separation character.  Usually ':'.  */
  char rpath_separator;

  char *map_filename;
  FILE *map_file;

  unsigned int split_by_reloc;
  bfd_size_type split_by_file;

  /* The size of the hash table to use.  */
  unsigned long hash_table_size;

  /* The maximum page size for ELF.  */
  bfd_vma maxpagesize;

  /* The common page size for ELF.  */
  bfd_vma commonpagesize;
} ld_config_type;

extern ld_config_type config;

extern FILE * saved_script_handle;
extern bfd_boolean force_make_executable;

extern int yyparse (void);
extern void add_cref (const char *, bfd *, asection *, bfd_vma);
extern bfd_boolean handle_asneeded_cref (bfd *, enum notice_asneeded_action);
extern void output_cref (FILE *);
extern void check_nocrossrefs (void);
extern void ld_abort (const char *, int, const char *) ATTRIBUTE_NORETURN;

/* If gcc >= 2.6, we can give a function name, too.  */
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 6)
#define __PRETTY_FUNCTION__  NULL
#endif

#undef abort
#define abort() ld_abort (__FILE__, __LINE__, __PRETTY_FUNCTION__)

#endif
