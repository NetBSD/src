/* objdump.c -- dump information about an object file.
   Copyright (C) 1990-2020 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */


/* Objdump overview.

   Objdump displays information about one or more object files, either on
   their own, or inside libraries.  It is commonly used as a disassembler,
   but it can also display information about file headers, symbol tables,
   relocations, debugging directives and more.

   The flow of execution is as follows:

   1. Command line arguments are checked for control switches and the
      information to be displayed is selected.

   2. Any remaining arguments are assumed to be object files, and they are
      processed in order by display_bfd().  If the file is an archive each
      of its elements is processed in turn.

   3. The file's target architecture and binary file format are determined
      by bfd_check_format().  If they are recognised, then dump_bfd() is
      called.

   4. dump_bfd() in turn calls separate functions to display the requested
      item(s) of information(s).  For example disassemble_data() is called if
      a disassembly has been requested.

   When disassembling the code loops through blocks of instructions bounded
   by symbols, calling disassemble_bytes() on each block.  The actual
   disassembling is done by the libopcodes library, via a function pointer
   supplied by the disassembler() function.  */

#include "sysdep.h"
#include "bfd.h"
#include "elf-bfd.h"
#include "coff-bfd.h"
#include "progress.h"
#include "bucomm.h"
#include "elfcomm.h"
#include "dwarf.h"
#include "ctf-api.h"
#include "getopt.h"
#include "safe-ctype.h"
#include "dis-asm.h"
#include "libiberty.h"
#include "demangle.h"
#include "filenames.h"
#include "debug.h"
#include "budbg.h"
#include "objdump.h"

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

/* Internal headers for the ELF .stab-dump code - sorry.  */
#define	BYTES_IN_WORD	32
#include "aout/aout64.h"

/* Exit status.  */
static int exit_status = 0;

static char *default_target = NULL;	/* Default at runtime.  */

/* The following variables are set based on arguments passed on the
   command line.  */
static int show_version = 0;		/* Show the version number.  */
static int dump_section_contents;	/* -s */
static int dump_section_headers;	/* -h */
static bfd_boolean dump_file_header;	/* -f */
static int dump_symtab;			/* -t */
static int dump_dynamic_symtab;		/* -T */
static int dump_reloc_info;		/* -r */
static int dump_dynamic_reloc_info;	/* -R */
static int dump_ar_hdrs;		/* -a */
static int dump_private_headers;	/* -p */
static char *dump_private_options;	/* -P */
static int prefix_addresses;		/* --prefix-addresses */
static int with_line_numbers;		/* -l */
static bfd_boolean with_source_code;	/* -S */
static int show_raw_insn;		/* --show-raw-insn */
static int dump_dwarf_section_info;	/* --dwarf */
static int dump_stab_section_info;	/* --stabs */
static int dump_ctf_section_info;       /* --ctf */
static char *dump_ctf_section_name;
static char *dump_ctf_parent_name;	/* --ctf-parent */
static int do_demangle;			/* -C, --demangle */
static bfd_boolean disassemble;		/* -d */
static bfd_boolean disassemble_all;	/* -D */
static int disassemble_zeroes;		/* --disassemble-zeroes */
static bfd_boolean formats_info;	/* -i */
static int wide_output;			/* -w */
static int insn_width;			/* --insn-width */
static bfd_vma start_address = (bfd_vma) -1; /* --start-address */
static bfd_vma stop_address = (bfd_vma) -1;  /* --stop-address */
static int dump_debugging;		/* --debugging */
static int dump_debugging_tags;		/* --debugging-tags */
static int suppress_bfd_header;
static int dump_special_syms = 0;	/* --special-syms */
static bfd_vma adjust_section_vma = 0;	/* --adjust-vma */
static int file_start_context = 0;      /* --file-start-context */
static bfd_boolean display_file_offsets;/* -F */
static const char *prefix;		/* --prefix */
static int prefix_strip;		/* --prefix-strip */
static size_t prefix_length;
static bfd_boolean unwind_inlines;	/* --inlines.  */
static const char * disasm_sym;		/* Disassembly start symbol.  */
static const char * source_comment;     /* --source_comment.  */
static bfd_boolean visualize_jumps = FALSE;          /* --visualize-jumps.  */
static bfd_boolean color_output = FALSE;             /* --visualize-jumps=color.  */
static bfd_boolean extended_color_output = FALSE;    /* --visualize-jumps=extended-color.  */

static int demangle_flags = DMGL_ANSI | DMGL_PARAMS;

/* A structure to record the sections mentioned in -j switches.  */
struct only
{
  const char * name; /* The name of the section.  */
  bfd_boolean  seen; /* A flag to indicate that the section has been found in one or more input files.  */
  struct only * next; /* Pointer to the next structure in the list.  */
};
/* Pointer to an array of 'only' structures.
   This pointer is NULL if the -j switch has not been used.  */
static struct only * only_list = NULL;

/* Variables for handling include file path table.  */
static const char **include_paths;
static int include_path_count;

/* Extra info to pass to the section disassembler and address printing
   function.  */
struct objdump_disasm_info
{
  bfd *              abfd;
  bfd_boolean        require_sec;
  arelent **         dynrelbuf;
  long               dynrelcount;
  disassembler_ftype disassemble_fn;
  arelent *          reloc;
  const char *       symbol;
};

/* Architecture to disassemble for, or default if NULL.  */
static char *machine = NULL;

/* Target specific options to the disassembler.  */
static char *disassembler_options = NULL;

/* Endianness to disassemble for, or default if BFD_ENDIAN_UNKNOWN.  */
static enum bfd_endian endian = BFD_ENDIAN_UNKNOWN;

/* The symbol table.  */
static asymbol **syms;

/* Number of symbols in `syms'.  */
static long symcount = 0;

/* The sorted symbol table.  */
static asymbol **sorted_syms;

/* Number of symbols in `sorted_syms'.  */
static long sorted_symcount = 0;

/* The dynamic symbol table.  */
static asymbol **dynsyms;

/* The synthetic symbol table.  */
static asymbol *synthsyms;
static long synthcount = 0;

/* Number of symbols in `dynsyms'.  */
static long dynsymcount = 0;

static bfd_byte *stabs;
static bfd_size_type stab_size;

static bfd_byte *strtab;
static bfd_size_type stabstr_size;

/* Handlers for -P/--private.  */
static const struct objdump_private_desc * const objdump_private_vectors[] =
  {
    OBJDUMP_PRIVATE_VECTORS
    NULL
  };

/* The list of detected jumps inside a function.  */
static struct jump_info *detected_jumps = NULL;

static void usage (FILE *, int) ATTRIBUTE_NORETURN;
static void
usage (FILE *stream, int status)
{
  fprintf (stream, _("Usage: %s <option(s)> <file(s)>\n"), program_name);
  fprintf (stream, _(" Display information from object <file(s)>.\n"));
  fprintf (stream, _(" At least one of the following switches must be given:\n"));
  fprintf (stream, _("\
  -a, --archive-headers    Display archive header information\n\
  -f, --file-headers       Display the contents of the overall file header\n\
  -p, --private-headers    Display object format specific file header contents\n\
  -P, --private=OPT,OPT... Display object format specific contents\n\
  -h, --[section-]headers  Display the contents of the section headers\n\
  -x, --all-headers        Display the contents of all headers\n\
  -d, --disassemble        Display assembler contents of executable sections\n\
  -D, --disassemble-all    Display assembler contents of all sections\n\
      --disassemble=<sym>  Display assembler contents from <sym>\n\
  -S, --source             Intermix source code with disassembly\n\
      --source-comment[=<txt>] Prefix lines of source code with <txt>\n\
  -s, --full-contents      Display the full contents of all sections requested\n\
  -g, --debugging          Display debug information in object file\n\
  -e, --debugging-tags     Display debug information using ctags style\n\
  -G, --stabs              Display (in raw form) any STABS info in the file\n\
  -W[lLiaprmfFsoRtUuTgAckK] or\n\
  --dwarf[=rawline,=decodedline,=info,=abbrev,=pubnames,=aranges,=macro,=frames,\n\
          =frames-interp,=str,=loc,=Ranges,=pubtypes,\n\
          =gdb_index,=trace_info,=trace_abbrev,=trace_aranges,\n\
          =addr,=cu_index,=links,=follow-links]\n\
                           Display DWARF info in the file\n\
  --ctf=SECTION            Display CTF info from SECTION\n\
  -t, --syms               Display the contents of the symbol table(s)\n\
  -T, --dynamic-syms       Display the contents of the dynamic symbol table\n\
  -r, --reloc              Display the relocation entries in the file\n\
  -R, --dynamic-reloc      Display the dynamic relocation entries in the file\n\
  @<file>                  Read options from <file>\n\
  -v, --version            Display this program's version number\n\
  -i, --info               List object formats and architectures supported\n\
  -H, --help               Display this information\n\
"));
  if (status != 2)
    {
      const struct objdump_private_desc * const *desc;

      fprintf (stream, _("\n The following switches are optional:\n"));
      fprintf (stream, _("\
  -b, --target=BFDNAME           Specify the target object format as BFDNAME\n\
  -m, --architecture=MACHINE     Specify the target architecture as MACHINE\n\
  -j, --section=NAME             Only display information for section NAME\n\
  -M, --disassembler-options=OPT Pass text OPT on to the disassembler\n\
  -EB --endian=big               Assume big endian format when disassembling\n\
  -EL --endian=little            Assume little endian format when disassembling\n\
      --file-start-context       Include context from start of file (with -S)\n\
  -I, --include=DIR              Add DIR to search list for source files\n\
  -l, --line-numbers             Include line numbers and filenames in output\n\
  -F, --file-offsets             Include file offsets when displaying information\n\
  -C, --demangle[=STYLE]         Decode mangled/processed symbol names\n\
                                  The STYLE, if specified, can be `auto', `gnu',\n\
                                  `lucid', `arm', `hp', `edg', `gnu-v3', `java'\n\
                                  or `gnat'\n\
      --recurse-limit            Enable a limit on recursion whilst demangling.  [Default]\n\
      --no-recurse-limit         Disable a limit on recursion whilst demangling\n\
  -w, --wide                     Format output for more than 80 columns\n\
  -z, --disassemble-zeroes       Do not skip blocks of zeroes when disassembling\n\
      --start-address=ADDR       Only process data whose address is >= ADDR\n\
      --stop-address=ADDR        Only process data whose address is < ADDR\n\
      --prefix-addresses         Print complete address alongside disassembly\n\
      --[no-]show-raw-insn       Display hex alongside symbolic disassembly\n\
      --insn-width=WIDTH         Display WIDTH bytes on a single line for -d\n\
      --adjust-vma=OFFSET        Add OFFSET to all displayed section addresses\n\
      --special-syms             Include special symbols in symbol dumps\n\
      --inlines                  Print all inlines for source line (with -l)\n\
      --prefix=PREFIX            Add PREFIX to absolute paths for -S\n\
      --prefix-strip=LEVEL       Strip initial directory names for -S\n"));
      fprintf (stream, _("\
      --dwarf-depth=N        Do not display DIEs at depth N or greater\n\
      --dwarf-start=N        Display DIEs starting with N, at the same depth\n\
                             or deeper\n\
      --dwarf-check          Make additional dwarf internal consistency checks.\
      \n\
      --ctf-parent=SECTION       Use SECTION as the CTF parent\n\
      --visualize-jumps          Visualize jumps by drawing ASCII art lines\n\
      --visualize-jumps=color    Use colors in the ASCII art\n\
      --visualize-jumps=extended-color   Use extended 8-bit color codes\n\
      --visualize-jumps=off      Disable jump visualization\n\n"));

      list_supported_targets (program_name, stream);
      list_supported_architectures (program_name, stream);

      disassembler_usage (stream);

      if (objdump_private_vectors[0] != NULL)
        {
          fprintf (stream,
                   _("\nOptions supported for -P/--private switch:\n"));
          for (desc = objdump_private_vectors; *desc != NULL; desc++)
            (*desc)->help (stream);
        }
    }
  if (REPORT_BUGS_TO[0] && status == 0)
    fprintf (stream, _("Report bugs to %s.\n"), REPORT_BUGS_TO);
  exit (status);
}

/* 150 isn't special; it's just an arbitrary non-ASCII char value.  */
enum option_values
  {
    OPTION_ENDIAN=150,
    OPTION_START_ADDRESS,
    OPTION_STOP_ADDRESS,
    OPTION_DWARF,
    OPTION_PREFIX,
    OPTION_PREFIX_STRIP,
    OPTION_INSN_WIDTH,
    OPTION_ADJUST_VMA,
    OPTION_DWARF_DEPTH,
    OPTION_DWARF_CHECK,
    OPTION_DWARF_START,
    OPTION_RECURSE_LIMIT,
    OPTION_NO_RECURSE_LIMIT,
    OPTION_INLINES,
    OPTION_SOURCE_COMMENT,
    OPTION_CTF,
    OPTION_CTF_PARENT,
    OPTION_VISUALIZE_JUMPS
  };

static struct option long_options[]=
{
  {"adjust-vma", required_argument, NULL, OPTION_ADJUST_VMA},
  {"all-headers", no_argument, NULL, 'x'},
  {"private-headers", no_argument, NULL, 'p'},
  {"private", required_argument, NULL, 'P'},
  {"architecture", required_argument, NULL, 'm'},
  {"archive-headers", no_argument, NULL, 'a'},
  {"debugging", no_argument, NULL, 'g'},
  {"debugging-tags", no_argument, NULL, 'e'},
  {"demangle", optional_argument, NULL, 'C'},
  {"disassemble", optional_argument, NULL, 'd'},
  {"disassemble-all", no_argument, NULL, 'D'},
  {"disassembler-options", required_argument, NULL, 'M'},
  {"disassemble-zeroes", no_argument, NULL, 'z'},
  {"dynamic-reloc", no_argument, NULL, 'R'},
  {"dynamic-syms", no_argument, NULL, 'T'},
  {"endian", required_argument, NULL, OPTION_ENDIAN},
  {"file-headers", no_argument, NULL, 'f'},
  {"file-offsets", no_argument, NULL, 'F'},
  {"file-start-context", no_argument, &file_start_context, 1},
  {"full-contents", no_argument, NULL, 's'},
  {"headers", no_argument, NULL, 'h'},
  {"help", no_argument, NULL, 'H'},
  {"info", no_argument, NULL, 'i'},
  {"line-numbers", no_argument, NULL, 'l'},
  {"no-show-raw-insn", no_argument, &show_raw_insn, -1},
  {"prefix-addresses", no_argument, &prefix_addresses, 1},
  {"recurse-limit", no_argument, NULL, OPTION_RECURSE_LIMIT},
  {"recursion-limit", no_argument, NULL, OPTION_RECURSE_LIMIT},
  {"no-recurse-limit", no_argument, NULL, OPTION_NO_RECURSE_LIMIT},
  {"no-recursion-limit", no_argument, NULL, OPTION_NO_RECURSE_LIMIT},
  {"reloc", no_argument, NULL, 'r'},
  {"section", required_argument, NULL, 'j'},
  {"section-headers", no_argument, NULL, 'h'},
  {"show-raw-insn", no_argument, &show_raw_insn, 1},
  {"source", no_argument, NULL, 'S'},
  {"source-comment", optional_argument, NULL, OPTION_SOURCE_COMMENT},
  {"special-syms", no_argument, &dump_special_syms, 1},
  {"include", required_argument, NULL, 'I'},
  {"dwarf", optional_argument, NULL, OPTION_DWARF},
  {"ctf", required_argument, NULL, OPTION_CTF},
  {"ctf-parent", required_argument, NULL, OPTION_CTF_PARENT},
  {"stabs", no_argument, NULL, 'G'},
  {"start-address", required_argument, NULL, OPTION_START_ADDRESS},
  {"stop-address", required_argument, NULL, OPTION_STOP_ADDRESS},
  {"syms", no_argument, NULL, 't'},
  {"target", required_argument, NULL, 'b'},
  {"version", no_argument, NULL, 'V'},
  {"wide", no_argument, NULL, 'w'},
  {"prefix", required_argument, NULL, OPTION_PREFIX},
  {"prefix-strip", required_argument, NULL, OPTION_PREFIX_STRIP},
  {"insn-width", required_argument, NULL, OPTION_INSN_WIDTH},
  {"dwarf-depth", required_argument, 0, OPTION_DWARF_DEPTH},
  {"dwarf-start", required_argument, 0, OPTION_DWARF_START},
  {"dwarf-check", no_argument, 0, OPTION_DWARF_CHECK},
  {"inlines", no_argument, 0, OPTION_INLINES},
  {"visualize-jumps", optional_argument, 0, OPTION_VISUALIZE_JUMPS},
  {0, no_argument, 0, 0}
};

static void
nonfatal (const char *msg)
{
  bfd_nonfatal (msg);
  exit_status = 1;
}

/* Returns a version of IN with any control characters
   replaced by escape sequences.  Uses a static buffer
   if necessary.  */

static const char *
sanitize_string (const char * in)
{
  static char *  buffer = NULL;
  static size_t  buffer_len = 0;
  const char *   original = in;
  char *         out;

  /* Paranoia.  */
  if (in == NULL)
    return "";

  /* See if any conversion is necessary.  In the majority
     of cases it will not be needed.  */
  do
    {
      char c = *in++;

      if (c == 0)
	return original;

      if (ISCNTRL (c))
	break;
    }
  while (1);

  /* Copy the input, translating as needed.  */
  in = original;
  if (buffer_len < (strlen (in) * 2))
    {
      free ((void *) buffer);
      buffer_len = strlen (in) * 2;
      buffer = xmalloc (buffer_len + 1);
    }

  out = buffer;
  do
    {
      char c = *in++;

      if (c == 0)
	break;

      if (!ISCNTRL (c))
	*out++ = c;
      else
	{
	  *out++ = '^';
	  *out++ = c + 0x40;
	}
    }
  while (1);

  *out = 0;
  return buffer;
}


/* Returns TRUE if the specified section should be dumped.  */

static bfd_boolean
process_section_p (asection * section)
{
  struct only * only;

  if (only_list == NULL)
    return TRUE;

  for (only = only_list; only; only = only->next)
    if (strcmp (only->name, section->name) == 0)
      {
	only->seen = TRUE;
	return TRUE;
      }

  return FALSE;
}

/* Add an entry to the 'only' list.  */

static void
add_only (char * name)
{
  struct only * only;

  /* First check to make sure that we do not
     already have an entry for this name.  */
  for (only = only_list; only; only = only->next)
    if (strcmp (only->name, name) == 0)
      return;

  only = xmalloc (sizeof * only);
  only->name = name;
  only->seen = FALSE;
  only->next = only_list;
  only_list = only;
}

/* Release the memory used by the 'only' list.
   PR 11225: Issue a warning message for unseen sections.
   Only do this if none of the sections were seen.  This is mainly to support
   tools like the GAS testsuite where an object file is dumped with a list of
   generic section names known to be present in a range of different file
   formats.  */

static void
free_only_list (void)
{
  bfd_boolean at_least_one_seen = FALSE;
  struct only * only;
  struct only * next;

  if (only_list == NULL)
    return;

  for (only = only_list; only; only = only->next)
    if (only->seen)
      {
	at_least_one_seen = TRUE;
	break;
      }

  for (only = only_list; only; only = next)
    {
      if (! at_least_one_seen)
	{
	  non_fatal (_("section '%s' mentioned in a -j option, "
		       "but not found in any input file"),
		     only->name);
	  exit_status = 1;
	}
      next = only->next;
      free (only);
    }
}


