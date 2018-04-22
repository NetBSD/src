/* objcopy.c -- copy object file from input to output, optionally massaging it.
   Copyright (C) 1991-2018 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "progress.h"
#include "getopt.h"
#include "libiberty.h"
#include "bucomm.h"
#include "budbg.h"
#include "filenames.h"
#include "fnmatch.h"
#include "elf-bfd.h"
#include "coff/internal.h"
#include "libcoff.h"
#include "safe-ctype.h"

/* FIXME: See bfd/peXXigen.c for why we include an architecture specific
   header in generic PE code.  */
#include "coff/i386.h"
#include "coff/pe.h"

static bfd_vma pe_file_alignment = (bfd_vma) -1;
static bfd_vma pe_heap_commit = (bfd_vma) -1;
static bfd_vma pe_heap_reserve = (bfd_vma) -1;
static bfd_vma pe_image_base = (bfd_vma) -1;
static bfd_vma pe_section_alignment = (bfd_vma) -1;
static bfd_vma pe_stack_commit = (bfd_vma) -1;
static bfd_vma pe_stack_reserve = (bfd_vma) -1;
static short pe_subsystem = -1;
static short pe_major_subsystem_version = -1;
static short pe_minor_subsystem_version = -1;

struct is_specified_symbol_predicate_data
{
  const char *  name;
  bfd_boolean	found;
};

/* A node includes symbol name mapping to support redefine_sym.  */
struct redefine_node
{
  char *source;
  char *target;
};

struct addsym_node
{
  struct addsym_node *next;
  char *    symdef;
  long      symval;
  flagword  flags;
  char *    section;
  char *    othersym;
};

typedef struct section_rename
{
  const char *            old_name;
  const char *            new_name;
  flagword                flags;
  struct section_rename * next;
}
section_rename;

/* List of sections to be renamed.  */
static section_rename *section_rename_list;

static asymbol **isympp = NULL;	/* Input symbols.  */
static asymbol **osympp = NULL;	/* Output symbols that survive stripping.  */

/* If `copy_byte' >= 0, copy 'copy_width' byte(s) of every `interleave' bytes.  */
static int copy_byte = -1;
static int interleave = 0; /* Initialised to 4 in copy_main().  */
static int copy_width = 1;

static bfd_boolean verbose;		/* Print file and target names.  */
static bfd_boolean preserve_dates;	/* Preserve input file timestamp.  */
static int deterministic = -1;		/* Enable deterministic archives.  */
static int status = 0;			/* Exit status.  */

static bfd_boolean    merge_notes = FALSE;	/* Merge note sections.  */
static bfd_byte *     merged_notes = NULL;	/* Contents on note section undergoing a merge.  */
static bfd_size_type  merged_size = 0;		/* New, smaller size of the merged note section.  */

enum strip_action
{
  STRIP_UNDEF,
  STRIP_NONE,		/* Don't strip.  */
  STRIP_DEBUG,		/* Strip all debugger symbols.  */
  STRIP_UNNEEDED,	/* Strip unnecessary symbols.  */
  STRIP_NONDEBUG,	/* Strip everything but debug info.  */
  STRIP_DWO,		/* Strip all DWO info.  */
  STRIP_NONDWO,		/* Strip everything but DWO info.  */
  STRIP_ALL		/* Strip all symbols.  */
};

/* Which symbols to remove.  */
static enum strip_action strip_symbols = STRIP_UNDEF;

enum locals_action
{
  LOCALS_UNDEF,
  LOCALS_START_L,	/* Discard locals starting with L.  */
  LOCALS_ALL		/* Discard all locals.  */
};

/* Which local symbols to remove.  Overrides STRIP_ALL.  */
static enum locals_action discard_locals;

/* Structure used to hold lists of sections and actions to take.  */
struct section_list
{
  struct section_list * next;	   /* Next section to change.  */
  const char *		pattern;   /* Section name pattern.  */
  bfd_boolean		used;	   /* Whether this entry was used.  */

  unsigned int          context;   /* What to do with matching sections.  */
  /* Flag bits used in the context field.
     COPY and REMOVE are mutually exlusive.  SET and ALTER are mutually exclusive.  */
#define SECTION_CONTEXT_REMOVE    (1 << 0) /* Remove this section.  */
#define SECTION_CONTEXT_COPY      (1 << 1) /* Copy this section, delete all non-copied section.  */
#define SECTION_CONTEXT_SET_VMA   (1 << 2) /* Set the sections' VMA address.  */
#define SECTION_CONTEXT_ALTER_VMA (1 << 3) /* Increment or decrement the section's VMA address.  */
#define SECTION_CONTEXT_SET_LMA   (1 << 4) /* Set the sections' LMA address.  */
#define SECTION_CONTEXT_ALTER_LMA (1 << 5) /* Increment or decrement the section's LMA address.  */
#define SECTION_CONTEXT_SET_FLAGS (1 << 6) /* Set the section's flags.  */
#define SECTION_CONTEXT_REMOVE_RELOCS (1 << 7) /* Remove relocations for this section.  */

  bfd_vma		vma_val;   /* Amount to change by or set to.  */
  bfd_vma		lma_val;   /* Amount to change by or set to.  */
  flagword		flags;	   /* What to set the section flags to.	 */
};

static struct section_list *change_sections;

/* TRUE if some sections are to be removed.  */
static bfd_boolean sections_removed;

/* TRUE if only some sections are to be copied.  */
static bfd_boolean sections_copied;

/* Changes to the start address.  */
static bfd_vma change_start = 0;
static bfd_boolean set_start_set = FALSE;
static bfd_vma set_start;

/* Changes to section addresses.  */
static bfd_vma change_section_address = 0;

/* Filling gaps between sections.  */
static bfd_boolean gap_fill_set = FALSE;
static bfd_byte gap_fill = 0;

/* Pad to a given address.  */
static bfd_boolean pad_to_set = FALSE;
static bfd_vma pad_to;

/* Use alternative machine code?  */
static unsigned long use_alt_mach_code = 0;

/* Output BFD flags user wants to set or clear */
static flagword bfd_flags_to_set;
static flagword bfd_flags_to_clear;

/* List of sections to add.  */
struct section_add
{
  /* Next section to add.  */
  struct section_add *next;
  /* Name of section to add.  */
  const char *name;
  /* Name of file holding section contents.  */
  const char *filename;
  /* Size of file.  */
  size_t size;
  /* Contents of file.  */
  bfd_byte *contents;
  /* BFD section, after it has been added.  */
  asection *section;
};

/* List of sections to add to the output BFD.  */
static struct section_add *add_sections;

/* List of sections to update in the output BFD.  */
static struct section_add *update_sections;

/* List of sections to dump from the output BFD.  */
static struct section_add *dump_sections;

/* If non-NULL the argument to --add-gnu-debuglink.
   This should be the filename to store in the .gnu_debuglink section.  */
static const char * gnu_debuglink_filename = NULL;

/* Whether to convert debugging information.  */
static bfd_boolean convert_debugging = FALSE;

/* Whether to compress/decompress DWARF debug sections.  */
static enum
{
  nothing = 0,
  compress = 1 << 0,
  compress_zlib = compress | 1 << 1,
  compress_gnu_zlib = compress | 1 << 2,
  compress_gabi_zlib = compress | 1 << 3,
  decompress = 1 << 4
} do_debug_sections = nothing;

/* Whether to generate ELF common symbols with the STT_COMMON type.  */
static enum bfd_link_elf_stt_common do_elf_stt_common = unchanged;

/* Whether to change the leading character in symbol names.  */
static bfd_boolean change_leading_char = FALSE;

/* Whether to remove the leading character from global symbol names.  */
static bfd_boolean remove_leading_char = FALSE;

/* Whether to permit wildcard in symbol comparison.  */
static bfd_boolean wildcard = FALSE;

/* True if --localize-hidden is in effect.  */
static bfd_boolean localize_hidden = FALSE;

/* List of symbols to strip, keep, localize, keep-global, weaken,
   or redefine.  */
static htab_t strip_specific_htab = NULL;
static htab_t strip_unneeded_htab = NULL;
static htab_t keep_specific_htab = NULL;
static htab_t localize_specific_htab = NULL;
static htab_t globalize_specific_htab = NULL;
static htab_t keepglobal_specific_htab = NULL;
static htab_t weaken_specific_htab = NULL;
static htab_t redefine_specific_htab = NULL;
static htab_t redefine_specific_reverse_htab = NULL;
static struct addsym_node *add_sym_list = NULL, **add_sym_tail = &add_sym_list;
static int add_symbols = 0;

/* If this is TRUE, we weaken global symbols (set BSF_WEAK).  */
static bfd_boolean weaken = FALSE;

/* If this is TRUE, we retain BSF_FILE symbols.  */
static bfd_boolean keep_file_symbols = FALSE;

/* Prefix symbols/sections.  */
static char *prefix_symbols_string = 0;
static char *prefix_sections_string = 0;
static char *prefix_alloc_sections_string = 0;

/* True if --extract-symbol was passed on the command line.  */
static bfd_boolean extract_symbol = FALSE;

/* If `reverse_bytes' is nonzero, then reverse the order of every chunk
   of <reverse_bytes> bytes within each output section.  */
static int reverse_bytes = 0;

/* For Coff objects, we may want to allow or disallow long section names,
   or preserve them where found in the inputs.  Debug info relies on them.  */
enum long_section_name_handling
{
  DISABLE,
  ENABLE,
  KEEP
};

/* The default long section handling mode is to preserve them.
   This is also the only behaviour for 'strip'.  */
static enum long_section_name_handling long_section_names = KEEP;

/* 150 isn't special; it's just an arbitrary non-ASCII char value.  */
enum command_line_switch
{
  OPTION_ADD_SECTION=150,
  OPTION_ADD_GNU_DEBUGLINK,
  OPTION_ADD_SYMBOL,
  OPTION_ALT_MACH_CODE,
  OPTION_CHANGE_ADDRESSES,
  OPTION_CHANGE_LEADING_CHAR,
  OPTION_CHANGE_SECTION_ADDRESS,
  OPTION_CHANGE_SECTION_LMA,
  OPTION_CHANGE_SECTION_VMA,
  OPTION_CHANGE_START,
  OPTION_CHANGE_WARNINGS,
  OPTION_COMPRESS_DEBUG_SECTIONS,
  OPTION_DEBUGGING,
  OPTION_DECOMPRESS_DEBUG_SECTIONS,
  OPTION_DUMP_SECTION,
  OPTION_ELF_STT_COMMON,
  OPTION_EXTRACT_DWO,
  OPTION_EXTRACT_SYMBOL,
  OPTION_FILE_ALIGNMENT,
  OPTION_FORMATS_INFO,
  OPTION_GAP_FILL,
  OPTION_GLOBALIZE_SYMBOL,
  OPTION_GLOBALIZE_SYMBOLS,
  OPTION_HEAP,
  OPTION_IMAGE_BASE,
  OPTION_IMPURE,
  OPTION_INTERLEAVE_WIDTH,
  OPTION_KEEPGLOBAL_SYMBOLS,
  OPTION_KEEP_FILE_SYMBOLS,
  OPTION_KEEP_SYMBOLS,
  OPTION_LOCALIZE_HIDDEN,
  OPTION_LOCALIZE_SYMBOLS,
  OPTION_LONG_SECTION_NAMES,
  OPTION_MERGE_NOTES,
  OPTION_NO_MERGE_NOTES,
  OPTION_NO_CHANGE_WARNINGS,
  OPTION_ONLY_KEEP_DEBUG,
  OPTION_PAD_TO,
  OPTION_PREFIX_ALLOC_SECTIONS,
  OPTION_PREFIX_SECTIONS,
  OPTION_PREFIX_SYMBOLS,
  OPTION_PURE,
  OPTION_READONLY_TEXT,
  OPTION_REDEFINE_SYM,
  OPTION_REDEFINE_SYMS,
  OPTION_REMOVE_LEADING_CHAR,
  OPTION_REMOVE_RELOCS,
  OPTION_RENAME_SECTION,
  OPTION_REVERSE_BYTES,
  OPTION_SECTION_ALIGNMENT,
  OPTION_SET_SECTION_FLAGS,
  OPTION_SET_START,
  OPTION_SREC_FORCES3,
  OPTION_SREC_LEN,
  OPTION_STACK,
  OPTION_STRIP_DWO,
  OPTION_STRIP_SYMBOLS,
  OPTION_STRIP_UNNEEDED,
  OPTION_STRIP_UNNEEDED_SYMBOL,
  OPTION_STRIP_UNNEEDED_SYMBOLS,
  OPTION_SUBSYSTEM,
  OPTION_UPDATE_SECTION,
  OPTION_WEAKEN,
  OPTION_WEAKEN_SYMBOLS,
  OPTION_WRITABLE_TEXT
};

/* Options to handle if running as "strip".  */

static struct option strip_options[] =
{
  {"disable-deterministic-archives", no_argument, 0, 'U'},
  {"discard-all", no_argument, 0, 'x'},
  {"discard-locals", no_argument, 0, 'X'},
  {"enable-deterministic-archives", no_argument, 0, 'D'},
  {"format", required_argument, 0, 'F'}, /* Obsolete */
  {"help", no_argument, 0, 'h'},
  {"info", no_argument, 0, OPTION_FORMATS_INFO},
  {"input-format", required_argument, 0, 'I'}, /* Obsolete */
  {"input-target", required_argument, 0, 'I'},
  {"keep-file-symbols", no_argument, 0, OPTION_KEEP_FILE_SYMBOLS},
  {"keep-symbol", required_argument, 0, 'K'},
  {"merge-notes", no_argument, 0, 'M'},
  {"no-merge-notes", no_argument, 0, OPTION_NO_MERGE_NOTES},
  {"only-keep-debug", no_argument, 0, OPTION_ONLY_KEEP_DEBUG},
  {"output-file", required_argument, 0, 'o'},
  {"output-format", required_argument, 0, 'O'},	/* Obsolete */
  {"output-target", required_argument, 0, 'O'},
  {"preserve-dates", no_argument, 0, 'p'},
  {"remove-section", required_argument, 0, 'R'},
  {"remove-relocations", required_argument, 0, OPTION_REMOVE_RELOCS},
  {"strip-all", no_argument, 0, 's'},
  {"strip-debug", no_argument, 0, 'S'},
  {"strip-dwo", no_argument, 0, OPTION_STRIP_DWO},
  {"strip-symbol", required_argument, 0, 'N'},
  {"strip-unneeded", no_argument, 0, OPTION_STRIP_UNNEEDED},
  {"target", required_argument, 0, 'F'},
  {"verbose", no_argument, 0, 'v'},
  {"version", no_argument, 0, 'V'},
  {"wildcard", no_argument, 0, 'w'},
  {0, no_argument, 0, 0}
};

/* Options to handle if running as "objcopy".  */

static struct option copy_options[] =
{
  {"add-gnu-debuglink", required_argument, 0, OPTION_ADD_GNU_DEBUGLINK},
  {"add-section", required_argument, 0, OPTION_ADD_SECTION},
  {"add-symbol", required_argument, 0, OPTION_ADD_SYMBOL},
  {"adjust-section-vma", required_argument, 0, OPTION_CHANGE_SECTION_ADDRESS},
  {"adjust-start", required_argument, 0, OPTION_CHANGE_START},
  {"adjust-vma", required_argument, 0, OPTION_CHANGE_ADDRESSES},
  {"adjust-warnings", no_argument, 0, OPTION_CHANGE_WARNINGS},
  {"alt-machine-code", required_argument, 0, OPTION_ALT_MACH_CODE},
  {"binary-architecture", required_argument, 0, 'B'},
  {"byte", required_argument, 0, 'b'},
  {"change-addresses", required_argument, 0, OPTION_CHANGE_ADDRESSES},
  {"change-leading-char", no_argument, 0, OPTION_CHANGE_LEADING_CHAR},
  {"change-section-address", required_argument, 0, OPTION_CHANGE_SECTION_ADDRESS},
  {"change-section-lma", required_argument, 0, OPTION_CHANGE_SECTION_LMA},
  {"change-section-vma", required_argument, 0, OPTION_CHANGE_SECTION_VMA},
  {"change-start", required_argument, 0, OPTION_CHANGE_START},
  {"change-warnings", no_argument, 0, OPTION_CHANGE_WARNINGS},
  {"compress-debug-sections", optional_argument, 0, OPTION_COMPRESS_DEBUG_SECTIONS},
  {"debugging", no_argument, 0, OPTION_DEBUGGING},
  {"decompress-debug-sections", no_argument, 0, OPTION_DECOMPRESS_DEBUG_SECTIONS},
  {"disable-deterministic-archives", no_argument, 0, 'U'},
  {"discard-all", no_argument, 0, 'x'},
  {"discard-locals", no_argument, 0, 'X'},
  {"dump-section", required_argument, 0, OPTION_DUMP_SECTION},
  {"elf-stt-common", required_argument, 0, OPTION_ELF_STT_COMMON},
  {"enable-deterministic-archives", no_argument, 0, 'D'},
  {"extract-dwo", no_argument, 0, OPTION_EXTRACT_DWO},
  {"extract-symbol", no_argument, 0, OPTION_EXTRACT_SYMBOL},
  {"file-alignment", required_argument, 0, OPTION_FILE_ALIGNMENT},
  {"format", required_argument, 0, 'F'}, /* Obsolete */
  {"gap-fill", required_argument, 0, OPTION_GAP_FILL},
  {"globalize-symbol", required_argument, 0, OPTION_GLOBALIZE_SYMBOL},
  {"globalize-symbols", required_argument, 0, OPTION_GLOBALIZE_SYMBOLS},
  {"heap", required_argument, 0, OPTION_HEAP},
  {"help", no_argument, 0, 'h'},
  {"image-base", required_argument, 0 , OPTION_IMAGE_BASE},
  {"impure", no_argument, 0, OPTION_IMPURE},
  {"info", no_argument, 0, OPTION_FORMATS_INFO},
  {"input-format", required_argument, 0, 'I'}, /* Obsolete */
  {"input-target", required_argument, 0, 'I'},
  {"interleave", optional_argument, 0, 'i'},
  {"interleave-width", required_argument, 0, OPTION_INTERLEAVE_WIDTH},
  {"keep-file-symbols", no_argument, 0, OPTION_KEEP_FILE_SYMBOLS},
  {"keep-global-symbol", required_argument, 0, 'G'},
  {"keep-global-symbols", required_argument, 0, OPTION_KEEPGLOBAL_SYMBOLS},
  {"keep-symbol", required_argument, 0, 'K'},
  {"keep-symbols", required_argument, 0, OPTION_KEEP_SYMBOLS},
  {"localize-hidden", no_argument, 0, OPTION_LOCALIZE_HIDDEN},
  {"localize-symbol", required_argument, 0, 'L'},
  {"localize-symbols", required_argument, 0, OPTION_LOCALIZE_SYMBOLS},
  {"long-section-names", required_argument, 0, OPTION_LONG_SECTION_NAMES},
  {"merge-notes", no_argument, 0, 'M'},
  {"no-merge-notes", no_argument, 0, OPTION_NO_MERGE_NOTES},
  {"no-adjust-warnings", no_argument, 0, OPTION_NO_CHANGE_WARNINGS},
  {"no-change-warnings", no_argument, 0, OPTION_NO_CHANGE_WARNINGS},
  {"only-keep-debug", no_argument, 0, OPTION_ONLY_KEEP_DEBUG},
  {"only-section", required_argument, 0, 'j'},
  {"output-format", required_argument, 0, 'O'},	/* Obsolete */
  {"output-target", required_argument, 0, 'O'},
  {"pad-to", required_argument, 0, OPTION_PAD_TO},
  {"prefix-alloc-sections", required_argument, 0, OPTION_PREFIX_ALLOC_SECTIONS},
  {"prefix-sections", required_argument, 0, OPTION_PREFIX_SECTIONS},
  {"prefix-symbols", required_argument, 0, OPTION_PREFIX_SYMBOLS},
  {"preserve-dates", no_argument, 0, 'p'},
  {"pure", no_argument, 0, OPTION_PURE},
  {"readonly-text", no_argument, 0, OPTION_READONLY_TEXT},
  {"redefine-sym", required_argument, 0, OPTION_REDEFINE_SYM},
  {"redefine-syms", required_argument, 0, OPTION_REDEFINE_SYMS},
  {"remove-leading-char", no_argument, 0, OPTION_REMOVE_LEADING_CHAR},
  {"remove-section", required_argument, 0, 'R'},
  {"remove-relocations", required_argument, 0, OPTION_REMOVE_RELOCS},
  {"rename-section", required_argument, 0, OPTION_RENAME_SECTION},
  {"reverse-bytes", required_argument, 0, OPTION_REVERSE_BYTES},
  {"section-alignment", required_argument, 0, OPTION_SECTION_ALIGNMENT},
  {"set-section-flags", required_argument, 0, OPTION_SET_SECTION_FLAGS},
  {"set-start", required_argument, 0, OPTION_SET_START},
  {"srec-forceS3", no_argument, 0, OPTION_SREC_FORCES3},
  {"srec-len", required_argument, 0, OPTION_SREC_LEN},
  {"stack", required_argument, 0, OPTION_STACK},
  {"strip-all", no_argument, 0, 'S'},
  {"strip-debug", no_argument, 0, 'g'},
  {"strip-dwo", no_argument, 0, OPTION_STRIP_DWO},
  {"strip-symbol", required_argument, 0, 'N'},
  {"strip-symbols", required_argument, 0, OPTION_STRIP_SYMBOLS},
  {"strip-unneeded", no_argument, 0, OPTION_STRIP_UNNEEDED},
  {"strip-unneeded-symbol", required_argument, 0, OPTION_STRIP_UNNEEDED_SYMBOL},
  {"strip-unneeded-symbols", required_argument, 0, OPTION_STRIP_UNNEEDED_SYMBOLS},
  {"subsystem", required_argument, 0, OPTION_SUBSYSTEM},
  {"target", required_argument, 0, 'F'},
  {"update-section", required_argument, 0, OPTION_UPDATE_SECTION},
  {"verbose", no_argument, 0, 'v'},
  {"version", no_argument, 0, 'V'},
  {"weaken", no_argument, 0, OPTION_WEAKEN},
  {"weaken-symbol", required_argument, 0, 'W'},
  {"weaken-symbols", required_argument, 0, OPTION_WEAKEN_SYMBOLS},
  {"wildcard", no_argument, 0, 'w'},
  {"writable-text", no_argument, 0, OPTION_WRITABLE_TEXT},
  {0, no_argument, 0, 0}
};