static void
dump_section_header (bfd *abfd, asection *section, void *data)
{
  char *comma = "";
  unsigned int opb = bfd_octets_per_byte (abfd, section);
  int longest_section_name = *((int *) data);

  /* Ignore linker created section.  See elfNN_ia64_object_p in
     bfd/elfxx-ia64.c.  */
  if (section->flags & SEC_LINKER_CREATED)
    return;

  /* PR 10413: Skip sections that we are ignoring.  */
  if (! process_section_p (section))
    return;

  printf ("%3d %-*s %08lx  ", section->index, longest_section_name,
	  sanitize_string (bfd_section_name (section)),
	  (unsigned long) bfd_section_size (section) / opb);
  bfd_printf_vma (abfd, bfd_section_vma (section));
  printf ("  ");
  bfd_printf_vma (abfd, section->lma);
  printf ("  %08lx  2**%u", (unsigned long) section->filepos,
	  bfd_section_alignment (section));
  if (! wide_output)
    printf ("\n                ");
  printf ("  ");

#define PF(x, y) \
  if (section->flags & x) { printf ("%s%s", comma, y); comma = ", "; }

  PF (SEC_HAS_CONTENTS, "CONTENTS");
  PF (SEC_ALLOC, "ALLOC");
  PF (SEC_CONSTRUCTOR, "CONSTRUCTOR");
  PF (SEC_LOAD, "LOAD");
  PF (SEC_RELOC, "RELOC");
  PF (SEC_READONLY, "READONLY");
  PF (SEC_CODE, "CODE");
  PF (SEC_DATA, "DATA");
  PF (SEC_ROM, "ROM");
  PF (SEC_DEBUGGING, "DEBUGGING");
  PF (SEC_NEVER_LOAD, "NEVER_LOAD");
  PF (SEC_EXCLUDE, "EXCLUDE");
  PF (SEC_SORT_ENTRIES, "SORT_ENTRIES");
  if (bfd_get_arch (abfd) == bfd_arch_tic54x)
    {
      PF (SEC_TIC54X_BLOCK, "BLOCK");
      PF (SEC_TIC54X_CLINK, "CLINK");
    }
  PF (SEC_SMALL_DATA, "SMALL_DATA");
  if (bfd_get_flavour (abfd) == bfd_target_coff_flavour)
    {
      PF (SEC_COFF_SHARED, "SHARED");
      PF (SEC_COFF_NOREAD, "NOREAD");
    }
  else if (bfd_get_flavour (abfd) == bfd_target_elf_flavour)
    {
      PF (SEC_ELF_OCTETS, "OCTETS");
      PF (SEC_ELF_PURECODE, "PURECODE");
    }
  PF (SEC_THREAD_LOCAL, "THREAD_LOCAL");
  PF (SEC_GROUP, "GROUP");
  if (bfd_get_arch (abfd) == bfd_arch_mep)
    {
      PF (SEC_MEP_VLIW, "VLIW");
    }

  if ((section->flags & SEC_LINK_ONCE) != 0)
    {
      const char *ls;
      struct coff_comdat_info *comdat;

      switch (section->flags & SEC_LINK_DUPLICATES)
	{
	default:
	  abort ();
	case SEC_LINK_DUPLICATES_DISCARD:
	  ls = "LINK_ONCE_DISCARD";
	  break;
	case SEC_LINK_DUPLICATES_ONE_ONLY:
	  ls = "LINK_ONCE_ONE_ONLY";
	  break;
	case SEC_LINK_DUPLICATES_SAME_SIZE:
	  ls = "LINK_ONCE_SAME_SIZE";
	  break;
	case SEC_LINK_DUPLICATES_SAME_CONTENTS:
	  ls = "LINK_ONCE_SAME_CONTENTS";
	  break;
	}
      printf ("%s%s", comma, ls);

      comdat = bfd_coff_get_comdat_section (abfd, section);
      if (comdat != NULL)
	printf (" (COMDAT %s %ld)", comdat->name, comdat->symbol);

      comma = ", ";
    }

  printf ("\n");
#undef PF
}

/* Called on each SECTION in ABFD, update the int variable pointed to by
   DATA which contains the string length of the longest section name.  */

static void
find_longest_section_name (bfd *abfd ATTRIBUTE_UNUSED,
			   asection *section, void *data)
{
  int *longest_so_far = (int *) data;
  const char *name;
  int len;

  /* Ignore linker created section.  */
  if (section->flags & SEC_LINKER_CREATED)
    return;

  /* Skip sections that we are ignoring.  */
  if (! process_section_p (section))
    return;

  name = bfd_section_name (section);
  len = (int) strlen (name);
  if (len > *longest_so_far)
    *longest_so_far = len;
}

static void
dump_headers (bfd *abfd)
{
  /* The default width of 13 is just an arbitrary choice.  */
  int max_section_name_length = 13;
  int bfd_vma_width;

#ifndef BFD64
  bfd_vma_width = 10;
#else
  /* With BFD64, non-ELF returns -1 and wants always 64 bit addresses.  */
  if (bfd_get_arch_size (abfd) == 32)
    bfd_vma_width = 10;
  else
    bfd_vma_width = 18;
#endif

  printf (_("Sections:\n"));

  if (wide_output)
    bfd_map_over_sections (abfd, find_longest_section_name,
                           &max_section_name_length);

  printf (_("Idx %-*s Size      %-*s%-*sFile off  Algn"),
	  max_section_name_length, "Name",
	  bfd_vma_width, "VMA",
	  bfd_vma_width, "LMA");

  if (wide_output)
    printf (_("  Flags"));
  printf ("\n");

  bfd_map_over_sections (abfd, dump_section_header,
                         &max_section_name_length);
}

static asymbol **
slurp_symtab (bfd *abfd)
{
  asymbol **sy = NULL;
  long storage;

  if (!(bfd_get_file_flags (abfd) & HAS_SYMS))
    {
      symcount = 0;
      return NULL;
    }

  storage = bfd_get_symtab_upper_bound (abfd);
  if (storage < 0)
    {
      non_fatal (_("failed to read symbol table from: %s"), bfd_get_filename (abfd));
      bfd_fatal (_("error message was"));
    }
  if (storage)
    {
      off_t filesize = bfd_get_file_size (abfd);

      /* qv PR 24707.  */
      if (filesize > 0
	  && filesize < storage
	  /* The MMO file format supports its own special compression
	     technique, so its sections can be larger than the file size.  */
	  && bfd_get_flavour (abfd) != bfd_target_mmo_flavour)	  
	{
	  bfd_nonfatal_message (bfd_get_filename (abfd), abfd, NULL,
				_("error: symbol table size (%#lx) is larger than filesize (%#lx)"),
			storage, (long) filesize);
	  exit_status = 1;
	  symcount = 0;
	  return NULL;
	}

      sy = (asymbol **) xmalloc (storage);
    }

  symcount = bfd_canonicalize_symtab (abfd, sy);
  if (symcount < 0)
    bfd_fatal (bfd_get_filename (abfd));
  return sy;
}

/* Read in the dynamic symbols.  */

static asymbol **
slurp_dynamic_symtab (bfd *abfd)
{
  asymbol **sy = NULL;
  long storage;

  storage = bfd_get_dynamic_symtab_upper_bound (abfd);
  if (storage < 0)
    {
      if (!(bfd_get_file_flags (abfd) & DYNAMIC))
	{
	  non_fatal (_("%s: not a dynamic object"), bfd_get_filename (abfd));
	  exit_status = 1;
	  dynsymcount = 0;
	  return NULL;
	}

      bfd_fatal (bfd_get_filename (abfd));
    }
  if (storage)
    sy = (asymbol **) xmalloc (storage);

  dynsymcount = bfd_canonicalize_dynamic_symtab (abfd, sy);
  if (dynsymcount < 0)
    bfd_fatal (bfd_get_filename (abfd));
  return sy;
}

/* Some symbol names are significant and should be kept in the
   table of sorted symbol names, even if they are marked as
   debugging/section symbols.  */

static bfd_boolean
is_significant_symbol_name (const char * name)
{
  return strncmp (name, ".plt", 4) == 0 || strcmp (name, ".got") == 0;
}

/* Filter out (in place) symbols that are useless for disassembly.
   COUNT is the number of elements in SYMBOLS.
   Return the number of useful symbols.  */

static long
remove_useless_symbols (asymbol **symbols, long count)
{
  asymbol **in_ptr = symbols, **out_ptr = symbols;

  while (--count >= 0)
    {
      asymbol *sym = *in_ptr++;

      if (sym->name == NULL || sym->name[0] == '\0')
	continue;
      if ((sym->flags & (BSF_DEBUGGING | BSF_SECTION_SYM))
	  && ! is_significant_symbol_name (sym->name))
	continue;
      if (bfd_is_und_section (sym->section)
	  || bfd_is_com_section (sym->section))
	continue;

      *out_ptr++ = sym;
    }
  return out_ptr - symbols;
}

static const asection *compare_section;

/* Sort symbols into value order.  */

static int
compare_symbols (const void *ap, const void *bp)
{
  const asymbol *a = * (const asymbol **) ap;
  const asymbol *b = * (const asymbol **) bp;
  const char *an;
  const char *bn;
  size_t anl;
  size_t bnl;
  bfd_boolean as, af, bs, bf;
  flagword aflags;
  flagword bflags;

  if (bfd_asymbol_value (a) > bfd_asymbol_value (b))
    return 1;
  else if (bfd_asymbol_value (a) < bfd_asymbol_value (b))
    return -1;

  /* Prefer symbols from the section currently being disassembled.
     Don't sort symbols from other sections by section, since there
     isn't much reason to prefer one section over another otherwise.
     See sym_ok comment for why we compare by section name.  */
  as = strcmp (compare_section->name, a->section->name) == 0;
  bs = strcmp (compare_section->name, b->section->name) == 0;
  if (as && !bs)
    return -1;
  if (!as && bs)
    return 1;

  an = bfd_asymbol_name (a);
  bn = bfd_asymbol_name (b);
  anl = strlen (an);
  bnl = strlen (bn);

  /* The symbols gnu_compiled and gcc2_compiled convey no real
     information, so put them after other symbols with the same value.  */
  af = (strstr (an, "gnu_compiled") != NULL
	|| strstr (an, "gcc2_compiled") != NULL);
  bf = (strstr (bn, "gnu_compiled") != NULL
	|| strstr (bn, "gcc2_compiled") != NULL);

  if (af && ! bf)
    return 1;
  if (! af && bf)
    return -1;

  /* We use a heuristic for the file name, to try to sort it after
     more useful symbols.  It may not work on non Unix systems, but it
     doesn't really matter; the only difference is precisely which
     symbol names get printed.  */

#define file_symbol(s, sn, snl)			\
  (((s)->flags & BSF_FILE) != 0			\
   || ((snl) > 2				\
       && (sn)[(snl) - 2] == '.'		\
       && ((sn)[(snl) - 1] == 'o'		\
	   || (sn)[(snl) - 1] == 'a')))

  af = file_symbol (a, an, anl);
  bf = file_symbol (b, bn, bnl);

  if (af && ! bf)
    return 1;
  if (! af && bf)
    return -1;

  /* Sort function and object symbols before global symbols before
     local symbols before section symbols before debugging symbols.  */

  aflags = a->flags;
  bflags = b->flags;

  if ((aflags & BSF_DEBUGGING) != (bflags & BSF_DEBUGGING))
    {
      if ((aflags & BSF_DEBUGGING) != 0)
	return 1;
      else
	return -1;
    }
  if ((aflags & BSF_SECTION_SYM) != (bflags & BSF_SECTION_SYM))
    {
      if ((aflags & BSF_SECTION_SYM) != 0)
	return 1;
      else
	return -1;
    }
  if ((aflags & BSF_FUNCTION) != (bflags & BSF_FUNCTION))
    {
      if ((aflags & BSF_FUNCTION) != 0)
	return -1;
      else
	return 1;
    }
  if ((aflags & BSF_OBJECT) != (bflags & BSF_OBJECT))
    {
      if ((aflags & BSF_OBJECT) != 0)
	return -1;
      else
	return 1;
    }
  if ((aflags & BSF_LOCAL) != (bflags & BSF_LOCAL))
    {
      if ((aflags & BSF_LOCAL) != 0)
	return 1;
      else
	return -1;
    }
  if ((aflags & BSF_GLOBAL) != (bflags & BSF_GLOBAL))
    {
      if ((aflags & BSF_GLOBAL) != 0)
	return -1;
      else
	return 1;
    }

  if (bfd_get_flavour (bfd_asymbol_bfd (a)) == bfd_target_elf_flavour
      && bfd_get_flavour (bfd_asymbol_bfd (b)) == bfd_target_elf_flavour)
    {
      bfd_vma asz, bsz;

      asz = 0;
      if ((a->flags & (BSF_SECTION_SYM | BSF_SYNTHETIC)) == 0)
	asz = ((elf_symbol_type *) a)->internal_elf_sym.st_size;
      bsz = 0;
      if ((b->flags & (BSF_SECTION_SYM | BSF_SYNTHETIC)) == 0)
	bsz = ((elf_symbol_type *) b)->internal_elf_sym.st_size;
      if (asz != bsz)
	return asz > bsz ? -1 : 1;
    }

  /* Symbols that start with '.' might be section names, so sort them
     after symbols that don't start with '.'.  */
  if (an[0] == '.' && bn[0] != '.')
    return 1;
  if (an[0] != '.' && bn[0] == '.')
    return -1;

  /* Finally, if we can't distinguish them in any other way, try to
     get consistent results by sorting the symbols by name.  */
  return strcmp (an, bn);
}

/* Sort relocs into address order.  */

static int
compare_relocs (const void *ap, const void *bp)
{
  const arelent *a = * (const arelent **) ap;
  const arelent *b = * (const arelent **) bp;

  if (a->address > b->address)
    return 1;
  else if (a->address < b->address)
    return -1;

  /* So that associated relocations tied to the same address show up
     in the correct order, we don't do any further sorting.  */
  if (a > b)
    return 1;
  else if (a < b)
    return -1;
  else
    return 0;
}

/* Print an address (VMA) to the output stream in INFO.
   If SKIP_ZEROES is TRUE, omit leading zeroes.  */

static void
objdump_print_value (bfd_vma vma, struct disassemble_info *inf,
		     bfd_boolean skip_zeroes)
{
  char buf[30];
  char *p;
  struct objdump_disasm_info *aux;

  aux = (struct objdump_disasm_info *) inf->application_data;
  bfd_sprintf_vma (aux->abfd, buf, vma);
  if (! skip_zeroes)
    p = buf;
  else
    {
      for (p = buf; *p == '0'; ++p)
	;
      if (*p == '\0')
	--p;
    }
  (*inf->fprintf_func) (inf->stream, "%s", p);
}

/* Print the name of a symbol.  */

static void
objdump_print_symname (bfd *abfd, struct disassemble_info *inf,
		       asymbol *sym)
{
  char *alloc;
  const char *name, *version_string = NULL;
  bfd_boolean hidden = FALSE;

  alloc = NULL;
  name = bfd_asymbol_name (sym);
  if (do_demangle && name[0] != '\0')
    {
      /* Demangle the name.  */
      alloc = bfd_demangle (abfd, name, demangle_flags);
      if (alloc != NULL)
	name = alloc;
    }

  if ((sym->flags & (BSF_SECTION_SYM | BSF_SYNTHETIC)) == 0)
    version_string = bfd_get_symbol_version_string (abfd, sym, &hidden);

  if (bfd_is_und_section (bfd_asymbol_section (sym)))
    hidden = TRUE;

  name = sanitize_string (name);

  if (inf != NULL)
    {
      (*inf->fprintf_func) (inf->stream, "%s", name);
      if (version_string && *version_string != '\0')
	(*inf->fprintf_func) (inf->stream, hidden ? "@%s" : "@@%s",
			      version_string);
    }
  else
    {
      printf ("%s", name);
      if (version_string && *version_string != '\0')
	printf (hidden ? "@%s" : "@@%s", version_string);
    }

  if (alloc != NULL)
    free (alloc);
}

static inline bfd_boolean
sym_ok (bfd_boolean               want_section,
	bfd *                     abfd ATTRIBUTE_UNUSED,
	long                      place,
	asection *                sec,
	struct disassemble_info * inf)
{
  if (want_section)
    {
      /* Note - we cannot just compare section pointers because they could
	 be different, but the same...  Ie the symbol that we are trying to
	 find could have come from a separate debug info file.  Under such
	 circumstances the symbol will be associated with a section in the
	 debug info file, whilst the section we want is in a normal file.
	 So the section pointers will be different, but the section names
	 will be the same.  */
      if (strcmp (bfd_section_name (sorted_syms[place]->section),
		  bfd_section_name (sec)) != 0)
	return FALSE;
    }

  return inf->symbol_is_valid (sorted_syms[place], inf);
}

/* Locate a symbol given a bfd and a section (from INFO->application_data),
   and a VMA.  If INFO->application_data->require_sec is TRUE, then always
   require the symbol to be in the section.  Returns NULL if there is no
   suitable symbol.  If PLACE is not NULL, then *PLACE is set to the index
   of the symbol in sorted_syms.  */

static asymbol *
find_symbol_for_address (bfd_vma vma,
			 struct disassemble_info *inf,
			 long *place)
{
  /* @@ Would it speed things up to cache the last two symbols returned,
     and maybe their address ranges?  For many processors, only one memory
     operand can be present at a time, so the 2-entry cache wouldn't be
     constantly churned by code doing heavy memory accesses.  */

  /* Indices in `sorted_syms'.  */
  long min = 0;
  long max_count = sorted_symcount;
  long thisplace;
  struct objdump_disasm_info *aux;
  bfd *abfd;
  asection *sec;
  unsigned int opb;
  bfd_boolean want_section;
  long rel_count;

  if (sorted_symcount < 1)
    return NULL;

  aux = (struct objdump_disasm_info *) inf->application_data;
  abfd = aux->abfd;
  sec = inf->section;
  opb = inf->octets_per_byte;

  /* Perform a binary search looking for the closest symbol to the
     required value.  We are searching the range (min, max_count].  */
  while (min + 1 < max_count)
    {
      asymbol *sym;

      thisplace = (max_count + min) / 2;
      sym = sorted_syms[thisplace];

      if (bfd_asymbol_value (sym) > vma)
	max_count = thisplace;
      else if (bfd_asymbol_value (sym) < vma)
	min = thisplace;
      else
	{
	  min = thisplace;
	  break;
	}
    }

  /* The symbol we want is now in min, the low end of the range we
     were searching.  If there are several symbols with the same
     value, we want the first one.  */
  thisplace = min;
  while (thisplace > 0
	 && (bfd_asymbol_value (sorted_syms[thisplace])
	     == bfd_asymbol_value (sorted_syms[thisplace - 1])))
    --thisplace;

  /* Prefer a symbol in the current section if we have multple symbols
     with the same value, as can occur with overlays or zero size
     sections.  */
  min = thisplace;
  while (min < max_count
	 && (bfd_asymbol_value (sorted_syms[min])
	     == bfd_asymbol_value (sorted_syms[thisplace])))
    {
      if (sym_ok (TRUE, abfd, min, sec, inf))
	{
	  thisplace = min;

	  if (place != NULL)
	    *place = thisplace;

	  return sorted_syms[thisplace];
	}
      ++min;
    }

  /* If the file is relocatable, and the symbol could be from this
     section, prefer a symbol from this section over symbols from
     others, even if the other symbol's value might be closer.

     Note that this may be wrong for some symbol references if the
     sections have overlapping memory ranges, but in that case there's
     no way to tell what's desired without looking at the relocation
     table.

     Also give the target a chance to reject symbols.  */
  want_section = (aux->require_sec
		  || ((abfd->flags & HAS_RELOC) != 0
		      && vma >= bfd_section_vma (sec)
		      && vma < (bfd_section_vma (sec)
				+ bfd_section_size (sec) / opb)));
  
  if (! sym_ok (want_section, abfd, thisplace, sec, inf))
    {
      long i;
      long newplace = sorted_symcount;

      for (i = min - 1; i >= 0; i--)
	{
	  if (sym_ok (want_section, abfd, i, sec, inf))
	    {
	      if (newplace == sorted_symcount)
		newplace = i;

	      if (bfd_asymbol_value (sorted_syms[i])
		  != bfd_asymbol_value (sorted_syms[newplace]))
		break;

	      /* Remember this symbol and keep searching until we reach
		 an earlier address.  */
	      newplace = i;
	    }
	}

      if (newplace != sorted_symcount)
	thisplace = newplace;
      else
	{
	  /* We didn't find a good symbol with a smaller value.
	     Look for one with a larger value.  */
	  for (i = thisplace + 1; i < sorted_symcount; i++)
	    {
	      if (sym_ok (want_section, abfd, i, sec, inf))
		{
		  thisplace = i;
		  break;
		}
	    }
	}

      if (! sym_ok (want_section, abfd, thisplace, sec, inf))
	/* There is no suitable symbol.  */
	return NULL;
    }

  /* If we have not found an exact match for the specified address
     and we have dynamic relocations available, then we can produce
     a better result by matching a relocation to the address and
     using the symbol associated with that relocation.  */
  rel_count = aux->dynrelcount;
  if (!want_section
      && sorted_syms[thisplace]->value != vma
      && rel_count > 0
      && aux->dynrelbuf != NULL
      && aux->dynrelbuf[0]->address <= vma
      && aux->dynrelbuf[rel_count - 1]->address >= vma
      /* If we have matched a synthetic symbol, then stick with that.  */
      && (sorted_syms[thisplace]->flags & BSF_SYNTHETIC) == 0)
    {
      arelent **  rel_low;
      arelent **  rel_high;

      rel_low = aux->dynrelbuf;
      rel_high = rel_low + rel_count - 1;
      while (rel_low <= rel_high)
	{
	  arelent **rel_mid = &rel_low[(rel_high - rel_low) / 2];
	  arelent * rel = *rel_mid;

	  if (rel->address == vma)
	    {
	      /* Absolute relocations do not provide a more helpful
	         symbolic address.  Find a non-absolute relocation
		 with the same address.  */
	      arelent **rel_vma = rel_mid;
	      for (rel_mid--;
		   rel_mid >= rel_low && rel_mid[0]->address == vma;
		   rel_mid--)
		rel_vma = rel_mid;

	      for (; rel_vma <= rel_high && rel_vma[0]->address == vma;
		   rel_vma++)
		{
		  rel = *rel_vma;
		  if (rel->sym_ptr_ptr != NULL
		      && ! bfd_is_abs_section ((* rel->sym_ptr_ptr)->section))
		    {
		      if (place != NULL)
			* place = thisplace;
		      return * rel->sym_ptr_ptr;
		    }
		}
	      break;
	    }

	  if (vma < rel->address)
	    rel_high = rel_mid;
	  else if (vma >= rel_mid[1]->address)
	    rel_low = rel_mid + 1;
	  else
	    break;
	}
    }

  if (place != NULL)
    *place = thisplace;

  return sorted_syms[thisplace];
}

/* Print an address and the offset to the nearest symbol.  */

static void
objdump_print_addr_with_sym (bfd *abfd, asection *sec, asymbol *sym,
			     bfd_vma vma, struct disassemble_info *inf,
			     bfd_boolean skip_zeroes)
{
  objdump_print_value (vma, inf, skip_zeroes);

  if (sym == NULL)
    {
      bfd_vma secaddr;

      (*inf->fprintf_func) (inf->stream, " <%s",
			    sanitize_string (bfd_section_name (sec)));
      secaddr = bfd_section_vma (sec);
      if (vma < secaddr)
	{
	  (*inf->fprintf_func) (inf->stream, "-0x");
	  objdump_print_value (secaddr - vma, inf, TRUE);
	}
      else if (vma > secaddr)
	{
	  (*inf->fprintf_func) (inf->stream, "+0x");
	  objdump_print_value (vma - secaddr, inf, TRUE);
	}
      (*inf->fprintf_func) (inf->stream, ">");
    }
  else
    {
      (*inf->fprintf_func) (inf->stream, " <");

      objdump_print_symname (abfd, inf, sym);

      if (bfd_asymbol_value (sym) == vma)
	;
      /* Undefined symbols in an executables and dynamic objects do not have
	 a value associated with them, so it does not make sense to display
	 an offset relative to them.  Normally we would not be provided with
	 this kind of symbol, but the target backend might choose to do so,
	 and the code in find_symbol_for_address might return an as yet
	 unresolved symbol associated with a dynamic reloc.  */
      else if ((bfd_get_file_flags (abfd) & (EXEC_P | DYNAMIC))
	       && bfd_is_und_section (sym->section))
	;
      else if (bfd_asymbol_value (sym) > vma)
	{
	  (*inf->fprintf_func) (inf->stream, "-0x");
	  objdump_print_value (bfd_asymbol_value (sym) - vma, inf, TRUE);
	}
      else if (vma > bfd_asymbol_value (sym))
	{
	  (*inf->fprintf_func) (inf->stream, "+0x");
	  objdump_print_value (vma - bfd_asymbol_value (sym), inf, TRUE);
	}

      (*inf->fprintf_func) (inf->stream, ">");
    }

  if (display_file_offsets)
    inf->fprintf_func (inf->stream, _(" (File Offset: 0x%lx)"),
			(long int)(sec->filepos + (vma - sec->vma)));
}

/* Print an address (VMA), symbolically if possible.
   If SKIP_ZEROES is TRUE, don't output leading zeroes.  */

static void
objdump_print_addr (bfd_vma vma,
		    struct disassemble_info *inf,
		    bfd_boolean skip_zeroes)
{
  struct objdump_disasm_info *aux;
  asymbol *sym = NULL;
  bfd_boolean skip_find = FALSE;

  aux = (struct objdump_disasm_info *) inf->application_data;

  if (sorted_symcount < 1)
    {
      (*inf->fprintf_func) (inf->stream, "0x");
      objdump_print_value (vma, inf, skip_zeroes);

      if (display_file_offsets)
	inf->fprintf_func (inf->stream, _(" (File Offset: 0x%lx)"),
			   (long int) (inf->section->filepos
				       + (vma - inf->section->vma)));
      return;
    }

  if (aux->reloc != NULL
      && aux->reloc->sym_ptr_ptr != NULL
      && * aux->reloc->sym_ptr_ptr != NULL)
    {
      sym = * aux->reloc->sym_ptr_ptr;

      /* Adjust the vma to the reloc.  */
      vma += bfd_asymbol_value (sym);

      if (bfd_is_und_section (bfd_asymbol_section (sym)))
	skip_find = TRUE;
    }

  if (!skip_find)
    sym = find_symbol_for_address (vma, inf, NULL);

  objdump_print_addr_with_sym (aux->abfd, inf->section, sym, vma, inf,
			       skip_zeroes);
}

/* Print VMA to INFO.  This function is passed to the disassembler
   routine.  */

static void
objdump_print_address (bfd_vma vma, struct disassemble_info *inf)
{
  objdump_print_addr (vma, inf, ! prefix_addresses);
}

/* Determine if the given address has a symbol associated with it.  */

static int
objdump_symbol_at_address (bfd_vma vma, struct disassemble_info * inf)
{
  asymbol * sym;

  sym = find_symbol_for_address (vma, inf, NULL);

  return (sym != NULL && (bfd_asymbol_value (sym) == vma));
}

/* Hold the last function name and the last line number we displayed
   in a disassembly.  */

static char *prev_functionname;
static unsigned int prev_line;
static unsigned int prev_discriminator;

/* We keep a list of all files that we have seen when doing a
   disassembly with source, so that we know how much of the file to
   display.  This can be important for inlined functions.  */

struct print_file_list
{
  struct print_file_list *next;
  const char *filename;
  const char *modname;
  const char *map;
  size_t mapsize;
  const char **linemap;
  unsigned maxline;
  unsigned last_line;
  unsigned max_printed;
  int first;
};

static struct print_file_list *print_files;

/* The number of preceding context lines to show when we start
   displaying a file for the first time.  */

#define SHOW_PRECEDING_CONTEXT_LINES (5)

/* Read a complete file into memory.  */

static const char *
slurp_file (const char *fn, size_t *size, struct stat *fst)
{
#ifdef HAVE_MMAP
  int ps = getpagesize ();
  size_t msize;
#endif
  const char *map;
  int fd = open (fn, O_RDONLY | O_BINARY);

  if (fd < 0)
    return NULL;
  if (fstat (fd, fst) < 0)
    {
      close (fd);
      return NULL;
    }
  *size = fst->st_size;
#ifdef HAVE_MMAP
  msize = (*size + ps - 1) & ~(ps - 1);
  map = mmap (NULL, msize, PROT_READ, MAP_SHARED, fd, 0);
  if (map != (char *) -1L)
    {
      close (fd);
      return map;
    }
#endif
  map = (const char *) malloc (*size);
  if (!map || (size_t) read (fd, (char *) map, *size) != *size)
    {
      free ((void *) map);
      map = NULL;
    }
  close (fd);
  return map;
}

#define line_map_decrease 5

/* Precompute array of lines for a mapped file. */

static const char **
index_file (const char *map, size_t size, unsigned int *maxline)
{
  const char *p, *lstart, *end;
  int chars_per_line = 45; /* First iteration will use 40.  */
  unsigned int lineno;
  const char **linemap = NULL;
  unsigned long line_map_size = 0;

  lineno = 0;
  lstart = map;
  end = map + size;

  for (p = map; p < end; p++)
    {
      if (*p == '\n')
	{
	  if (p + 1 < end && p[1] == '\r')
	    p++;
	}
      else if (*p == '\r')
	{
	  if (p + 1 < end && p[1] == '\n')
	    p++;
	}
      else
	continue;

      /* End of line found.  */

      if (linemap == NULL || line_map_size < lineno + 1)
	{
	  unsigned long newsize;

	  chars_per_line -= line_map_decrease;
	  if (chars_per_line <= 1)
	    chars_per_line = 1;
	  line_map_size = size / chars_per_line + 1;
	  if (line_map_size < lineno + 1)
	    line_map_size = lineno + 1;
	  newsize = line_map_size * sizeof (char *);
	  linemap = (const char **) xrealloc (linemap, newsize);
	}

      linemap[lineno++] = lstart;
      lstart = p + 1;
    }

  *maxline = lineno;
  return linemap;
}

/* Tries to open MODNAME, and if successful adds a node to print_files
   linked list and returns that node.  Returns NULL on failure.  */

static struct print_file_list *
try_print_file_open (const char *origname, const char *modname, struct stat *fst)
{
  struct print_file_list *p;

  p = (struct print_file_list *) xmalloc (sizeof (struct print_file_list));

  p->map = slurp_file (modname, &p->mapsize, fst);
  if (p->map == NULL)
    {
      free (p);
      return NULL;
    }

  p->linemap = index_file (p->map, p->mapsize, &p->maxline);
  p->last_line = 0;
  p->max_printed = 0;
  p->filename = origname;
  p->modname = modname;
  p->next = print_files;
  p->first = 1;
  print_files = p;
  return p;
}

/* If the source file, as described in the symtab, is not found
   try to locate it in one of the paths specified with -I
   If found, add location to print_files linked list.  */

static struct print_file_list *
update_source_path (const char *filename, bfd *abfd)
{
  struct print_file_list *p;
  const char *fname;
  struct stat fst;
  int i;

  p = try_print_file_open (filename, filename, &fst);
  if (p == NULL)
    {
      if (include_path_count == 0)
	return NULL;

      /* Get the name of the file.  */
      fname = lbasename (filename);

      /* If file exists under a new path, we need to add it to the list
	 so that show_line knows about it.  */
      for (i = 0; i < include_path_count; i++)
	{
	  char *modname = concat (include_paths[i], "/", fname,
				  (const char *) 0);

	  p = try_print_file_open (filename, modname, &fst);
	  if (p)
	    break;

	  free (modname);
	}
    }

  if (p != NULL)
    {
      long mtime = bfd_get_mtime (abfd);

      if (fst.st_mtime > mtime)
	warn (_("source file %s is more recent than object file\n"),
	      filename);
    }

  return p;
}

/* Print a source file line.  */

static void
print_line (struct print_file_list *p, unsigned int linenum)
{
  const char *l;
  size_t len;

  --linenum;
  if (linenum >= p->maxline)
    return;
  l = p->linemap [linenum];
  if (source_comment != NULL && strlen (l) > 0)
    printf ("%s", source_comment);
  len = strcspn (l, "\n\r");
  /* Test fwrite return value to quiet glibc warning.  */
  if (len == 0 || fwrite (l, len, 1, stdout) == 1)
    putchar ('\n');
}

/* Print a range of source code lines. */

static void
dump_lines (struct print_file_list *p, unsigned int start, unsigned int end)
{
  if (p->map == NULL)
    return;
  while (start <= end)
    {
      print_line (p, start);
      start++;
    }
}

/* Show the line number, or the source line, in a disassembly
   listing.  */

static void
show_line (bfd *abfd, asection *section, bfd_vma addr_offset)
{
  const char *filename;
  const char *functionname;
  unsigned int linenumber;
  unsigned int discriminator;
  bfd_boolean reloc;
  char *path = NULL;

  if (! with_line_numbers && ! with_source_code)
    return;

  if (! bfd_find_nearest_line_discriminator (abfd, section, syms, addr_offset,
					     &filename, &functionname,
					     &linenumber, &discriminator))
    return;

  if (filename != NULL && *filename == '\0')
    filename = NULL;
  if (functionname != NULL && *functionname == '\0')
    functionname = NULL;

  if (filename
      && IS_ABSOLUTE_PATH (filename)
      && prefix)
    {
      char *path_up;
      const char *fname = filename;

      path = xmalloc (prefix_length + PATH_MAX + 1);

      if (prefix_length)
	memcpy (path, prefix, prefix_length);
      path_up = path + prefix_length;

      /* Build relocated filename, stripping off leading directories
	 from the initial filename if requested.  */
      if (prefix_strip > 0)
	{
	  int level = 0;
	  const char *s;

	  /* Skip selected directory levels.  */
	  for (s = fname + 1; *s != '\0' && level < prefix_strip; s++)
	    if (IS_DIR_SEPARATOR (*s))
	      {
		fname = s;
		level++;
	      }
	}

      /* Update complete filename.  */
      strncpy (path_up, fname, PATH_MAX);
      path_up[PATH_MAX] = '\0';

      filename = path;
      reloc = TRUE;
    }
  else
    reloc = FALSE;

  if (with_line_numbers)
    {
      if (functionname != NULL
	  && (prev_functionname == NULL
	      || strcmp (functionname, prev_functionname) != 0))
	{
	  printf ("%s():\n", sanitize_string (functionname));
	  prev_line = -1;
	}
      if (linenumber > 0
	  && (linenumber != prev_line
	      || discriminator != prev_discriminator))
	{
	  if (discriminator > 0)
	    printf ("%s:%u (discriminator %u)\n",
		    filename == NULL ? "???" : sanitize_string (filename),
		    linenumber, discriminator);
	  else
	    printf ("%s:%u\n", filename == NULL
		    ? "???" : sanitize_string (filename),
		    linenumber);
	}
      if (unwind_inlines)
	{
	  const char *filename2;
	  const char *functionname2;
	  unsigned line2;

	  while (bfd_find_inliner_info (abfd, &filename2, &functionname2,
					&line2))
	    {
	      printf ("inlined by %s:%u",
		      sanitize_string (filename2), line2);
	      printf (" (%s)\n", sanitize_string (functionname2));
	    }
	}
    }

  if (with_source_code
      && filename != NULL
      && linenumber > 0)
    {
      struct print_file_list **pp, *p;
      unsigned l;

      for (pp = &print_files; *pp != NULL; pp = &(*pp)->next)
	if (filename_cmp ((*pp)->filename, filename) == 0)
	  break;
      p = *pp;

      if (p == NULL)
	{
	  if (reloc)
	    filename = xstrdup (filename);
	  p = update_source_path (filename, abfd);
	}

      if (p != NULL && linenumber != p->last_line)
	{
	  if (file_start_context && p->first)
	    l = 1;
	  else
	    {
	      l = linenumber - SHOW_PRECEDING_CONTEXT_LINES;
	      if (l >= linenumber)
		l = 1;
	      if (p->max_printed >= l)
		{
		  if (p->max_printed < linenumber)
		    l = p->max_printed + 1;
		  else
		    l = linenumber;
		}
	    }
	  dump_lines (p, l, linenumber);
	  if (p->max_printed < linenumber)
	    p->max_printed = linenumber;
	  p->last_line = linenumber;
	  p->first = 0;
	}
    }

  if (functionname != NULL
      && (prev_functionname == NULL
	  || strcmp (functionname, prev_functionname) != 0))
    {
      if (prev_functionname != NULL)
	free (prev_functionname);
      prev_functionname = (char *) xmalloc (strlen (functionname) + 1);
      strcpy (prev_functionname, functionname);
    }

  if (linenumber > 0 && linenumber != prev_line)
    prev_line = linenumber;

  if (discriminator != prev_discriminator)
    prev_discriminator = discriminator;

  if (path)
    free (path);
}

/* Pseudo FILE object for strings.  */
typedef struct
{
  char *buffer;
  size_t pos;
  size_t alloc;
} SFILE;

/* sprintf to a "stream".  */

static int ATTRIBUTE_PRINTF_2
objdump_sprintf (SFILE *f, const char *format, ...)
{
  size_t n;
  va_list args;

  while (1)
    {
      size_t space = f->alloc - f->pos;

      va_start (args, format);
      n = vsnprintf (f->buffer + f->pos, space, format, args);
      va_end (args);

      if (space > n)
	break;

      f->alloc = (f->alloc + n) * 2;
      f->buffer = (char *) xrealloc (f->buffer, f->alloc);
    }
  f->pos += n;

  return n;
}

/* Code for generating (colored) diagrams of control flow start and end
   points.  */

/* Structure used to store the properties of a jump.  */

struct jump_info
{
  /* The next jump, or NULL if this is the last object.  */
  struct jump_info *next;
  /* The previous jump, or NULL if this is the first object.  */
  struct jump_info *prev;
  /* The start addresses of the jump.  */
  struct
    {
      /* The list of start addresses.  */
      bfd_vma *addresses;
      /* The number of elements.  */
      size_t count;
      /* The maximum number of elements that fit into the array.  */
      size_t max_count;
    } start;
  /* The end address of the jump.  */
  bfd_vma end;
  /* The drawing level of the jump.  */
  int level;
};

/* Construct a jump object for a jump from start
   to end with the corresponding level.  */

static struct jump_info *
jump_info_new (bfd_vma start, bfd_vma end, int level)
{
  struct jump_info *result = xmalloc (sizeof (struct jump_info));

  result->next = NULL;
  result->prev = NULL;
  result->start.addresses = xmalloc (sizeof (bfd_vma *) * 2);
  result->start.addresses[0] = start;
  result->start.count = 1;
  result->start.max_count = 2;
  result->end = end;
  result->level = level;

  return result;
}

/* Free a jump object and return the next object
   or NULL if this was the last one.  */

static struct jump_info *
jump_info_free (struct jump_info *ji)
{
  struct jump_info *result = NULL;

  if (ji)
    {
      result = ji->next;
      if (ji->start.addresses)
	free (ji->start.addresses);
      free (ji);
    }

  return result;
}

/* Get the smallest value of all start and end addresses.  */

static bfd_vma
jump_info_min_address (const struct jump_info *ji)
{
  bfd_vma min_address = ji->end;
  size_t i;

  for (i = ji->start.count; i-- > 0;)
    if (ji->start.addresses[i] < min_address)
      min_address = ji->start.addresses[i];
  return min_address;
}