/* IMPORTS */
extern char *program_name;

/* This flag distinguishes between strip and objcopy:
   1 means this is 'strip'; 0 means this is 'objcopy'.
   -1 means if we should use argv[0] to decide.  */
extern int is_strip;

/* The maximum length of an S record.  This variable is defined in srec.c
   and can be modified by the --srec-len parameter.  */
extern unsigned int _bfd_srec_len;

/* Restrict the generation of Srecords to type S3 only.
   This variable is defined in bfd/srec.c and can be toggled
   on by the --srec-forceS3 command line switch.  */
extern bfd_boolean _bfd_srec_forceS3;

/* Forward declarations.  */
static void setup_section (bfd *, asection *, void *);
static void setup_bfd_headers (bfd *, bfd *);
static void copy_relocations_in_section (bfd *, asection *, void *);
static void copy_section (bfd *, asection *, void *);
static void get_sections (bfd *, asection *, void *);
static int compare_section_lma (const void *, const void *);
static void mark_symbols_used_in_relocations (bfd *, asection *, void *);
static bfd_boolean write_debugging_info (bfd *, void *, long *, asymbol ***);
static const char *lookup_sym_redefinition (const char *);
static const char *find_section_rename (const char *, flagword *);

ATTRIBUTE_NORETURN static void
copy_usage (FILE *stream, int exit_status)
{
  fprintf (stream, _("Usage: %s [option(s)] in-file [out-file]\n"), program_name);
  fprintf (stream, _(" Copies a binary file, possibly transforming it in the process\n"));
  fprintf (stream, _(" The options are:\n"));
  fprintf (stream, _("\
  -I --input-target <bfdname>      Assume input file is in format <bfdname>\n\
  -O --output-target <bfdname>     Create an output file in format <bfdname>\n\
  -B --binary-architecture <arch>  Set output arch, when input is arch-less\n\
  -F --target <bfdname>            Set both input and output format to <bfdname>\n\
     --debugging                   Convert debugging information, if possible\n\
  -p --preserve-dates              Copy modified/access timestamps to the output\n"));
  if (DEFAULT_AR_DETERMINISTIC)
    fprintf (stream, _("\
  -D --enable-deterministic-archives\n\
                                   Produce deterministic output when stripping archives (default)\n\
  -U --disable-deterministic-archives\n\
                                   Disable -D behavior\n"));
  else
    fprintf (stream, _("\
  -D --enable-deterministic-archives\n\
                                   Produce deterministic output when stripping archives\n\
  -U --disable-deterministic-archives\n\
                                   Disable -D behavior (default)\n"));
  fprintf (stream, _("\
  -j --only-section <name>         Only copy section <name> into the output\n\
     --add-gnu-debuglink=<file>    Add section .gnu_debuglink linking to <file>\n\
  -R --remove-section <name>       Remove section <name> from the output\n\
     --remove-relocations <name>   Remove relocations from section <name>\n\
  -S --strip-all                   Remove all symbol and relocation information\n\
  -g --strip-debug                 Remove all debugging symbols & sections\n\
     --strip-dwo                   Remove all DWO sections\n\
     --strip-unneeded              Remove all symbols not needed by relocations\n\
  -N --strip-symbol <name>         Do not copy symbol <name>\n\
     --strip-unneeded-symbol <name>\n\
                                   Do not copy symbol <name> unless needed by\n\
                                     relocations\n\
     --only-keep-debug             Strip everything but the debug information\n\
     --extract-dwo                 Copy only DWO sections\n\
     --extract-symbol              Remove section contents but keep symbols\n\
  -K --keep-symbol <name>          Do not strip symbol <name>\n\
     --keep-file-symbols           Do not strip file symbol(s)\n\
     --localize-hidden             Turn all ELF hidden symbols into locals\n\
  -L --localize-symbol <name>      Force symbol <name> to be marked as a local\n\
     --globalize-symbol <name>     Force symbol <name> to be marked as a global\n\
  -G --keep-global-symbol <name>   Localize all symbols except <name>\n\
  -W --weaken-symbol <name>        Force symbol <name> to be marked as a weak\n\
     --weaken                      Force all global symbols to be marked as weak\n\
  -w --wildcard                    Permit wildcard in symbol comparison\n\
  -x --discard-all                 Remove all non-global symbols\n\
  -X --discard-locals              Remove any compiler-generated symbols\n\
  -i --interleave[=<number>]       Only copy N out of every <number> bytes\n\
     --interleave-width <number>   Set N for --interleave\n\
  -b --byte <num>                  Select byte <num> in every interleaved block\n\
     --gap-fill <val>              Fill gaps between sections with <val>\n\
     --pad-to <addr>               Pad the last section up to address <addr>\n\
     --set-start <addr>            Set the start address to <addr>\n\
    {--change-start|--adjust-start} <incr>\n\
                                   Add <incr> to the start address\n\
    {--change-addresses|--adjust-vma} <incr>\n\
                                   Add <incr> to LMA, VMA and start addresses\n\
    {--change-section-address|--adjust-section-vma} <name>{=|+|-}<val>\n\
                                   Change LMA and VMA of section <name> by <val>\n\
     --change-section-lma <name>{=|+|-}<val>\n\
                                   Change the LMA of section <name> by <val>\n\
     --change-section-vma <name>{=|+|-}<val>\n\
                                   Change the VMA of section <name> by <val>\n\
    {--[no-]change-warnings|--[no-]adjust-warnings}\n\
                                   Warn if a named section does not exist\n\
     --set-section-flags <name>=<flags>\n\
                                   Set section <name>'s properties to <flags>\n\
     --add-section <name>=<file>   Add section <name> found in <file> to output\n\
     --update-section <name>=<file>\n\
                                   Update contents of section <name> with\n\
                                   contents found in <file>\n\
     --dump-section <name>=<file>  Dump the contents of section <name> into <file>\n\
     --rename-section <old>=<new>[,<flags>] Rename section <old> to <new>\n\
     --long-section-names {enable|disable|keep}\n\
                                   Handle long section names in Coff objects.\n\
     --change-leading-char         Force output format's leading character style\n\
     --remove-leading-char         Remove leading character from global symbols\n\
     --reverse-bytes=<num>         Reverse <num> bytes at a time, in output sections with content\n\
     --redefine-sym <old>=<new>    Redefine symbol name <old> to <new>\n\
     --redefine-syms <file>        --redefine-sym for all symbol pairs \n\
                                     listed in <file>\n\
     --srec-len <number>           Restrict the length of generated Srecords\n\
     --srec-forceS3                Restrict the type of generated Srecords to S3\n\
     --strip-symbols <file>        -N for all symbols listed in <file>\n\
     --strip-unneeded-symbols <file>\n\
                                   --strip-unneeded-symbol for all symbols listed\n\
                                     in <file>\n\
     --keep-symbols <file>         -K for all symbols listed in <file>\n\
     --localize-symbols <file>     -L for all symbols listed in <file>\n\
     --globalize-symbols <file>    --globalize-symbol for all in <file>\n\
     --keep-global-symbols <file>  -G for all symbols listed in <file>\n\
     --weaken-symbols <file>       -W for all symbols listed in <file>\n\
     --add-symbol <name>=[<section>:]<value>[,<flags>]  Add a symbol\n\
     --alt-machine-code <index>    Use the target's <index>'th alternative machine\n\
     --writable-text               Mark the output text as writable\n\
     --readonly-text               Make the output text write protected\n\
     --pure                        Mark the output file as demand paged\n\
     --impure                      Mark the output file as impure\n\
     --prefix-symbols <prefix>     Add <prefix> to start of every symbol name\n\
     --prefix-sections <prefix>    Add <prefix> to start of every section name\n\
     --prefix-alloc-sections <prefix>\n\
                                   Add <prefix> to start of every allocatable\n\
                                     section name\n\
     --file-alignment <num>        Set PE file alignment to <num>\n\
     --heap <reserve>[,<commit>]   Set PE reserve/commit heap to <reserve>/\n\
                                   <commit>\n\
     --image-base <address>        Set PE image base to <address>\n\
     --section-alignment <num>     Set PE section alignment to <num>\n\
     --stack <reserve>[,<commit>]  Set PE reserve/commit stack to <reserve>/\n\
                                   <commit>\n\
     --subsystem <name>[:<version>]\n\
                                   Set PE subsystem to <name> [& <version>]\n\
     --compress-debug-sections[={none|zlib|zlib-gnu|zlib-gabi}]\n\
                                   Compress DWARF debug sections using zlib\n\
     --decompress-debug-sections   Decompress DWARF debug sections using zlib\n\
     --elf-stt-common=[yes|no]     Generate ELF common symbols with STT_COMMON\n\
                                     type\n\
  -M  --merge-notes                Remove redundant entries in note sections\n\
      --no-merge-notes             Do not attempt to remove redundant notes (default)\n\
  -v --verbose                     List all object files modified\n\
  @<file>                          Read options from <file>\n\
  -V --version                     Display this program's version number\n\
  -h --help                        Display this output\n\
     --info                        List object formats & architectures supported\n\
"));
  list_supported_targets (program_name, stream);
  if (REPORT_BUGS_TO[0] && exit_status == 0)
    fprintf (stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
  exit (exit_status);
}

ATTRIBUTE_NORETURN static void
strip_usage (FILE *stream, int exit_status)
{
  fprintf (stream, _("Usage: %s <option(s)> in-file(s)\n"), program_name);
  fprintf (stream, _(" Removes symbols and sections from files\n"));
  fprintf (stream, _(" The options are:\n"));
  fprintf (stream, _("\
  -I --input-target=<bfdname>      Assume input file is in format <bfdname>\n\
  -O --output-target=<bfdname>     Create an output file in format <bfdname>\n\
  -F --target=<bfdname>            Set both input and output format to <bfdname>\n\
  -p --preserve-dates              Copy modified/access timestamps to the output\n\
"));
  if (DEFAULT_AR_DETERMINISTIC)
    fprintf (stream, _("\
  -D --enable-deterministic-archives\n\
                                   Produce deterministic output when stripping archives (default)\n\
  -U --disable-deterministic-archives\n\
                                   Disable -D behavior\n"));
  else
    fprintf (stream, _("\
  -D --enable-deterministic-archives\n\
                                   Produce deterministic output when stripping archives\n\
  -U --disable-deterministic-archives\n\
                                   Disable -D behavior (default)\n"));
  fprintf (stream, _("\
  -R --remove-section=<name>       Also remove section <name> from the output\n\
     --remove-relocations <name>   Remove relocations from section <name>\n\
  -s --strip-all                   Remove all symbol and relocation information\n\
  -g -S -d --strip-debug           Remove all debugging symbols & sections\n\
     --strip-dwo                   Remove all DWO sections\n\
     --strip-unneeded              Remove all symbols not needed by relocations\n\
     --only-keep-debug             Strip everything but the debug information\n\
  -M  --merge-notes                Remove redundant entries in note sections (default)\n\
      --no-merge-notes             Do not attempt to remove redundant notes\n\
  -N --strip-symbol=<name>         Do not copy symbol <name>\n\
  -K --keep-symbol=<name>          Do not strip symbol <name>\n\
     --keep-file-symbols           Do not strip file symbol(s)\n\
  -w --wildcard                    Permit wildcard in symbol comparison\n\
  -x --discard-all                 Remove all non-global symbols\n\
  -X --discard-locals              Remove any compiler-generated symbols\n\
  -v --verbose                     List all object files modified\n\
  -V --version                     Display this program's version number\n\
  -h --help                        Display this output\n\
     --info                        List object formats & architectures supported\n\
  -o <file>                        Place stripped output into <file>\n\
"));

  list_supported_targets (program_name, stream);
  if (REPORT_BUGS_TO[0] && exit_status == 0)
    fprintf (stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
  exit (exit_status);
}

/* Parse section flags into a flagword, with a fatal error if the
   string can't be parsed.  */

static flagword
parse_flags (const char *s)
{
  flagword ret;
  const char *snext;
  int len;

  ret = SEC_NO_FLAGS;

  do
    {
      snext = strchr (s, ',');
      if (snext == NULL)
	len = strlen (s);
      else
	{
	  len = snext - s;
	  ++snext;
	}

      if (0) ;
#define PARSE_FLAG(fname,fval)					\
      else if (strncasecmp (fname, s, len) == 0) ret |= fval
      PARSE_FLAG ("alloc", SEC_ALLOC);
      PARSE_FLAG ("load", SEC_LOAD);
      PARSE_FLAG ("noload", SEC_NEVER_LOAD);
      PARSE_FLAG ("readonly", SEC_READONLY);
      PARSE_FLAG ("debug", SEC_DEBUGGING);
      PARSE_FLAG ("code", SEC_CODE);
      PARSE_FLAG ("data", SEC_DATA);
      PARSE_FLAG ("rom", SEC_ROM);
      PARSE_FLAG ("share", SEC_COFF_SHARED);
      PARSE_FLAG ("contents", SEC_HAS_CONTENTS);
      PARSE_FLAG ("merge", SEC_MERGE);
      PARSE_FLAG ("strings", SEC_STRINGS);
#undef PARSE_FLAG
      else
	{
	  char *copy;

	  copy = (char *) xmalloc (len + 1);
	  strncpy (copy, s, len);
	  copy[len] = '\0';
	  non_fatal (_("unrecognized section flag `%s'"), copy);
	  fatal (_("supported flags: %s"),
		 "alloc, load, noload, readonly, debug, code, data, rom, share, contents, merge, strings");
	}

      s = snext;
    }
  while (s != NULL);

  return ret;
}

/* Parse symbol flags into a flagword, with a fatal error if the
   string can't be parsed.  */

static flagword
parse_symflags (const char *s, char **other)
{
  flagword ret;
  const char *snext;
  size_t len;

  ret = BSF_NO_FLAGS;

  do
    {
      snext = strchr (s, ',');
      if (snext == NULL)
	len = strlen (s);
      else
	{
	  len = snext - s;
	  ++snext;
	}

#define PARSE_FLAG(fname, fval)						\
      else if (len == sizeof fname - 1					\
	       && strncasecmp (fname, s, len) == 0)			\
	ret |= fval

#define PARSE_OTHER(fname, fval)					\
      else if (len >= sizeof fname					\
	       && strncasecmp (fname, s, sizeof fname - 1) == 0)	\
	fval = xstrndup (s + sizeof fname - 1, len - sizeof fname + 1)

      if (0) ;
      PARSE_FLAG ("local", BSF_LOCAL);
      PARSE_FLAG ("global", BSF_GLOBAL);
      PARSE_FLAG ("export", BSF_EXPORT);
      PARSE_FLAG ("debug", BSF_DEBUGGING);
      PARSE_FLAG ("function", BSF_FUNCTION);
      PARSE_FLAG ("weak", BSF_WEAK);
      PARSE_FLAG ("section", BSF_SECTION_SYM);
      PARSE_FLAG ("constructor", BSF_CONSTRUCTOR);
      PARSE_FLAG ("warning", BSF_WARNING);
      PARSE_FLAG ("indirect", BSF_INDIRECT);
      PARSE_FLAG ("file", BSF_FILE);
      PARSE_FLAG ("object", BSF_OBJECT);
      PARSE_FLAG ("synthetic", BSF_SYNTHETIC);
      PARSE_FLAG ("indirect-function", BSF_GNU_INDIRECT_FUNCTION | BSF_FUNCTION);
      PARSE_FLAG ("unique-object", BSF_GNU_UNIQUE | BSF_OBJECT);
      PARSE_OTHER ("before=", *other);

#undef PARSE_FLAG
#undef PARSE_OTHER
      else
	{
	  char *copy;

	  copy = (char *) xmalloc (len + 1);
	  strncpy (copy, s, len);
	  copy[len] = '\0';
	  non_fatal (_("unrecognized symbol flag `%s'"), copy);
	  fatal (_("supported flags: %s"),
		 "local, global, export, debug, function, weak, section, "
		 "constructor, warning, indirect, file, object, synthetic, "
		 "indirect-function, unique-object, before=<othersym>");
	}

      s = snext;
    }
  while (s != NULL);

  return ret;
}

/* Find and optionally add an entry in the change_sections list.

   We need to be careful in how we match section names because of the support
   for wildcard characters.  For example suppose that the user has invoked
   objcopy like this:

       --set-section-flags .debug_*=debug
       --set-section-flags .debug_str=readonly,debug
       --change-section-address .debug_*ranges=0x1000

   With the idea that all debug sections will receive the DEBUG flag, the
   .debug_str section will also receive the READONLY flag and the
   .debug_ranges and .debug_aranges sections will have their address set to
   0x1000.  (This may not make much sense, but it is just an example).

   When adding the section name patterns to the section list we need to make
   sure that previous entries do not match with the new entry, unless the
   match is exact.  (In which case we assume that the user is overriding
   the previous entry with the new context).

   When matching real section names to the section list we make use of the
   wildcard characters, but we must do so in context.  Eg if we are setting
   section addresses then we match for .debug_ranges but not for .debug_info.

   Finally, if ADD is false and we do find a match, we mark the section list
   entry as used.  */

static struct section_list *
find_section_list (const char *name, bfd_boolean add, unsigned int context)
{
  struct section_list *p, *match = NULL;

  /* assert ((context & ((1 << 7) - 1)) != 0); */

  for (p = change_sections; p != NULL; p = p->next)
    {
      if (add)
	{
	  if (strcmp (p->pattern, name) == 0)
	    {
	      /* Check for context conflicts.  */
	      if (((p->context & SECTION_CONTEXT_REMOVE)
		   && (context & SECTION_CONTEXT_COPY))
		  || ((context & SECTION_CONTEXT_REMOVE)
		      && (p->context & SECTION_CONTEXT_COPY)))
		fatal (_("error: %s both copied and removed"), name);

	      if (((p->context & SECTION_CONTEXT_SET_VMA)
		  && (context & SECTION_CONTEXT_ALTER_VMA))
		  || ((context & SECTION_CONTEXT_SET_VMA)
		      && (context & SECTION_CONTEXT_ALTER_VMA)))
		fatal (_("error: %s both sets and alters VMA"), name);

	      if (((p->context & SECTION_CONTEXT_SET_LMA)
		  && (context & SECTION_CONTEXT_ALTER_LMA))
		  || ((context & SECTION_CONTEXT_SET_LMA)
		      && (context & SECTION_CONTEXT_ALTER_LMA)))
		fatal (_("error: %s both sets and alters LMA"), name);

	      /* Extend the context.  */
	      p->context |= context;
	      return p;
	    }
	}
      /* If we are not adding a new name/pattern then
	 only check for a match if the context applies.  */
      else if (p->context & context)
        {
          /* We could check for the presence of wildchar characters
             first and choose between calling strcmp and fnmatch,
             but is that really worth it ?  */
          if (p->pattern [0] == '!')
            {
              if (fnmatch (p->pattern + 1, name, 0) == 0)
                {
                  p->used = TRUE;
                  return NULL;
                }
            }
          else
            {
              if (fnmatch (p->pattern, name, 0) == 0)
                {
                  if (match == NULL)
                    match = p;
                }
            }
        }
    }

  if (! add)
    {
      if (match != NULL)
        match->used = TRUE;
      return match;
    }

  p = (struct section_list *) xmalloc (sizeof (struct section_list));
  p->pattern = name;
  p->used = FALSE;
  p->context = context;
  p->vma_val = 0;
  p->lma_val = 0;
  p->flags = 0;
  p->next = change_sections;
  change_sections = p;

  return p;
}

/* S1 is the entry node already in the table, S2 is the key node.  */

static int
eq_string_redefnode (const void *s1, const void *s2)
{
  struct redefine_node *node1 = (struct redefine_node *) s1;
  struct redefine_node *node2 = (struct redefine_node *) s2;
  return !strcmp ((const char *) node1->source, (const char *) node2->source);
}

/* P is redefine node.  Hash value is generated from its "source" filed.  */

static hashval_t
htab_hash_redefnode (const void *p)
{
  struct redefine_node *redefnode = (struct redefine_node *) p;
  return htab_hash_string (redefnode->source);
}

/* Create hashtab used for redefine node.  */

static htab_t
create_symbol2redef_htab (void)
{
  return htab_create_alloc (16, htab_hash_redefnode, eq_string_redefnode, NULL,
			    xcalloc, free);
}

/* There is htab_hash_string but no htab_eq_string. Makes sense.  */

static int
eq_string (const void *s1, const void *s2)
{
  return strcmp ((const char *) s1, (const char *) s2) == 0;
}

static htab_t
create_symbol_htab (void)
{
  return htab_create_alloc (16, htab_hash_string, eq_string, NULL, xcalloc, free);
}

static void
create_symbol_htabs (void)
{
  strip_specific_htab = create_symbol_htab ();
  strip_unneeded_htab = create_symbol_htab ();
  keep_specific_htab = create_symbol_htab ();
  localize_specific_htab = create_symbol_htab ();
  globalize_specific_htab = create_symbol_htab ();
  keepglobal_specific_htab = create_symbol_htab ();
  weaken_specific_htab = create_symbol_htab ();
  redefine_specific_htab = create_symbol2redef_htab ();
  /* As there is no bidirectional hash table in libiberty, need a reverse table
     to check duplicated target string.  */
  redefine_specific_reverse_htab = create_symbol_htab ();
}

/* Add a symbol to strip_specific_list.  */

static void
add_specific_symbol (const char *name, htab_t htab)
{
  *htab_find_slot (htab, name, INSERT) = (char *) name;
}

/* Like add_specific_symbol, but the element type is void *.  */

static void
add_specific_symbol_node (const void *node, htab_t htab)
{
  *htab_find_slot (htab, node, INSERT) = (void *) node;
}

/* Add symbols listed in `filename' to strip_specific_list.  */

#define IS_WHITESPACE(c)      ((c) == ' ' || (c) == '\t')
#define IS_LINE_TERMINATOR(c) ((c) == '\n' || (c) == '\r' || (c) == '\0')

static void
add_specific_symbols (const char *filename, htab_t htab)
{
  off_t  size;
  FILE * f;
  char * line;
  char * buffer;
  unsigned int line_count;

  size = get_file_size (filename);
  if (size == 0)
    {
      status = 1;
      return;
    }

  buffer = (char *) xmalloc (size + 2);
  f = fopen (filename, FOPEN_RT);
  if (f == NULL)
    fatal (_("cannot open '%s': %s"), filename, strerror (errno));

  if (fread (buffer, 1, size, f) == 0 || ferror (f))
    fatal (_("%s: fread failed"), filename);

  fclose (f);
  buffer [size] = '\n';
  buffer [size + 1] = '\0';

  line_count = 1;

  for (line = buffer; * line != '\0'; line ++)
    {
      char * eol;
      char * name;
      char * name_end;
      int finished = FALSE;

      for (eol = line;; eol ++)
	{
	  switch (* eol)
	    {
	    case '\n':
	      * eol = '\0';
	      /* Cope with \n\r.  */
	      if (eol[1] == '\r')
		++ eol;
	      finished = TRUE;
	      break;

	    case '\r':
	      * eol = '\0';
	      /* Cope with \r\n.  */
	      if (eol[1] == '\n')
		++ eol;
	      finished = TRUE;
	      break;

	    case 0:
	      finished = TRUE;
	      break;

	    case '#':
	      /* Line comment, Terminate the line here, in case a
		 name is present and then allow the rest of the
		 loop to find the real end of the line.  */
	      * eol = '\0';
	      break;

	    default:
	      break;
	    }

	  if (finished)
	    break;
	}

      /* A name may now exist somewhere between 'line' and 'eol'.
	 Strip off leading whitespace and trailing whitespace,
	 then add it to the list.  */
      for (name = line; IS_WHITESPACE (* name); name ++)
	;
      for (name_end = name;
	   (! IS_WHITESPACE (* name_end))
	   && (! IS_LINE_TERMINATOR (* name_end));
	   name_end ++)
	;

      if (! IS_LINE_TERMINATOR (* name_end))
	{
	  char * extra;

	  for (extra = name_end + 1; IS_WHITESPACE (* extra); extra ++)
	    ;

	  if (! IS_LINE_TERMINATOR (* extra))
	    non_fatal (_("%s:%d: Ignoring rubbish found on this line"),
		       filename, line_count);
	}

      * name_end = '\0';

      if (name_end > name)
	add_specific_symbol (name, htab);

      /* Advance line pointer to end of line.  The 'eol ++' in the for
	 loop above will then advance us to the start of the next line.  */
      line = eol;
      line_count ++;
    }
}

/* See whether a symbol should be stripped or kept
   based on strip_specific_list and keep_symbols.  */

static int
is_specified_symbol_predicate (void **slot, void *data)
{
  struct is_specified_symbol_predicate_data *d =
      (struct is_specified_symbol_predicate_data *) data;
  const char *slot_name = (char *) *slot;

  if (*slot_name != '!')
    {
      if (! fnmatch (slot_name, d->name, 0))
	{
	  d->found = TRUE;
	  /* Continue traversal, there might be a non-match rule.  */
	  return 1;
	}
    }
  else
    {
      if (! fnmatch (slot_name + 1, d->name, 0))
	{
	  d->found = FALSE;
	  /* Stop traversal.  */
	  return 0;
	}
    }

  /* Continue traversal.  */
  return 1;
}

static bfd_boolean
is_specified_symbol (const char *name, htab_t htab)
{
  if (wildcard)
    {
      struct is_specified_symbol_predicate_data data;

      data.name = name;
      data.found = FALSE;

      htab_traverse (htab, is_specified_symbol_predicate, &data);

      return data.found;
    }

  return htab_find (htab, name) != NULL;
}

/* Return a pointer to the symbol used as a signature for GROUP.  */

static asymbol *
group_signature (asection *group)
{
  bfd *abfd = group->owner;
  Elf_Internal_Shdr *ghdr;

  /* PR 20089: An earlier error may have prevented us from loading the symbol table.  */
  if (isympp == NULL)
    return NULL;

  if (bfd_get_flavour (abfd) != bfd_target_elf_flavour)
    return NULL;

  ghdr = &elf_section_data (group)->this_hdr;
  if (ghdr->sh_link < elf_numsections (abfd))
    {
      const struct elf_backend_data *bed = get_elf_backend_data (abfd);
      Elf_Internal_Shdr *symhdr = elf_elfsections (abfd) [ghdr->sh_link];

      if (symhdr->sh_type == SHT_SYMTAB
	  && ghdr->sh_info > 0
	  && ghdr->sh_info < (symhdr->sh_size / bed->s->sizeof_sym))
	return isympp[ghdr->sh_info - 1];
    }
  return NULL;
}

/* Return TRUE if the section is a DWO section.  */

static bfd_boolean
is_dwo_section (bfd *abfd ATTRIBUTE_UNUSED, asection *sec)
{
  const char *name = bfd_get_section_name (abfd, sec);
  int len = strlen (name);

  return strncmp (name + len - 4, ".dwo", 4) == 0;
}

/* Return TRUE if section SEC is in the update list.  */

static bfd_boolean
is_update_section (bfd *abfd ATTRIBUTE_UNUSED, asection *sec)
{
  if (update_sections != NULL)
    {
      struct section_add *pupdate;

      for (pupdate = update_sections;
	   pupdate != NULL;
	   pupdate = pupdate->next)
	{
	  if (strcmp (sec->name, pupdate->name) == 0)
	    return TRUE;
	}
    }

  return FALSE;
}

static bfd_boolean
is_merged_note_section (bfd * abfd, asection * sec)
{
  if (merge_notes
      && bfd_get_flavour (abfd) == bfd_target_elf_flavour
      && elf_section_data (sec)->this_hdr.sh_type == SHT_NOTE
      /* FIXME: We currently only support merging GNU_BUILD_NOTEs.
	 We should add support for more note types.  */
      && ((elf_section_data (sec)->this_hdr.sh_flags & SHF_GNU_BUILD_NOTE) != 0
	  /* Old versions of GAS (prior to 2.27) could not set the section
	     flags to OS-specific values, so we also accept sections with the
	     expected name.  */
	  || (strcmp (sec->name, GNU_BUILD_ATTRS_SECTION_NAME) == 0)))
    return TRUE;

  return FALSE;
}

/* See if a non-group section is being removed.  */

static bfd_boolean
is_strip_section_1 (bfd *abfd ATTRIBUTE_UNUSED, asection *sec)
{
  if (sections_removed || sections_copied)
    {
      struct section_list *p;
      struct section_list *q;

      p = find_section_list (bfd_get_section_name (abfd, sec), FALSE,
			     SECTION_CONTEXT_REMOVE);
      q = find_section_list (bfd_get_section_name (abfd, sec), FALSE,
			     SECTION_CONTEXT_COPY);

      if (p && q)
	fatal (_("error: section %s matches both remove and copy options"),
	       bfd_get_section_name (abfd, sec));
      if (p && is_update_section (abfd, sec))
	fatal (_("error: section %s matches both update and remove options"),
	       bfd_get_section_name (abfd, sec));

      if (p != NULL)
	return TRUE;
      if (sections_copied && q == NULL)
	return TRUE;
    }

  if ((bfd_get_section_flags (abfd, sec) & SEC_DEBUGGING) != 0)
    {
      if (strip_symbols == STRIP_DEBUG
	  || strip_symbols == STRIP_UNNEEDED
	  || strip_symbols == STRIP_ALL
	  || discard_locals == LOCALS_ALL
	  || convert_debugging)
	{
	  /* By default we don't want to strip .reloc section.
	     This section has for pe-coff special meaning.   See
	     pe-dll.c file in ld, and peXXigen.c in bfd for details.  */
	  if (strcmp (bfd_get_section_name (abfd, sec), ".reloc") != 0)
	    return TRUE;
	}

      if (strip_symbols == STRIP_DWO)
	return is_dwo_section (abfd, sec);

      if (strip_symbols == STRIP_NONDEBUG)
	return FALSE;
    }

  if (strip_symbols == STRIP_NONDWO)
    return !is_dwo_section (abfd, sec);

  return FALSE;
}

/* See if a section is being removed.  */

static bfd_boolean
is_strip_section (bfd *abfd ATTRIBUTE_UNUSED, asection *sec)
{
  if (is_strip_section_1 (abfd, sec))
    return TRUE;

  if ((bfd_get_section_flags (abfd, sec) & SEC_GROUP) != 0)
    {
      asymbol *gsym;
      const char *gname;
      asection *elt, *first;

      /* PR binutils/3181
	 If we are going to strip the group signature symbol, then
	 strip the group section too.  */
      gsym = group_signature (sec);
      if (gsym != NULL)
	gname = gsym->name;
      else
	gname = sec->name;
      if ((strip_symbols == STRIP_ALL
	   && !is_specified_symbol (gname, keep_specific_htab))
	  || is_specified_symbol (gname, strip_specific_htab))
	return TRUE;

      /* Remove the group section if all members are removed.  */
      first = elt = elf_next_in_group (sec);
      while (elt != NULL)
	{
	  if (!is_strip_section_1 (abfd, elt))
	    return FALSE;
	  elt = elf_next_in_group (elt);
	  if (elt == first)
	    break;
	}

      return TRUE;
    }

  return FALSE;
}

static bfd_boolean
is_nondebug_keep_contents_section (bfd *ibfd, asection *isection)
{
  /* Always keep ELF note sections.  */
  if (ibfd->xvec->flavour == bfd_target_elf_flavour)
    return (elf_section_type (isection) == SHT_NOTE);

  /* Always keep the .buildid section for PE/COFF.

     Strictly, this should be written "always keep the section storing the debug
     directory", but that may be the .text section for objects produced by some
     tools, which it is not sensible to keep.  */
  if (ibfd->xvec->flavour == bfd_target_coff_flavour)
    return (strcmp (bfd_get_section_name (ibfd, isection), ".buildid") == 0);

  return FALSE;
}

/* Return true if SYM is a hidden symbol.  */

static bfd_boolean
is_hidden_symbol (asymbol *sym)
{
  elf_symbol_type *elf_sym;

  elf_sym = elf_symbol_from (sym->the_bfd, sym);
  if (elf_sym != NULL)
    switch (ELF_ST_VISIBILITY (elf_sym->internal_elf_sym.st_other))
      {
      case STV_HIDDEN:
      case STV_INTERNAL:
	return TRUE;
      }
  return FALSE;
}

static bfd_boolean
need_sym_before (struct addsym_node **node, const char *sym)
{
  int count;
  struct addsym_node *ptr = add_sym_list;

  /* 'othersym' symbols are at the front of the list.  */
  for (count = 0; count < add_symbols; count++)
    {
      if (!ptr->othersym)
	break;
      else if (strcmp (ptr->othersym, sym) == 0)
	{
	  free (ptr->othersym);
	  ptr->othersym = ""; /* Empty name is hopefully never a valid symbol name.  */
	  *node = ptr;
	  return TRUE;
	}
      ptr = ptr->next;
    }
  return FALSE;
}

static asymbol *
create_new_symbol (struct addsym_node *ptr, bfd *obfd)
{
  asymbol *sym = bfd_make_empty_symbol (obfd);

  bfd_asymbol_name (sym) = ptr->symdef;
  sym->value = ptr->symval;
  sym->flags = ptr->flags;
  if (ptr->section)
    {
      asection *sec = bfd_get_section_by_name (obfd, ptr->section);
      if (!sec)
	fatal (_("Section %s not found"), ptr->section);
      sym->section = sec;
    }
  else
    sym->section = bfd_abs_section_ptr;
  return sym;
}

/* Choose which symbol entries to copy; put the result in OSYMS.
   We don't copy in place, because that confuses the relocs.
   Return the number of symbols to print.  */

static unsigned int
filter_symbols (bfd *abfd, bfd *obfd, asymbol **osyms,
		asymbol **isyms, long symcount)
{
  asymbol **from = isyms, **to = osyms;
  long src_count = 0, dst_count = 0;
  int relocatable = (abfd->flags & (EXEC_P | DYNAMIC)) == 0;

  for (; src_count < symcount; src_count++)
    {
      asymbol *sym = from[src_count];
      flagword flags = sym->flags;
      char *name = (char *) bfd_asymbol_name (sym);
      bfd_boolean keep;
      bfd_boolean used_in_reloc = FALSE;
      bfd_boolean undefined;
      bfd_boolean rem_leading_char;
      bfd_boolean add_leading_char;

      undefined = bfd_is_und_section (bfd_get_section (sym));

      if (add_sym_list)
	{
	  struct addsym_node *ptr;

	  if (need_sym_before (&ptr, name))
	    to[dst_count++] = create_new_symbol (ptr, obfd);
	}

      if (htab_elements (redefine_specific_htab) || section_rename_list)
	{
	  char *new_name;

	  new_name = (char *) lookup_sym_redefinition (name);
	  if (new_name == name
	      && (flags & BSF_SECTION_SYM) != 0)
	    new_name = (char *) find_section_rename (name, NULL);
	  bfd_asymbol_name (sym) = new_name;
	  name = new_name;
	}

      /* Check if we will remove the current leading character.  */
      rem_leading_char =
	(name[0] == bfd_get_symbol_leading_char (abfd))
	&& (change_leading_char
	    || (remove_leading_char
		&& ((flags & (BSF_GLOBAL | BSF_WEAK)) != 0
		    || undefined
		    || bfd_is_com_section (bfd_get_section (sym)))));

      /* Check if we will add a new leading character.  */
      add_leading_char =
	change_leading_char
	&& (bfd_get_symbol_leading_char (obfd) != '\0')
	&& (bfd_get_symbol_leading_char (abfd) == '\0'
	    || (name[0] == bfd_get_symbol_leading_char (abfd)));

      /* Short circuit for change_leading_char if we can do it in-place.  */
      if (rem_leading_char && add_leading_char && !prefix_symbols_string)
	{
	  name[0] = bfd_get_symbol_leading_char (obfd);
	  bfd_asymbol_name (sym) = name;
	  rem_leading_char = FALSE;
	  add_leading_char = FALSE;
	}

      /* Remove leading char.  */
      if (rem_leading_char)
	bfd_asymbol_name (sym) = ++name;

      /* Add new leading char and/or prefix.  */
      if (add_leading_char || prefix_symbols_string)
	{
	  char *n, *ptr;

	  ptr = n = (char *) xmalloc (1 + strlen (prefix_symbols_string)
				      + strlen (name) + 1);
	  if (add_leading_char)
	    *ptr++ = bfd_get_symbol_leading_char (obfd);

	  if (prefix_symbols_string)
	    {
	      strcpy (ptr, prefix_symbols_string);
	      ptr += strlen (prefix_symbols_string);
	    }

	  strcpy (ptr, name);
	  bfd_asymbol_name (sym) = n;
	  name = n;
	}

      if (strip_symbols == STRIP_ALL)
	keep = FALSE;
      else if ((flags & BSF_KEEP) != 0		/* Used in relocation.  */
	       || ((flags & BSF_SECTION_SYM) != 0
		   && ((*bfd_get_section (sym)->symbol_ptr_ptr)->flags
		       & BSF_KEEP) != 0))
	{
	  keep = TRUE;
	  used_in_reloc = TRUE;
	}
      else if (relocatable			/* Relocatable file.  */
	       && ((flags & (BSF_GLOBAL | BSF_WEAK)) != 0
		   || bfd_is_com_section (bfd_get_section (sym))))
	keep = TRUE;
      else if (bfd_decode_symclass (sym) == 'I')
	/* Global symbols in $idata sections need to be retained
	   even if relocatable is FALSE.  External users of the
	   library containing the $idata section may reference these
	   symbols.  */
	keep = TRUE;
      else if ((flags & BSF_GLOBAL) != 0	/* Global symbol.  */
	       || (flags & BSF_WEAK) != 0
	       || undefined
	       || bfd_is_com_section (bfd_get_section (sym)))
	keep = strip_symbols != STRIP_UNNEEDED;
      else if ((flags & BSF_DEBUGGING) != 0)	/* Debugging symbol.  */
	keep = (strip_symbols != STRIP_DEBUG
		&& strip_symbols != STRIP_UNNEEDED
		&& ! convert_debugging);
      else if (bfd_coff_get_comdat_section (abfd, bfd_get_section (sym)))
	/* COMDAT sections store special information in local
	   symbols, so we cannot risk stripping any of them.  */
	keep = TRUE;
      else			/* Local symbol.  */
	keep = (strip_symbols != STRIP_UNNEEDED
		&& (discard_locals != LOCALS_ALL
		    && (discard_locals != LOCALS_START_L
			|| ! bfd_is_local_label (abfd, sym))));

      if (keep && is_specified_symbol (name, strip_specific_htab))
	{
	  /* There are multiple ways to set 'keep' above, but if it
	     was the relocatable symbol case, then that's an error.  */
	  if (used_in_reloc)
	    {
	      non_fatal (_("not stripping symbol `%s' because it is named in a relocation"), name);
	      status = 1;
	    }
	  else
	    keep = FALSE;
	}

      if (keep
	  && !(flags & BSF_KEEP)
	  && is_specified_symbol (name, strip_unneeded_htab))
	keep = FALSE;

      if (!keep
	  && ((keep_file_symbols && (flags & BSF_FILE))
	      || is_specified_symbol (name, keep_specific_htab)))
	keep = TRUE;

      if (keep && is_strip_section (abfd, bfd_get_section (sym)))
	keep = FALSE;

      if (keep)
	{
	  if ((flags & BSF_GLOBAL) != 0
	      && (weaken || is_specified_symbol (name, weaken_specific_htab)))
	    {
	      sym->flags &= ~ BSF_GLOBAL;
	      sym->flags |= BSF_WEAK;
	    }

	  if (!undefined
	      && (flags & (BSF_GLOBAL | BSF_WEAK))
	      && (is_specified_symbol (name, localize_specific_htab)
		  || (htab_elements (keepglobal_specific_htab) != 0
		      && ! is_specified_symbol (name, keepglobal_specific_htab))
		  || (localize_hidden && is_hidden_symbol (sym))))
	    {
	      sym->flags &= ~ (BSF_GLOBAL | BSF_WEAK);
	      sym->flags |= BSF_LOCAL;
	    }

	  if (!undefined
	      && (flags & BSF_LOCAL)
	      && is_specified_symbol (name, globalize_specific_htab))
	    {
	      sym->flags &= ~ BSF_LOCAL;
	      sym->flags |= BSF_GLOBAL;
	    }

	  to[dst_count++] = sym;
	}
    }
  if (add_sym_list)
    {
      struct addsym_node *ptr = add_sym_list;

      for (src_count = 0; src_count < add_symbols; src_count++)
	{
	  if (ptr->othersym)
	    {
	      if (strcmp (ptr->othersym, ""))
		fatal (_("'before=%s' not found"), ptr->othersym);
	    }
	  else
	    to[dst_count++] = create_new_symbol (ptr, obfd);

	  ptr = ptr->next;
	}
    }

  to[dst_count] = NULL;

  return dst_count;
}

/* Find the redefined name of symbol SOURCE.  */

static const char *
lookup_sym_redefinition (const char *source)
{
  struct redefine_node key_node = {(char *) source, NULL};
  struct redefine_node *redef_node
    = (struct redefine_node *) htab_find (redefine_specific_htab, &key_node);

  return redef_node == NULL ? source : redef_node->target;
}

/* Insert a node into symbol redefine hash tabel.  */

static void
add_redefine_and_check (const char *cause, const char *source,
			const char *target)
{
  struct redefine_node *new_node
    = (struct redefine_node *) xmalloc (sizeof (struct redefine_node));

  new_node->source = strdup (source);
  new_node->target = strdup (target);

  if (htab_find (redefine_specific_htab, new_node) != HTAB_EMPTY_ENTRY)
    fatal (_("%s: Multiple redefinition of symbol \"%s\""),
	   cause, source);

  if (htab_find (redefine_specific_reverse_htab, target) != HTAB_EMPTY_ENTRY)
    fatal (_("%s: Symbol \"%s\" is target of more than one redefinition"),
	   cause, target);

  /* Insert the NEW_NODE into hash table for quick search.  */
  add_specific_symbol_node (new_node, redefine_specific_htab);

  /* Insert the target string into the reverse hash table, this is needed for
     duplicated target string check.  */
  add_specific_symbol (new_node->target, redefine_specific_reverse_htab);

}

/* Handle the --redefine-syms option.  Read lines containing "old new"
   from the file, and add them to the symbol redefine list.  */

static void
add_redefine_syms_file (const char *filename)
{
  FILE *file;
  char *buf;
  size_t bufsize;
  size_t len;
  size_t outsym_off;
  int c, lineno;

  file = fopen (filename, "r");
  if (file == NULL)
    fatal (_("couldn't open symbol redefinition file %s (error: %s)"),
	   filename, strerror (errno));

  bufsize = 100;
  buf = (char *) xmalloc (bufsize + 1 /* For the terminating NUL.  */);

  lineno = 1;
  c = getc (file);
  len = 0;
  outsym_off = 0;
  while (c != EOF)
    {
      /* Collect the input symbol name.  */
      while (! IS_WHITESPACE (c) && ! IS_LINE_TERMINATOR (c) && c != EOF)
	{
	  if (c == '#')
	    goto comment;
	  buf[len++] = c;
	  if (len >= bufsize)
	    {
	      bufsize *= 2;
	      buf = (char *) xrealloc (buf, bufsize + 1);
	    }
	  c = getc (file);
	}
      buf[len++] = '\0';
      if (c == EOF)
	break;

      /* Eat white space between the symbol names.  */
      while (IS_WHITESPACE (c))
	c = getc (file);
      if (c == '#' || IS_LINE_TERMINATOR (c))
	goto comment;
      if (c == EOF)
	break;

      /* Collect the output symbol name.  */
      outsym_off = len;
      while (! IS_WHITESPACE (c) && ! IS_LINE_TERMINATOR (c) && c != EOF)
	{
	  if (c == '#')
	    goto comment;
	  buf[len++] = c;
	  if (len >= bufsize)
	    {
	      bufsize *= 2;
	      buf = (char *) xrealloc (buf, bufsize + 1);
	    }
	  c = getc (file);
	}
      buf[len++] = '\0';
      if (c == EOF)
	break;

      /* Eat white space at end of line.  */
      while (! IS_LINE_TERMINATOR(c) && c != EOF && IS_WHITESPACE (c))
	c = getc (file);
      if (c == '#')
	goto comment;
      /* Handle \r\n.  */
      if ((c == '\r' && (c = getc (file)) == '\n')
	  || c == '\n' || c == EOF)
	{
	end_of_line:
	  /* Append the redefinition to the list.  */
	  if (buf[0] != '\0')
	    add_redefine_and_check (filename, &buf[0], &buf[outsym_off]);

	  lineno++;
	  len = 0;
	  outsym_off = 0;
	  if (c == EOF)
	    break;
	  c = getc (file);
	  continue;
	}
      else
	fatal (_("%s:%d: garbage found at end of line"), filename, lineno);
    comment:
      if (len != 0 && (outsym_off == 0 || outsym_off == len))
	fatal (_("%s:%d: missing new symbol name"), filename, lineno);
      buf[len++] = '\0';

      /* Eat the rest of the line and finish it.  */
      while (c != '\n' && c != EOF)
	c = getc (file);
      goto end_of_line;
    }

  if (len != 0)
    fatal (_("%s:%d: premature end of file"), filename, lineno);

  free (buf);
}

/* Copy unknown object file IBFD onto OBFD.
   Returns TRUE upon success, FALSE otherwise.  */

static bfd_boolean
copy_unknown_object (bfd *ibfd, bfd *obfd)
{
  char *cbuf;
  int tocopy;
  long ncopied;
  long size;
  struct stat buf;

  if (bfd_stat_arch_elt (ibfd, &buf) != 0)
    {
      bfd_nonfatal_message (NULL, ibfd, NULL, NULL);
      return FALSE;
    }

  size = buf.st_size;
  if (size < 0)
    {
      non_fatal (_("stat returns negative size for `%s'"),
		 bfd_get_archive_filename (ibfd));
      return FALSE;
    }

  if (bfd_seek (ibfd, (file_ptr) 0, SEEK_SET) != 0)
    {
      bfd_nonfatal (bfd_get_archive_filename (ibfd));
      return FALSE;
    }

  if (verbose)
    printf (_("copy from `%s' [unknown] to `%s' [unknown]\n"),
	    bfd_get_archive_filename (ibfd), bfd_get_filename (obfd));

  cbuf = (char *) xmalloc (BUFSIZE);
  ncopied = 0;
  while (ncopied < size)
    {
      tocopy = size - ncopied;
      if (tocopy > BUFSIZE)
	tocopy = BUFSIZE;

      if (bfd_bread (cbuf, (bfd_size_type) tocopy, ibfd)
	  != (bfd_size_type) tocopy)
	{
	  bfd_nonfatal_message (NULL, ibfd, NULL, NULL);
	  free (cbuf);
	  return FALSE;
	}

      if (bfd_bwrite (cbuf, (bfd_size_type) tocopy, obfd)
	  != (bfd_size_type) tocopy)
	{
	  bfd_nonfatal_message (NULL, obfd, NULL, NULL);
	  free (cbuf);
	  return FALSE;
	}

      ncopied += tocopy;
    }

  /* We should at least to be able to read it back when copying an
     unknown object in an archive.  */
  chmod (bfd_get_filename (obfd), buf.st_mode | S_IRUSR);
  free (cbuf);
  return TRUE;
}

/* Returns the number of bytes needed to store VAL.  */

static inline unsigned int
num_bytes (unsigned long val)
{
  unsigned int count = 0;

  /* FIXME: There must be a faster way to do this.  */
  while (val)
    {
      count ++;
      val >>= 8;
    }
  return count;
}

typedef struct objcopy_internal_note
{
  Elf_Internal_Note  note;
  bfd_vma            start;
  bfd_vma            end;
  bfd_boolean        modified;
} objcopy_internal_note;
  
/* Returns TRUE if a gap does, or could, exist between the address range
   covered by PNOTE1 and PNOTE2.  */

static bfd_boolean
gap_exists (objcopy_internal_note * pnote1,
	    objcopy_internal_note * pnote2)
{
  /* Without range end notes, we assume that a gap might exist.  */
  if (pnote1->end == 0 || pnote2->end == 0)
    return TRUE;

  /* FIXME: Alignment of 16 bytes taken from x86_64 binaries.
     Really we should extract the alignment of the section covered by the notes.  */
  return BFD_ALIGN (pnote1->end, 16) < pnote2->start;
}

static bfd_boolean
is_open_note (objcopy_internal_note * pnote)
{
  return (pnote->note.type == NT_GNU_BUILD_ATTRIBUTE_OPEN);
}

static bfd_boolean
is_func_note (objcopy_internal_note * pnote)
{
  return (pnote->note.type == NT_GNU_BUILD_ATTRIBUTE_FUNC);
}

static bfd_boolean
is_64bit (bfd * abfd)
{
  /* Should never happen, but let's be paranoid.  */
  if (bfd_get_flavour (abfd) != bfd_target_elf_flavour)
    return FALSE;

  return elf_elfheader (abfd)->e_ident[EI_CLASS] == ELFCLASS64;
}

/* Merge the notes on SEC, removing redundant entries.
   Returns the new, smaller size of the section upon success.  */

static bfd_size_type
merge_gnu_build_notes (bfd * abfd, asection * sec, bfd_size_type size, bfd_byte * contents)
{
  objcopy_internal_note *  pnotes_end;
  objcopy_internal_note *  pnotes = NULL;
  objcopy_internal_note *  pnote;
  bfd_size_type       remain = size;
  unsigned            version_1_seen = 0;
  unsigned            version_2_seen = 0;
  unsigned            version_3_seen = 0;
  bfd_boolean         duplicate_found = FALSE;
  const char *        err = NULL;
  bfd_byte *          in = contents;
  int                 attribute_type_byte;
  int                 val_start;
  unsigned long       previous_func_start = 0;
  unsigned long       previous_open_start = 0;
  unsigned long       previous_func_end = 0;
  unsigned long       previous_open_end = 0;
  long                relsize;


  relsize = bfd_get_reloc_upper_bound (abfd, sec);
  if (relsize > 0)
    {
      arelent **  relpp;
      long        relcount;

      /* If there are relocs associated with this section then we
	 cannot safely merge it.  */
      relpp = (arelent **) xmalloc (relsize);
      relcount = bfd_canonicalize_reloc (abfd, sec, relpp, isympp);
      free (relpp);
      if (relcount != 0)
	goto done;
    }
  
  /* Make a copy of the notes and convert to our internal format.
     Minimum size of a note is 12 bytes.  */
  pnote = pnotes = (objcopy_internal_note *) xcalloc ((size / 12), sizeof (* pnote));
  while (remain >= 12)
    {
      bfd_vma start, end;

      pnote->note.namesz = (bfd_get_32 (abfd, in    ) + 3) & ~3;
      pnote->note.descsz = (bfd_get_32 (abfd, in + 4) + 3) & ~3;
      pnote->note.type   =  bfd_get_32 (abfd, in + 8);

      if (pnote->note.type    != NT_GNU_BUILD_ATTRIBUTE_OPEN
	  && pnote->note.type != NT_GNU_BUILD_ATTRIBUTE_FUNC)
	{
	  err = _("corrupt GNU build attribute note: wrong note type");
	  goto done;
	}

      if (pnote->note.namesz + pnote->note.descsz + 12 > remain)
	{
	  err = _("corrupt GNU build attribute note: note too big");
	  goto done;
	}

      if (pnote->note.namesz < 2)
	{
	  err = _("corrupt GNU build attribute note: name too small");
	  goto done;
	}

      pnote->note.namedata = (char *)(in + 12);
      pnote->note.descdata = (char *)(in + 12 + pnote->note.namesz);

      remain -= 12 + pnote->note.namesz + pnote->note.descsz;
      in     += 12 + pnote->note.namesz + pnote->note.descsz;

      if (pnote->note.namesz > 2
	  && pnote->note.namedata[0] == '$'
	  && pnote->note.namedata[1] == GNU_BUILD_ATTRIBUTE_VERSION
	  && pnote->note.namedata[2] == '1')
	++ version_1_seen;
      else if (pnote->note.namesz > 4
	       && pnote->note.namedata[0] == 'G'
	       && pnote->note.namedata[1] == 'A'
	       && pnote->note.namedata[2] == '$'
	       && pnote->note.namedata[3] == GNU_BUILD_ATTRIBUTE_VERSION)
	{
	  if (pnote->note.namedata[4] == '2')
	    ++ version_2_seen;
	  else if (pnote->note.namedata[4] == '3')
	    ++ version_3_seen;
	  else
	    {
	      err = _("corrupt GNU build attribute note: unsupported version");
	      goto done;
	    }
	}

      switch (pnote->note.descsz)
	{
	case 0:
	  start = end = 0;
	  break;

	case 4:
	  start = bfd_get_32 (abfd, pnote->note.descdata);
	  /* FIXME: For version 1 and 2 notes we should try to
	     calculate the end address by finding a symbol whose
	     value is START, and then adding in its size.

	     For now though, since v1 and v2 was not intended to
	     handle gaps, we chose an artificially large end
	     address.  */
	  end = (bfd_vma) 0x7ffffffffffffffUL;
	  break;
	  
	case 8:
	  if (! is_64bit (abfd))
	    {
	      start = bfd_get_32 (abfd, pnote->note.descdata);
	      end = bfd_get_32 (abfd, pnote->note.descdata + 4);
	    }
	  else
	    {
	      start = bfd_get_64 (abfd, pnote->note.descdata);
	      /* FIXME: For version 1 and 2 notes we should try to
		 calculate the end address by finding a symbol whose
		 value is START, and then adding in its size.

		 For now though, since v1 and v2 was not intended to
		 handle gaps, we chose an artificially large end
		 address.  */
	      end = (bfd_vma) 0x7ffffffffffffffUL;
	    }
	  break;

	case 16:
	  start = bfd_get_64 (abfd, pnote->note.descdata);
	  end = bfd_get_64 (abfd, pnote->note.descdata + 8);
	  break;
	  
	default:
	  err = _("corrupt GNU build attribute note: bad description size");
	  goto done;
	}

      if (is_open_note (pnote))
	{
	  if (start)
	    previous_open_start = start;

	  pnote->start = previous_open_start;

	  if (end)
	    previous_open_end = end;

	  pnote->end = previous_open_end;
	}
      else
	{
	  if (start)
	    previous_func_start = start;

	  pnote->start = previous_func_start;

	  if (end)
	    previous_func_end = end;

	  pnote->end = previous_func_end;
	}

      if (pnote->note.namedata[pnote->note.namesz - 1] != 0)
	{
	  err = _("corrupt GNU build attribute note: name not NUL terminated");
	  goto done;
	}

      pnote ++;
    }

  pnotes_end = pnote;

  /* Check that the notes are valid.  */
  if (remain != 0)
    {
      err = _("corrupt GNU build attribute notes: excess data at end");
      goto done;
    }

  if (version_1_seen == 0 && version_2_seen == 0 && version_3_seen == 0)
    {
      err = _("bad GNU build attribute notes: no known versions detected");
      goto done;
    }

  if ((version_1_seen > 0 && version_2_seen > 0)
      || (version_1_seen > 0 && version_3_seen > 0)
      || (version_2_seen > 0 && version_3_seen > 0))
    {
      err = _("bad GNU build attribute notes: multiple different versions");
      goto done;
    }

  /* Merging is only needed if there is more than one version note...  */
  if (version_1_seen == 1 || version_2_seen == 1 || version_3_seen == 1)
    goto done;

  attribute_type_byte = version_1_seen ? 1 : 3;
  val_start = attribute_type_byte + 1;

  /* The first note should be the first version note.  */
  if (pnotes[0].note.namedata[attribute_type_byte] != GNU_BUILD_ATTRIBUTE_VERSION)
    {
      err = _("bad GNU build attribute notes: first note not version note");
      goto done;
    }

  /* Now merge the notes.  The rules are:
     1. Preserve the ordering of the notes.
     2. Preserve any NT_GNU_BUILD_ATTRIBUTE_FUNC notes.
     3. Eliminate any NT_GNU_BUILD_ATTRIBUTE_OPEN notes that have the same
        full name field as the immediately preceeding note with the same type
	of name and whose address ranges coincide.
	IE - it there are gaps in the coverage of the notes, then these gaps
	must be preserved.
     4. Combine the numeric value of any NT_GNU_BUILD_ATTRIBUTE_OPEN notes
        of type GNU_BUILD_ATTRIBUTE_STACK_SIZE.
     5. If an NT_GNU_BUILD_ATTRIBUTE_OPEN note is going to be preserved and
        its description field is empty then the nearest preceeding OPEN note
	with a non-empty description field must also be preserved *OR* the
	description field of the note must be changed to contain the starting
	address to which it refers.  */
  for (pnote = pnotes + 1; pnote < pnotes_end; pnote ++)
    {
      int                      note_type;
      objcopy_internal_note *  back;
      objcopy_internal_note *  prev_open_with_range = NULL;

      /* Rule 2 - preserve function notes.  */
      if (! is_open_note (pnote))
	continue;

      note_type = pnote->note.namedata[attribute_type_byte];

      /* Scan backwards from pnote, looking for duplicates.
	 Clear the type field of any found - but do not delete them just yet.  */
      for (back = pnote - 1; back >= pnotes; back --)
	{
	  int back_type = back->note.namedata[attribute_type_byte];

	  /* If this is the first open note with an address
	     range that	we have encountered then record it.  */
	  if (prev_open_with_range == NULL
	      && back->note.descsz > 0
	      && ! is_func_note (back))
	    prev_open_with_range = back;

	  if (! is_open_note (back))
	    continue;

	  /* If the two notes are different then keep on searching.  */
	  if (back_type != note_type)
	    continue;

	  /* Rule 4 - combine stack size notes.  */
	  if (back_type == GNU_BUILD_ATTRIBUTE_STACK_SIZE)
	    {
	      unsigned char * name;
	      unsigned long   note_val;
	      unsigned long   back_val;
	      unsigned int    shift;
	      unsigned int    bytes;
	      unsigned long   byte;

	      for (shift = 0, note_val = 0,
		     bytes = pnote->note.namesz - val_start,
		     name = (unsigned char *) pnote->note.namedata + val_start;
		   bytes--;)
		{
		  byte = (* name ++) & 0xff;
		  note_val |= byte << shift;
		  shift += 8;
		}

	      for (shift = 0, back_val = 0,
		     bytes = back->note.namesz - val_start,
		     name = (unsigned char *) back->note.namedata + val_start;
		   bytes--;)
		{
		  byte = (* name ++) & 0xff;
		  back_val |= byte << shift;
		  shift += 8;
		}

	      back_val += note_val;
	      if (num_bytes (back_val) >= back->note.namesz - val_start)
		{
		  /* We have a problem - the new value requires more bytes of
		     storage in the name field than are available.  Currently
		     we have no way of fixing this, so we just preserve both
		     notes.  */
		  continue;
		}

	      /* Write the new val into back.  */
	      name = (unsigned char *) back->note.namedata + val_start;
	      while (name < (unsigned char *) back->note.namedata
		     + back->note.namesz)
		{
		  byte = back_val & 0xff;
		  * name ++ = byte;
		  if (back_val == 0)
		    break;
		  back_val >>= 8;
		}

	      duplicate_found = TRUE;
	      pnote->note.type = 0;
	      break;
	    }

	  /* Rule 3 - combine identical open notes.  */
	  if (back->note.namesz == pnote->note.namesz
	      && memcmp (back->note.namedata,
			 pnote->note.namedata, back->note.namesz) == 0
	      && ! gap_exists (back, pnote))
	    {
	      duplicate_found = TRUE;
	      pnote->note.type = 0;

	      if (pnote->end > back->end)
		back->end = pnote->end;

	      if (version_3_seen)
		back->modified = TRUE;
	      break;
	    }

	  /* Rule 5 - Since we are keeping this note we must check to see
	     if its description refers back to an earlier OPEN version
	     note that has been scheduled for deletion.  If so then we
	     must make sure that version note is also preserved.  */
	  if (version_3_seen)
	    {
	      /* As of version 3 we can just
		 move the range into the note.  */
	      pnote->modified = TRUE;
	      pnote->note.type = NT_GNU_BUILD_ATTRIBUTE_FUNC;
	      back->modified = TRUE;
	      back->note.type = NT_GNU_BUILD_ATTRIBUTE_FUNC;
	    }
	  else
	    {
	      if (pnote->note.descsz == 0
		  && prev_open_with_range != NULL
		  && prev_open_with_range->note.type == 0)
		prev_open_with_range->note.type = NT_GNU_BUILD_ATTRIBUTE_OPEN;
	    }

	  /* We have found a similar attribute but the details do not match.
	     Stop searching backwards.  */
	  break;
	}
    }

  if (duplicate_found)
    {
      bfd_byte *     new_contents;
      bfd_byte *     old;
      bfd_byte *     new;
      bfd_size_type  new_size;
      bfd_vma        prev_start = 0;
      bfd_vma        prev_end = 0;

      /* Eliminate the duplicates.  */
      new = new_contents = xmalloc (size);
      for (pnote = pnotes, old = contents;
	   pnote < pnotes_end;
	   pnote ++)
	{
	  bfd_size_type note_size = 12 + pnote->note.namesz + pnote->note.descsz;

	  if (pnote->note.type != 0)
	    {
	      if (pnote->modified)
		{
		  /* If the note has been modified then we must copy it by
		     hand, potentially adding in a new description field.  */
		  if (pnote->start == prev_start && pnote->end == prev_end)
		    {
		      bfd_put_32 (abfd, pnote->note.namesz, new);
		      bfd_put_32 (abfd, 0, new + 4);
		      bfd_put_32 (abfd, pnote->note.type, new + 8);
		      new += 12;
		      memcpy (new, pnote->note.namedata, pnote->note.namesz);
		      new += pnote->note.namesz;
		    }
		  else
		    {
		      bfd_put_32 (abfd, pnote->note.namesz, new);
		      bfd_put_32 (abfd, is_64bit (abfd) ? 16 : 8, new + 4);
		      bfd_put_32 (abfd, pnote->note.type, new + 8);
		      new += 12;
		      memcpy (new, pnote->note.namedata, pnote->note.namesz);
		      new += pnote->note.namesz;
		      if (is_64bit (abfd))
			{
			  bfd_put_64 (abfd, pnote->start, new);
			  bfd_put_64 (abfd, pnote->end, new + 8);
			  new += 16;
			}
		      else
			{
			  bfd_put_32 (abfd, pnote->start, new);
			  bfd_put_32 (abfd, pnote->end, new + 4);
			  new += 8;
			}
		    }
		}
	      else
		{
		  memcpy (new, old, note_size);
		  new += note_size;
		}
	      prev_start = pnote->start;
	      prev_end = pnote->end;
	    }

	  old += note_size;
	}

      new_size = new - new_contents;
      memcpy (contents, new_contents, new_size);
      size = new_size;
      free (new_contents);
    }

 done:
  if (err)
    {
      bfd_set_error (bfd_error_bad_value);
      bfd_nonfatal_message (NULL, abfd, sec, err);
      status = 1;
    }

  free (pnotes);
  return size;
}

/* Copy object file IBFD onto OBFD.
   Returns TRUE upon success, FALSE otherwise.  */

static bfd_boolean
copy_object (bfd *ibfd, bfd *obfd, const bfd_arch_info_type *input_arch)
{
  bfd_vma start;
  long symcount;
  asection **osections = NULL;
  asection *osec;
  asection *gnu_debuglink_section = NULL;
  bfd_size_type *gaps = NULL;
  bfd_size_type max_gap = 0;
  long symsize;
  void *dhandle;
  enum bfd_architecture iarch;
  unsigned int imach;
  unsigned int c, i;

  if (ibfd->xvec->byteorder != obfd->xvec->byteorder
      && ibfd->xvec->byteorder != BFD_ENDIAN_UNKNOWN
      && obfd->xvec->byteorder != BFD_ENDIAN_UNKNOWN)
    {
      /* PR 17636: Call non-fatal so that we return to our parent who
	 may need to tidy temporary files.  */
      non_fatal (_("Unable to change endianness of input file(s)"));
      return FALSE;
    }

  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    {
      bfd_nonfatal_message (NULL, obfd, NULL, NULL);
      return FALSE;
    }

  if (ibfd->sections == NULL)
    {
      non_fatal (_("error: the input file '%s' has no sections"),
		 bfd_get_archive_filename (ibfd));
      return FALSE;
    }

  if (ibfd->xvec->flavour != bfd_target_elf_flavour)
    {
      if ((do_debug_sections & compress) != 0
	  && do_debug_sections != compress)
	{
	  non_fatal (_("--compress-debug-sections=[zlib|zlib-gnu|zlib-gabi] is unsupported on `%s'"),
		     bfd_get_archive_filename (ibfd));
	  return FALSE;
	}

      if (do_elf_stt_common)
	{
	  non_fatal (_("--elf-stt-common=[yes|no] is unsupported on `%s'"),
		     bfd_get_archive_filename (ibfd));
	  return FALSE;
	}
    }

  if (verbose)
    printf (_("copy from `%s' [%s] to `%s' [%s]\n"),
	    bfd_get_archive_filename (ibfd), bfd_get_target (ibfd),
	    bfd_get_filename (obfd), bfd_get_target (obfd));

  if (extract_symbol)
    start = 0;
  else
    {
      if (set_start_set)
	start = set_start;
      else
	start = bfd_get_start_address (ibfd);
      start += change_start;
    }

  /* Neither the start address nor the flags
     need to be set for a core file.  */
  if (bfd_get_format (obfd) != bfd_core)
    {
      flagword flags;

      flags = bfd_get_file_flags (ibfd);
      flags |= bfd_flags_to_set;
      flags &= ~bfd_flags_to_clear;
      flags &= bfd_applicable_file_flags (obfd);

      if (strip_symbols == STRIP_ALL)
	flags &= ~HAS_RELOC;

      if (!bfd_set_start_address (obfd, start)
	  || !bfd_set_file_flags (obfd, flags))
	{
	  bfd_nonfatal_message (NULL, ibfd, NULL, NULL);
	  return FALSE;
	}
    }

  /* Copy architecture of input file to output file.  */
  iarch = bfd_get_arch (ibfd);
  imach = bfd_get_mach (ibfd);
  if (input_arch)
    {
      if (bfd_get_arch_info (ibfd) == NULL
	  || bfd_get_arch_info (ibfd)->arch == bfd_arch_unknown)
	{
	  iarch = input_arch->arch;
	  imach = input_arch->mach;
	}
      else
	non_fatal (_("Input file `%s' ignores binary architecture parameter."),
		   bfd_get_archive_filename (ibfd));
    }
  if (!bfd_set_arch_mach (obfd, iarch, imach)
      && (ibfd->target_defaulted
	  || bfd_get_arch (ibfd) != bfd_get_arch (obfd)))
    {
      if (bfd_get_arch (ibfd) == bfd_arch_unknown)
	non_fatal (_("Unable to recognise the format of the input file `%s'"),
		   bfd_get_archive_filename (ibfd));
      else
	non_fatal (_("Output file cannot represent architecture `%s'"),
		   bfd_printable_arch_mach (bfd_get_arch (ibfd),
					    bfd_get_mach (ibfd)));
      return FALSE;
    }

  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    {
      bfd_nonfatal_message (NULL, ibfd, NULL, NULL);
      return FALSE;
    }

  if (bfd_get_flavour (obfd) == bfd_target_coff_flavour
      && bfd_pei_p (obfd))
    {
      /* Set up PE parameters.  */
      pe_data_type *pe = pe_data (obfd);

      /* Copy PE parameters before changing them.  */
      if (ibfd->xvec->flavour == bfd_target_coff_flavour
	  && bfd_pei_p (ibfd))
	pe->pe_opthdr = pe_data (ibfd)->pe_opthdr;

      if (pe_file_alignment != (bfd_vma) -1)
	pe->pe_opthdr.FileAlignment = pe_file_alignment;
      else
	pe_file_alignment = PE_DEF_FILE_ALIGNMENT;

      if (pe_heap_commit != (bfd_vma) -1)
	pe->pe_opthdr.SizeOfHeapCommit = pe_heap_commit;

      if (pe_heap_reserve != (bfd_vma) -1)
	pe->pe_opthdr.SizeOfHeapCommit = pe_heap_reserve;

      if (pe_image_base != (bfd_vma) -1)
	pe->pe_opthdr.ImageBase = pe_image_base;

      if (pe_section_alignment != (bfd_vma) -1)
	pe->pe_opthdr.SectionAlignment = pe_section_alignment;
      else
	pe_section_alignment = PE_DEF_SECTION_ALIGNMENT;

      if (pe_stack_commit != (bfd_vma) -1)
	pe->pe_opthdr.SizeOfStackCommit = pe_stack_commit;

      if (pe_stack_reserve != (bfd_vma) -1)
	pe->pe_opthdr.SizeOfStackCommit = pe_stack_reserve;

      if (pe_subsystem != -1)
	pe->pe_opthdr.Subsystem = pe_subsystem;

      if (pe_major_subsystem_version != -1)
	pe->pe_opthdr.MajorSubsystemVersion = pe_major_subsystem_version;

      if (pe_minor_subsystem_version != -1)
	pe->pe_opthdr.MinorSubsystemVersion = pe_minor_subsystem_version;

      if (pe_file_alignment > pe_section_alignment)
	{
	  char file_alignment[20], section_alignment[20];

	  sprintf_vma (file_alignment, pe_file_alignment);
	  sprintf_vma (section_alignment, pe_section_alignment);
	  non_fatal (_("warning: file alignment (0x%s) > section alignment (0x%s)"),

		     file_alignment, section_alignment);
	}
    }

  if (isympp)
    free (isympp);

  if (osympp != isympp)
    free (osympp);

  isympp = NULL;
  osympp = NULL;

  symsize = bfd_get_symtab_upper_bound (ibfd);
  if (symsize < 0)
    {
      bfd_nonfatal_message (NULL, ibfd, NULL, NULL);
      return FALSE;
    }

  osympp = isympp = (asymbol **) xmalloc (symsize);
  symcount = bfd_canonicalize_symtab (ibfd, isympp);
  if (symcount < 0)
    {
      bfd_nonfatal_message (NULL, ibfd, NULL, NULL);
      return FALSE;
    }
  /* PR 17512: file:  d6323821
     If the symbol table could not be loaded do not pretend that we have
     any symbols.  This trips us up later on when we load the relocs.  */
  if (symcount == 0)
    {
      free (isympp);
      osympp = isympp = NULL;
    }

  /* BFD mandates that all output sections be created and sizes set before
     any output is done.  Thus, we traverse all sections multiple times.  */
  bfd_map_over_sections (ibfd, setup_section, obfd);

  if (!extract_symbol)
    setup_bfd_headers (ibfd, obfd);

  if (add_sections != NULL)
    {
      struct section_add *padd;
      struct section_list *pset;

      for (padd = add_sections; padd != NULL; padd = padd->next)
	{
	  flagword flags;

	  pset = find_section_list (padd->name, FALSE,
				    SECTION_CONTEXT_SET_FLAGS);
	  if (pset != NULL)
	    flags = pset->flags | SEC_HAS_CONTENTS;
	  else
	    flags = SEC_HAS_CONTENTS | SEC_READONLY | SEC_DATA;

	  /* bfd_make_section_with_flags() does not return very helpful
	     error codes, so check for the most likely user error first.  */
	  if (bfd_get_section_by_name (obfd, padd->name))
	    {
	      bfd_nonfatal_message (NULL, obfd, NULL,
				    _("can't add section '%s'"), padd->name);
	      return FALSE;
	    }
	  else
	    {
	      /* We use LINKER_CREATED here so that the backend hooks
		 will create any special section type information,
		 instead of presuming we know what we're doing merely
		 because we set the flags.  */
	      padd->section = bfd_make_section_with_flags
		(obfd, padd->name, flags | SEC_LINKER_CREATED);
	      if (padd->section == NULL)
		{
		  bfd_nonfatal_message (NULL, obfd, NULL,
					_("can't create section `%s'"),
					padd->name);
		  return FALSE;
		}
	    }

	  if (! bfd_set_section_size (obfd, padd->section, padd->size))
	    {
	      bfd_nonfatal_message (NULL, obfd, padd->section, NULL);
	      return FALSE;
	    }

	  pset = find_section_list (padd->name, FALSE,
				    SECTION_CONTEXT_SET_VMA | SECTION_CONTEXT_ALTER_VMA);
	  if (pset != NULL
	      && ! bfd_set_section_vma (obfd, padd->section, pset->vma_val))
	    {
	      bfd_nonfatal_message (NULL, obfd, padd->section, NULL);
	      return FALSE;
	    }

	  pset = find_section_list (padd->name, FALSE,
				    SECTION_CONTEXT_SET_LMA | SECTION_CONTEXT_ALTER_LMA);
	  if (pset != NULL)
	    {
	      padd->section->lma = pset->lma_val;

	      if (! bfd_set_section_alignment
		  (obfd, padd->section,
		   bfd_section_alignment (obfd, padd->section)))
		{
		  bfd_nonfatal_message (NULL, obfd, padd->section, NULL);
		  return FALSE;
		}
	    }
	}
    }

  if (update_sections != NULL)
    {
      struct section_add *pupdate;

      for (pupdate = update_sections;
	   pupdate != NULL;
	   pupdate = pupdate->next)
	{
	  pupdate->section = bfd_get_section_by_name (ibfd, pupdate->name);
	  if (pupdate->section == NULL)
	    {
	      non_fatal (_("error: %s not found, can't be updated"), pupdate->name);
	      return FALSE;
	    }

	  osec = pupdate->section->output_section;
	  if (! bfd_set_section_size (obfd, osec, pupdate->size))
	    {
	      bfd_nonfatal_message (NULL, obfd, osec, NULL);
	      return FALSE;
	    }
	}
    }

  if (merge_notes)
    {
      /* This palaver is necessary because we must set the output
	 section size first, before its contents are ready.  */
      osec = bfd_get_section_by_name (ibfd, GNU_BUILD_ATTRS_SECTION_NAME);
      if (osec && is_merged_note_section (ibfd, osec))
	{
	  bfd_size_type size;
	  
	  size = bfd_get_section_size (osec);
	  if (size == 0)
	    {
	      bfd_nonfatal_message (NULL, ibfd, osec, _("warning: note section is empty"));
	      merge_notes = FALSE;
	    }
	  else if (! bfd_get_full_section_contents (ibfd, osec, & merged_notes))
	    {
	      bfd_nonfatal_message (NULL, ibfd, osec, _("warning: could not load note section"));
	      free (merged_notes);
	      merged_notes = NULL;
	      merge_notes = FALSE;
	    }
	  else
	    {
	      merged_size = merge_gnu_build_notes (ibfd, osec, size, merged_notes);
	      if (merged_size == size)
		{
		  /* Merging achieves nothing.  */
		  free (merged_notes);
		  merged_notes = NULL;
		  merge_notes = FALSE;
		  merged_size = 0;
		}
	      else
		{
		  if (osec->output_section == NULL
		      || ! bfd_set_section_size (obfd, osec->output_section, merged_size))
		    {
		      bfd_nonfatal_message (NULL, obfd, osec, _("warning: failed to set merged notes size"));
		      free (merged_notes);
		      merged_notes = NULL;
		      merge_notes = FALSE;
		      merged_size = 0;
		    }
		}
	    }
	}
    }

  if (dump_sections != NULL)
    {
      struct section_add * pdump;

      for (pdump = dump_sections; pdump != NULL; pdump = pdump->next)
	{
	  osec = bfd_get_section_by_name (ibfd, pdump->name);
	  if (osec == NULL)
	    {
	      bfd_nonfatal_message (NULL, ibfd, NULL,
				    _("can't dump section '%s' - it does not exist"),
				    pdump->name);
	      continue;
	    }

	  if ((bfd_get_section_flags (ibfd, osec) & SEC_HAS_CONTENTS) == 0)
	    {
	      bfd_nonfatal_message (NULL, ibfd, osec,
				    _("can't dump section - it has no contents"));
	      continue;
	    }

	  bfd_size_type size = bfd_get_section_size (osec);
	  if (size == 0)
	    {
	      bfd_nonfatal_message (NULL, ibfd, osec,
				    _("can't dump section - it is empty"));
	      continue;
	    }

	  FILE * f;
	  f = fopen (pdump->filename, FOPEN_WB);
	  if (f == NULL)
	    {
	      bfd_nonfatal_message (pdump->filename, NULL, NULL,
				    _("could not open section dump file"));
	      continue;
	    }

	  bfd_byte *contents;
	  if (bfd_malloc_and_get_section (ibfd, osec, &contents))
	    {
	      if (fwrite (contents, 1, size, f) != size)
		{
		  non_fatal (_("error writing section contents to %s (error: %s)"),
			     pdump->filename,
			     strerror (errno));
		  free (contents);
		  return FALSE;
		}
	    }
	  else
	    bfd_nonfatal_message (NULL, ibfd, osec,
				  _("could not retrieve section contents"));

	  fclose (f);
	  free (contents);
	}
    }

  if (gnu_debuglink_filename != NULL)
    {
      /* PR 15125: Give a helpful warning message if
	 the debuglink section already exists, and
	 allow the rest of the copy to complete.  */
      if (bfd_get_section_by_name (obfd, ".gnu_debuglink"))
	{
	  non_fatal (_("%s: debuglink section already exists"),
		     bfd_get_filename (obfd));
	  gnu_debuglink_filename = NULL;
	}
      else
	{
	  gnu_debuglink_section = bfd_create_gnu_debuglink_section
	    (obfd, gnu_debuglink_filename);

	  if (gnu_debuglink_section == NULL)
	    {
	      bfd_nonfatal_message (NULL, obfd, NULL,
				    _("cannot create debug link section `%s'"),
				    gnu_debuglink_filename);
	      return FALSE;
	    }

	  /* Special processing for PE format files.  We
	     have no way to distinguish PE from COFF here.  */
	  if (bfd_get_flavour (obfd) == bfd_target_coff_flavour)
	    {
	      bfd_vma debuglink_vma;
	      asection * highest_section;

	      /* The PE spec requires that all sections be adjacent and sorted
		 in ascending order of VMA.  It also specifies that debug
		 sections should be last.  This is despite the fact that debug
		 sections are not loaded into memory and so in theory have no
		 use for a VMA.

		 This means that the debuglink section must be given a non-zero
		 VMA which makes it contiguous with other debug sections.  So
		 walk the current section list, find the section with the
		 highest VMA and start the debuglink section after that one.  */
	      for (osec = obfd->sections, highest_section = NULL;
		   osec != NULL;
		   osec = osec->next)
		if (osec->vma > 0
		    && (highest_section == NULL
			|| osec->vma > highest_section->vma))
		  highest_section = osec;

	      if (highest_section)
		debuglink_vma = BFD_ALIGN (highest_section->vma
					   + highest_section->size,
					   /* FIXME: We ought to be using
					      COFF_PAGE_SIZE here or maybe
					      bfd_get_section_alignment() (if it
					      was set) but since this is for PE
					      and we know the required alignment
					      it is easier just to hard code it.  */
					   0x1000);
	      else
		/* Umm, not sure what to do in this case.  */
		debuglink_vma = 0x1000;

	      bfd_set_section_vma (obfd, gnu_debuglink_section, debuglink_vma);
	    }
	}
    }

  c = bfd_count_sections (obfd);
  if (c != 0
      && (gap_fill_set || pad_to_set))
    {
      asection **set;

      /* We must fill in gaps between the sections and/or we must pad
	 the last section to a specified address.  We do this by
	 grabbing a list of the sections, sorting them by VMA, and
	 increasing the section sizes as required to fill the gaps.
	 We write out the gap contents below.  */

      osections = (asection **) xmalloc (c * sizeof (asection *));
      set = osections;
      bfd_map_over_sections (obfd, get_sections, &set);

      qsort (osections, c, sizeof (asection *), compare_section_lma);

      gaps = (bfd_size_type *) xmalloc (c * sizeof (bfd_size_type));
      memset (gaps, 0, c * sizeof (bfd_size_type));

      if (gap_fill_set)
	{
	  for (i = 0; i < c - 1; i++)
	    {
	      flagword flags;
	      bfd_size_type size;
	      bfd_vma gap_start, gap_stop;

	      flags = bfd_get_section_flags (obfd, osections[i]);
	      if ((flags & SEC_HAS_CONTENTS) == 0
		  || (flags & SEC_LOAD) == 0)
		continue;

	      size = bfd_section_size (obfd, osections[i]);
	      gap_start = bfd_section_lma (obfd, osections[i]) + size;
	      gap_stop = bfd_section_lma (obfd, osections[i + 1]);
	      if (gap_start < gap_stop)
		{
		  if (! bfd_set_section_size (obfd, osections[i],
					      size + (gap_stop - gap_start)))
		    {
		      bfd_nonfatal_message (NULL, obfd, osections[i],
					    _("Can't fill gap after section"));
		      status = 1;
		      break;
		    }
		  gaps[i] = gap_stop - gap_start;
		  if (max_gap < gap_stop - gap_start)
		    max_gap = gap_stop - gap_start;
		}
	    }
	}

      if (pad_to_set)
	{
	  bfd_vma lma;
	  bfd_size_type size;

	  lma = bfd_section_lma (obfd, osections[c - 1]);
	  size = bfd_section_size (obfd, osections[c - 1]);
	  if (lma + size < pad_to)
	    {
	      if (! bfd_set_section_size (obfd, osections[c - 1],
					  pad_to - lma))
		{
		  bfd_nonfatal_message (NULL, obfd, osections[c - 1],
					_("can't add padding"));
		  status = 1;
		}
	      else
		{
		  gaps[c - 1] = pad_to - (lma + size);
		  if (max_gap < pad_to - (lma + size))
		    max_gap = pad_to - (lma + size);
		}
	    }
	}
    }

  /* Symbol filtering must happen after the output sections
     have been created, but before their contents are set.  */
  dhandle = NULL;
  if (convert_debugging)
    dhandle = read_debugging_info (ibfd, isympp, symcount, FALSE);

  if (strip_symbols == STRIP_DEBUG
      || strip_symbols == STRIP_ALL
      || strip_symbols == STRIP_UNNEEDED
      || strip_symbols == STRIP_NONDEBUG
      || strip_symbols == STRIP_DWO
      || strip_symbols == STRIP_NONDWO
      || discard_locals != LOCALS_UNDEF
      || localize_hidden
      || htab_elements (strip_specific_htab) != 0
      || htab_elements (keep_specific_htab) != 0
      || htab_elements (localize_specific_htab) != 0
      || htab_elements (globalize_specific_htab) != 0
      || htab_elements (keepglobal_specific_htab) != 0
      || htab_elements (weaken_specific_htab) != 0
      || htab_elements (redefine_specific_htab) != 0
      || prefix_symbols_string
      || sections_removed
      || sections_copied
      || convert_debugging
      || change_leading_char
      || remove_leading_char
      || section_rename_list
      || weaken
      || add_symbols)
    {
      /* Mark symbols used in output relocations so that they
	 are kept, even if they are local labels or static symbols.

	 Note we iterate over the input sections examining their
	 relocations since the relocations for the output sections
	 haven't been set yet.  mark_symbols_used_in_relocations will
	 ignore input sections which have no corresponding output
	 section.  */
      if (strip_symbols != STRIP_ALL)
	bfd_map_over_sections (ibfd,
			       mark_symbols_used_in_relocations,
			       isympp);
      osympp = (asymbol **) xmalloc ((symcount + add_symbols + 1) * sizeof (asymbol *));
      symcount = filter_symbols (ibfd, obfd, osympp, isympp, symcount);
    }

  if (convert_debugging && dhandle != NULL)
    {
      if (! write_debugging_info (obfd, dhandle, &symcount, &osympp))
	{
	  status = 1;
	  return FALSE;
	}
    }

  bfd_set_symtab (obfd, osympp, symcount);

  /* This has to happen before section positions are set.  */
  bfd_map_over_sections (ibfd, copy_relocations_in_section, obfd);

  /* This has to happen after the symbol table has been set.  */
  bfd_map_over_sections (ibfd, copy_section, obfd);

  if (add_sections != NULL)
    {
      struct section_add *padd;

      for (padd = add_sections; padd != NULL; padd = padd->next)
	{
	  if (! bfd_set_section_contents (obfd, padd->section, padd->contents,
					  0, padd->size))
	    {
	      bfd_nonfatal_message (NULL, obfd, padd->section, NULL);
	      return FALSE;
	    }
	}
    }

  if (update_sections != NULL)
    {
      struct section_add *pupdate;

      for (pupdate = update_sections;
	   pupdate != NULL;
	   pupdate = pupdate->next)
	{
	  osec = pupdate->section->output_section;
	  if (! bfd_set_section_contents (obfd, osec, pupdate->contents,
					  0, pupdate->size))
	    {
	      bfd_nonfatal_message (NULL, obfd, osec, NULL);
	      return FALSE;
	    }
	}
    }

  if (merge_notes)
    {
      osec = bfd_get_section_by_name (obfd, GNU_BUILD_ATTRS_SECTION_NAME);
      if (osec && is_merged_note_section (obfd, osec))
	{
	  if (! bfd_set_section_contents (obfd, osec, merged_notes, 0, merged_size))
	    {
	      bfd_nonfatal_message (NULL, obfd, osec, _("error: failed to copy merged notes into output"));
	      return FALSE;
	    }
	}
      else if (! is_strip)
	bfd_nonfatal_message (NULL, obfd, osec, _("could not find any mergeable note sections"));
      free (merged_notes);
      merged_notes = NULL;
      merge_notes = FALSE;
    }

  if (gnu_debuglink_filename != NULL)
    {
      if (! bfd_fill_in_gnu_debuglink_section
	  (obfd, gnu_debuglink_section, gnu_debuglink_filename))
	{
	  bfd_nonfatal_message (NULL, obfd, NULL,
				_("cannot fill debug link section `%s'"),
				gnu_debuglink_filename);
	  return FALSE;
	}
    }

  if (gap_fill_set || pad_to_set)
    {
      bfd_byte *buf;

      /* Fill in the gaps.  */
      if (max_gap > 8192)
	max_gap = 8192;
      buf = (bfd_byte *) xmalloc (max_gap);
      memset (buf, gap_fill, max_gap);

      c = bfd_count_sections (obfd);
      for (i = 0; i < c; i++)
	{
	  if (gaps[i] != 0)
	    {
	      bfd_size_type left;
	      file_ptr off;

	      left = gaps[i];
	      off = bfd_section_size (obfd, osections[i]) - left;

	      while (left > 0)
		{
		  bfd_size_type now;

		  if (left > 8192)
		    now = 8192;
		  else
		    now = left;

		  if (! bfd_set_section_contents (obfd, osections[i], buf,
						  off, now))
		    {
		      bfd_nonfatal_message (NULL, obfd, osections[i], NULL);
		      free (buf);
		      return FALSE;
		    }

		  left -= now;
		  off += now;
		}
	    }
	}
      free (buf);
    }

  /* Allow the BFD backend to copy any private data it understands
     from the input BFD to the output BFD.  This is done last to
     permit the routine to look at the filtered symbol table, which is
     important for the ECOFF code at least.  */
  if (! bfd_copy_private_bfd_data (ibfd, obfd))
    {
      bfd_nonfatal_message (NULL, obfd, NULL,
			    _("error copying private BFD data"));
      return FALSE;
    }

  /* Switch to the alternate machine code.  We have to do this at the
     very end, because we only initialize the header when we create
     the first section.  */
  if (use_alt_mach_code != 0)
    {
      if (! bfd_alt_mach_code (obfd, use_alt_mach_code))
	{
	  non_fatal (_("this target does not support %lu alternative machine codes"),
		     use_alt_mach_code);
	  if (bfd_get_flavour (obfd) == bfd_target_elf_flavour)
	    {
	      non_fatal (_("treating that number as an absolute e_machine value instead"));
	      elf_elfheader (obfd)->e_machine = use_alt_mach_code;
	    }
	  else
	    non_fatal (_("ignoring the alternative value"));
	}
    }

  return TRUE;
}

/* Read each archive element in turn from IBFD, copy the
   contents to temp file, and keep the temp file handle.
   If 'force_output_target' is TRUE then make sure that
   all elements in the new archive are of the type
   'output_target'.  */

static void
copy_archive (bfd *ibfd, bfd *obfd, const char *output_target,
	      bfd_boolean force_output_target,
	      const bfd_arch_info_type *input_arch)
{
  struct name_list
    {
      struct name_list *next;
      const char *name;
      bfd *obfd;
    } *list, *l;
  bfd **ptr = &obfd->archive_head;
  bfd *this_element;
  char *dir;
  const char *filename;

  /* Make a temp directory to hold the contents.  */
  dir = make_tempdir (bfd_get_filename (obfd));
  if (dir == NULL)
    fatal (_("cannot create tempdir for archive copying (error: %s)"),
	   strerror (errno));

  if (strip_symbols == STRIP_ALL)
    obfd->has_armap = FALSE;
  else
    obfd->has_armap = ibfd->has_armap;
  obfd->is_thin_archive = ibfd->is_thin_archive;

  if (deterministic)
    obfd->flags |= BFD_DETERMINISTIC_OUTPUT;

  list = NULL;

  this_element = bfd_openr_next_archived_file (ibfd, NULL);

  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    {
      status = 1;
      bfd_nonfatal_message (NULL, obfd, NULL, NULL);
      goto cleanup_and_exit;
    }

  while (!status && this_element != NULL)
    {
      char *output_name;
      bfd *output_bfd;
      bfd *last_element;
      struct stat buf;
      int stat_status = 0;
      bfd_boolean del = TRUE;
      bfd_boolean ok_object;

      /* PR binutils/17533: Do not allow directory traversal
	 outside of the current directory tree by archive members.  */
      if (! is_valid_archive_path (bfd_get_filename (this_element)))
	{
	  non_fatal (_("illegal pathname found in archive member: %s"),
		     bfd_get_filename (this_element));
	  status = 1;
	  goto cleanup_and_exit;
	}

      /* Create an output file for this member.  */
      output_name = concat (dir, "/",
			    bfd_get_filename (this_element), (char *) 0);

      /* If the file already exists, make another temp dir.  */
      if (stat (output_name, &buf) >= 0)
	{
	  output_name = make_tempdir (output_name);
	  if (output_name == NULL)
	    {
	      non_fatal (_("cannot create tempdir for archive copying (error: %s)"),
			 strerror (errno));
	      status = 1;
	      goto cleanup_and_exit;
	    }

	  l = (struct name_list *) xmalloc (sizeof (struct name_list));
	  l->name = output_name;
	  l->next = list;
	  l->obfd = NULL;
	  list = l;
	  output_name = concat (output_name, "/",
				bfd_get_filename (this_element), (char *) 0);
	}

      if (preserve_dates)
	{
	  stat_status = bfd_stat_arch_elt (this_element, &buf);

	  if (stat_status != 0)
	    non_fatal (_("internal stat error on %s"),
		       bfd_get_filename (this_element));
	}

      l = (struct name_list *) xmalloc (sizeof (struct name_list));
      l->name = output_name;
      l->next = list;
      l->obfd = NULL;
      list = l;

      ok_object = bfd_check_format (this_element, bfd_object);
      if (!ok_object)
	bfd_nonfatal_message (NULL, this_element, NULL,
			      _("Unable to recognise the format of file"));

      /* PR binutils/3110: Cope with archives
	 containing multiple target types.  */
      if (force_output_target || !ok_object)
	output_bfd = bfd_openw (output_name, output_target);
      else
	output_bfd = bfd_openw (output_name, bfd_get_target (this_element));

      if (output_bfd == NULL)
	{
	  bfd_nonfatal_message (output_name, NULL, NULL, NULL);
	  status = 1;
	  goto cleanup_and_exit;
	}

      if (ok_object)
	{
	  del = !copy_object (this_element, output_bfd, input_arch);

	  if (del && bfd_get_arch (this_element) == bfd_arch_unknown)
	    /* Try again as an unknown object file.  */
	    ok_object = FALSE;
	  else if (!bfd_close (output_bfd))
	    {
	      bfd_nonfatal_message (output_name, NULL, NULL, NULL);
	      /* Error in new object file. Don't change archive.  */
	      status = 1;
	    }
	}

      if (!ok_object)
	{
	  del = !copy_unknown_object (this_element, output_bfd);
	  if (!bfd_close_all_done (output_bfd))
	    {
	      bfd_nonfatal_message (output_name, NULL, NULL, NULL);
	      /* Error in new object file. Don't change archive.  */
	      status = 1;
	    }
	}

      if (del)
	{
	  unlink (output_name);
	  status = 1;
	}
      else
	{
	  if (preserve_dates && stat_status == 0)
	    set_times (output_name, &buf);

	  /* Open the newly output file and attach to our list.  */
	  output_bfd = bfd_openr (output_name, output_target);

	  l->obfd = output_bfd;

	  *ptr = output_bfd;
	  ptr = &output_bfd->archive_next;

	  last_element = this_element;

	  this_element = bfd_openr_next_archived_file (ibfd, last_element);

	  bfd_close (last_element);
	}
    }
  *ptr = NULL;

  filename = bfd_get_filename (obfd);
  if (!bfd_close (obfd))
    {
      status = 1;
      bfd_nonfatal_message (filename, NULL, NULL, NULL);
    }

  filename = bfd_get_filename (ibfd);
  if (!bfd_close (ibfd))
    {
      status = 1;
      bfd_nonfatal_message (filename, NULL, NULL, NULL);
    }

 cleanup_and_exit:
  /* Delete all the files that we opened.  */
  for (l = list; l != NULL; l = l->next)
    {
      if (l->obfd == NULL)
	rmdir (l->name);
      else
	{
	  bfd_close (l->obfd);
	  unlink (l->name);
	}
    }

  rmdir (dir);
}

static void
set_long_section_mode (bfd *output_bfd, bfd *input_bfd, enum long_section_name_handling style)
{
  /* This is only relevant to Coff targets.  */
  if (bfd_get_flavour (output_bfd) == bfd_target_coff_flavour)
    {
      if (style == KEEP
	  && bfd_get_flavour (input_bfd) == bfd_target_coff_flavour)
	style = bfd_coff_long_section_names (input_bfd) ? ENABLE : DISABLE;
      bfd_coff_set_long_section_names (output_bfd, style != DISABLE);
    }
}

/* The top-level control.  */

static void
copy_file (const char *input_filename, const char *output_filename,
	   const char *input_target,   const char *output_target,
	   const bfd_arch_info_type *input_arch)
{
  bfd *ibfd;
  char **obj_matching;
  char **core_matching;
  off_t size = get_file_size (input_filename);

  if (size < 1)
    {
      if (size == 0)
	non_fatal (_("error: the input file '%s' is empty"),
		   input_filename);
      status = 1;
      return;
    }

  /* To allow us to do "strip *" without dying on the first
     non-object file, failures are nonfatal.  */
  ibfd = bfd_openr (input_filename, input_target);
  if (ibfd == NULL)
    {
      bfd_nonfatal_message (input_filename, NULL, NULL, NULL);
      status = 1;
      return;
    }

  switch (do_debug_sections)
    {
    case compress:
    case compress_zlib:
    case compress_gnu_zlib:
    case compress_gabi_zlib:
      ibfd->flags |= BFD_COMPRESS;
      /* Don't check if input is ELF here since this information is
	 only available after bfd_check_format_matches is called.  */
      if (do_debug_sections != compress_gnu_zlib)
	ibfd->flags |= BFD_COMPRESS_GABI;
      break;
    case decompress:
      ibfd->flags |= BFD_DECOMPRESS;
      break;
    default:
      break;
    }

  switch (do_elf_stt_common)
    {
    case elf_stt_common:
      ibfd->flags |= BFD_CONVERT_ELF_COMMON | BFD_USE_ELF_STT_COMMON;
      break;
      break;
    case no_elf_stt_common:
      ibfd->flags |= BFD_CONVERT_ELF_COMMON;
      break;
    default:
      break;
    }

  if (bfd_check_format (ibfd, bfd_archive))
    {
      bfd_boolean force_output_target;
      bfd *obfd;

      /* bfd_get_target does not return the correct value until
	 bfd_check_format succeeds.  */
      if (output_target == NULL)
	{
	  output_target = bfd_get_target (ibfd);
	  force_output_target = FALSE;
	}
      else
	force_output_target = TRUE;

      obfd = bfd_openw (output_filename, output_target);
      if (obfd == NULL)
	{
	  bfd_nonfatal_message (output_filename, NULL, NULL, NULL);
	  status = 1;
	  return;
	}
      /* This is a no-op on non-Coff targets.  */
      set_long_section_mode (obfd, ibfd, long_section_names);

      copy_archive (ibfd, obfd, output_target, force_output_target, input_arch);
    }
  else if (bfd_check_format_matches (ibfd, bfd_object, &obj_matching))
    {
      bfd *obfd;
    do_copy:

      /* bfd_get_target does not return the correct value until
	 bfd_check_format succeeds.  */
      if (output_target == NULL)
	output_target = bfd_get_target (ibfd);

      obfd = bfd_openw (output_filename, output_target);
      if (obfd == NULL)
 	{
 	  bfd_nonfatal_message (output_filename, NULL, NULL, NULL);
 	  status = 1;
 	  return;
 	}
      /* This is a no-op on non-Coff targets.  */
      set_long_section_mode (obfd, ibfd, long_section_names);

      if (! copy_object (ibfd, obfd, input_arch))
	status = 1;

      /* PR 17512: file: 0f15796a.
	 If the file could not be copied it may not be in a writeable
	 state.  So use bfd_close_all_done to avoid the possibility of
	 writing uninitialised data into the file.  */
      if (! (status ? bfd_close_all_done (obfd) : bfd_close (obfd)))
	{
	  status = 1;
	  bfd_nonfatal_message (output_filename, NULL, NULL, NULL);
	  return;
	}

      if (!bfd_close (ibfd))
	{
	  status = 1;
	  bfd_nonfatal_message (input_filename, NULL, NULL, NULL);
	  return;
	}
    }
  else
    {
      bfd_error_type obj_error = bfd_get_error ();
      bfd_error_type core_error;

      if (bfd_check_format_matches (ibfd, bfd_core, &core_matching))
	{
	  /* This probably can't happen..  */
	  if (obj_error == bfd_error_file_ambiguously_recognized)
	    free (obj_matching);
	  goto do_copy;
	}

      core_error = bfd_get_error ();
      /* Report the object error in preference to the core error.  */
      if (obj_error != core_error)
	bfd_set_error (obj_error);

      bfd_nonfatal_message (input_filename, NULL, NULL, NULL);

      if (obj_error == bfd_error_file_ambiguously_recognized)
	{
	  list_matching_formats (obj_matching);
	  free (obj_matching);
	}
      if (core_error == bfd_error_file_ambiguously_recognized)
	{
	  list_matching_formats (core_matching);
	  free (core_matching);
	}

      status = 1;
    }
}

/* Add a name to the section renaming list.  */

static void
add_section_rename (const char * old_name, const char * new_name,
		    flagword flags)
{
  section_rename * srename;

  /* Check for conflicts first.  */
  for (srename = section_rename_list; srename != NULL; srename = srename->next)
    if (strcmp (srename->old_name, old_name) == 0)
      {
	/* Silently ignore duplicate definitions.  */
	if (strcmp (srename->new_name, new_name) == 0
	    && srename->flags == flags)
	  return;

	fatal (_("Multiple renames of section %s"), old_name);
      }

  srename = (section_rename *) xmalloc (sizeof (* srename));

  srename->old_name = old_name;
  srename->new_name = new_name;
  srename->flags    = flags;
  srename->next     = section_rename_list;

  section_rename_list = srename;
}

/* Check the section rename list for a new name of the input section
   called OLD_NAME.  Returns the new name if one is found and sets
   RETURNED_FLAGS if non-NULL to the flags to be used for this section.  */

static const char *
find_section_rename (const char *old_name, flagword *returned_flags)
{
  const section_rename *srename;

  for (srename = section_rename_list; srename != NULL; srename = srename->next)
    if (strcmp (srename->old_name, old_name) == 0)
      {
	if (returned_flags != NULL && srename->flags != (flagword) -1)
	  *returned_flags = srename->flags;

	return srename->new_name;
      }

  return old_name;
}

/* Once each of the sections is copied, we may still need to do some
   finalization work for private section headers.  Do that here.  */

static void
setup_bfd_headers (bfd *ibfd, bfd *obfd)
{
  /* Allow the BFD backend to copy any private data it understands
     from the input section to the output section.  */
  if (! bfd_copy_private_header_data (ibfd, obfd))
    {
      status = 1;
      bfd_nonfatal_message (NULL, ibfd, NULL,
			    _("error in private header data"));
      return;
    }

  /* All went well.  */
  return;
}

/* Create a section in OBFD with the same
   name and attributes as ISECTION in IBFD.  */

static void
setup_section (bfd *ibfd, sec_ptr isection, void *obfdarg)
{
  bfd *obfd = (bfd *) obfdarg;
  struct section_list *p;
  sec_ptr osection;
  bfd_size_type size;
  bfd_vma vma;
  bfd_vma lma;
  flagword flags;
  const char *err;
  const char * name;
  char *prefix = NULL;
  bfd_boolean make_nobits;

  if (is_strip_section (ibfd, isection))
    return;

  /* Get the, possibly new, name of the output section.  */
  name = bfd_section_name (ibfd, isection);
  flags = bfd_get_section_flags (ibfd, isection);
  name = find_section_rename (name, &flags);

  /* Prefix sections.  */
  if ((prefix_alloc_sections_string)
      && (bfd_get_section_flags (ibfd, isection) & SEC_ALLOC))
    prefix = prefix_alloc_sections_string;
  else if (prefix_sections_string)
    prefix = prefix_sections_string;

  if (prefix)
    {
      char *n;

      n = (char *) xmalloc (strlen (prefix) + strlen (name) + 1);
      strcpy (n, prefix);
      strcat (n, name);
      name = n;
    }

  make_nobits = FALSE;

  p = find_section_list (bfd_section_name (ibfd, isection), FALSE,
			 SECTION_CONTEXT_SET_FLAGS);
  if (p != NULL)
    flags = p->flags | (flags & (SEC_HAS_CONTENTS | SEC_RELOC));
  else if (strip_symbols == STRIP_NONDEBUG
	   && (flags & (SEC_ALLOC | SEC_GROUP)) != 0
	   && !is_nondebug_keep_contents_section (ibfd, isection))
    {
      flags &= ~(SEC_HAS_CONTENTS | SEC_LOAD | SEC_GROUP);
      if (obfd->xvec->flavour == bfd_target_elf_flavour)
	{
	  make_nobits = TRUE;

	  /* Twiddle the input section flags so that it seems to
	     elf.c:copy_private_bfd_data that section flags have not
	     changed between input and output sections.  This hack
	     prevents wholesale rewriting of the program headers.  */
	  isection->flags &= ~(SEC_HAS_CONTENTS | SEC_LOAD | SEC_GROUP);
	}
    }

  osection = bfd_make_section_anyway_with_flags (obfd, name, flags);

  if (osection == NULL)
    {
      err = _("failed to create output section");
      goto loser;
    }

  if (make_nobits)
    elf_section_type (osection) = SHT_NOBITS;

  size = bfd_section_size (ibfd, isection);
  size = bfd_convert_section_size (ibfd, isection, obfd, size);
  if (copy_byte >= 0)
    size = (size + interleave - 1) / interleave * copy_width;
  else if (extract_symbol)
    size = 0;
  if (! bfd_set_section_size (obfd, osection, size))
    {
      err = _("failed to set size");
      goto loser;
    }

  vma = bfd_section_vma (ibfd, isection);
  p = find_section_list (bfd_section_name (ibfd, isection), FALSE,
			 SECTION_CONTEXT_ALTER_VMA | SECTION_CONTEXT_SET_VMA);
  if (p != NULL)
    {
      if (p->context & SECTION_CONTEXT_SET_VMA)
	vma = p->vma_val;
      else
	vma += p->vma_val;
    }
  else
    vma += change_section_address;

  if (! bfd_set_section_vma (obfd, osection, vma))
    {
      err = _("failed to set vma");
      goto loser;
    }

  lma = isection->lma;
  p = find_section_list (bfd_section_name (ibfd, isection), FALSE,
			 SECTION_CONTEXT_ALTER_LMA | SECTION_CONTEXT_SET_LMA);
  if (p != NULL)
    {
      if (p->context & SECTION_CONTEXT_ALTER_LMA)
	lma += p->lma_val;
      else
	lma = p->lma_val;
    }
  else
    lma += change_section_address;

  osection->lma = lma;

  /* FIXME: This is probably not enough.  If we change the LMA we
     may have to recompute the header for the file as well.  */
  if (!bfd_set_section_alignment (obfd,
				  osection,
				  bfd_section_alignment (ibfd, isection)))
    {
      err = _("failed to set alignment");
      goto loser;
    }

  /* Copy merge entity size.  */
  osection->entsize = isection->entsize;

  /* Copy compress status.  */
  osection->compress_status = isection->compress_status;

  /* This used to be mangle_section; we do here to avoid using
     bfd_get_section_by_name since some formats allow multiple
     sections with the same name.  */
  isection->output_section = osection;
  isection->output_offset = 0;

  if ((isection->flags & SEC_GROUP) != 0)
    {
      asymbol *gsym = group_signature (isection);

      if (gsym != NULL)
	{
	  gsym->flags |= BSF_KEEP;
	  if (ibfd->xvec->flavour == bfd_target_elf_flavour)
	    elf_group_id (isection) = gsym;
	}
    }

  /* Allow the BFD backend to copy any private data it understands
     from the input section to the output section.  */
  if (!bfd_copy_private_section_data (ibfd, isection, obfd, osection))
    {
      err = _("failed to copy private data");
      goto loser;
    }

  /* All went well.  */
  return;

 loser:
  status = 1;
  bfd_nonfatal_message (NULL, obfd, osection, err);
}

/* Return TRUE if input section ISECTION should be skipped.  */

static bfd_boolean
skip_section (bfd *ibfd, sec_ptr isection, bfd_boolean skip_copy)
{
  sec_ptr osection;
  bfd_size_type size;
  flagword flags;

  /* If we have already failed earlier on,
     do not keep on generating complaints now.  */
  if (status != 0)
    return TRUE;

  if (extract_symbol)
    return TRUE;

  if (is_strip_section (ibfd, isection))
    return TRUE;

  if (is_update_section (ibfd, isection))
    return TRUE;

  /* When merging a note section we skip the copying of the contents,
     but not the copying of the relocs associated with the contents.  */
  if (skip_copy && is_merged_note_section (ibfd, isection))
    return TRUE;

  flags = bfd_get_section_flags (ibfd, isection);
  if ((flags & SEC_GROUP) != 0)
    return TRUE;

  osection = isection->output_section;
  size = bfd_get_section_size (isection);

  if (size == 0 || osection == 0)
    return TRUE;

  return FALSE;
}

/* Add section SECTION_PATTERN to the list of sections that will have their
   relocations removed.  */

static void
handle_remove_relocations_option (const char *section_pattern)
{
  find_section_list (section_pattern, TRUE, SECTION_CONTEXT_REMOVE_RELOCS);
}

/* Return TRUE if ISECTION from IBFD should have its relocations removed,
   otherwise return FALSE.  If the user has requested that relocations be
   removed from a section that does not have relocations then this
   function will still return TRUE.  */

static bfd_boolean
discard_relocations (bfd *ibfd ATTRIBUTE_UNUSED, asection *isection)
{
  return (find_section_list (bfd_section_name (ibfd, isection), FALSE,
			     SECTION_CONTEXT_REMOVE_RELOCS) != NULL);
}

/* Wrapper for dealing with --remove-section (-R) command line arguments.
   A special case is detected here, if the user asks to remove a relocation
   section (one starting with ".rela." or ".rel.") then this removal must
   be done using a different technique.  */

static void
handle_remove_section_option (const char *section_pattern)
{
  if (strncmp (section_pattern, ".rela.", 6) == 0)
    handle_remove_relocations_option (section_pattern + 5);
  else if (strncmp (section_pattern, ".rel.", 5) == 0)
    handle_remove_relocations_option (section_pattern + 4);
  else
    {
      find_section_list (section_pattern, TRUE, SECTION_CONTEXT_REMOVE);
      sections_removed = TRUE;
    }
}

/* Copy relocations in input section ISECTION of IBFD to an output
   section with the same name in OBFDARG.  If stripping then don't
   copy any relocation info.  */

static void
copy_relocations_in_section (bfd *ibfd, sec_ptr isection, void *obfdarg)
{
  bfd *obfd = (bfd *) obfdarg;
  long relsize;
  arelent **relpp;
  long relcount;
  sec_ptr osection;

 if (skip_section (ibfd, isection, FALSE))
    return;

  osection = isection->output_section;

  /* Core files and DWO files do not need to be relocated.  */
  if (bfd_get_format (obfd) == bfd_core
      || strip_symbols == STRIP_NONDWO
      || discard_relocations (ibfd, isection))
    relsize = 0;
  else
    {
      relsize = bfd_get_reloc_upper_bound (ibfd, isection);

      if (relsize < 0)
	{
	  /* Do not complain if the target does not support relocations.  */
	  if (relsize == -1 && bfd_get_error () == bfd_error_invalid_operation)
	    relsize = 0;
	  else
	    {
	      status = 1;
	      bfd_nonfatal_message (NULL, ibfd, isection, NULL);
	      return;
	    }
	}
    }

  if (relsize == 0)
    {
      bfd_set_reloc (obfd, osection, NULL, 0);
      osection->flags &= ~SEC_RELOC;
    }
  else
    {
      if (isection->orelocation != NULL)
	{
	  /* Some other function has already set up the output relocs
	     for us, so scan those instead of the default relocs.  */
	  relcount = isection->reloc_count;
	  relpp = isection->orelocation;
	}
      else
	{
	  relpp = (arelent **) xmalloc (relsize);
	  relcount = bfd_canonicalize_reloc (ibfd, isection, relpp, isympp);
	  if (relcount < 0)
	    {
	      status = 1;
	      bfd_nonfatal_message (NULL, ibfd, isection,
				    _("relocation count is negative"));
	      return;
	    }
	}

      if (strip_symbols == STRIP_ALL)
	{
	  /* Remove relocations which are not in
	     keep_strip_specific_list.  */
	  arelent **temp_relpp;
	  long temp_relcount = 0;
	  long i;

	  temp_relpp = (arelent **) xmalloc (relsize);
	  for (i = 0; i < relcount; i++)
	    {
	      /* PR 17512: file: 9e907e0c.  */
	      if (relpp[i]->sym_ptr_ptr
		  /* PR 20096 */
		  && * relpp[i]->sym_ptr_ptr)
		if (is_specified_symbol (bfd_asymbol_name (*relpp[i]->sym_ptr_ptr),
					 keep_specific_htab))
		  temp_relpp [temp_relcount++] = relpp [i];
	    }
	  relcount = temp_relcount;
	  if (isection->orelocation == NULL)
	    free (relpp);
	  relpp = temp_relpp;
	}

      bfd_set_reloc (obfd, osection, relcount == 0 ? NULL : relpp, relcount);
      if (relcount == 0)
	{
	  osection->flags &= ~SEC_RELOC;
	  free (relpp);
	}
    }
}

/* Copy the data of input section ISECTION of IBFD
   to an output section with the same name in OBFD.  */

static void
copy_section (bfd *ibfd, sec_ptr isection, void *obfdarg)
{
  bfd *obfd = (bfd *) obfdarg;
  struct section_list *p;
  sec_ptr osection;
  bfd_size_type size;

  if (skip_section (ibfd, isection, TRUE))
    return;

  osection = isection->output_section;
  /* The output SHF_COMPRESSED section size is different from input if
     ELF classes of input and output aren't the same.  We can't use
     the output section size since --interleave will shrink the output
     section.   Size will be updated if the section is converted.   */
  size = bfd_get_section_size (isection);

  if (bfd_get_section_flags (ibfd, isection) & SEC_HAS_CONTENTS
      && bfd_get_section_flags (obfd, osection) & SEC_HAS_CONTENTS)
    {
      bfd_byte *memhunk = NULL;

      if (!bfd_get_full_section_contents (ibfd, isection, &memhunk)
	  || !bfd_convert_section_contents (ibfd, isection, obfd,
					    &memhunk, &size))
	{
	  status = 1;
	  bfd_nonfatal_message (NULL, ibfd, isection, NULL);
	  free (memhunk);
	  return;
	}

      if (reverse_bytes)
	{
	  /* We don't handle leftover bytes (too many possible behaviors,
	     and we don't know what the user wants).  The section length
	     must be a multiple of the number of bytes to swap.  */
	  if ((size % reverse_bytes) == 0)
	    {
	      unsigned long i, j;
	      bfd_byte b;

	      for (i = 0; i < size; i += reverse_bytes)
		for (j = 0; j < (unsigned long)(reverse_bytes / 2); j++)
		  {
		    bfd_byte *m = (bfd_byte *) memhunk;

		    b = m[i + j];
		    m[i + j] = m[(i + reverse_bytes) - (j + 1)];
		    m[(i + reverse_bytes) - (j + 1)] = b;
		  }
	    }
	  else
	    /* User must pad the section up in order to do this.  */
	    fatal (_("cannot reverse bytes: length of section %s must be evenly divisible by %d"),
		   bfd_section_name (ibfd, isection), reverse_bytes);
	}

      if (copy_byte >= 0)
	{
	  /* Keep only every `copy_byte'th byte in MEMHUNK.  */
	  char *from = (char *) memhunk + copy_byte;
	  char *to = (char *) memhunk;
	  char *end = (char *) memhunk + size;
	  int i;

	  /* If the section address is not exactly divisible by the interleave,
	     then we must bias the from address.  If the copy_byte is less than
	     the bias, then we must skip forward one interleave, and increment
	     the final lma.  */
	  int extra = isection->lma % interleave;
	  from -= extra;
	  if (copy_byte < extra)
	    from += interleave;

	  for (; from < end; from += interleave)
	    for (i = 0; i < copy_width; i++)
	      {
		if (&from[i] >= end)
		  break;
		*to++ = from[i];
	      }

	  size = (size + interleave - 1 - copy_byte) / interleave * copy_width;
	  osection->lma /= interleave;
	  if (copy_byte < extra)
	    osection->lma++;
	}

      if (!bfd_set_section_contents (obfd, osection, memhunk, 0, size))
	{
	  status = 1;
	  bfd_nonfatal_message (NULL, obfd, osection, NULL);
	  free (memhunk);
	  return;
	}
      free (memhunk);
    }
  else if ((p = find_section_list (bfd_get_section_name (ibfd, isection),
				   FALSE, SECTION_CONTEXT_SET_FLAGS)) != NULL
	   && (p->flags & SEC_HAS_CONTENTS) != 0)
    {
      void *memhunk = xmalloc (size);

      /* We don't permit the user to turn off the SEC_HAS_CONTENTS
	 flag--they can just remove the section entirely and add it
	 back again.  However, we do permit them to turn on the
	 SEC_HAS_CONTENTS flag, and take it to mean that the section
	 contents should be zeroed out.  */

      memset (memhunk, 0, size);
      if (! bfd_set_section_contents (obfd, osection, memhunk, 0, size))
	{
	  status = 1;
	  bfd_nonfatal_message (NULL, obfd, osection, NULL);
	  free (memhunk);
	  return;
	}
      free (memhunk);
    }
}

/* Get all the sections.  This is used when --gap-fill or --pad-to is
   used.  */

static void
get_sections (bfd *obfd ATTRIBUTE_UNUSED, asection *osection, void *secppparg)
{
  asection ***secppp = (asection ***) secppparg;

  **secppp = osection;
  ++(*secppp);
}

/* Sort sections by VMA.  This is called via qsort, and is used when
   --gap-fill or --pad-to is used.  We force non loadable or empty
   sections to the front, where they are easier to ignore.  */

static int
compare_section_lma (const void *arg1, const void *arg2)
{
  const asection *const *sec1 = (const asection * const *) arg1;
  const asection *const *sec2 = (const asection * const *) arg2;
  flagword flags1, flags2;

  /* Sort non loadable sections to the front.  */
  flags1 = (*sec1)->flags;
  flags2 = (*sec2)->flags;
  if ((flags1 & SEC_HAS_CONTENTS) == 0
      || (flags1 & SEC_LOAD) == 0)
    {
      if ((flags2 & SEC_HAS_CONTENTS) != 0
	  && (flags2 & SEC_LOAD) != 0)
	return -1;
    }
  else
    {
      if ((flags2 & SEC_HAS_CONTENTS) == 0
	  || (flags2 & SEC_LOAD) == 0)
	return 1;
    }

  /* Sort sections by LMA.  */
  if ((*sec1)->lma > (*sec2)->lma)
    return 1;
  else if ((*sec1)->lma < (*sec2)->lma)
    return -1;

  /* Sort sections with the same LMA by size.  */
  if (bfd_get_section_size (*sec1) > bfd_get_section_size (*sec2))
    return 1;
  else if (bfd_get_section_size (*sec1) < bfd_get_section_size (*sec2))
    return -1;

  return 0;
}

/* Mark all the symbols which will be used in output relocations with
   the BSF_KEEP flag so that those symbols will not be stripped.

   Ignore relocations which will not appear in the output file.  */

static void
mark_symbols_used_in_relocations (bfd *ibfd, sec_ptr isection, void *symbolsarg)
{
  asymbol **symbols = (asymbol **) symbolsarg;
  long relsize;
  arelent **relpp;
  long relcount, i;

  /* Ignore an input section with no corresponding output section.  */
  if (isection->output_section == NULL)
    return;

  relsize = bfd_get_reloc_upper_bound (ibfd, isection);
  if (relsize < 0)
    {
      /* Do not complain if the target does not support relocations.  */
      if (relsize == -1 && bfd_get_error () == bfd_error_invalid_operation)
	return;
      bfd_fatal (bfd_get_filename (ibfd));
    }

  if (relsize == 0)
    return;

  relpp = (arelent **) xmalloc (relsize);
  relcount = bfd_canonicalize_reloc (ibfd, isection, relpp, symbols);
  if (relcount < 0)
    bfd_fatal (bfd_get_filename (ibfd));

  /* Examine each symbol used in a relocation.  If it's not one of the
     special bfd section symbols, then mark it with BSF_KEEP.  */
  for (i = 0; i < relcount; i++)
    {
      /* See PRs 20923 and 20930 for reproducers for the NULL tests.  */
      if (relpp[i]->sym_ptr_ptr != NULL
	  && * relpp[i]->sym_ptr_ptr != NULL
	  && *relpp[i]->sym_ptr_ptr != bfd_com_section_ptr->symbol
	  && *relpp[i]->sym_ptr_ptr != bfd_abs_section_ptr->symbol
	  && *relpp[i]->sym_ptr_ptr != bfd_und_section_ptr->symbol)
	(*relpp[i]->sym_ptr_ptr)->flags |= BSF_KEEP;
    }

  if (relpp != NULL)
    free (relpp);
}

/* Write out debugging information.  */

static bfd_boolean
write_debugging_info (bfd *obfd, void *dhandle,
		      long *symcountp ATTRIBUTE_UNUSED,
		      asymbol ***symppp ATTRIBUTE_UNUSED)
{
  if (bfd_get_flavour (obfd) == bfd_target_ieee_flavour)
    return write_ieee_debugging_info (obfd, dhandle);

  if (bfd_get_flavour (obfd) == bfd_target_coff_flavour
      || bfd_get_flavour (obfd) == bfd_target_elf_flavour)
    {
      bfd_byte *syms, *strings;
      bfd_size_type symsize, stringsize;
      asection *stabsec, *stabstrsec;
      flagword flags;

      if (! write_stabs_in_sections_debugging_info (obfd, dhandle, &syms,
						    &symsize, &strings,
						    &stringsize))
	return FALSE;

      flags = SEC_HAS_CONTENTS | SEC_READONLY | SEC_DEBUGGING;
      stabsec = bfd_make_section_with_flags (obfd, ".stab", flags);
      stabstrsec = bfd_make_section_with_flags (obfd, ".stabstr", flags);
      if (stabsec == NULL
	  || stabstrsec == NULL
	  || ! bfd_set_section_size (obfd, stabsec, symsize)
	  || ! bfd_set_section_size (obfd, stabstrsec, stringsize)
	  || ! bfd_set_section_alignment (obfd, stabsec, 2)
	  || ! bfd_set_section_alignment (obfd, stabstrsec, 0))
	{
	  bfd_nonfatal_message (NULL, obfd, NULL,
				_("can't create debugging section"));
	  return FALSE;
	}

      /* We can get away with setting the section contents now because
	 the next thing the caller is going to do is copy over the
	 real sections.  We may someday have to split the contents
	 setting out of this function.  */
      if (! bfd_set_section_contents (obfd, stabsec, syms, 0, symsize)
	  || ! bfd_set_section_contents (obfd, stabstrsec, strings, 0,
					 stringsize))
	{
	  bfd_nonfatal_message (NULL, obfd, NULL,
				_("can't set debugging section contents"));
	  return FALSE;
	}

      return TRUE;
    }

  bfd_nonfatal_message (NULL, obfd, NULL,
			_("don't know how to write debugging information for %s"),
			bfd_get_target (obfd));
  return FALSE;
}

/* If neither -D nor -U was specified explicitly,
   then use the configured default.  */
static void
default_deterministic (void)
{
  if (deterministic < 0)
    deterministic = DEFAULT_AR_DETERMINISTIC;
}

static int
strip_main (int argc, char *argv[])
{
  char *input_target = NULL;
  char *output_target = NULL;
  bfd_boolean show_version = FALSE;
  bfd_boolean formats_info = FALSE;
  int c;
  int i;
  char *output_file = NULL;

  merge_notes = TRUE;

  while ((c = getopt_long (argc, argv, "I:O:F:K:MN:R:o:sSpdgxXHhVvwDU",
			   strip_options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case 'I':
	  input_target = optarg;
	  break;
	case 'O':
	  output_target = optarg;
	  break;
	case 'F':
	  input_target = output_target = optarg;
	  break;
	case 'R':
	  handle_remove_section_option (optarg);
	  break;
	case OPTION_REMOVE_RELOCS:
	  handle_remove_relocations_option (optarg);
	  break;
	case 's':
	  strip_symbols = STRIP_ALL;
	  break;
	case 'S':
	case 'g':
	case 'd':	/* Historic BSD alias for -g.  Used by early NetBSD.  */
	  strip_symbols = STRIP_DEBUG;
	  break;
	case OPTION_STRIP_DWO:
	  strip_symbols = STRIP_DWO;
	  break;
	case OPTION_STRIP_UNNEEDED:
	  strip_symbols = STRIP_UNNEEDED;
	  break;
	case 'K':
	  add_specific_symbol (optarg, keep_specific_htab);
	  break;
	case 'M':
	  merge_notes = TRUE;
	  break;
	case OPTION_NO_MERGE_NOTES:
	  merge_notes = FALSE;
	  break;
	case 'N':
	  add_specific_symbol (optarg, strip_specific_htab);
	  break;
	case 'o':
	  output_file = optarg;
	  break;
	case 'p':
	  preserve_dates = TRUE;
	  break;
	case 'D':
	  deterministic = TRUE;
	  break;
	case 'U':
	  deterministic = FALSE;
	  break;
	case 'x':
	  discard_locals = LOCALS_ALL;
	  break;
	case 'X':
	  discard_locals = LOCALS_START_L;
	  break;
	case 'v':
	  verbose = TRUE;
	  break;
	case 'V':
	  show_version = TRUE;
	  break;
	case OPTION_FORMATS_INFO:
	  formats_info = TRUE;
	  break;
	case OPTION_ONLY_KEEP_DEBUG:
	  strip_symbols = STRIP_NONDEBUG;
	  break;
	case OPTION_KEEP_FILE_SYMBOLS:
	  keep_file_symbols = 1;
	  break;
	case 0:
	  /* We've been given a long option.  */
	  break;
	case 'w':
	  wildcard = TRUE;
	  break;
	case 'H':
	case 'h':
	  strip_usage (stdout, 0);
	default:
	  strip_usage (stderr, 1);
	}
    }

  if (formats_info)
    {
      display_info ();
      return 0;
    }

  if (show_version)
    print_version ("strip");

  default_deterministic ();

  /* Default is to strip all symbols.  */
  if (strip_symbols == STRIP_UNDEF
      && discard_locals == LOCALS_UNDEF
      && htab_elements (strip_specific_htab) == 0)
    strip_symbols = STRIP_ALL;

  if (output_target == NULL)
    output_target = input_target;

  i = optind;
  if (i == argc
      || (output_file != NULL && (i + 1) < argc))
    strip_usage (stderr, 1);

  for (; i < argc; i++)
    {
      int hold_status = status;
      struct stat statbuf;
      char *tmpname;

      if (get_file_size (argv[i]) < 1)
	{
	  status = 1;
	  continue;
	}

      if (preserve_dates)
	/* No need to check the return value of stat().
	   It has already been checked in get_file_size().  */
	stat (argv[i], &statbuf);

      if (output_file == NULL
	  || filename_cmp (argv[i], output_file) == 0)
	tmpname = make_tempname (argv[i]);
      else
	tmpname = output_file;

      if (tmpname == NULL)
	{
	  bfd_nonfatal_message (argv[i], NULL, NULL,
				_("could not create temporary file to hold stripped copy"));
	  status = 1;
	  continue;
	}

      status = 0;
      copy_file (argv[i], tmpname, input_target, output_target, NULL);
      if (status == 0)
	{
	  if (preserve_dates)
	    set_times (tmpname, &statbuf);
	  if (output_file != tmpname)
	    status = (smart_rename (tmpname,
				    output_file ? output_file : argv[i],
				    preserve_dates) != 0);
	  if (status == 0)
	    status = hold_status;
	}
      else
	unlink_if_ordinary (tmpname);
      if (output_file != tmpname)
	free (tmpname);
    }

  return status;
}

/* Set up PE subsystem.  */

static void
set_pe_subsystem (const char *s)
{
  const char *version, *subsystem;
  size_t i;
  static const struct
    {
      const char *name;
      const char set_def;
      const short value;
    }
  v[] =
    {
      { "native", 0, IMAGE_SUBSYSTEM_NATIVE },
      { "windows", 0, IMAGE_SUBSYSTEM_WINDOWS_GUI },
      { "console", 0, IMAGE_SUBSYSTEM_WINDOWS_CUI },
      { "posix", 0, IMAGE_SUBSYSTEM_POSIX_CUI },
      { "wince", 0, IMAGE_SUBSYSTEM_WINDOWS_CE_GUI },
      { "efi-app", 1, IMAGE_SUBSYSTEM_EFI_APPLICATION },
      { "efi-bsd", 1, IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER },
      { "efi-rtd", 1, IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER },
      { "sal-rtd", 1, IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER },
      { "xbox", 0, IMAGE_SUBSYSTEM_XBOX }
    };
  short value;
  char *copy;
  int set_def = -1;

  /* Check for the presence of a version number.  */
  version = strchr (s, ':');
  if (version == NULL)
    subsystem = s;
  else
    {
      int len = version - s;
      copy = xstrdup (s);
      subsystem = copy;
      copy[len] = '\0';
      version = copy + 1 + len;
      pe_major_subsystem_version = strtoul (version, &copy, 0);
      if (*copy == '.')
	pe_minor_subsystem_version = strtoul (copy + 1, &copy, 0);
      if (*copy != '\0')
	non_fatal (_("%s: bad version in PE subsystem"), s);
    }

  /* Check for numeric subsystem.  */
  value = (short) strtol (subsystem, &copy, 0);
  if (*copy == '\0')
    {
      for (i = 0; i < ARRAY_SIZE (v); i++)
	if (v[i].value == value)
	  {
	    pe_subsystem = value;
	    set_def = v[i].set_def;
	    break;
	  }
    }
  else
    {
      /* Search for subsystem by name.  */
      for (i = 0; i < ARRAY_SIZE (v); i++)
	if (strcmp (subsystem, v[i].name) == 0)
	  {
	    pe_subsystem = v[i].value;
	    set_def = v[i].set_def;
	    break;
	  }
    }

  switch (set_def)
    {
    case -1:
      fatal (_("unknown PE subsystem: %s"), s);
      break;
    case 0:
      break;
    default:
      if (pe_file_alignment == (bfd_vma) -1)
	pe_file_alignment = PE_DEF_FILE_ALIGNMENT;
      if (pe_section_alignment == (bfd_vma) -1)
	pe_section_alignment = PE_DEF_SECTION_ALIGNMENT;
      break;
    }
  if (s != subsystem)
    free ((char *) subsystem);
}

/* Convert EFI target to PEI target.  */

static void
convert_efi_target (char *efi)
{
  efi[0] = 'p';
  efi[1] = 'e';
  efi[2] = 'i';

  if (strcmp (efi + 4, "ia32") == 0)
    {
      /* Change ia32 to i386.  */
      efi[5]= '3';
      efi[6]= '8';
      efi[7]= '6';
    }
  else if (strcmp (efi + 4, "x86_64") == 0)
    {
      /* Change x86_64 to x86-64.  */
      efi[7] = '-';
    }
}

/* Allocate and return a pointer to a struct section_add, initializing the
   structure using ARG, a string in the format "sectionname=filename".
   The returned structure will have its next pointer set to NEXT.  The
   OPTION field is the name of the command line option currently being
   parsed, and is only used if an error needs to be reported.  */

static struct section_add *
init_section_add (const char *arg,
		  struct section_add *next,
		  const char *option)
{
  struct section_add *pa;
  const char *s;

  s = strchr (arg, '=');
  if (s == NULL)
    fatal (_("bad format for %s"), option);

  pa = (struct section_add *) xmalloc (sizeof (struct section_add));
  pa->name = xstrndup (arg, s - arg);
  pa->filename = s + 1;
  pa->next = next;
  pa->contents = NULL;
  pa->size = 0;

  return pa;
}

/* Load the file specified in PA, allocating memory to hold the file
   contents, and store a pointer to the allocated memory in the contents
   field of PA.  The size field of PA is also updated.  All errors call
   FATAL.  */

static void
section_add_load_file (struct section_add *pa)
{
  size_t off, alloc;
  FILE *f;

  /* We don't use get_file_size so that we can do
     --add-section .note.GNU_stack=/dev/null
     get_file_size doesn't work on /dev/null.  */

  f = fopen (pa->filename, FOPEN_RB);
  if (f == NULL)
    fatal (_("cannot open: %s: %s"),
	   pa->filename, strerror (errno));

  off = 0;
  alloc = 4096;
  pa->contents = (bfd_byte *) xmalloc (alloc);
  while (!feof (f))
    {
      off_t got;

      if (off == alloc)
	{
	  alloc <<= 1;
	  pa->contents = (bfd_byte *) xrealloc (pa->contents, alloc);
	}

      got = fread (pa->contents + off, 1, alloc - off, f);
      if (ferror (f))
	fatal (_("%s: fread failed"), pa->filename);

      off += got;
    }

  pa->size = off;

  fclose (f);
}

static int
copy_main (int argc, char *argv[])
{
  char *input_filename = NULL;
  char *output_filename = NULL;
  char *tmpname;
  char *input_target = NULL;
  char *output_target = NULL;
  bfd_boolean show_version = FALSE;
  bfd_boolean change_warn = TRUE;
  bfd_boolean formats_info = FALSE;
  int c;
  struct stat statbuf;
  const bfd_arch_info_type *input_arch = NULL;

  while ((c = getopt_long (argc, argv, "b:B:i:I:j:K:MN:s:O:d:F:L:G:R:SpgxXHhVvW:wDU",
			   copy_options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case 'b':
	  copy_byte = atoi (optarg);
	  if (copy_byte < 0)
	    fatal (_("byte number must be non-negative"));
	  break;

	case 'B':
	  input_arch = bfd_scan_arch (optarg);
	  if (input_arch == NULL)
	    fatal (_("architecture %s unknown"), optarg);
	  break;

	case 'i':
	  if (optarg)
	    {
	      interleave = atoi (optarg);
	      if (interleave < 1)
		fatal (_("interleave must be positive"));
	    }
	  else
	    interleave = 4;
	  break;

	case OPTION_INTERLEAVE_WIDTH:
	  copy_width = atoi (optarg);
	  if (copy_width < 1)
	    fatal(_("interleave width must be positive"));
	  break;

	case 'I':
	case 's':		/* "source" - 'I' is preferred */
	  input_target = optarg;
	  break;

	case 'O':
	case 'd':		/* "destination" - 'O' is preferred */
	  output_target = optarg;
	  break;

	case 'F':
	  input_target = output_target = optarg;
	  break;

	case 'j':
	  find_section_list (optarg, TRUE, SECTION_CONTEXT_COPY);
	  sections_copied = TRUE;
	  break;

	case 'R':
	  handle_remove_section_option (optarg);
	  break;

        case OPTION_REMOVE_RELOCS:
	  handle_remove_relocations_option (optarg);
	  break;

	case 'S':
	  strip_symbols = STRIP_ALL;
	  break;

	case 'g':
	  strip_symbols = STRIP_DEBUG;
	  break;

	case OPTION_STRIP_DWO:
	  strip_symbols = STRIP_DWO;
	  break;

	case OPTION_STRIP_UNNEEDED:
	  strip_symbols = STRIP_UNNEEDED;
	  break;

	case OPTION_ONLY_KEEP_DEBUG:
	  strip_symbols = STRIP_NONDEBUG;
	  break;

	case OPTION_KEEP_FILE_SYMBOLS:
	  keep_file_symbols = 1;
	  break;

	case OPTION_ADD_GNU_DEBUGLINK:
	  long_section_names = ENABLE ;
	  gnu_debuglink_filename = optarg;
	  break;

	case 'K':
	  add_specific_symbol (optarg, keep_specific_htab);
	  break;

	case 'M':
	  merge_notes = TRUE;
	  break;
	case OPTION_NO_MERGE_NOTES:
	  merge_notes = FALSE;
	  break;

	case 'N':
	  add_specific_symbol (optarg, strip_specific_htab);
	  break;

	case OPTION_STRIP_UNNEEDED_SYMBOL:
	  add_specific_symbol (optarg, strip_unneeded_htab);
	  break;

	case 'L':
	  add_specific_symbol (optarg, localize_specific_htab);
	  break;

	case OPTION_GLOBALIZE_SYMBOL:
	  add_specific_symbol (optarg, globalize_specific_htab);
	  break;

	case 'G':
	  add_specific_symbol (optarg, keepglobal_specific_htab);
	  break;

	case 'W':
	  add_specific_symbol (optarg, weaken_specific_htab);
	  break;

	case 'p':
	  preserve_dates = TRUE;
	  break;

	case 'D':
	  deterministic = TRUE;
	  break;

	case 'U':
	  deterministic = FALSE;
	  break;

	case 'w':
	  wildcard = TRUE;
	  break;

	case 'x':
	  discard_locals = LOCALS_ALL;
	  break;

	case 'X':
	  discard_locals = LOCALS_START_L;
	  break;

	case 'v':
	  verbose = TRUE;
	  break;

	case 'V':
	  show_version = TRUE;
	  break;

	case OPTION_FORMATS_INFO:
	  formats_info = TRUE;
	  break;

	case OPTION_WEAKEN:
	  weaken = TRUE;
	  break;

	case OPTION_ADD_SECTION:
	  add_sections = init_section_add (optarg, add_sections,
					   "--add-section");
	  section_add_load_file (add_sections);
	  break;

	case OPTION_UPDATE_SECTION:
	  update_sections = init_section_add (optarg, update_sections,
					      "--update-section");
	  section_add_load_file (update_sections);
	  break;

	case OPTION_DUMP_SECTION:
	  dump_sections = init_section_add (optarg, dump_sections,
					    "--dump-section");
	  break;

	case OPTION_ADD_SYMBOL:
	  {
	    char *s, *t;
	    struct addsym_node *newsym = xmalloc (sizeof *newsym);

	    newsym->next = NULL;
	    s = strchr (optarg, '=');
	    if (s == NULL)
	      fatal (_("bad format for %s"), "--add-symbol");
	    t = strchr (s + 1, ':');

	    newsym->symdef = xstrndup (optarg, s - optarg);
	    if (t)
	      {
		newsym->section = xstrndup (s + 1, t - (s + 1));
		newsym->symval = strtol (t + 1, NULL, 0);
	      }
	    else
	      {
		newsym->section = NULL;
		newsym->symval = strtol (s + 1, NULL, 0);
		t = s;
	      }

	    t = strchr (t + 1, ',');
	    newsym->othersym = NULL;
	    if (t)
	      newsym->flags = parse_symflags (t+1, &newsym->othersym);
	    else
	      newsym->flags = BSF_GLOBAL;

	    /* Keep 'othersym' symbols at the front of the list.  */
	    if (newsym->othersym)
	      {
		newsym->next = add_sym_list;
		if (!add_sym_list)
		  add_sym_tail = &newsym->next;
		add_sym_list = newsym;
	      }
	    else
	      {
		*add_sym_tail = newsym;
		add_sym_tail = &newsym->next;
	      }
	    add_symbols++;
	  }
	  break;

	case OPTION_CHANGE_START:
	  change_start = parse_vma (optarg, "--change-start");
	  break;

	case OPTION_CHANGE_SECTION_ADDRESS:
	case OPTION_CHANGE_SECTION_LMA:
	case OPTION_CHANGE_SECTION_VMA:
	  {
	    struct section_list * p;
	    unsigned int context = 0;
	    const char *s;
	    int len;
	    char *name;
	    char *option = NULL;
	    bfd_vma val;

	    switch (c)
	      {
	      case OPTION_CHANGE_SECTION_ADDRESS:
		option = "--change-section-address";
		context = SECTION_CONTEXT_ALTER_LMA | SECTION_CONTEXT_ALTER_VMA;
		break;
	      case OPTION_CHANGE_SECTION_LMA:
		option = "--change-section-lma";
		context = SECTION_CONTEXT_ALTER_LMA;
		break;
	      case OPTION_CHANGE_SECTION_VMA:
		option = "--change-section-vma";
		context = SECTION_CONTEXT_ALTER_VMA;
		break;
	      }

	    s = strchr (optarg, '=');
	    if (s == NULL)
	      {
		s = strchr (optarg, '+');
		if (s == NULL)
		  {
		    s = strchr (optarg, '-');
		    if (s == NULL)
		      fatal (_("bad format for %s"), option);
		  }
	      }
	    else
	      {
		/* Correct the context.  */
		switch (c)
		  {
		  case OPTION_CHANGE_SECTION_ADDRESS:
		    context = SECTION_CONTEXT_SET_LMA | SECTION_CONTEXT_SET_VMA;
		    break;
		  case OPTION_CHANGE_SECTION_LMA:
		    context = SECTION_CONTEXT_SET_LMA;
		    break;
		  case OPTION_CHANGE_SECTION_VMA:
		    context = SECTION_CONTEXT_SET_VMA;
		    break;
		  }
	      }

	    len = s - optarg;
	    name = (char *) xmalloc (len + 1);
	    strncpy (name, optarg, len);
	    name[len] = '\0';

	    p = find_section_list (name, TRUE, context);

	    val = parse_vma (s + 1, option);
	    if (*s == '-')
	      val = - val;

	    switch (c)
	      {
	      case OPTION_CHANGE_SECTION_ADDRESS:
		p->vma_val = val;
		/* Fall through.  */

	      case OPTION_CHANGE_SECTION_LMA:
		p->lma_val = val;
		break;

	      case OPTION_CHANGE_SECTION_VMA:
		p->vma_val = val;
		break;
	      }
	  }
	  break;

	case OPTION_CHANGE_ADDRESSES:
	  change_section_address = parse_vma (optarg, "--change-addresses");
	  change_start = change_section_address;
	  break;

	case OPTION_CHANGE_WARNINGS:
	  change_warn = TRUE;
	  break;

	case OPTION_CHANGE_LEADING_CHAR:
	  change_leading_char = TRUE;
	  break;

	case OPTION_COMPRESS_DEBUG_SECTIONS:
	  if (optarg)
	    {
	      if (strcasecmp (optarg, "none") == 0)
		do_debug_sections = decompress;
	      else if (strcasecmp (optarg, "zlib") == 0)
		do_debug_sections = compress_zlib;
	      else if (strcasecmp (optarg, "zlib-gnu") == 0)
		do_debug_sections = compress_gnu_zlib;
	      else if (strcasecmp (optarg, "zlib-gabi") == 0)
		do_debug_sections = compress_gabi_zlib;
	      else
		fatal (_("unrecognized --compress-debug-sections type `%s'"),
		       optarg);
	    }
	  else
	    do_debug_sections = compress;
	  break;

	case OPTION_DEBUGGING:
	  convert_debugging = TRUE;
	  break;

	case OPTION_DECOMPRESS_DEBUG_SECTIONS:
	  do_debug_sections = decompress;
	  break;

	case OPTION_ELF_STT_COMMON:
	  if (strcasecmp (optarg, "yes") == 0)
	    do_elf_stt_common = elf_stt_common;
	  else if (strcasecmp (optarg, "no") == 0)
	    do_elf_stt_common = no_elf_stt_common;
	  else
	    fatal (_("unrecognized --elf-stt-common= option `%s'"),
		   optarg);
	  break;

	case OPTION_GAP_FILL:
	  {
	    bfd_vma gap_fill_vma;

	    gap_fill_vma = parse_vma (optarg, "--gap-fill");
	    gap_fill = (bfd_byte) gap_fill_vma;
	    if ((bfd_vma) gap_fill != gap_fill_vma)
	      {
		char buff[20];

		sprintf_vma (buff, gap_fill_vma);

		non_fatal (_("Warning: truncating gap-fill from 0x%s to 0x%x"),
			   buff, gap_fill);
	      }
	    gap_fill_set = TRUE;
	  }
	  break;

	case OPTION_NO_CHANGE_WARNINGS:
	  change_warn = FALSE;
	  break;

	case OPTION_PAD_TO:
	  pad_to = parse_vma (optarg, "--pad-to");
	  pad_to_set = TRUE;
	  break;

	case OPTION_REMOVE_LEADING_CHAR:
	  remove_leading_char = TRUE;
	  break;

	case OPTION_REDEFINE_SYM:
	  {
	    /* Insert this redefinition onto redefine_specific_htab.  */

	    int len;
	    const char *s;
	    const char *nextarg;
	    char *source, *target;

	    s = strchr (optarg, '=');
	    if (s == NULL)
	      fatal (_("bad format for %s"), "--redefine-sym");

	    len = s - optarg;
	    source = (char *) xmalloc (len + 1);
	    strncpy (source, optarg, len);
	    source[len] = '\0';

	    nextarg = s + 1;
	    len = strlen (nextarg);
	    target = (char *) xmalloc (len + 1);
	    strcpy (target, nextarg);

	    add_redefine_and_check ("--redefine-sym", source, target);

	    free (source);
	    free (target);
	  }
	  break;

	case OPTION_REDEFINE_SYMS:
	  add_redefine_syms_file (optarg);
	  break;

	case OPTION_SET_SECTION_FLAGS:
	  {
	    struct section_list *p;
	    const char *s;
	    int len;
	    char *name;

	    s = strchr (optarg, '=');
	    if (s == NULL)
	      fatal (_("bad format for %s"), "--set-section-flags");

	    len = s - optarg;
	    name = (char *) xmalloc (len + 1);
	    strncpy (name, optarg, len);
	    name[len] = '\0';

	    p = find_section_list (name, TRUE, SECTION_CONTEXT_SET_FLAGS);

	    p->flags = parse_flags (s + 1);
	  }
	  break;

	case OPTION_RENAME_SECTION:
	  {
	    flagword flags;
	    const char *eq, *fl;
	    char *old_name;
	    char *new_name;
	    unsigned int len;

	    eq = strchr (optarg, '=');
	    if (eq == NULL)
	      fatal (_("bad format for %s"), "--rename-section");

	    len = eq - optarg;
	    if (len == 0)
	      fatal (_("bad format for %s"), "--rename-section");

	    old_name = (char *) xmalloc (len + 1);
	    strncpy (old_name, optarg, len);
	    old_name[len] = 0;

	    eq++;
	    fl = strchr (eq, ',');
	    if (fl)
	      {
		flags = parse_flags (fl + 1);
		len = fl - eq;
	      }
	    else
	      {
		flags = -1;
		len = strlen (eq);
	      }

	    if (len == 0)
	      fatal (_("bad format for %s"), "--rename-section");

	    new_name = (char *) xmalloc (len + 1);
	    strncpy (new_name, eq, len);
	    new_name[len] = 0;

	    add_section_rename (old_name, new_name, flags);
	  }
	  break;

	case OPTION_SET_START:
	  set_start = parse_vma (optarg, "--set-start");
	  set_start_set = TRUE;
	  break;

	case OPTION_SREC_LEN:
	  _bfd_srec_len = parse_vma (optarg, "--srec-len");
	  break;

	case OPTION_SREC_FORCES3:
	  _bfd_srec_forceS3 = TRUE;
	  break;

	case OPTION_STRIP_SYMBOLS:
	  add_specific_symbols (optarg, strip_specific_htab);
	  break;

	case OPTION_STRIP_UNNEEDED_SYMBOLS:
	  add_specific_symbols (optarg, strip_unneeded_htab);
	  break;

	case OPTION_KEEP_SYMBOLS:
	  add_specific_symbols (optarg, keep_specific_htab);
	  break;

	case OPTION_LOCALIZE_HIDDEN:
	  localize_hidden = TRUE;
	  break;

	case OPTION_LOCALIZE_SYMBOLS:
	  add_specific_symbols (optarg, localize_specific_htab);
	  break;

	case OPTION_LONG_SECTION_NAMES:
	  if (!strcmp ("enable", optarg))
	    long_section_names = ENABLE;
	  else if (!strcmp ("disable", optarg))
	    long_section_names = DISABLE;
	  else if (!strcmp ("keep", optarg))
	    long_section_names = KEEP;
	  else
	    fatal (_("unknown long section names option '%s'"), optarg);
	  break;

	case OPTION_GLOBALIZE_SYMBOLS:
	  add_specific_symbols (optarg, globalize_specific_htab);
	  break;

	case OPTION_KEEPGLOBAL_SYMBOLS:
	  add_specific_symbols (optarg, keepglobal_specific_htab);
	  break;

	case OPTION_WEAKEN_SYMBOLS:
	  add_specific_symbols (optarg, weaken_specific_htab);
	  break;

	case OPTION_ALT_MACH_CODE:
	  use_alt_mach_code = strtoul (optarg, NULL, 0);
	  if (use_alt_mach_code == 0)
	    fatal (_("unable to parse alternative machine code"));
	  break;

	case OPTION_PREFIX_SYMBOLS:
	  prefix_symbols_string = optarg;
	  break;

	case OPTION_PREFIX_SECTIONS:
	  prefix_sections_string = optarg;
	  break;

	case OPTION_PREFIX_ALLOC_SECTIONS:
	  prefix_alloc_sections_string = optarg;
	  break;

	case OPTION_READONLY_TEXT:
	  bfd_flags_to_set |= WP_TEXT;
	  bfd_flags_to_clear &= ~WP_TEXT;
	  break;

	case OPTION_WRITABLE_TEXT:
	  bfd_flags_to_clear |= WP_TEXT;
	  bfd_flags_to_set &= ~WP_TEXT;
	  break;

	case OPTION_PURE:
	  bfd_flags_to_set |= D_PAGED;
	  bfd_flags_to_clear &= ~D_PAGED;
	  break;

	case OPTION_IMPURE:
	  bfd_flags_to_clear |= D_PAGED;
	  bfd_flags_to_set &= ~D_PAGED;
	  break;

	case OPTION_EXTRACT_DWO:
	  strip_symbols = STRIP_NONDWO;
	  break;

	case OPTION_EXTRACT_SYMBOL:
	  extract_symbol = TRUE;
	  break;

	case OPTION_REVERSE_BYTES:
	  {
	    int prev = reverse_bytes;

	    reverse_bytes = atoi (optarg);
	    if ((reverse_bytes <= 0) || ((reverse_bytes % 2) != 0))
	      fatal (_("number of bytes to reverse must be positive and even"));

	    if (prev && prev != reverse_bytes)
	      non_fatal (_("Warning: ignoring previous --reverse-bytes value of %d"),
			 prev);
	    break;
	  }

	case OPTION_FILE_ALIGNMENT:
	  pe_file_alignment = parse_vma (optarg, "--file-alignment");
	  break;

	case OPTION_HEAP:
	  {
	    char *end;
	    pe_heap_reserve = strtoul (optarg, &end, 0);
	    if (end == optarg
		|| (*end != '.' && *end != '\0'))
	      non_fatal (_("%s: invalid reserve value for --heap"),
			 optarg);
	    else if (*end != '\0')
	      {
		pe_heap_commit = strtoul (end + 1, &end, 0);
		if (*end != '\0')
		  non_fatal (_("%s: invalid commit value for --heap"),
			     optarg);
	      }
	  }
	  break;

	case OPTION_IMAGE_BASE:
	  pe_image_base = parse_vma (optarg, "--image-base");
	  break;

	case OPTION_SECTION_ALIGNMENT:
	  pe_section_alignment = parse_vma (optarg,
					    "--section-alignment");
	  break;

	case OPTION_SUBSYSTEM:
	  set_pe_subsystem (optarg);
	  break;

	case OPTION_STACK:
	  {
	    char *end;
	    pe_stack_reserve = strtoul (optarg, &end, 0);
	    if (end == optarg
		|| (*end != '.' && *end != '\0'))
	      non_fatal (_("%s: invalid reserve value for --stack"),
			 optarg);
	    else if (*end != '\0')
	      {
		pe_stack_commit = strtoul (end + 1, &end, 0);
		if (*end != '\0')
		  non_fatal (_("%s: invalid commit value for --stack"),
			     optarg);
	      }
	  }
	  break;

	case 0:
	  /* We've been given a long option.  */
	  break;

	case 'H':
	case 'h':
	  copy_usage (stdout, 0);

	default:
	  copy_usage (stderr, 1);
	}
    }

  if (formats_info)
    {
      display_info ();
      return 0;
    }

  if (show_version)
    print_version ("objcopy");

  if (interleave && copy_byte == -1)
    fatal (_("interleave start byte must be set with --byte"));

  if (copy_byte >= interleave)
    fatal (_("byte number must be less than interleave"));

  if (copy_width > interleave - copy_byte)
    fatal (_("interleave width must be less than or equal to interleave - byte`"));

  if (optind == argc || optind + 2 < argc)
    copy_usage (stderr, 1);

  input_filename = argv[optind];
  if (optind + 1 < argc)
    output_filename = argv[optind + 1];

  default_deterministic ();

  /* Default is to strip no symbols.  */
  if (strip_symbols == STRIP_UNDEF && discard_locals == LOCALS_UNDEF)
    strip_symbols = STRIP_NONE;

  if (output_target == NULL)
    output_target = input_target;

  /* Convert input EFI target to PEI target.  */
  if (input_target != NULL
      && strncmp (input_target, "efi-", 4) == 0)
    {
      char *efi;

      efi = xstrdup (output_target + 4);
      if (strncmp (efi, "bsdrv-", 6) == 0
	  || strncmp (efi, "rtdrv-", 6) == 0)
	efi += 2;
      else if (strncmp (efi, "app-", 4) != 0)
	fatal (_("unknown input EFI target: %s"), input_target);

      input_target = efi;
      convert_efi_target (efi);
    }

  /* Convert output EFI target to PEI target.  */
  if (output_target != NULL
      && strncmp (output_target, "efi-", 4) == 0)
    {
      char *efi;

      efi = xstrdup (output_target + 4);
      if (strncmp (efi, "app-", 4) == 0)
	{
	  if (pe_subsystem == -1)
	    pe_subsystem = IMAGE_SUBSYSTEM_EFI_APPLICATION;
	}
      else if (strncmp (efi, "bsdrv-", 6) == 0)
	{
	  if (pe_subsystem == -1)
	    pe_subsystem = IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
	  efi += 2;
	}
      else if (strncmp (efi, "rtdrv-", 6) == 0)
	{
	  if (pe_subsystem == -1)
	    pe_subsystem = IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
	  efi += 2;
	}
      else
	fatal (_("unknown output EFI target: %s"), output_target);

      if (pe_file_alignment == (bfd_vma) -1)
	pe_file_alignment = PE_DEF_FILE_ALIGNMENT;
      if (pe_section_alignment == (bfd_vma) -1)
	pe_section_alignment = PE_DEF_SECTION_ALIGNMENT;

      output_target = efi;
      convert_efi_target (efi);
    }

  if (preserve_dates)
    if (stat (input_filename, & statbuf) < 0)
      fatal (_("warning: could not locate '%s'.  System error message: %s"),
	     input_filename, strerror (errno));

  /* If there is no destination file, or the source and destination files
     are the same, then create a temp and rename the result into the input.  */
  if (output_filename == NULL
      || filename_cmp (input_filename, output_filename) == 0)
    tmpname = make_tempname (input_filename);
  else
    tmpname = output_filename;

  if (tmpname == NULL)
    fatal (_("warning: could not create temporary file whilst copying '%s', (error: %s)"),
	   input_filename, strerror (errno));

  copy_file (input_filename, tmpname, input_target, output_target, input_arch);
  if (status == 0)
    {
      if (preserve_dates)
	set_times (tmpname, &statbuf);
      if (tmpname != output_filename)
	status = (smart_rename (tmpname, input_filename,
				preserve_dates) != 0);
    }
  else
    unlink_if_ordinary (tmpname);

  if (tmpname != output_filename)
    free (tmpname);

  if (change_warn)
    {
      struct section_list *p;

      for (p = change_sections; p != NULL; p = p->next)
	{
	  if (! p->used)
	    {
	      if (p->context & (SECTION_CONTEXT_SET_VMA | SECTION_CONTEXT_ALTER_VMA))
		{
		  char buff [20];

		  sprintf_vma (buff, p->vma_val);

		  /* xgettext:c-format */
		  non_fatal (_("%s %s%c0x%s never used"),
			     "--change-section-vma",
			     p->pattern,
			     p->context & SECTION_CONTEXT_SET_VMA ? '=' : '+',
			     buff);
		}

	      if (p->context & (SECTION_CONTEXT_SET_LMA | SECTION_CONTEXT_ALTER_LMA))
		{
		  char buff [20];

		  sprintf_vma (buff, p->lma_val);

		  /* xgettext:c-format */
		  non_fatal (_("%s %s%c0x%s never used"),
			     "--change-section-lma",
			     p->pattern,
			     p->context & SECTION_CONTEXT_SET_LMA ? '=' : '+',
			     buff);
		}
	    }
	}
    }

  return 0;
}

int
main (int argc, char *argv[])
{
#if defined (HAVE_SETLOCALE) && defined (HAVE_LC_MESSAGES)
  setlocale (LC_MESSAGES, "");
#endif
#if defined (HAVE_SETLOCALE)
  setlocale (LC_CTYPE, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  program_name = argv[0];
  xmalloc_set_program_name (program_name);

  START_PROGRESS (program_name, 0);

  expandargv (&argc, &argv);

  strip_symbols = STRIP_UNDEF;
  discard_locals = LOCALS_UNDEF;

  bfd_init ();
  set_default_bfd_target ();

  if (is_strip < 0)
    {
      int i = strlen (program_name);
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
      /* Drop the .exe suffix, if any.  */
      if (i > 4 && FILENAME_CMP (program_name + i - 4, ".exe") == 0)
	{
	  i -= 4;
	  program_name[i] = '\0';
	}
#endif
      is_strip = (i >= 5 && FILENAME_CMP (program_name + i - 5, "strip") == 0);
    }

  create_symbol_htabs ();

  if (argv != NULL)
    bfd_set_error_program_name (argv[0]);

  if (is_strip)
    strip_main (argc, argv);
  else
    copy_main (argc, argv);

  END_PROGRESS (program_name);

  return status;
}