/* Get the largest value of all start and end addresses.  */

static bfd_vma
jump_info_max_address (const struct jump_info *ji)
{
  bfd_vma max_address = ji->end;
  size_t i;

  for (i = ji->start.count; i-- > 0;)
    if (ji->start.addresses[i] > max_address)
      max_address = ji->start.addresses[i];
  return max_address;
}

/* Get the target address of a jump.  */

static bfd_vma
jump_info_end_address (const struct jump_info *ji)
{
  return ji->end;
}

/* Test if an address is one of the start addresses of a jump.  */

static bfd_boolean
jump_info_is_start_address (const struct jump_info *ji, bfd_vma address)
{
  bfd_boolean result = FALSE;
  size_t i;

  for (i = ji->start.count; i-- > 0;)
    if (address == ji->start.addresses[i])
      {
	result = TRUE;
	break;
      }

  return result;
}

/* Test if an address is the target address of a jump.  */

static bfd_boolean
jump_info_is_end_address (const struct jump_info *ji, bfd_vma address)
{
  return (address == ji->end);
}

/* Get the difference between the smallest and largest address of a jump.  */

static bfd_vma
jump_info_size (const struct jump_info *ji)
{
  return jump_info_max_address (ji) - jump_info_min_address (ji);
}

/* Unlink a jump object from a list.  */

static void
jump_info_unlink (struct jump_info *node,
		  struct jump_info **base)
{
  if (node->next)
    node->next->prev = node->prev;
  if (node->prev)
    node->prev->next = node->next;
  else
    *base = node->next;
  node->next = NULL;
  node->prev = NULL;
}

/* Insert unlinked jump info node into a list.  */

static void
jump_info_insert (struct jump_info *node,
		  struct jump_info *target,
		  struct jump_info **base)
{
  node->next = target;
  node->prev = target->prev;
  target->prev = node;
  if (node->prev)
    node->prev->next = node;
  else
    *base = node;
}

/* Add unlinked node to the front of a list.  */

static void
jump_info_add_front (struct jump_info *node,
		     struct jump_info **base)
{
  node->next = *base;
  if (node->next)
    node->next->prev = node;
  node->prev = NULL;
  *base = node;
}

/* Move linked node to target position.  */

static void
jump_info_move_linked (struct jump_info *node,
		       struct jump_info *target,
		       struct jump_info **base)
{
  /* Unlink node.  */
  jump_info_unlink (node, base);
  /* Insert node at target position.  */
  jump_info_insert (node, target, base);
}

/* Test if two jumps intersect.  */

static bfd_boolean
jump_info_intersect (const struct jump_info *a,
		     const struct jump_info *b)
{
  return ((jump_info_max_address (a) >= jump_info_min_address (b))
	  && (jump_info_min_address (a) <= jump_info_max_address (b)));
}

/* Merge two compatible jump info objects.  */

static void
jump_info_merge (struct jump_info **base)
{
  struct jump_info *a;

  for (a = *base; a; a = a->next)
    {
      struct jump_info *b;

      for (b = a->next; b; b = b->next)
	{
	  /* Merge both jumps into one.  */
	  if (a->end == b->end)
	    {
	      /* Reallocate addresses.  */
	      size_t needed_size = a->start.count + b->start.count;
	      size_t i;

	      if (needed_size > a->start.max_count)
		{
		  a->start.max_count += b->start.max_count;
		  a->start.addresses =
		    xrealloc (a->start.addresses,
			      a->start.max_count * sizeof (bfd_vma *));
		}

	      /* Append start addresses.  */
	      for (i = 0; i < b->start.count; ++i)
		a->start.addresses[a->start.count++] =
		  b->start.addresses[i];

	      /* Remove and delete jump.  */
	      struct jump_info *tmp = b->prev;
	      jump_info_unlink (b, base);
	      jump_info_free (b);
	      b = tmp;
	    }
	}
    }
}

/* Sort jumps by their size and starting point using a stable
   minsort. This could be improved if sorting performance is
   an issue, for example by using mergesort.  */

static void
jump_info_sort (struct jump_info **base)
{
  struct jump_info *current_element = *base;

  while (current_element)
    {
      struct jump_info *best_match = current_element;
      struct jump_info *runner = current_element->next;
      bfd_vma best_size = jump_info_size (best_match);

      while (runner)
	{
	  bfd_vma runner_size = jump_info_size (runner);

	  if ((runner_size < best_size)
	      || ((runner_size == best_size)
		  && (jump_info_min_address (runner)
		      < jump_info_min_address (best_match))))
	    {
	      best_match = runner;
	      best_size = runner_size;
	    }

	  runner = runner->next;
	}

      if (best_match == current_element)
	current_element = current_element->next;
      else
	jump_info_move_linked (best_match, current_element, base);
    }
}

/* Visualize all jumps at a given address.  */

static void
jump_info_visualize_address (bfd_vma address,
			     int max_level,
			     char *line_buffer,
			     uint8_t *color_buffer)
{
  struct jump_info *ji = detected_jumps;
  size_t len = (max_level + 1) * 3;

  /* Clear line buffer.  */
  memset (line_buffer, ' ', len);
  memset (color_buffer, 0, len);

  /* Iterate over jumps and add their ASCII art.  */
  while (ji)
    {
      /* Discard jumps that are never needed again.  */
      if (jump_info_max_address (ji) < address)
	{
	  struct jump_info *tmp = ji;

	  ji = ji->next;
	  jump_info_unlink (tmp, &detected_jumps);
	  jump_info_free (tmp);
	  continue;
	}

      /* This jump intersects with the current address.  */
      if (jump_info_min_address (ji) <= address)
	{
	  /* Hash target address to get an even
	     distribution between all values.  */
	  bfd_vma hash_address = jump_info_end_address (ji);
	  uint8_t color = iterative_hash_object (hash_address, 0);
	  /* Fetch line offset.  */
	  int offset = (max_level - ji->level) * 3;

	  /* Draw start line.  */
	  if (jump_info_is_start_address (ji, address))
	    {
	      size_t i = offset + 1;

	      for (; i < len - 1; ++i)
		if (line_buffer[i] == ' ')
		  {
		    line_buffer[i] = '-';
		    color_buffer[i] = color;
		  }

	      if (line_buffer[i] == ' ')
		{
		  line_buffer[i] = '-';
		  color_buffer[i] = color;
		}
	      else if (line_buffer[i] == '>')
		{
		  line_buffer[i] = 'X';
		  color_buffer[i] = color;
		}

	      if (line_buffer[offset] == ' ')
		{
		  if (address <= ji->end)
		    line_buffer[offset] =
		      (jump_info_min_address (ji) == address) ? '/': '+';
		  else
		    line_buffer[offset] =
		      (jump_info_max_address (ji) == address) ? '\\': '+';
		  color_buffer[offset] = color;
		}
	    }
	  /* Draw jump target.  */
	  else if (jump_info_is_end_address (ji, address))
	    {
	      size_t i = offset + 1;

	      for (; i < len - 1; ++i)
		if (line_buffer[i] == ' ')
		  {
		    line_buffer[i] = '-';
		    color_buffer[i] = color;
		  }

	      if (line_buffer[i] == ' ')
		{
		  line_buffer[i] = '>';
		  color_buffer[i] = color;
		}
	      else if (line_buffer[i] == '-')
		{
		  line_buffer[i] = 'X';
		  color_buffer[i] = color;
		}

	      if (line_buffer[offset] == ' ')
		{
		  if (jump_info_min_address (ji) < address)
		    line_buffer[offset] =
		      (jump_info_max_address (ji) > address) ? '>' : '\\';
		  else
		    line_buffer[offset] = '/';
		  color_buffer[offset] = color;
		}
	    }
	  /* Draw intermediate line segment.  */
	  else if (line_buffer[offset] == ' ')
	    {
	      line_buffer[offset] = '|';
	      color_buffer[offset] = color;
	    }
	}

      ji = ji->next;
    }
}

/* Clone of disassemble_bytes to detect jumps inside a function.  */
/* FIXME: is this correct? Can we strip it down even further?  */

static struct jump_info *
disassemble_jumps (struct disassemble_info * inf,
		   disassembler_ftype        disassemble_fn,
		   bfd_vma                   start_offset,
		   bfd_vma                   stop_offset,
		   bfd_vma		     rel_offset,
		   arelent ***               relppp,
		   arelent **                relppend)
{
  struct objdump_disasm_info *aux;
  struct jump_info *jumps = NULL;
  asection *section;
  bfd_vma addr_offset;
  unsigned int opb = inf->octets_per_byte;
  int octets = opb;
  SFILE sfile;

  aux = (struct objdump_disasm_info *) inf->application_data;
  section = inf->section;

  sfile.alloc = 120;
  sfile.buffer = (char *) xmalloc (sfile.alloc);
  sfile.pos = 0;

  inf->insn_info_valid = 0;
  inf->fprintf_func = (fprintf_ftype) objdump_sprintf;
  inf->stream = &sfile;

  addr_offset = start_offset;
  while (addr_offset < stop_offset)
    {
      int previous_octets;

      /* Remember the length of the previous instruction.  */
      previous_octets = octets;
      octets = 0;

      sfile.pos = 0;
      inf->bytes_per_line = 0;
      inf->bytes_per_chunk = 0;
      inf->flags = ((disassemble_all ? DISASSEMBLE_DATA : 0)
        | (wide_output ? WIDE_OUTPUT : 0));
      if (machine)
	inf->flags |= USER_SPECIFIED_MACHINE_TYPE;

      if (inf->disassembler_needs_relocs
	  && (bfd_get_file_flags (aux->abfd) & EXEC_P) == 0
	  && (bfd_get_file_flags (aux->abfd) & DYNAMIC) == 0
	  && *relppp < relppend)
	{
	  bfd_signed_vma distance_to_rel;

	  distance_to_rel = (**relppp)->address - (rel_offset + addr_offset);

	  /* Check to see if the current reloc is associated with
	     the instruction that we are about to disassemble.  */
	  if (distance_to_rel == 0
	      /* FIXME: This is wrong.  We are trying to catch
		 relocs that are addressed part way through the
		 current instruction, as might happen with a packed
		 VLIW instruction.  Unfortunately we do not know the
		 length of the current instruction since we have not
		 disassembled it yet.  Instead we take a guess based
		 upon the length of the previous instruction.  The
		 proper solution is to have a new target-specific
		 disassembler function which just returns the length
		 of an instruction at a given address without trying
		 to display its disassembly. */
	      || (distance_to_rel > 0
		&& distance_to_rel < (bfd_signed_vma) (previous_octets/ opb)))
	    {
	      inf->flags |= INSN_HAS_RELOC;
	    }
	}

      if (! disassemble_all
	  && (section->flags & (SEC_CODE | SEC_HAS_CONTENTS))
	  == (SEC_CODE | SEC_HAS_CONTENTS))
	/* Set a stop_vma so that the disassembler will not read
	   beyond the next symbol.  We assume that symbols appear on
	   the boundaries between instructions.  We only do this when
	   disassembling code of course, and when -D is in effect.  */
	inf->stop_vma = section->vma + stop_offset;

      inf->stop_offset = stop_offset;

      /* Extract jump information.  */
      inf->insn_info_valid = 0;
      octets = (*disassemble_fn) (section->vma + addr_offset, inf);
      /* Test if a jump was detected.  */
      if (inf->insn_info_valid
	  && ((inf->insn_type == dis_branch)
	      || (inf->insn_type == dis_condbranch)
	      || (inf->insn_type == dis_jsr)
	      || (inf->insn_type == dis_condjsr))
	  && (inf->target >= section->vma + start_offset)
	  && (inf->target < section->vma + stop_offset))
	{
	  struct jump_info *ji =
	    jump_info_new (section->vma + addr_offset, inf->target, -1);
	  jump_info_add_front (ji, &jumps);
	}

      inf->stop_vma = 0;

      addr_offset += octets / opb;
    }

  inf->fprintf_func = (fprintf_ftype) fprintf;
  inf->stream = stdout;

  free (sfile.buffer);

  /* Merge jumps.  */
  jump_info_merge (&jumps);
  /* Process jumps.  */
  jump_info_sort (&jumps);

  /* Group jumps by level.  */
  struct jump_info *last_jump = jumps;
  int max_level = -1;

  while (last_jump)
    {
      /* The last jump is part of the next group.  */
      struct jump_info *base = last_jump;
      /* Increment level.  */
      base->level = ++max_level;

      /* Find jumps that can be combined on the same
	 level, with the largest jumps tested first.
	 This has the advantage that large jumps are on
	 lower levels and do not intersect with small
	 jumps that get grouped on higher levels.  */
      struct jump_info *exchange_item = last_jump->next;
      struct jump_info *it = exchange_item;

      for (; it; it = it->next)
	{
	  /* Test if the jump intersects with any
	     jump from current group.  */
	  bfd_boolean ok = TRUE;
	  struct jump_info *it_collision;

	  for (it_collision = base;
	       it_collision != exchange_item;
	       it_collision = it_collision->next)
	    {
	      /* This jump intersects so we leave it out.  */
	      if (jump_info_intersect (it_collision, it))
		{
		  ok = FALSE;
		  break;
		}
	    }

	  /* Add jump to group.  */
	  if (ok)
	    {
	      /* Move current element to the front.  */
	      if (it != exchange_item)
		{
		  struct jump_info *save = it->prev;
		  jump_info_move_linked (it, exchange_item, &jumps);
		  last_jump = it;
		  it = save;
		}
	      else
		{
		  last_jump = exchange_item;
		  exchange_item = exchange_item->next;
		}
	      last_jump->level = max_level;
	    }
	}

      /* Move to next group.  */
      last_jump = exchange_item;
    }

  return jumps;
}

/* The number of zeroes we want to see before we start skipping them.
   The number is arbitrarily chosen.  */

#define DEFAULT_SKIP_ZEROES 8

/* The number of zeroes to skip at the end of a section.  If the
   number of zeroes at the end is between SKIP_ZEROES_AT_END and
   SKIP_ZEROES, they will be disassembled.  If there are fewer than
   SKIP_ZEROES_AT_END, they will be skipped.  This is a heuristic
   attempt to avoid disassembling zeroes inserted by section
   alignment.  */

#define DEFAULT_SKIP_ZEROES_AT_END 3

static int
null_print (const void * stream ATTRIBUTE_UNUSED, const char * format ATTRIBUTE_UNUSED, ...)
{
  return 1;
}

/* Disassemble some data in memory between given values.  */

static void
disassemble_bytes (struct disassemble_info * inf,
		   disassembler_ftype        disassemble_fn,
		   bfd_boolean               insns,
		   bfd_byte *                data,
		   bfd_vma                   start_offset,
		   bfd_vma                   stop_offset,
		   bfd_vma		     rel_offset,
		   arelent ***               relppp,
		   arelent **                relppend)
{
  struct objdump_disasm_info *aux;
  asection *section;
  int octets_per_line;
  int skip_addr_chars;
  bfd_vma addr_offset;
  unsigned int opb = inf->octets_per_byte;
  unsigned int skip_zeroes = inf->skip_zeroes;
  unsigned int skip_zeroes_at_end = inf->skip_zeroes_at_end;
  int octets = opb;
  SFILE sfile;

  aux = (struct objdump_disasm_info *) inf->application_data;
  section = inf->section;

  sfile.alloc = 120;
  sfile.buffer = (char *) xmalloc (sfile.alloc);
  sfile.pos = 0;

  if (insn_width)
    octets_per_line = insn_width;
  else if (insns)
    octets_per_line = 4;
  else
    octets_per_line = 16;

  /* Figure out how many characters to skip at the start of an
     address, to make the disassembly look nicer.  We discard leading
     zeroes in chunks of 4, ensuring that there is always a leading
     zero remaining.  */
  skip_addr_chars = 0;
  if (! prefix_addresses)
    {
      char buf[30];

      bfd_sprintf_vma (aux->abfd, buf, section->vma + section->size / opb);

      while (buf[skip_addr_chars] == '0')
	++skip_addr_chars;

      /* Don't discard zeros on overflow.  */
      if (buf[skip_addr_chars] == '\0' && section->vma != 0)
	skip_addr_chars = 0;

      if (skip_addr_chars != 0)
	skip_addr_chars = (skip_addr_chars - 1) & -4;
    }

  inf->insn_info_valid = 0;

  /* Determine maximum level. */
  uint8_t *color_buffer = NULL;
  char *line_buffer = NULL;
  int max_level = -1;

  /* Some jumps were detected.  */
  if (detected_jumps)
    {
      struct jump_info *ji;

      /* Find maximum jump level.  */
      for (ji = detected_jumps; ji; ji = ji->next)
	{
	  if (ji->level > max_level)
	    max_level = ji->level;
	}

      /* Allocate buffers.  */
      size_t len = (max_level + 1) * 3 + 1;
      line_buffer = xmalloc (len);
      line_buffer[len - 1] = 0;
      color_buffer = xmalloc (len);
      color_buffer[len - 1] = 0;
    }

  addr_offset = start_offset;
  while (addr_offset < stop_offset)
    {
      bfd_vma z;
      bfd_boolean need_nl = FALSE;

      octets = 0;

      /* Make sure we don't use relocs from previous instructions.  */
      aux->reloc = NULL;

      /* If we see more than SKIP_ZEROES octets of zeroes, we just
	 print `...'.  */
      for (z = addr_offset * opb; z < stop_offset * opb; z++)
	if (data[z] != 0)
	  break;
      if (! disassemble_zeroes
	  && (inf->insn_info_valid == 0
	      || inf->branch_delay_insns == 0)
	  && (z - addr_offset * opb >= skip_zeroes
	      || (z == stop_offset * opb &&
		  z - addr_offset * opb < skip_zeroes_at_end)))
	{
	  /* If there are more nonzero octets to follow, we only skip
	     zeroes in multiples of 4, to try to avoid running over
	     the start of an instruction which happens to start with
	     zero.  */
	  if (z != stop_offset * opb)
	    z = addr_offset * opb + ((z - addr_offset * opb) &~ 3);

	  octets = z - addr_offset * opb;

	  /* If we are going to display more data, and we are displaying
	     file offsets, then tell the user how many zeroes we skip
	     and the file offset from where we resume dumping.  */
	  if (display_file_offsets && ((addr_offset + (octets / opb)) < stop_offset))
	    printf ("\t... (skipping %d zeroes, resuming at file offset: 0x%lx)\n",
		    octets / opb,
		    (unsigned long) (section->filepos
				     + (addr_offset + (octets / opb))));
	  else
	    printf ("\t...\n");
	}
      else
	{
	  char buf[50];
	  int bpc = 0;
	  int pb = 0;

	  if (with_line_numbers || with_source_code)
	    show_line (aux->abfd, section, addr_offset);

	  if (! prefix_addresses)
	    {
	      char *s;

	      bfd_sprintf_vma (aux->abfd, buf, section->vma + addr_offset);
	      for (s = buf + skip_addr_chars; *s == '0'; s++)
		*s = ' ';
	      if (*s == '\0')
		*--s = '0';
	      printf ("%s:\t", buf + skip_addr_chars);
	    }
	  else
	    {
	      aux->require_sec = TRUE;
	      objdump_print_address (section->vma + addr_offset, inf);
	      aux->require_sec = FALSE;
	      putchar (' ');
	    }

	  /* Visualize jumps. */
	  if (line_buffer)
	    {
	      jump_info_visualize_address (section->vma + addr_offset,
					   max_level,
					   line_buffer,
					   color_buffer);

	      size_t line_buffer_size = strlen (line_buffer);
	      char last_color = 0;
	      size_t i;

	      for (i = 0; i <= line_buffer_size; ++i)
		{
		  if (color_output)
		    {
		      uint8_t color = (i < line_buffer_size) ? color_buffer[i]: 0;

		      if (color != last_color)
			{
			  if (color)
			    if (extended_color_output)
			      /* Use extended 8bit color, but
			         do not choose dark colors.  */
			      printf ("\033[38;5;%dm", 124 + (color % 108));
			    else
			      /* Use simple terminal colors.  */
			      printf ("\033[%dm", 31 + (color % 7));
			  else
			    /* Clear color.  */
			    printf ("\033[0m");
			  last_color = color;
			}
		    }
		  putchar ((i < line_buffer_size) ? line_buffer[i]: ' ');
		}
	    }

	  if (insns)
	    {
	      sfile.pos = 0;
	      inf->fprintf_func = (fprintf_ftype) objdump_sprintf;
	      inf->stream = &sfile;
	      inf->bytes_per_line = 0;
	      inf->bytes_per_chunk = 0;
	      inf->flags = ((disassemble_all ? DISASSEMBLE_DATA : 0)
			    | (wide_output ? WIDE_OUTPUT : 0));
	      if (machine)
		inf->flags |= USER_SPECIFIED_MACHINE_TYPE;

	      if (inf->disassembler_needs_relocs
		  && (bfd_get_file_flags (aux->abfd) & EXEC_P) == 0
		  && (bfd_get_file_flags (aux->abfd) & DYNAMIC) == 0
		  && *relppp < relppend)
		{
		  bfd_signed_vma distance_to_rel;
		  int insn_size = 0;
		  int max_reloc_offset
		    = aux->abfd->arch_info->max_reloc_offset_into_insn;

		  distance_to_rel = ((**relppp)->address - rel_offset
				     - addr_offset);

		  if (distance_to_rel > 0
		      && (max_reloc_offset < 0
			  || distance_to_rel <= max_reloc_offset))
		    {
		      /* This reloc *might* apply to the current insn,
			 starting somewhere inside it.  Discover the length
			 of the current insn so that the check below will
			 work.  */
		      if (insn_width)
			insn_size = insn_width;
		      else
			{
			  /* We find the length by calling the dissassembler
			     function with a dummy print handler.  This should
			     work unless the disassembler is not expecting to
			     be called multiple times for the same address.

			     This does mean disassembling the instruction
			     twice, but we only do this when there is a high
			     probability that there is a reloc that will
			     affect the instruction.  */
			  inf->fprintf_func = (fprintf_ftype) null_print;
			  insn_size = disassemble_fn (section->vma
						      + addr_offset, inf);
			  inf->fprintf_func = (fprintf_ftype) objdump_sprintf;
			}
		    }

		  /* Check to see if the current reloc is associated with
		     the instruction that we are about to disassemble.  */
		  if (distance_to_rel == 0
		      || (distance_to_rel > 0
			  && distance_to_rel < insn_size / (int) opb))
		    {
		      inf->flags |= INSN_HAS_RELOC;
		      aux->reloc = **relppp;
		    }
		}

	      if (! disassemble_all
		  && (section->flags & (SEC_CODE | SEC_HAS_CONTENTS))
		  == (SEC_CODE | SEC_HAS_CONTENTS))
		/* Set a stop_vma so that the disassembler will not read
		   beyond the next symbol.  We assume that symbols appear on
		   the boundaries between instructions.  We only do this when
		   disassembling code of course, and when -D is in effect.  */
		inf->stop_vma = section->vma + stop_offset;

	      inf->stop_offset = stop_offset;
	      octets = (*disassemble_fn) (section->vma + addr_offset, inf);

	      inf->stop_vma = 0;
	      inf->fprintf_func = (fprintf_ftype) fprintf;
	      inf->stream = stdout;
	      if (insn_width == 0 && inf->bytes_per_line != 0)
		octets_per_line = inf->bytes_per_line;
	      if (octets < (int) opb)
		{
		  if (sfile.pos)
		    printf ("%s\n", sfile.buffer);
		  if (octets >= 0)
		    {
		      non_fatal (_("disassemble_fn returned length %d"),
				 octets);
		      exit_status = 1;
		    }
		  break;
		}
	    }
	  else
	    {
	      bfd_vma j;

	      octets = octets_per_line;
	      if (addr_offset + octets / opb > stop_offset)
		octets = (stop_offset - addr_offset) * opb;

	      for (j = addr_offset * opb; j < addr_offset * opb + octets; ++j)
		{
		  if (ISPRINT (data[j]))
		    buf[j - addr_offset * opb] = data[j];
		  else
		    buf[j - addr_offset * opb] = '.';
		}
	      buf[j - addr_offset * opb] = '\0';
	    }

	  if (prefix_addresses
	      ? show_raw_insn > 0
	      : show_raw_insn >= 0)
	    {
	      bfd_vma j;

	      /* If ! prefix_addresses and ! wide_output, we print
		 octets_per_line octets per line.  */
	      pb = octets;
	      if (pb > octets_per_line && ! prefix_addresses && ! wide_output)
		pb = octets_per_line;

	      if (inf->bytes_per_chunk)
		bpc = inf->bytes_per_chunk;
	      else
		bpc = 1;

	      for (j = addr_offset * opb; j < addr_offset * opb + pb; j += bpc)
		{
		  /* PR 21580: Check for a buffer ending early.  */
		  if (j + bpc <= stop_offset * opb)
		    {
		      int k;

		      if (inf->display_endian == BFD_ENDIAN_LITTLE)
			{
			  for (k = bpc - 1; k >= 0; k--)
			    printf ("%02x", (unsigned) data[j + k]);
			}
		      else
			{
			  for (k = 0; k < bpc; k++)
			    printf ("%02x", (unsigned) data[j + k]);
			}
		    }
		  putchar (' ');
		}

	      for (; pb < octets_per_line; pb += bpc)
		{
		  int k;

		  for (k = 0; k < bpc; k++)
		    printf ("  ");
		  putchar (' ');
		}

	      /* Separate raw data from instruction by extra space.  */
	      if (insns)
		putchar ('\t');
	      else
		printf ("    ");
	    }

	  if (! insns)
	    printf ("%s", buf);
	  else if (sfile.pos)
	    printf ("%s", sfile.buffer);

	  if (prefix_addresses
	      ? show_raw_insn > 0
	      : show_raw_insn >= 0)
	    {
	      while (pb < octets)
		{
		  bfd_vma j;
		  char *s;

		  putchar ('\n');
		  j = addr_offset * opb + pb;

		  bfd_sprintf_vma (aux->abfd, buf, section->vma + j / opb);
		  for (s = buf + skip_addr_chars; *s == '0'; s++)
		    *s = ' ';
		  if (*s == '\0')
		    *--s = '0';
		  printf ("%s:\t", buf + skip_addr_chars);

		  pb += octets_per_line;
		  if (pb > octets)
		    pb = octets;
		  for (; j < addr_offset * opb + pb; j += bpc)
		    {
		      /* PR 21619: Check for a buffer ending early.  */
		      if (j + bpc <= stop_offset * opb)
			{
			  int k;

			  if (inf->display_endian == BFD_ENDIAN_LITTLE)
			    {
			      for (k = bpc - 1; k >= 0; k--)
				printf ("%02x", (unsigned) data[j + k]);
			    }
			  else
			    {
			      for (k = 0; k < bpc; k++)
				printf ("%02x", (unsigned) data[j + k]);
			    }
			}
		      putchar (' ');
		    }
		}
	    }

	  if (!wide_output)
	    putchar ('\n');
	  else
	    need_nl = TRUE;
	}

      while ((*relppp) < relppend
	     && (**relppp)->address < rel_offset + addr_offset + octets / opb)
	{
	  if (dump_reloc_info || dump_dynamic_reloc_info)
	    {
	      arelent *q;

	      q = **relppp;

	      if (wide_output)
		putchar ('\t');
	      else
		printf ("\t\t\t");

	      objdump_print_value (section->vma - rel_offset + q->address,
				   inf, TRUE);

	      if (q->howto == NULL)
		printf (": *unknown*\t");
	      else if (q->howto->name)
		printf (": %s\t", q->howto->name);
	      else
		printf (": %d\t", q->howto->type);

	      if (q->sym_ptr_ptr == NULL || *q->sym_ptr_ptr == NULL)
		printf ("*unknown*");
	      else
		{
		  const char *sym_name;

		  sym_name = bfd_asymbol_name (*q->sym_ptr_ptr);
		  if (sym_name != NULL && *sym_name != '\0')
		    objdump_print_symname (aux->abfd, inf, *q->sym_ptr_ptr);
		  else
		    {
		      asection *sym_sec;

		      sym_sec = bfd_asymbol_section (*q->sym_ptr_ptr);
		      sym_name = bfd_section_name (sym_sec);
		      if (sym_name == NULL || *sym_name == '\0')
			sym_name = "*unknown*";
		      printf ("%s", sanitize_string (sym_name));
		    }
		}

	      if (q->addend)
		{
		  bfd_signed_vma addend = q->addend;
		  if (addend < 0)
		    {
		      printf ("-0x");
		      addend = -addend;
		    }
		  else
		    printf ("+0x");
		  objdump_print_value (addend, inf, TRUE);
		}

	      printf ("\n");
	      need_nl = FALSE;
	    }
	  ++(*relppp);
	}

      if (need_nl)
	printf ("\n");

      addr_offset += octets / opb;
    }

  free (sfile.buffer);
  free (line_buffer);
  free (color_buffer);
}

static void
disassemble_section (bfd *abfd, asection *section, void *inf)
{
  const struct elf_backend_data * bed;
  bfd_vma                      sign_adjust = 0;
  struct disassemble_info *    pinfo = (struct disassemble_info *) inf;
  struct objdump_disasm_info * paux;
  unsigned int                 opb = pinfo->octets_per_byte;
  bfd_byte *                   data = NULL;
  bfd_size_type                datasize = 0;
  arelent **                   rel_pp = NULL;
  arelent **                   rel_ppstart = NULL;
  arelent **                   rel_ppend;
  bfd_vma                      stop_offset;
  asymbol *                    sym = NULL;
  long                         place = 0;
  long                         rel_count;
  bfd_vma                      rel_offset;
  unsigned long                addr_offset;
  bfd_boolean                  do_print;
  enum loop_control
  {
   stop_offset_reached,
   function_sym,
   next_sym
  } loop_until;

  /* Sections that do not contain machine
     code are not normally disassembled.  */
  if (! disassemble_all
      && only_list == NULL
      && ((section->flags & (SEC_CODE | SEC_HAS_CONTENTS))
	  != (SEC_CODE | SEC_HAS_CONTENTS)))
    return;

  if (! process_section_p (section))
    return;

  datasize = bfd_section_size (section);
  if (datasize == 0)
    return;

  if (start_address == (bfd_vma) -1
      || start_address < section->vma)
    addr_offset = 0;
  else
    addr_offset = start_address - section->vma;

  if (stop_address == (bfd_vma) -1)
    stop_offset = datasize / opb;
  else
    {
      if (stop_address < section->vma)
	stop_offset = 0;
      else
	stop_offset = stop_address - section->vma;
      if (stop_offset > datasize / opb)
	stop_offset = datasize / opb;
    }

  if (addr_offset >= stop_offset)
    return;

  /* Decide which set of relocs to use.  Load them if necessary.  */
  paux = (struct objdump_disasm_info *) pinfo->application_data;
  if (paux->dynrelbuf && dump_dynamic_reloc_info)
    {
      rel_pp = paux->dynrelbuf;
      rel_count = paux->dynrelcount;
      /* Dynamic reloc addresses are absolute, non-dynamic are section
	 relative.  REL_OFFSET specifies the reloc address corresponding
	 to the start of this section.  */
      rel_offset = section->vma;
    }
  else
    {
      rel_count = 0;
      rel_pp = NULL;
      rel_offset = 0;

      if ((section->flags & SEC_RELOC) != 0
	  && (dump_reloc_info || pinfo->disassembler_needs_relocs))
	{
	  long relsize;

	  relsize = bfd_get_reloc_upper_bound (abfd, section);
	  if (relsize < 0)
	    bfd_fatal (bfd_get_filename (abfd));

	  if (relsize > 0)
	    {
	      rel_ppstart = rel_pp = (arelent **) xmalloc (relsize);
	      rel_count = bfd_canonicalize_reloc (abfd, section, rel_pp, syms);
	      if (rel_count < 0)
		bfd_fatal (bfd_get_filename (abfd));

	      /* Sort the relocs by address.  */
	      qsort (rel_pp, rel_count, sizeof (arelent *), compare_relocs);
	    }
	}
    }
  rel_ppend = rel_pp + rel_count;

  if (!bfd_malloc_and_get_section (abfd, section, &data))
    {
      non_fatal (_("Reading section %s failed because: %s"),
		 section->name, bfd_errmsg (bfd_get_error ()));
      return;
    }

  pinfo->buffer = data;
  pinfo->buffer_vma = section->vma;
  pinfo->buffer_length = datasize;
  pinfo->section = section;

  /* Sort the symbols into value and section order.  */
  compare_section = section;
  qsort (sorted_syms, sorted_symcount, sizeof (asymbol *), compare_symbols);

  /* Skip over the relocs belonging to addresses below the
     start address.  */
  while (rel_pp < rel_ppend
	 && (*rel_pp)->address < rel_offset + addr_offset)
    ++rel_pp;

  printf (_("\nDisassembly of section %s:\n"), sanitize_string (section->name));

  /* Find the nearest symbol forwards from our current position.  */
  paux->require_sec = TRUE;
  sym = (asymbol *) find_symbol_for_address (section->vma + addr_offset,
                                             (struct disassemble_info *) inf,
                                             &place);
  paux->require_sec = FALSE;

  /* PR 9774: If the target used signed addresses then we must make
     sure that we sign extend the value that we calculate for 'addr'
     in the loop below.  */
  if (bfd_get_flavour (abfd) == bfd_target_elf_flavour
      && (bed = get_elf_backend_data (abfd)) != NULL
      && bed->sign_extend_vma)
    sign_adjust = (bfd_vma) 1 << (bed->s->arch_size - 1);

  /* Disassemble a block of instructions up to the address associated with
     the symbol we have just found.  Then print the symbol and find the
     next symbol on.  Repeat until we have disassembled the entire section
     or we have reached the end of the address range we are interested in.  */
  do_print = paux->symbol == NULL;
  loop_until = stop_offset_reached;

  while (addr_offset < stop_offset)
    {
      bfd_vma addr;
      asymbol *nextsym;
      bfd_vma nextstop_offset;
      bfd_boolean insns;

      addr = section->vma + addr_offset;
      addr = ((addr & ((sign_adjust << 1) - 1)) ^ sign_adjust) - sign_adjust;

      if (sym != NULL && bfd_asymbol_value (sym) <= addr)
	{
	  int x;

	  for (x = place;
	       (x < sorted_symcount
		&& (bfd_asymbol_value (sorted_syms[x]) <= addr));
	       ++x)
	    continue;

	  pinfo->symbols = sorted_syms + place;
	  pinfo->num_symbols = x - place;
	  pinfo->symtab_pos = place;
	}
      else
	{
	  pinfo->symbols = NULL;
	  pinfo->num_symbols = 0;
	  pinfo->symtab_pos = -1;
	}

      /* If we are only disassembling from a specific symbol,
	 check to see if we should start or stop displaying.  */
      if (sym && paux->symbol)
	{
	  if (do_print)
	    {
	      /* See if we should stop printing.  */
	      switch (loop_until)
		{
		case function_sym:
		  if (sym->flags & BSF_FUNCTION)
		    do_print = FALSE;
		  break;

		case stop_offset_reached:
		  /* Handled by the while loop.  */
		  break;

		case next_sym:
		  /* FIXME: There is an implicit assumption here
		     that the name of sym is different from
		     paux->symbol.  */
		  if (! bfd_is_local_label (abfd, sym))
		    do_print = FALSE;
		  break;
		}
	    }
	  else
	    {
	      const char * name = bfd_asymbol_name (sym);
	      char * alloc = NULL;

	      if (do_demangle && name[0] != '\0')
		{
		  /* Demangle the name.  */
		  alloc = bfd_demangle (abfd, name, demangle_flags);
		  if (alloc != NULL)
		    name = alloc;
		}

	      /* We are not currently printing.  Check to see
		 if the current symbol matches the requested symbol.  */
	      if (streq (name, paux->symbol))
		{
		  do_print = TRUE;

		  if (sym->flags & BSF_FUNCTION)
		    {
		      if (bfd_get_flavour (abfd) == bfd_target_elf_flavour
			  && ((elf_symbol_type *) sym)->internal_elf_sym.st_size > 0)
			{
			  /* Sym is a function symbol with a size associated
			     with it.  Turn on automatic disassembly for the
			     next VALUE bytes.  */
			  stop_offset = addr_offset
			    + ((elf_symbol_type *) sym)->internal_elf_sym.st_size;
			  loop_until = stop_offset_reached;
			}
		      else
			{
			  /* Otherwise we need to tell the loop heuristic to
			     loop until the next function symbol is encountered.  */
			  loop_until = function_sym;
			}
		    }
		  else
		    {
		      /* Otherwise loop until the next symbol is encountered.  */
		      loop_until = next_sym;
		    }
		}

	      free (alloc);
	    }
	}

      if (! prefix_addresses && do_print)
	{
	  pinfo->fprintf_func (pinfo->stream, "\n");
	  objdump_print_addr_with_sym (abfd, section, sym, addr,
				       pinfo, FALSE);
	  pinfo->fprintf_func (pinfo->stream, ":\n");
	}

      if (sym != NULL && bfd_asymbol_value (sym) > addr)
	nextsym = sym;
      else if (sym == NULL)
	nextsym = NULL;
      else
	{
#define is_valid_next_sym(SYM) \
  (strcmp (bfd_section_name ((SYM)->section), bfd_section_name (section)) == 0 \
   && (bfd_asymbol_value (SYM) > bfd_asymbol_value (sym)) \
   && pinfo->symbol_is_valid (SYM, pinfo))

	  /* Search forward for the next appropriate symbol in
	     SECTION.  Note that all the symbols are sorted
	     together into one big array, and that some sections
	     may have overlapping addresses.  */
	  while (place < sorted_symcount
		 && ! is_valid_next_sym (sorted_syms [place]))
	    ++place;

	  if (place >= sorted_symcount)
	    nextsym = NULL;
	  else
	    nextsym = sorted_syms[place];
	}

      if (sym != NULL && bfd_asymbol_value (sym) > addr)
	nextstop_offset = bfd_asymbol_value (sym) - section->vma;
      else if (nextsym == NULL)
	nextstop_offset = stop_offset;
      else
	nextstop_offset = bfd_asymbol_value (nextsym) - section->vma;

      if (nextstop_offset > stop_offset
	  || nextstop_offset <= addr_offset)
	nextstop_offset = stop_offset;

      /* If a symbol is explicitly marked as being an object
	 rather than a function, just dump the bytes without
	 disassembling them.  */
      if (disassemble_all
	  || sym == NULL
	  || sym->section != section
	  || bfd_asymbol_value (sym) > addr
	  || ((sym->flags & BSF_OBJECT) == 0
	      && (strstr (bfd_asymbol_name (sym), "gnu_compiled")
		  == NULL)
	      && (strstr (bfd_asymbol_name (sym), "gcc2_compiled")
		  == NULL))
	  || (sym->flags & BSF_FUNCTION) != 0)
	insns = TRUE;
      else
	insns = FALSE;

      if (do_print)
	{
	  /* Resolve symbol name.  */
	  if (visualize_jumps && abfd && sym && sym->name)
	    {
	      struct disassemble_info di;
	      SFILE sf;

	      sf.alloc = strlen (sym->name) + 40;
	      sf.buffer = (char*) xmalloc (sf.alloc);
	      sf.pos = 0;
	      di.fprintf_func = (fprintf_ftype) objdump_sprintf;
	      di.stream = &sf;

	      objdump_print_symname (abfd, &di, sym);

	      /* Fetch jump information.  */
	      detected_jumps = disassemble_jumps
		(pinfo, paux->disassemble_fn,
		 addr_offset, nextstop_offset,
		 rel_offset, &rel_pp, rel_ppend);

	      /* Free symbol name.  */
	      free (sf.buffer);
	    }

	  /* Add jumps to output.  */
	  disassemble_bytes (pinfo, paux->disassemble_fn, insns, data,
			     addr_offset, nextstop_offset,
			     rel_offset, &rel_pp, rel_ppend);

	  /* Free jumps.  */
	  while (detected_jumps)
	    {
	      detected_jumps = jump_info_free (detected_jumps);
	    }
	}

      addr_offset = nextstop_offset;
      sym = nextsym;
    }

  free (data);

  if (rel_ppstart != NULL)
    free (rel_ppstart);
}

/* Disassemble the contents of an object file.  */

static void
disassemble_data (bfd *abfd)
{
  struct disassemble_info disasm_info;
  struct objdump_disasm_info aux;
  long i;

  print_files = NULL;
  prev_functionname = NULL;
  prev_line = -1;
  prev_discriminator = 0;

  /* We make a copy of syms to sort.  We don't want to sort syms
     because that will screw up the relocs.  */
  sorted_symcount = symcount ? symcount : dynsymcount;
  sorted_syms = (asymbol **) xmalloc ((sorted_symcount + synthcount)
                                      * sizeof (asymbol *));
  memcpy (sorted_syms, symcount ? syms : dynsyms,
	  sorted_symcount * sizeof (asymbol *));

  sorted_symcount = remove_useless_symbols (sorted_syms, sorted_symcount);

  for (i = 0; i < synthcount; ++i)
    {
      sorted_syms[sorted_symcount] = synthsyms + i;
      ++sorted_symcount;
    }

  init_disassemble_info (&disasm_info, stdout, (fprintf_ftype) fprintf);

  disasm_info.application_data = (void *) &aux;
  aux.abfd = abfd;
  aux.require_sec = FALSE;
  aux.dynrelbuf = NULL;
  aux.dynrelcount = 0;
  aux.reloc = NULL;
  aux.symbol = disasm_sym;

  disasm_info.print_address_func = objdump_print_address;
  disasm_info.symbol_at_address_func = objdump_symbol_at_address;

  if (machine != NULL)
    {
      const bfd_arch_info_type *inf = bfd_scan_arch (machine);

      if (inf == NULL)
	fatal (_("can't use supplied machine %s"), machine);

      abfd->arch_info = inf;
    }

  if (endian != BFD_ENDIAN_UNKNOWN)
    {
      struct bfd_target *xvec;

      xvec = (struct bfd_target *) xmalloc (sizeof (struct bfd_target));
      memcpy (xvec, abfd->xvec, sizeof (struct bfd_target));
      xvec->byteorder = endian;
      abfd->xvec = xvec;
    }

  /* Use libopcodes to locate a suitable disassembler.  */
  aux.disassemble_fn = disassembler (bfd_get_arch (abfd),
				     bfd_big_endian (abfd),
				     bfd_get_mach (abfd), abfd);
  if (!aux.disassemble_fn)
    {
      non_fatal (_("can't disassemble for architecture %s\n"),
		 bfd_printable_arch_mach (bfd_get_arch (abfd), 0));
      exit_status = 1;
      return;
    }

  disasm_info.flavour = bfd_get_flavour (abfd);
  disasm_info.arch = bfd_get_arch (abfd);
  disasm_info.mach = bfd_get_mach (abfd);
  disasm_info.disassembler_options = disassembler_options;
  disasm_info.octets_per_byte = bfd_octets_per_byte (abfd, NULL);
  disasm_info.skip_zeroes = DEFAULT_SKIP_ZEROES;
  disasm_info.skip_zeroes_at_end = DEFAULT_SKIP_ZEROES_AT_END;
  disasm_info.disassembler_needs_relocs = FALSE;

  if (bfd_big_endian (abfd))
    disasm_info.display_endian = disasm_info.endian = BFD_ENDIAN_BIG;
  else if (bfd_little_endian (abfd))
    disasm_info.display_endian = disasm_info.endian = BFD_ENDIAN_LITTLE;
  else
    /* ??? Aborting here seems too drastic.  We could default to big or little
       instead.  */
    disasm_info.endian = BFD_ENDIAN_UNKNOWN;

  /* Allow the target to customize the info structure.  */
  disassemble_init_for_target (& disasm_info);

  /* Pre-load the dynamic relocs as we may need them during the disassembly.  */
    {
      long relsize = bfd_get_dynamic_reloc_upper_bound (abfd);

      if (relsize < 0 && dump_dynamic_reloc_info)
	bfd_fatal (bfd_get_filename (abfd));

      if (relsize > 0)
	{
	  aux.dynrelbuf = (arelent **) xmalloc (relsize);
	  aux.dynrelcount = bfd_canonicalize_dynamic_reloc (abfd,
							    aux.dynrelbuf,
							    dynsyms);
	  if (aux.dynrelcount < 0)
	    bfd_fatal (bfd_get_filename (abfd));

	  /* Sort the relocs by address.  */
	  qsort (aux.dynrelbuf, aux.dynrelcount, sizeof (arelent *),
		 compare_relocs);
	}
    }
  disasm_info.symtab = sorted_syms;
  disasm_info.symtab_size = sorted_symcount;

  bfd_map_over_sections (abfd, disassemble_section, & disasm_info);

  if (aux.dynrelbuf != NULL)
    free (aux.dynrelbuf);
  free (sorted_syms);
  disassemble_free_target (&disasm_info);
}

static bfd_boolean
load_specific_debug_section (enum dwarf_section_display_enum debug,
			     asection *sec, void *file)
{
  struct dwarf_section *section = &debug_displays [debug].section;
  bfd *abfd = (bfd *) file;
  bfd_byte *contents;
  bfd_size_type amt;
  size_t alloced;

  if (section->start != NULL)
    {
      /* If it is already loaded, do nothing.  */
      if (streq (section->filename, bfd_get_filename (abfd)))
	return TRUE;
      free (section->start);
    }

  section->filename = bfd_get_filename (abfd);
  section->reloc_info = NULL;
  section->num_relocs = 0;
  section->address = bfd_section_vma (sec);
  section->user_data = sec;
  section->size = bfd_section_size (sec);
  /* PR 24360: On 32-bit hosts sizeof (size_t) < sizeof (bfd_size_type). */
  alloced = amt = section->size + 1;
  if (alloced != amt || alloced == 0)
    {
      section->start = NULL;
      free_debug_section (debug);
      printf (_("\nSection '%s' has an invalid size: %#llx.\n"),
	      sanitize_string (section->name),
	      (unsigned long long) section->size);
      return FALSE;
    }
  section->start = contents = malloc (alloced);
  if (section->start == NULL
      || !bfd_get_full_section_contents (abfd, sec, &contents))
    {
      free_debug_section (debug);
      printf (_("\nCan't get contents for section '%s'.\n"),
	      sanitize_string (section->name));
      return FALSE;
    }
  /* Ensure any string section has a terminating NUL.  */
  section->start[section->size] = 0;

  if ((abfd->flags & (EXEC_P | DYNAMIC)) == 0
      && debug_displays [debug].relocate)
    {
      long         reloc_size;
      bfd_boolean  ret;

      bfd_cache_section_contents (sec, section->start);

      ret = bfd_simple_get_relocated_section_contents (abfd,
						       sec,
						       section->start,
						       syms) != NULL;

      if (! ret)
        {
          free_debug_section (debug);
          printf (_("\nCan't get contents for section '%s'.\n"),
	          sanitize_string (section->name));
          return FALSE;
        }

      reloc_size = bfd_get_reloc_upper_bound (abfd, sec);
      if (reloc_size > 0)
	{
	  unsigned long reloc_count;
	  arelent **relocs;

	  relocs = (arelent **) xmalloc (reloc_size);

	  reloc_count = bfd_canonicalize_reloc (abfd, sec, relocs, NULL);
	  if (reloc_count == 0)
	    free (relocs);
	  else
	    {
	      section->reloc_info = relocs;
	      section->num_relocs = reloc_count;
	    }
	}
    }

  return TRUE;
}

bfd_boolean
reloc_at (struct dwarf_section * dsec, dwarf_vma offset)
{
  arelent ** relocs;
  arelent * rp;

  if (dsec == NULL || dsec->reloc_info == NULL)
    return FALSE;

  relocs = (arelent **) dsec->reloc_info;

  for (; (rp = * relocs) != NULL; ++ relocs)
    if (rp->address == offset)
      return TRUE;

  return FALSE;
}

bfd_boolean
load_debug_section (enum dwarf_section_display_enum debug, void *file)
{
  struct dwarf_section *section = &debug_displays [debug].section;
  bfd *abfd = (bfd *) file;
  asection *sec;

  /* If it is already loaded, do nothing.  */
  if (section->start != NULL)
    {
      if (streq (section->filename, bfd_get_filename (abfd)))
	return TRUE;
    }

  /* Locate the debug section.  */
  sec = bfd_get_section_by_name (abfd, section->uncompressed_name);
  if (sec != NULL)
    section->name = section->uncompressed_name;
  else
    {
      sec = bfd_get_section_by_name (abfd, section->compressed_name);
      if (sec != NULL)
        section->name = section->compressed_name;
    }
  if (sec == NULL)
    return FALSE;

  return load_specific_debug_section (debug, sec, file);
}

void
free_debug_section (enum dwarf_section_display_enum debug)
{
  struct dwarf_section *section = &debug_displays [debug].section;

  if (section->start == NULL)
    return;

  /* PR 17512: file: 0f67f69d.  */
  if (section->user_data != NULL)
    {
      asection * sec = (asection *) section->user_data;

      /* If we are freeing contents that are also pointed to by the BFD
	 library's section structure then make sure to update those pointers
	 too.  Otherwise, the next time we try to load data for this section
	 we can end up using a stale pointer.  */
      if (section->start == sec->contents)
	{
	  sec->contents = NULL;
	  sec->flags &= ~ SEC_IN_MEMORY;
	  sec->compress_status = COMPRESS_SECTION_NONE;
	}
    }

  free ((char *) section->start);
  section->start = NULL;
  section->address = 0;
  section->size = 0;
}

void
close_debug_file (void * file)
{
  bfd * abfd = (bfd *) file;

  bfd_close (abfd);
}

void *
open_debug_file (const char * pathname)
{
  bfd * data;

  data = bfd_openr (pathname, NULL);
  if (data == NULL)
    return NULL;

  if (! bfd_check_format (data, bfd_object))
    return NULL;
  
  return data;
}

#if HAVE_LIBDEBUGINFOD
/* Return a hex string represention of the build-id.  */

unsigned char *
get_build_id (void * data)
{
  unsigned i;
  char * build_id_str;
  bfd * abfd = (bfd *) data;
  const struct bfd_build_id * build_id;

  build_id = abfd->build_id;
  if (build_id == NULL)
    return NULL;

  build_id_str = malloc (build_id->size * 2 + 1);
  if (build_id_str == NULL)
    return NULL;

  for (i = 0; i < build_id->size; i++)
    sprintf (build_id_str + (i * 2), "%02x", build_id->data[i]);
  build_id_str[build_id->size * 2] = '\0';

  return (unsigned char *)build_id_str;
}
#endif /* HAVE_LIBDEBUGINFOD */

static void
dump_dwarf_section (bfd *abfd, asection *section,
		    void *arg ATTRIBUTE_UNUSED)
{
  const char *name = bfd_section_name (section);
  const char *match;
  int i;

  if (CONST_STRNEQ (name, ".gnu.linkonce.wi."))
    match = ".debug_info";
  else
    match = name;

  for (i = 0; i < max; i++)
    if ((strcmp (debug_displays [i].section.uncompressed_name, match) == 0
	 || strcmp (debug_displays [i].section.compressed_name, match) == 0)
	&& debug_displays [i].enabled != NULL
	&& *debug_displays [i].enabled)
      {
	struct dwarf_section *sec = &debug_displays [i].section;

	if (strcmp (sec->uncompressed_name, match) == 0)
	  sec->name = sec->uncompressed_name;
	else
	  sec->name = sec->compressed_name;
	if (load_specific_debug_section ((enum dwarf_section_display_enum) i,
                                         section, abfd))
	  {
	    debug_displays [i].display (sec, abfd);

	    if (i != info && i != abbrev)
	      free_debug_section ((enum dwarf_section_display_enum) i);
	  }
	break;
      }
}

/* Dump the dwarf debugging information.  */

static void
dump_dwarf (bfd *abfd)
{
  /* The byte_get pointer should have been set at the start of dump_bfd().  */
  if (byte_get == NULL)
    {
      warn (_("File %s does not contain any dwarf debug information\n"),
	    bfd_get_filename (abfd));
      return;
    }

  switch (bfd_get_arch (abfd))
    {
    case bfd_arch_s12z:
      /* S12Z has a 24 bit address space.  But the only known
	 producer of dwarf_info encodes addresses into 32 bits.  */
      eh_addr_size = 4;
      break;

    default:
      eh_addr_size = bfd_arch_bits_per_address (abfd) / 8;
      break;
    }

  init_dwarf_regnames_by_bfd_arch_and_mach (bfd_get_arch (abfd),
					    bfd_get_mach (abfd));

  bfd_map_over_sections (abfd, dump_dwarf_section, NULL);
}

/* Read ABFD's stabs section STABSECT_NAME, and return a pointer to
   it.  Return NULL on failure.   */

static bfd_byte *
read_section_stabs (bfd *abfd, const char *sect_name, bfd_size_type *size_ptr,
		    bfd_size_type *entsize_ptr)
{
  asection *stabsect;
  bfd_byte *contents;

  stabsect = bfd_get_section_by_name (abfd, sect_name);
  if (stabsect == NULL)
    {
      printf (_("No %s section present\n\n"),
	      sanitize_string (sect_name));
      return FALSE;
    }

  if (!bfd_malloc_and_get_section (abfd, stabsect, &contents))
    {
      non_fatal (_("reading %s section of %s failed: %s"),
		 sect_name, bfd_get_filename (abfd),
		 bfd_errmsg (bfd_get_error ()));
      exit_status = 1;
      free (contents);
      return NULL;
    }

  *size_ptr = bfd_section_size (stabsect);
  if (entsize_ptr)
    *entsize_ptr = stabsect->entsize;

  return contents;
}

/* Stabs entries use a 12 byte format:
     4 byte string table index
     1 byte stab type
     1 byte stab other field
     2 byte stab desc field
     4 byte stab value
   FIXME: This will have to change for a 64 bit object format.  */

#define STRDXOFF  (0)
#define TYPEOFF   (4)
#define OTHEROFF  (5)
#define DESCOFF   (6)
#define VALOFF    (8)
#define STABSIZE (12)

/* Print ABFD's stabs section STABSECT_NAME (in `stabs'),
   using string table section STRSECT_NAME (in `strtab').  */

static void
print_section_stabs (bfd *abfd,
		     const char *stabsect_name,
		     unsigned *string_offset_ptr)
{
  int i;
  unsigned file_string_table_offset = 0;
  unsigned next_file_string_table_offset = *string_offset_ptr;
  bfd_byte *stabp, *stabs_end;

  stabp = stabs;
  stabs_end = stabp + stab_size;

  printf (_("Contents of %s section:\n\n"), sanitize_string (stabsect_name));
  printf ("Symnum n_type n_othr n_desc n_value  n_strx String\n");

  /* Loop through all symbols and print them.

     We start the index at -1 because there is a dummy symbol on
     the front of stabs-in-{coff,elf} sections that supplies sizes.  */
  for (i = -1; stabp <= stabs_end - STABSIZE; stabp += STABSIZE, i++)
    {
      const char *name;
      unsigned long strx;
      unsigned char type, other;
      unsigned short desc;
      bfd_vma value;

      strx = bfd_h_get_32 (abfd, stabp + STRDXOFF);
      type = bfd_h_get_8 (abfd, stabp + TYPEOFF);
      other = bfd_h_get_8 (abfd, stabp + OTHEROFF);
      desc = bfd_h_get_16 (abfd, stabp + DESCOFF);
      value = bfd_h_get_32 (abfd, stabp + VALOFF);

      printf ("\n%-6d ", i);
      /* Either print the stab name, or, if unnamed, print its number
	 again (makes consistent formatting for tools like awk).  */
      name = bfd_get_stab_name (type);
      if (name != NULL)
	printf ("%-6s", sanitize_string (name));
      else if (type == N_UNDF)
	printf ("HdrSym");
      else
	printf ("%-6d", type);
      printf (" %-6d %-6d ", other, desc);
      bfd_printf_vma (abfd, value);
      printf (" %-6lu", strx);

      /* Symbols with type == 0 (N_UNDF) specify the length of the
	 string table associated with this file.  We use that info
	 to know how to relocate the *next* file's string table indices.  */
      if (type == N_UNDF)
	{
	  file_string_table_offset = next_file_string_table_offset;
	  next_file_string_table_offset += value;
	}
      else
	{
	  bfd_size_type amt = strx + file_string_table_offset;

	  /* Using the (possibly updated) string table offset, print the
	     string (if any) associated with this symbol.  */
	  if (amt < stabstr_size)
	    /* PR 17512: file: 079-79389-0.001:0.1.
	       FIXME: May need to sanitize this string before displaying.  */
	    printf (" %.*s", (int)(stabstr_size - amt), strtab + amt);
	  else
	    printf (" *");
	}
    }
  printf ("\n\n");
  *string_offset_ptr = next_file_string_table_offset;
}

typedef struct
{
  const char * section_name;
  const char * string_section_name;
  unsigned string_offset;
}
stab_section_names;

static void
find_stabs_section (bfd *abfd, asection *section, void *names)
{
  int len;
  stab_section_names * sought = (stab_section_names *) names;

  /* Check for section names for which stabsect_name is a prefix, to
     handle .stab.N, etc.  */
  len = strlen (sought->section_name);

  /* If the prefix matches, and the files section name ends with a
     nul or a digit, then we match.  I.e., we want either an exact
     match or a section followed by a number.  */
  if (strncmp (sought->section_name, section->name, len) == 0
      && (section->name[len] == 0
	  || (section->name[len] == '.' && ISDIGIT (section->name[len + 1]))))
    {
      if (strtab == NULL)
	strtab = read_section_stabs (abfd, sought->string_section_name,
				     &stabstr_size, NULL);

      if (strtab)
	{
	  stabs = read_section_stabs (abfd, section->name, &stab_size, NULL);
	  if (stabs)
	    print_section_stabs (abfd, section->name, &sought->string_offset);
	}
    }
}

static void
dump_stabs_section (bfd *abfd, char *stabsect_name, char *strsect_name)
{
  stab_section_names s;

  s.section_name = stabsect_name;
  s.string_section_name = strsect_name;
  s.string_offset = 0;

  bfd_map_over_sections (abfd, find_stabs_section, & s);

  free (strtab);
  strtab = NULL;
}

/* Dump the any sections containing stabs debugging information.  */

static void
dump_stabs (bfd *abfd)
{
  dump_stabs_section (abfd, ".stab", ".stabstr");
  dump_stabs_section (abfd, ".stab.excl", ".stab.exclstr");
  dump_stabs_section (abfd, ".stab.index", ".stab.indexstr");

  /* For Darwin.  */
  dump_stabs_section (abfd, "LC_SYMTAB.stabs", "LC_SYMTAB.stabstr");

  dump_stabs_section (abfd, "$GDB_SYMBOLS$", "$GDB_STRINGS$");
}

static void
dump_bfd_header (bfd *abfd)
{
  char *comma = "";

  printf (_("architecture: %s, "),
	  bfd_printable_arch_mach (bfd_get_arch (abfd),
				   bfd_get_mach (abfd)));
  printf (_("flags 0x%08x:\n"), abfd->flags & ~BFD_FLAGS_FOR_BFD_USE_MASK);

#define PF(x, y)    if (abfd->flags & x) {printf ("%s%s", comma, y); comma=", ";}
  PF (HAS_RELOC, "HAS_RELOC");
  PF (EXEC_P, "EXEC_P");
  PF (HAS_LINENO, "HAS_LINENO");
  PF (HAS_DEBUG, "HAS_DEBUG");
  PF (HAS_SYMS, "HAS_SYMS");
  PF (HAS_LOCALS, "HAS_LOCALS");
  PF (DYNAMIC, "DYNAMIC");
  PF (WP_TEXT, "WP_TEXT");
  PF (D_PAGED, "D_PAGED");
  PF (BFD_IS_RELAXABLE, "BFD_IS_RELAXABLE");
  printf (_("\nstart address 0x"));
  bfd_printf_vma (abfd, abfd->start_address);
  printf ("\n");
}


/* Formatting callback function passed to ctf_dump.  Returns either the pointer
   it is passed, or a pointer to newly-allocated storage, in which case
   dump_ctf() will free it when it no longer needs it.  */

static char *
dump_ctf_indent_lines (ctf_sect_names_t sect ATTRIBUTE_UNUSED,
		       char *s, void *arg)
{
  const char *blanks = arg;
  char *new_s;

  if (asprintf (&new_s, "%s%s", blanks, s) < 0)
    return s;
  return new_s;
}

/* Make a ctfsect suitable for ctf_bfdopen_ctfsect().  */
static ctf_sect_t
make_ctfsect (const char *name, bfd_byte *data,
	      bfd_size_type size)
{
  ctf_sect_t ctfsect;

  ctfsect.cts_name = name;
  ctfsect.cts_entsize = 1;
  ctfsect.cts_size = size;
  ctfsect.cts_data = data;

  return ctfsect;
}

/* Dump one CTF archive member.  */

static int
dump_ctf_archive_member (ctf_file_t *ctf, const char *name, void *arg)
{
  ctf_file_t *parent = (ctf_file_t *) arg;
  const char *things[] = {"Header", "Labels", "Data objects",
			  "Function objects", "Variables", "Types", "Strings",
			  ""};
  const char **thing;
  size_t i;

  /* Only print out the name of non-default-named archive members.
     The name .ctf appears everywhere, even for things that aren't
     really archives, so printing it out is liable to be confusing.

     The parent, if there is one, is the default-owned archive member:
     avoid importing it into itself.  (This does no harm, but looks
     confusing.)  */

  if (strcmp (name, ".ctf") != 0)
    {
      printf (_("\nCTF archive member: %s:\n"), sanitize_string (name));
      ctf_import (ctf, parent);
    }

  for (i = 0, thing = things; *thing[0]; thing++, i++)
    {
      ctf_dump_state_t *s = NULL;
      char *item;

      printf ("\n  %s:\n", *thing);
      while ((item = ctf_dump (ctf, &s, i, dump_ctf_indent_lines,
			       (void *) "    ")) != NULL)
	{
	  printf ("%s\n", item);
	  free (item);
	}

      if (ctf_errno (ctf))
	{
	  non_fatal (_("Iteration failed: %s, %s\n"), *thing,
		   ctf_errmsg (ctf_errno (ctf)));
	  break;
	}
    }
  return 0;
}

/* Dump the CTF debugging information.  */

static void
dump_ctf (bfd *abfd, const char *sect_name, const char *parent_name)
{
  ctf_archive_t *ctfa, *parenta = NULL, *lookparent;
  bfd_byte *ctfdata, *parentdata = NULL;
  bfd_size_type ctfsize, parentsize;
  ctf_sect_t ctfsect;
  ctf_file_t *parent = NULL;
  int err;

  if ((ctfdata = read_section_stabs (abfd, sect_name, &ctfsize, NULL)) == NULL)
      bfd_fatal (bfd_get_filename (abfd));

  if (parent_name
      && (parentdata = read_section_stabs (abfd, parent_name, &parentsize,
					   NULL)) == NULL)
      bfd_fatal (bfd_get_filename (abfd));

  /* Load the CTF file and dump it.  */

  ctfsect = make_ctfsect (sect_name, ctfdata, ctfsize);
  if ((ctfa = ctf_bfdopen_ctfsect (abfd, &ctfsect, &err)) == NULL)
    {
      non_fatal (_("CTF open failure: %s\n"), ctf_errmsg (err));
      bfd_fatal (bfd_get_filename (abfd));
    }

  if (parentdata)
    {
      ctfsect = make_ctfsect (parent_name, parentdata, parentsize);
      if ((parenta = ctf_bfdopen_ctfsect (abfd, &ctfsect, &err)) == NULL)
	{
	  non_fatal (_("CTF open failure: %s\n"), ctf_errmsg (err));
	  bfd_fatal (bfd_get_filename (abfd));
	}

      lookparent = parenta;
    }
  else
    lookparent = ctfa;

  /* Assume that the applicable parent archive member is the default one.
     (This is what all known implementations are expected to do, if they
     put CTFs and their parents in archives together.)  */
  if ((parent = ctf_arc_open_by_name (lookparent, NULL, &err)) == NULL)
    {
      non_fatal (_("CTF open failure: %s\n"), ctf_errmsg (err));
      bfd_fatal (bfd_get_filename (abfd));
    }

  printf (_("Contents of CTF section %s:\n"), sanitize_string (sect_name));

  ctf_archive_iter (ctfa, dump_ctf_archive_member, parent);
  ctf_file_close (parent);
  ctf_close (ctfa);
  ctf_close (parenta);
  free (parentdata);
  free (ctfdata);
}


static void
dump_bfd_private_header (bfd *abfd)
{
  if (!bfd_print_private_bfd_data (abfd, stdout))
    non_fatal (_("warning: private headers incomplete: %s"),
	       bfd_errmsg (bfd_get_error ()));
}

static void
dump_target_specific (bfd *abfd)
{
  const struct objdump_private_desc * const *desc;
  struct objdump_private_option *opt;
  char *e, *b;

  /* Find the desc.  */
  for (desc = objdump_private_vectors; *desc != NULL; desc++)
    if ((*desc)->filter (abfd))
      break;

  if (*desc == NULL)
    {
      non_fatal (_("option -P/--private not supported by this file"));
      return;
    }

  /* Clear all options.  */
  for (opt = (*desc)->options; opt->name; opt++)
    opt->selected = FALSE;

  /* Decode options.  */
  b = dump_private_options;
  do
    {
      e = strchr (b, ',');

      if (e)
        *e = 0;

      for (opt = (*desc)->options; opt->name; opt++)
        if (strcmp (opt->name, b) == 0)
          {
            opt->selected = TRUE;
            break;
          }
      if (opt->name == NULL)
        non_fatal (_("target specific dump '%s' not supported"), b);

      if (e)
        {
          *e = ',';
          b = e + 1;
        }
    }
  while (e != NULL);

  /* Dump.  */
  (*desc)->dump (abfd);
}

/* Display a section in hexadecimal format with associated characters.
   Each line prefixed by the zero padded address.  */

static void
dump_section (bfd *abfd, asection *section, void *dummy ATTRIBUTE_UNUSED)
{
  bfd_byte *data = NULL;
  bfd_size_type datasize;
  bfd_vma addr_offset;
  bfd_vma start_offset;
  bfd_vma stop_offset;
  unsigned int opb = bfd_octets_per_byte (abfd, section);
  /* Bytes per line.  */
  const int onaline = 16;
  char buf[64];
  int count;
  int width;

  if ((section->flags & SEC_HAS_CONTENTS) == 0)
    return;

  if (! process_section_p (section))
    return;

  if ((datasize = bfd_section_size (section)) == 0)
    return;

  /* Compute the address range to display.  */
  if (start_address == (bfd_vma) -1
      || start_address < section->vma)
    start_offset = 0;
  else
    start_offset = start_address - section->vma;

  if (stop_address == (bfd_vma) -1)
    stop_offset = datasize / opb;
  else
    {
      if (stop_address < section->vma)
	stop_offset = 0;
      else
	stop_offset = stop_address - section->vma;

      if (stop_offset > datasize / opb)
	stop_offset = datasize / opb;
    }

  if (start_offset >= stop_offset)
    return;

  printf (_("Contents of section %s:"), sanitize_string (section->name));
  if (display_file_offsets)
    printf (_("  (Starting at file offset: 0x%lx)"),
	    (unsigned long) (section->filepos + start_offset));
  printf ("\n");

  if (!bfd_get_full_section_contents (abfd, section, &data))
    {
      non_fatal (_("Reading section %s failed because: %s"),
		 section->name, bfd_errmsg (bfd_get_error ()));
      return;
    }

  width = 4;

  bfd_sprintf_vma (abfd, buf, start_offset + section->vma);
  if (strlen (buf) >= sizeof (buf))
    abort ();

  count = 0;
  while (buf[count] == '0' && buf[count+1] != '\0')
    count++;
  count = strlen (buf) - count;
  if (count > width)
    width = count;

  bfd_sprintf_vma (abfd, buf, stop_offset + section->vma - 1);
  if (strlen (buf) >= sizeof (buf))
    abort ();

  count = 0;
  while (buf[count] == '0' && buf[count+1] != '\0')
    count++;
  count = strlen (buf) - count;
  if (count > width)
    width = count;

  for (addr_offset = start_offset;
       addr_offset < stop_offset; addr_offset += onaline / opb)
    {
      bfd_size_type j;

      bfd_sprintf_vma (abfd, buf, (addr_offset + section->vma));
      count = strlen (buf);
      if ((size_t) count >= sizeof (buf))
	abort ();

      putchar (' ');
      while (count < width)
	{
	  putchar ('0');
	  count++;
	}
      fputs (buf + count - width, stdout);
      putchar (' ');

      for (j = addr_offset * opb;
	   j < addr_offset * opb + onaline; j++)
	{
	  if (j < stop_offset * opb)
	    printf ("%02x", (unsigned) (data[j]));
	  else
	    printf ("  ");
	  if ((j & 3) == 3)
	    printf (" ");
	}

      printf (" ");
      for (j = addr_offset * opb;
	   j < addr_offset * opb + onaline; j++)
	{
	  if (j >= stop_offset * opb)
	    printf (" ");
	  else
	    printf ("%c", ISPRINT (data[j]) ? data[j] : '.');
	}
      putchar ('\n');
    }
  free (data);
}

/* Actually display the various requested regions.  */

static void
dump_data (bfd *abfd)
{
  bfd_map_over_sections (abfd, dump_section, NULL);
}

/* Should perhaps share code and display with nm?  */

static void
dump_symbols (bfd *abfd ATTRIBUTE_UNUSED, bfd_boolean dynamic)
{
  asymbol **current;
  long max_count;
  long count;

  if (dynamic)
    {
      current = dynsyms;
      max_count = dynsymcount;
      printf ("DYNAMIC SYMBOL TABLE:\n");
    }
  else
    {
      current = syms;
      max_count = symcount;
      printf ("SYMBOL TABLE:\n");
    }

  if (max_count == 0)
    printf (_("no symbols\n"));

  for (count = 0; count < max_count; count++)
    {
      bfd *cur_bfd;

      if (*current == NULL)
	printf (_("no information for symbol number %ld\n"), count);

      else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
	printf (_("could not determine the type of symbol number %ld\n"),
		count);

      else if (process_section_p ((* current)->section)
	       && (dump_special_syms
		   || !bfd_is_target_special_symbol (cur_bfd, *current)))
	{
	  const char *name = (*current)->name;

	  if (do_demangle && name != NULL && *name != '\0')
	    {
	      char *alloc;

	      /* If we want to demangle the name, we demangle it
		 here, and temporarily clobber it while calling
		 bfd_print_symbol.  FIXME: This is a gross hack.  */
	      alloc = bfd_demangle (cur_bfd, name, demangle_flags);
	      if (alloc != NULL)
		(*current)->name = alloc;
	      bfd_print_symbol (cur_bfd, stdout, *current,
				bfd_print_symbol_all);
	      if (alloc != NULL)
		{
		  (*current)->name = name;
		  free (alloc);
		}
	    }
	  else
	    bfd_print_symbol (cur_bfd, stdout, *current,
			      bfd_print_symbol_all);
	  printf ("\n");
	}

      current++;
    }
  printf ("\n\n");
}

static void
dump_reloc_set (bfd *abfd, asection *sec, arelent **relpp, long relcount)
{
  arelent **p;
  char *last_filename, *last_functionname;
  unsigned int last_line;
  unsigned int last_discriminator;

  /* Get column headers lined up reasonably.  */
  {
    static int width;

    if (width == 0)
      {
	char buf[30];

	bfd_sprintf_vma (abfd, buf, (bfd_vma) -1);
	width = strlen (buf) - 7;
      }
    printf ("OFFSET %*s TYPE %*s VALUE \n", width, "", 12, "");
  }

  last_filename = NULL;
  last_functionname = NULL;
  last_line = 0;
  last_discriminator = 0;

  for (p = relpp; relcount && *p != NULL; p++, relcount--)
    {
      arelent *q = *p;
      const char *filename, *functionname;
      unsigned int linenumber;
      unsigned int discriminator;
      const char *sym_name;
      const char *section_name;
      bfd_vma addend2 = 0;

      if (start_address != (bfd_vma) -1
	  && q->address < start_address)
	continue;
      if (stop_address != (bfd_vma) -1
	  && q->address > stop_address)
	continue;

      if (with_line_numbers
	  && sec != NULL
	  && bfd_find_nearest_line_discriminator (abfd, sec, syms, q->address,
                                                  &filename, &functionname,
                                                  &linenumber, &discriminator))
	{
	  if (functionname != NULL
	      && (last_functionname == NULL
		  || strcmp (functionname, last_functionname) != 0))
	    {
	      printf ("%s():\n", sanitize_string (functionname));
	      if (last_functionname != NULL)
		free (last_functionname);
	      last_functionname = xstrdup (functionname);
	    }

	  if (linenumber > 0
	      && (linenumber != last_line
		  || (filename != NULL
		      && last_filename != NULL
		      && filename_cmp (filename, last_filename) != 0)
                  || (discriminator != last_discriminator)))
	    {
              if (discriminator > 0)
                printf ("%s:%u\n", filename == NULL ? "???" :
			sanitize_string (filename), linenumber);
              else
                printf ("%s:%u (discriminator %u)\n",
			filename == NULL ? "???" : sanitize_string (filename),
                        linenumber, discriminator);
	      last_line = linenumber;
	      last_discriminator = discriminator;
	      if (last_filename != NULL)
		free (last_filename);
	      if (filename == NULL)
		last_filename = NULL;
	      else
		last_filename = xstrdup (filename);
	    }
	}

      if (q->sym_ptr_ptr && *q->sym_ptr_ptr)
	{
	  sym_name = (*(q->sym_ptr_ptr))->name;
	  section_name = (*(q->sym_ptr_ptr))->section->name;
	}
      else
	{
	  sym_name = NULL;
	  section_name = NULL;
	}

      bfd_printf_vma (abfd, q->address);
      if (q->howto == NULL)
	printf (" *unknown*         ");
      else if (q->howto->name)
	{
	  const char *name = q->howto->name;

	  /* R_SPARC_OLO10 relocations contain two addends.
	     But because 'arelent' lacks enough storage to
	     store them both, the 64-bit ELF Sparc backend
	     records this as two relocations.  One R_SPARC_LO10
	     and one R_SPARC_13, both pointing to the same
	     address.  This is merely so that we have some
	     place to store both addend fields.

	     Undo this transformation, otherwise the output
	     will be confusing.  */
	  if (abfd->xvec->flavour == bfd_target_elf_flavour
	      && elf_tdata (abfd)->elf_header->e_machine == EM_SPARCV9
	      && relcount > 1
	      && !strcmp (q->howto->name, "R_SPARC_LO10"))
	    {
	      arelent *q2 = *(p + 1);
	      if (q2 != NULL
		  && q2->howto
		  && q->address == q2->address
		  && !strcmp (q2->howto->name, "R_SPARC_13"))
		{
		  name = "R_SPARC_OLO10";
		  addend2 = q2->addend;
		  p++;
		}
	    }
	  printf (" %-16s  ", name);
	}
      else
	printf (" %-16d  ", q->howto->type);

      if (sym_name)
	{
	  objdump_print_symname (abfd, NULL, *q->sym_ptr_ptr);
	}
      else
	{
	  if (section_name == NULL)
	    section_name = "*unknown*";
	  printf ("[%s]", sanitize_string (section_name));
	}

      if (q->addend)
	{
	  bfd_signed_vma addend = q->addend;
	  if (addend < 0)
	    {
	      printf ("-0x");
	      addend = -addend;
	    }
	  else
	    printf ("+0x");
	  bfd_printf_vma (abfd, addend);
	}
      if (addend2)
	{
	  printf ("+0x");
	  bfd_printf_vma (abfd, addend2);
	}

      printf ("\n");
    }

  if (last_filename != NULL)
    free (last_filename);
  if (last_functionname != NULL)
    free (last_functionname);
}

static void
dump_relocs_in_section (bfd *abfd,
			asection *section,
			void *dummy ATTRIBUTE_UNUSED)
{
  arelent **relpp = NULL;
  long relcount;
  long relsize;

  if (   bfd_is_abs_section (section)
      || bfd_is_und_section (section)
      || bfd_is_com_section (section)
      || (! process_section_p (section))
      || ((section->flags & SEC_RELOC) == 0))
    return;

  printf ("RELOCATION RECORDS FOR [%s]:", sanitize_string (section->name));

  relsize = bfd_get_reloc_upper_bound (abfd, section);
  if (relsize == 0)
    {
      printf (" (none)\n\n");
      return;
    }

  if (relsize < 0)
    relcount = relsize;
  else
    {
      relpp = (arelent **) xmalloc (relsize);
      relcount = bfd_canonicalize_reloc (abfd, section, relpp, syms);
    }

  if (relcount < 0)
    {
      printf ("\n");
      non_fatal (_("failed to read relocs in: %s"),
		 sanitize_string (bfd_get_filename (abfd)));
      bfd_fatal (_("error message was"));
    }
  else if (relcount == 0)
    printf (" (none)\n\n");
  else
    {
      printf ("\n");
      dump_reloc_set (abfd, section, relpp, relcount);
      printf ("\n\n");
    }
  free (relpp);
}

static void
dump_relocs (bfd *abfd)
{
  bfd_map_over_sections (abfd, dump_relocs_in_section, NULL);
}

static void
dump_dynamic_relocs (bfd *abfd)
{
  long relsize;
  arelent **relpp;
  long relcount;

  relsize = bfd_get_dynamic_reloc_upper_bound (abfd);
  if (relsize < 0)
    bfd_fatal (bfd_get_filename (abfd));

  printf ("DYNAMIC RELOCATION RECORDS");

  if (relsize == 0)
    printf (" (none)\n\n");
  else
    {
      relpp = (arelent **) xmalloc (relsize);
      relcount = bfd_canonicalize_dynamic_reloc (abfd, relpp, dynsyms);

      if (relcount < 0)
	bfd_fatal (bfd_get_filename (abfd));
      else if (relcount == 0)
	printf (" (none)\n\n");
      else
	{
	  printf ("\n");
	  dump_reloc_set (abfd, NULL, relpp, relcount);
	  printf ("\n\n");
	}
      free (relpp);
    }
}

/* Creates a table of paths, to search for source files.  */

static void
add_include_path (const char *path)
{
  if (path[0] == 0)
    return;
  include_path_count++;
  include_paths = (const char **)
      xrealloc (include_paths, include_path_count * sizeof (*include_paths));
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  if (path[1] == ':' && path[2] == 0)
    path = concat (path, ".", (const char *) 0);
#endif
  include_paths[include_path_count - 1] = path;
}

static void
adjust_addresses (bfd *abfd ATTRIBUTE_UNUSED,
		  asection *section,
		  void *arg)
{
  if ((section->flags & SEC_DEBUGGING) == 0)
    {
      bfd_boolean *has_reloc_p = (bfd_boolean *) arg;
      section->vma += adjust_section_vma;
      if (*has_reloc_p)
	section->lma += adjust_section_vma;
    }
}

/* Return the sign-extended form of an ARCH_SIZE sized VMA.  */

static bfd_vma
sign_extend_address (bfd *abfd ATTRIBUTE_UNUSED,
		     bfd_vma vma,
		     unsigned arch_size)
{
  bfd_vma mask;
  mask = (bfd_vma) 1 << (arch_size - 1);
  return (((vma & ((mask << 1) - 1)) ^ mask) - mask);
}

/* Dump selected contents of ABFD.  */

static void
dump_bfd (bfd *abfd, bfd_boolean is_mainfile)
{
  const struct elf_backend_data * bed;

  if (bfd_big_endian (abfd))
    byte_get = byte_get_big_endian;
  else if (bfd_little_endian (abfd))
    byte_get = byte_get_little_endian;
  else
    byte_get = NULL;

  /* Load any separate debug information files.
     We do this now and without checking do_follow_links because separate
     debug info files may contain symbol tables that we will need when
     displaying information about the main file.  Any memory allocated by
     load_separate_debug_files will be released when we call
     free_debug_memory below.
     
     The test on is_mainfile is there because the chain of separate debug
     info files is a global variable shared by all invocations of dump_bfd.  */
  if (is_mainfile)
    {
      load_separate_debug_files (abfd, bfd_get_filename (abfd));

      /* If asked to do so, recursively dump the separate files.  */
      if (do_follow_links)
	{
	  separate_info * i;

	  for (i = first_separate_info; i != NULL; i = i->next)
	    dump_bfd (i->handle, FALSE);
	}
    }

  /* Adjust user-specified start and stop limits for targets that use
     signed addresses.  */
  if (bfd_get_flavour (abfd) == bfd_target_elf_flavour
      && (bed = get_elf_backend_data (abfd)) != NULL
      && bed->sign_extend_vma)
    {
      start_address = sign_extend_address (abfd, start_address,
					   bed->s->arch_size);
      stop_address = sign_extend_address (abfd, stop_address,
					  bed->s->arch_size);
    }

  /* If we are adjusting section VMA's, change them all now.  Changing
     the BFD information is a hack.  However, we must do it, or
     bfd_find_nearest_line will not do the right thing.  */
  if (adjust_section_vma != 0)
    {
      bfd_boolean has_reloc = (abfd->flags & HAS_RELOC);
      bfd_map_over_sections (abfd, adjust_addresses, &has_reloc);
    }

  if (! dump_debugging_tags && ! suppress_bfd_header)
    printf (_("\n%s:     file format %s\n"),
	    sanitize_string (bfd_get_filename (abfd)),
	    abfd->xvec->name);
  if (dump_ar_hdrs)
    print_arelt_descr (stdout, abfd, TRUE, FALSE);
  if (dump_file_header)
    dump_bfd_header (abfd);
  if (dump_private_headers)
    dump_bfd_private_header (abfd);
  if (dump_private_options != NULL)
    dump_target_specific (abfd);
  if (! dump_debugging_tags && ! suppress_bfd_header)
    putchar ('\n');

  if (dump_symtab
      || dump_reloc_info
      || disassemble
      || dump_debugging
      || dump_dwarf_section_info)
    {
      syms = slurp_symtab (abfd);

      /* If following links, load any symbol tables from the linked files as well.  */
      if (do_follow_links && is_mainfile)
	{
	  separate_info * i;

	  for (i = first_separate_info; i != NULL; i = i->next)
	    {
	      asymbol **  extra_syms;
	      long        old_symcount = symcount;
	      
	      extra_syms = slurp_symtab (i->handle);

	      if (extra_syms)
		{
		  if (old_symcount == 0)
		    {
		      syms = extra_syms;
		    }
		  else
		    {
		      syms = xrealloc (syms, (symcount + old_symcount) * sizeof (asymbol *));
		      memcpy (syms + old_symcount,
			      extra_syms,
			      symcount * sizeof (asymbol *));
		    }
		}

	      symcount += old_symcount;
	    }
	}
    }

  if (dump_section_headers)
    dump_headers (abfd);

  if (dump_dynamic_symtab || dump_dynamic_reloc_info
      || (disassemble && bfd_get_dynamic_symtab_upper_bound (abfd) > 0))
    dynsyms = slurp_dynamic_symtab (abfd);

  if (disassemble)
    {
      synthcount = bfd_get_synthetic_symtab (abfd, symcount, syms,
					     dynsymcount, dynsyms, &synthsyms);
      if (synthcount < 0)
	synthcount = 0;
    }

  if (dump_symtab)
    dump_symbols (abfd, FALSE);
  if (dump_dynamic_symtab)
    dump_symbols (abfd, TRUE);
  if (dump_dwarf_section_info)
    dump_dwarf (abfd);
  if (dump_ctf_section_info)
    dump_ctf (abfd, dump_ctf_section_name, dump_ctf_parent_name);
  if (dump_stab_section_info)
    dump_stabs (abfd);
  if (dump_reloc_info && ! disassemble)
    dump_relocs (abfd);
  if (dump_dynamic_reloc_info && ! disassemble)
    dump_dynamic_relocs (abfd);
  if (dump_section_contents)
    dump_data (abfd);
  if (disassemble)
    disassemble_data (abfd);

  if (dump_debugging)
    {
      void *dhandle;

      dhandle = read_debugging_info (abfd, syms, symcount, TRUE);
      if (dhandle != NULL)
	{
	  if (!print_debugging_info (stdout, dhandle, abfd, syms,
				     bfd_demangle,
				     dump_debugging_tags ? TRUE : FALSE))
	    {
	      non_fatal (_("%s: printing debugging information failed"),
			 bfd_get_filename (abfd));
	      exit_status = 1;
	    }

	  free (dhandle);
	}
      /* PR 6483: If there was no STABS debug info in the file, try
	 DWARF instead.  */
      else if (! dump_dwarf_section_info)
	{
	  dwarf_select_sections_all ();
	  dump_dwarf (abfd);
	}
    }

  if (syms)
    {
      free (syms);
      syms = NULL;
    }

  if (dynsyms)
    {
      free (dynsyms);
      dynsyms = NULL;
    }

  if (synthsyms)
    {
      free (synthsyms);
      synthsyms = NULL;
    }

  symcount = 0;
  dynsymcount = 0;
  synthcount = 0;

  if (is_mainfile)
    free_debug_memory ();
}

static void
display_object_bfd (bfd *abfd)
{
  char **matching;

  if (bfd_check_format_matches (abfd, bfd_object, &matching))
    {
      dump_bfd (abfd, TRUE);
      return;
    }

  if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
    {
      nonfatal (bfd_get_filename (abfd));
      list_matching_formats (matching);
      free (matching);
      return;
    }

  if (bfd_get_error () != bfd_error_file_not_recognized)
    {
      nonfatal (bfd_get_filename (abfd));
      return;
    }

  if (bfd_check_format_matches (abfd, bfd_core, &matching))
    {
      dump_bfd (abfd, TRUE);
      return;
    }

  nonfatal (bfd_get_filename (abfd));

  if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
    {
      list_matching_formats (matching);
      free (matching);
    }
}

static void
display_any_bfd (bfd *file, int level)
{
  /* Decompress sections unless dumping the section contents.  */
  if (!dump_section_contents)
    file->flags |= BFD_DECOMPRESS;

  /* If the file is an archive, process all of its elements.  */
  if (bfd_check_format (file, bfd_archive))
    {
      bfd *arfile = NULL;
      bfd *last_arfile = NULL;

      if (level == 0)
        printf (_("In archive %s:\n"), sanitize_string (bfd_get_filename (file)));
      else if (level > 100)
	{
	  /* Prevent corrupted files from spinning us into an
	     infinite loop.  100 is an arbitrary heuristic.  */
	  fatal (_("Archive nesting is too deep"));
	  return;
	}
      else
        printf (_("In nested archive %s:\n"),
		sanitize_string (bfd_get_filename (file)));

      for (;;)
	{
	  bfd_set_error (bfd_error_no_error);

	  arfile = bfd_openr_next_archived_file (file, arfile);
	  if (arfile == NULL)
	    {
	      if (bfd_get_error () != bfd_error_no_more_archived_files)
		nonfatal (bfd_get_filename (file));
	      break;
	    }

	  display_any_bfd (arfile, level + 1);

	  if (last_arfile != NULL)
	    {
	      bfd_close (last_arfile);
	      /* PR 17512: file: ac585d01.  */
	      if (arfile == last_arfile)
		{
		  last_arfile = NULL;
		  break;
		}
	    }
	  last_arfile = arfile;
	}

      if (last_arfile != NULL)
	bfd_close (last_arfile);
    }
  else
    display_object_bfd (file);
}

static void
display_file (char *filename, char *target, bfd_boolean last_file)
{
  bfd *file;

  if (get_file_size (filename) < 1)
    {
      exit_status = 1;
      return;
    }

  file = bfd_openr (filename, target);
  if (file == NULL)
    {
      nonfatal (filename);
      return;
    }

  display_any_bfd (file, 0);

  /* This is an optimization to improve the speed of objdump, especially when
     dumping a file with lots of associated debug informatiom.  Calling
     bfd_close on such a file can take a non-trivial amount of time as there
     are lots of lists to walk and buffers to free.  This is only really
     necessary however if we are about to load another file and we need the
     memory back.  Otherwise, if we are about to exit, then we can save (a lot
     of) time by only doing a quick close, and allowing the OS to reclaim the
     memory for us.  */
  if (! last_file)
    bfd_close (file);
  else
    bfd_close_all_done (file);
}

int
main (int argc, char **argv)
{
  int c;
  char *target = default_target;
  bfd_boolean seenflag = FALSE;

#if defined (HAVE_SETLOCALE)
#if defined (HAVE_LC_MESSAGES)
  setlocale (LC_MESSAGES, "");
#endif
  setlocale (LC_CTYPE, "");
#endif

  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  program_name = *argv;
  xmalloc_set_program_name (program_name);
  bfd_set_error_program_name (program_name);

  START_PROGRESS (program_name, 0);

  expandargv (&argc, &argv);

  if (bfd_init () != BFD_INIT_MAGIC)
    fatal (_("fatal error: libbfd ABI mismatch"));
  set_default_bfd_target ();

  while ((c = getopt_long (argc, argv,
			   "pP:ib:m:M:VvCdDlfFaHhrRtTxsSI:j:wE:zgeGW::",
			   long_options, (int *) 0))
	 != EOF)
    {
      switch (c)
	{
	case 0:
	  break;		/* We've been given a long option.  */
	case 'm':
	  machine = optarg;
	  break;
	case 'M':
	  {
	    char *options;
	    if (disassembler_options)
	      /* Ignore potential memory leak for now.  */
	      options = concat (disassembler_options, ",",
				optarg, (const char *) NULL);
	    else
	      options = optarg;
	    disassembler_options = remove_whitespace_and_extra_commas (options);
	  }
	  break;
	case 'j':
	  add_only (optarg);
	  break;
	case 'F':
	  display_file_offsets = TRUE;
	  break;
	case 'l':
	  with_line_numbers = TRUE;
	  break;
	case 'b':
	  target = optarg;
	  break;
	case 'C':
	  do_demangle = TRUE;
	  if (optarg != NULL)
	    {
	      enum demangling_styles style;

	      style = cplus_demangle_name_to_style (optarg);
	      if (style == unknown_demangling)
		fatal (_("unknown demangling style `%s'"),
		       optarg);

	      cplus_demangle_set_style (style);
	    }
	  break;
	case OPTION_RECURSE_LIMIT:
	  demangle_flags &= ~ DMGL_NO_RECURSE_LIMIT;
	  break;
	case OPTION_NO_RECURSE_LIMIT:
	  demangle_flags |= DMGL_NO_RECURSE_LIMIT;
	  break;
	case 'w':
	  do_wide = wide_output = TRUE;
	  break;
	case OPTION_ADJUST_VMA:
	  adjust_section_vma = parse_vma (optarg, "--adjust-vma");
	  break;
	case OPTION_START_ADDRESS:
	  start_address = parse_vma (optarg, "--start-address");
	  if ((stop_address != (bfd_vma) -1) && stop_address <= start_address)
	    fatal (_("error: the start address should be before the end address"));
	  break;
	case OPTION_STOP_ADDRESS:
	  stop_address = parse_vma (optarg, "--stop-address");
	  if ((start_address != (bfd_vma) -1) && stop_address <= start_address)
	    fatal (_("error: the stop address should be after the start address"));
	  break;
	case OPTION_PREFIX:
	  prefix = optarg;
	  prefix_length = strlen (prefix);
	  /* Remove an unnecessary trailing '/' */
	  while (IS_DIR_SEPARATOR (prefix[prefix_length - 1]))
	    prefix_length--;
	  break;
	case OPTION_PREFIX_STRIP:
	  prefix_strip = atoi (optarg);
	  if (prefix_strip < 0)
	    fatal (_("error: prefix strip must be non-negative"));
	  break;
	case OPTION_INSN_WIDTH:
	  insn_width = strtoul (optarg, NULL, 0);
	  if (insn_width <= 0)
	    fatal (_("error: instruction width must be positive"));
	  break;
	case OPTION_INLINES:
	  unwind_inlines = TRUE;
	  break;
	case OPTION_VISUALIZE_JUMPS:
	  visualize_jumps = TRUE;
	  color_output = FALSE;
	  extended_color_output = FALSE;
	  if (optarg != NULL)
	    {
	      if (streq (optarg, "color"))
		color_output = TRUE;
	      else if (streq (optarg, "extended-color"))
		{
		  color_output = TRUE;
		  extended_color_output = TRUE;
		}
	      else if (streq (optarg, "off"))
		visualize_jumps = FALSE;
	      else
		nonfatal (_("unrecognized argument to --visualize-option"));
	    }
	  break;
	case 'E':
	  if (strcmp (optarg, "B") == 0)
	    endian = BFD_ENDIAN_BIG;
	  else if (strcmp (optarg, "L") == 0)
	    endian = BFD_ENDIAN_LITTLE;
	  else
	    {
	      nonfatal (_("unrecognized -E option"));
	      usage (stderr, 1);
	    }
	  break;
	case OPTION_ENDIAN:
	  if (strncmp (optarg, "big", strlen (optarg)) == 0)
	    endian = BFD_ENDIAN_BIG;
	  else if (strncmp (optarg, "little", strlen (optarg)) == 0)
	    endian = BFD_ENDIAN_LITTLE;
	  else
	    {
	      non_fatal (_("unrecognized --endian type `%s'"), optarg);
	      exit_status = 1;
	      usage (stderr, 1);
	    }
	  break;

	case 'f':
	  dump_file_header = TRUE;
	  seenflag = TRUE;
	  break;
	case 'i':
	  formats_info = TRUE;
	  seenflag = TRUE;
	  break;
	case 'I':
	  add_include_path (optarg);
	  break;
	case 'p':
	  dump_private_headers = TRUE;
	  seenflag = TRUE;
	  break;
	case 'P':
	  dump_private_options = optarg;
	  seenflag = TRUE;
	  break;
	case 'x':
	  dump_private_headers = TRUE;
	  dump_symtab = TRUE;
	  dump_reloc_info = TRUE;
	  dump_file_header = TRUE;
	  dump_ar_hdrs = TRUE;
	  dump_section_headers = TRUE;
	  seenflag = TRUE;
	  break;
	case 't':
	  dump_symtab = TRUE;
	  seenflag = TRUE;
	  break;
	case 'T':
	  dump_dynamic_symtab = TRUE;
	  seenflag = TRUE;
	  break;
	case 'd':
	  disassemble = TRUE;
	  seenflag = TRUE;
	  disasm_sym = optarg;
	  break;
	case 'z':
	  disassemble_zeroes = TRUE;
	  break;
	case 'D':
	  disassemble = TRUE;
	  disassemble_all = TRUE;
	  seenflag = TRUE;
	  break;
	case 'S':
	  disassemble = TRUE;
	  with_source_code = TRUE;
	  seenflag = TRUE;
	  break;
	case OPTION_SOURCE_COMMENT:
	  disassemble = TRUE;
	  with_source_code = TRUE;
	  seenflag = TRUE;
	  if (optarg)
	    source_comment = xstrdup (sanitize_string (optarg));
	  else
	    source_comment = xstrdup ("# ");
	  break;
	case 'g':
	  dump_debugging = 1;
	  seenflag = TRUE;
	  break;
	case 'e':
	  dump_debugging = 1;
	  dump_debugging_tags = 1;
	  do_demangle = TRUE;
	  seenflag = TRUE;
	  break;
	case 'W':
	  dump_dwarf_section_info = TRUE;
	  seenflag = TRUE;
	  if (optarg)
	    dwarf_select_sections_by_letters (optarg);
	  else
	    dwarf_select_sections_all ();
	  break;
	case OPTION_DWARF:
	  dump_dwarf_section_info = TRUE;
	  seenflag = TRUE;
	  if (optarg)
	    dwarf_select_sections_by_names (optarg);
	  else
	    dwarf_select_sections_all ();
	  break;
	case OPTION_DWARF_DEPTH:
	  {
	    char *cp;
	    dwarf_cutoff_level = strtoul (optarg, & cp, 0);
	  }
	  break;
	case OPTION_DWARF_START:
	  {
	    char *cp;
	    dwarf_start_die = strtoul (optarg, & cp, 0);
	    suppress_bfd_header = 1;
	  }
	  break;
	case OPTION_DWARF_CHECK:
	  dwarf_check = TRUE;
	  break;
	case OPTION_CTF:
	  dump_ctf_section_info = TRUE;
	  dump_ctf_section_name = xstrdup (optarg);
	  seenflag = TRUE;
	  break;
	case OPTION_CTF_PARENT:
	  dump_ctf_parent_name = xstrdup (optarg);
	  break;
	case 'G':
	  dump_stab_section_info = TRUE;
	  seenflag = TRUE;
	  break;
	case 's':
	  dump_section_contents = TRUE;
	  seenflag = TRUE;
	  break;
	case 'r':
	  dump_reloc_info = TRUE;
	  seenflag = TRUE;
	  break;
	case 'R':
	  dump_dynamic_reloc_info = TRUE;
	  seenflag = TRUE;
	  break;
	case 'a':
	  dump_ar_hdrs = TRUE;
	  seenflag = TRUE;
	  break;
	case 'h':
	  dump_section_headers = TRUE;
	  seenflag = TRUE;
	  break;
	case 'v':
	case 'V':
	  show_version = TRUE;
	  seenflag = TRUE;
	  break;

	case 'H':
	  usage (stdout, 0);
	  /* No need to set seenflag or to break - usage() does not return.  */
	default:
	  usage (stderr, 1);
	}
    }

  if (show_version)
    print_version ("objdump");

  if (!seenflag)
    usage (stderr, 2);

  if (formats_info)
    exit_status = display_info ();
  else
    {
      if (optind == argc)
	display_file ("a.out", target, TRUE);
      else
	for (; optind < argc;)
	  {
	    display_file (argv[optind], target, optind == argc - 1);
	    optind++;
	  }
    }

  free_only_list ();
  free (dump_ctf_section_name);
  free (dump_ctf_parent_name);
  free ((void *) source_comment);

  END_PROGRESS (program_name);

  return exit_status;
}
