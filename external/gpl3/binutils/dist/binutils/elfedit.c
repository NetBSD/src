/* elfedit.c -- Update the ELF header of an ELF format file
   Copyright 2010, 2011, 2012
   Free Software Foundation, Inc.

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
#include <assert.h>

#if __GNUC__ >= 2
/* Define BFD64 here, even if our default architecture is 32 bit ELF
   as this will allow us to read in and parse 64bit and 32bit ELF files.
   Only do this if we believe that the compiler can support a 64 bit
   data type.  For now we only rely on GCC being able to do this.  */
#define BFD64
#endif

#include "bfd.h"
#include "elfcomm.h"
#include "bucomm.h"

#include "elf/common.h"
#include "elf/external.h"
#include "elf/internal.h"

#include "getopt.h"
#include "libiberty.h"
#include "safe-ctype.h"
#include "filenames.h"

char * program_name = "elfedit";
static long archive_file_offset;
static unsigned long archive_file_size;
static Elf_Internal_Ehdr elf_header;
static Elf32_External_Ehdr ehdr32;
static Elf64_External_Ehdr ehdr64;
static int input_elf_machine = -1;
static int output_elf_machine = -1;
static int input_elf_type = -1;
static int output_elf_type = -1;
static int input_elf_osabi = -1;
static int output_elf_osabi = -1;
static int input_elf_class = -1;

static int
update_elf_header (const char *file_name, FILE *file)
{
  int class, machine, type, status, osabi;

  if (elf_header.e_ident[EI_MAG0] != ELFMAG0
      || elf_header.e_ident[EI_MAG1] != ELFMAG1
      || elf_header.e_ident[EI_MAG2] != ELFMAG2
      || elf_header.e_ident[EI_MAG3] != ELFMAG3)
    {
      error
	(_("%s: Not an ELF file - wrong magic bytes at the start\n"),
	 file_name);
      return 0;
    }

  if (elf_header.e_ident[EI_VERSION] != EV_CURRENT)
    {
      error
	(_("%s: Unsupported EI_VERSION: %d is not %d\n"),
	 file_name, elf_header.e_ident[EI_VERSION],
	 EV_CURRENT);
      return 0;
    }

  /* Return if e_machine is the same as output_elf_machine.  */
  if (output_elf_machine == elf_header.e_machine)
    return 1;

  class = elf_header.e_ident[EI_CLASS];

  /* Skip if class doesn't match. */
  if (input_elf_class != -1 && class != input_elf_class)
    {
      error
	(_("%s: Unmatched EI_CLASS: %d is not %d\n"),
	 file_name, class, input_elf_class);
      return 0;
    }

  machine = elf_header.e_machine;

  /* Skip if e_machine doesn't match. */
  if (input_elf_machine != -1 && machine != input_elf_machine)
    {
      error
	(_("%s: Unmatched e_machine: %d is not %d\n"),
	 file_name, machine, input_elf_machine);
      return 0;
    }

  type = elf_header.e_type;

  /* Skip if e_type doesn't match. */
  if (input_elf_type != -1 && type != input_elf_type)
    {
      error
	(_("%s: Unmatched e_type: %d is not %d\n"),
	 file_name, type, input_elf_type);
      return 0;
    }

  osabi = elf_header.e_ident[EI_OSABI];

  /* Skip if OSABI doesn't match. */
  if (input_elf_osabi != -1 && osabi != input_elf_osabi)
    {
      error
	(_("%s: Unmatched EI_OSABI: %d is not %d\n"),
	 file_name, osabi, input_elf_osabi);
      return 0;
    }

  /* Update e_machine, e_type and EI_OSABI.  */
  switch (class)
    {
    default:
      /* We should never get here.  */
      abort ();
      break;
    case ELFCLASS32:
      if (output_elf_machine != -1)
	BYTE_PUT (ehdr32.e_machine, output_elf_machine);
      if (output_elf_type != -1)
	BYTE_PUT (ehdr32.e_type, output_elf_type);
      if (output_elf_osabi != -1)
	ehdr32.e_ident[EI_OSABI] = output_elf_osabi;
      status = fwrite (&ehdr32, sizeof (ehdr32), 1, file) == 1;
      break;
    case ELFCLASS64:
      if (output_elf_machine != -1)
	BYTE_PUT (ehdr64.e_machine, output_elf_machine);
      if (output_elf_type != -1)
	BYTE_PUT (ehdr64.e_type, output_elf_type);
      if (output_elf_osabi != -1)
	ehdr64.e_ident[EI_OSABI] = output_elf_osabi;
      status = fwrite (&ehdr64, sizeof (ehdr64), 1, file) == 1;
      break;
    }

  if (status != 1)
    error (_("%s: Failed to update ELF header: %s\n"),
	       file_name, strerror (errno));

  return status;
}

static int
get_file_header (FILE * file)
{
  /* Read in the identity array.  */
  if (fread (elf_header.e_ident, EI_NIDENT, 1, file) != 1)
    return 0;

  /* Determine how to read the rest of the header.  */
  switch (elf_header.e_ident[EI_DATA])
    {
    default: /* fall through */
    case ELFDATANONE: /* fall through */
    case ELFDATA2LSB:
      byte_get = byte_get_little_endian;
      byte_put = byte_put_little_endian;
      break;
    case ELFDATA2MSB:
      byte_get = byte_get_big_endian;
      byte_put = byte_put_big_endian;
      break;
    }

  /* Read in the rest of the header.  For now we only support 32 bit
     and 64 bit ELF files.  */
  switch (elf_header.e_ident[EI_CLASS])
    {
    default:
      error (_("Unsupported EI_CLASS: %d\n"),
		 elf_header.e_ident[EI_CLASS]);
      return 0;

    case ELFCLASS32:
      if (fread (ehdr32.e_type, sizeof (ehdr32) - EI_NIDENT,
		 1, file) != 1)
	return 0;

      elf_header.e_type      = BYTE_GET (ehdr32.e_type);
      elf_header.e_machine   = BYTE_GET (ehdr32.e_machine);
      elf_header.e_version   = BYTE_GET (ehdr32.e_version);
      elf_header.e_entry     = BYTE_GET (ehdr32.e_entry);
      elf_header.e_phoff     = BYTE_GET (ehdr32.e_phoff);
      elf_header.e_shoff     = BYTE_GET (ehdr32.e_shoff);
      elf_header.e_flags     = BYTE_GET (ehdr32.e_flags);
      elf_header.e_ehsize    = BYTE_GET (ehdr32.e_ehsize);
      elf_header.e_phentsize = BYTE_GET (ehdr32.e_phentsize);
      elf_header.e_phnum     = BYTE_GET (ehdr32.e_phnum);
      elf_header.e_shentsize = BYTE_GET (ehdr32.e_shentsize);
      elf_header.e_shnum     = BYTE_GET (ehdr32.e_shnum);
      elf_header.e_shstrndx  = BYTE_GET (ehdr32.e_shstrndx);

      memcpy (&ehdr32, &elf_header, EI_NIDENT);
      break;

    case ELFCLASS64:
      /* If we have been compiled with sizeof (bfd_vma) == 4, then
	 we will not be able to cope with the 64bit data found in
	 64 ELF files.  Detect this now and abort before we start
	 overwriting things.  */
      if (sizeof (bfd_vma) < 8)
	{
	  error (_("This executable has been built without support for a\n\
64 bit data type and so it cannot process 64 bit ELF files.\n"));
	  return 0;
	}

      if (fread (ehdr64.e_type, sizeof (ehdr64) - EI_NIDENT,
		 1, file) != 1)
	return 0;

      elf_header.e_type      = BYTE_GET (ehdr64.e_type);
      elf_header.e_machine   = BYTE_GET (ehdr64.e_machine);
      elf_header.e_version   = BYTE_GET (ehdr64.e_version);
      elf_header.e_entry     = BYTE_GET (ehdr64.e_entry);
      elf_header.e_phoff     = BYTE_GET (ehdr64.e_phoff);
      elf_header.e_shoff     = BYTE_GET (ehdr64.e_shoff);
      elf_header.e_flags     = BYTE_GET (ehdr64.e_flags);
      elf_header.e_ehsize    = BYTE_GET (ehdr64.e_ehsize);
      elf_header.e_phentsize = BYTE_GET (ehdr64.e_phentsize);
      elf_header.e_phnum     = BYTE_GET (ehdr64.e_phnum);
      elf_header.e_shentsize = BYTE_GET (ehdr64.e_shentsize);
      elf_header.e_shnum     = BYTE_GET (ehdr64.e_shnum);
      elf_header.e_shstrndx  = BYTE_GET (ehdr64.e_shstrndx);

      memcpy (&ehdr64, &elf_header, EI_NIDENT);
      break;
    }
  return 1;
}

/* Process one ELF object file according to the command line options.
   This file may actually be stored in an archive.  The file is
   positioned at the start of the ELF object.  */

static int
process_object (const char *file_name, FILE *file)
{
  /* Rememeber where we are.  */
  long offset = ftell (file);

  if (! get_file_header (file))
    {
      error (_("%s: Failed to read ELF header\n"), file_name);
      return 1;
    }

  /* Go to the position of the ELF header.  */
  if (fseek (file, offset, SEEK_SET) != 0)
    {
      error (_("%s: Failed to seek to ELF header\n"), file_name);
    }

  if (! update_elf_header (file_name, file))
    return 1;

  return 0;
}

/* Process an ELF archive.
   On entry the file is positioned just after the ARMAG string.  */

static int
process_archive (const char * file_name, FILE * file,
		 bfd_boolean is_thin_archive)
{
  struct archive_info arch;
  struct archive_info nested_arch;
  size_t got;
  int ret;

  /* The ARCH structure is used to hold information about this archive.  */
  arch.file_name = NULL;
  arch.file = NULL;
  arch.index_array = NULL;
  arch.sym_table = NULL;
  arch.longnames = NULL;

  /* The NESTED_ARCH structure is used as a single-item cache of information
     about a nested archive (when members of a thin archive reside within
     another regular archive file).  */
  nested_arch.file_name = NULL;
  nested_arch.file = NULL;
  nested_arch.index_array = NULL;
  nested_arch.sym_table = NULL;
  nested_arch.longnames = NULL;

  if (setup_archive (&arch, file_name, file, is_thin_archive, FALSE) != 0)
    {
      ret = 1;
      goto out;
    }

  ret = 0;

  while (1)
    {
      char * name;
      size_t namelen;
      char * qualified_name;

      /* Read the next archive header.  */
      if (fseek (file, arch.next_arhdr_offset, SEEK_SET) != 0)
        {
          error (_("%s: failed to seek to next archive header\n"),
		     file_name);
          return 1;
        }
      got = fread (&arch.arhdr, 1, sizeof arch.arhdr, file);
      if (got != sizeof arch.arhdr)
        {
          if (got == 0)
	    break;
          error (_("%s: failed to read archive header\n"),
		     file_name);
          ret = 1;
          break;
        }
      if (memcmp (arch.arhdr.ar_fmag, ARFMAG, 2) != 0)
        {
          error (_("%s: did not find a valid archive header\n"),
		     arch.file_name);
          ret = 1;
          break;
        }

      arch.next_arhdr_offset += sizeof arch.arhdr;

      archive_file_size = strtoul (arch.arhdr.ar_size, NULL, 10);
      if (archive_file_size & 01)
        ++archive_file_size;

      name = get_archive_member_name (&arch, &nested_arch);
      if (name == NULL)
	{
	  error (_("%s: bad archive file name\n"), file_name);
	  ret = 1;
	  break;
	}
      namelen = strlen (name);

      qualified_name = make_qualified_name (&arch, &nested_arch, name);
      if (qualified_name == NULL)
	{
	  error (_("%s: bad archive file name\n"), file_name);
	  ret = 1;
	  break;
	}

      if (is_thin_archive && arch.nested_member_origin == 0)
        {
          /* This is a proxy for an external member of a thin archive.  */
          FILE *member_file;
          char *member_file_name = adjust_relative_path (file_name,
							 name, namelen);
          if (member_file_name == NULL)
            {
              ret = 1;
              break;
            }

          member_file = fopen (member_file_name, "r+b");
          if (member_file == NULL)
            {
              error (_("Input file '%s' is not readable\n"),
			 member_file_name);
              free (member_file_name);
              ret = 1;
              break;
            }

          archive_file_offset = arch.nested_member_origin;

          ret |= process_object (qualified_name, member_file);

          fclose (member_file);
          free (member_file_name);
        }
      else if (is_thin_archive)
        {
          /* This is a proxy for a member of a nested archive.  */
          archive_file_offset = arch.nested_member_origin + sizeof arch.arhdr;

          /* The nested archive file will have been opened and setup by
             get_archive_member_name.  */
          if (fseek (nested_arch.file, archive_file_offset,
		     SEEK_SET) != 0)
            {
              error (_("%s: failed to seek to archive member\n"),
			 nested_arch.file_name);
              ret = 1;
              break;
            }

          ret |= process_object (qualified_name, nested_arch.file);
        }
      else
        {
          archive_file_offset = arch.next_arhdr_offset;
          arch.next_arhdr_offset += archive_file_size;

          ret |= process_object (qualified_name, file);
        }

      free (qualified_name);
    }

 out:
  if (nested_arch.file != NULL)
    fclose (nested_arch.file);
  release_archive (&nested_arch);
  release_archive (&arch);

  return ret;
}

static int
check_file (const char *file_name, struct stat *statbuf_p)
{
  struct stat statbuf;

  if (statbuf_p == NULL)
    statbuf_p = &statbuf;

  if (stat (file_name, statbuf_p) < 0)
    {
      if (errno == ENOENT)
	error (_("'%s': No such file\n"), file_name);
      else
	error (_("Could not locate '%s'.  System error message: %s\n"),
		   file_name, strerror (errno));
      return 1;
    }

  if (! S_ISREG (statbuf_p->st_mode))
    {
      error (_("'%s' is not an ordinary file\n"), file_name);
      return 1;
    }

  return 0;
}

static int
process_file (const char *file_name)
{
  FILE * file;
  char armag[SARMAG];
  int ret;

  if (check_file (file_name, NULL))
    return 1;

  file = fopen (file_name, "r+b");
  if (file == NULL)
    {
      error (_("Input file '%s' is not readable\n"), file_name);
      return 1;
    }

  if (fread (armag, SARMAG, 1, file) != 1)
    {
      error (_("%s: Failed to read file's magic number\n"),
		 file_name);
      fclose (file);
      return 1;
    }

  if (memcmp (armag, ARMAG, SARMAG) == 0)
    ret = process_archive (file_name, file, FALSE);
  else if (memcmp (armag, ARMAGT, SARMAG) == 0)
    ret = process_archive (file_name, file, TRUE);
  else
    {
      rewind (file);
      archive_file_size = archive_file_offset = 0;
      ret = process_object (file_name, file);
    }

  fclose (file);

  return ret;
}

static const struct
{
  int osabi;
  const char *name;
}
osabis[] =
{
  { ELFOSABI_NONE, "none" },
  { ELFOSABI_HPUX, "HPUX" },
  { ELFOSABI_NETBSD, "NetBSD" },
  { ELFOSABI_GNU, "GNU" },
  { ELFOSABI_GNU, "Linux" },
  { ELFOSABI_SOLARIS, "Solaris" },
  { ELFOSABI_AIX, "AIX" },
  { ELFOSABI_IRIX, "Irix" },
  { ELFOSABI_FREEBSD, "FreeBSD" },
  { ELFOSABI_TRU64, "TRU64" },
  { ELFOSABI_MODESTO, "Modesto" },
  { ELFOSABI_OPENBSD, "OpenBSD" },
  { ELFOSABI_OPENVMS, "OpenVMS" },
  { ELFOSABI_NSK, "NSK" },
  { ELFOSABI_AROS, "AROS" },
  { ELFOSABI_FENIXOS, "FenixOS" }
};

/* Return ELFOSABI_XXX for an OSABI string, OSABI.  */

static int
elf_osabi (const char *osabi)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (osabis); i++)
    if (strcasecmp (osabi, osabis[i].name) == 0)
      return osabis[i].osabi;

  error (_("Unknown OSABI: %s\n"), osabi);

  return -1;
}

/* Return EM_XXX for a machine string, MACH.  */

static int
elf_machine (const char *mach)
{
  if (strcasecmp (mach, "l1om") == 0)
    return EM_L1OM;
  if (strcasecmp (mach, "k1om") == 0)
    return EM_K1OM;
  if (strcasecmp (mach, "x86_64") == 0)
    return EM_X86_64;
  if (strcasecmp (mach, "x86-64") == 0)
    return EM_X86_64;
  if (strcasecmp (mach, "none") == 0)
    return EM_NONE;

  error (_("Unknown machine type: %s\n"), mach);

  return -1;
}

/* Return ELF class for a machine type, MACH.  */

static int
elf_class (int mach)
{
  switch (mach)
    {
    case EM_L1OM:
    case EM_K1OM:
    case EM_X86_64:
      return ELFCLASS64;
    case EM_NONE:
      return ELFCLASSNONE;
    default:
      error (_("Unknown machine type: %d\n"), mach);
      return -1;
    }
}

/* Return ET_XXX for a type string, TYPE.  */

static int
elf_type (const char *type)
{
  if (strcasecmp (type, "rel") == 0)
    return ET_REL;
  if (strcasecmp (type, "exec") == 0)
    return ET_EXEC;
  if (strcasecmp (type, "dyn") == 0)
    return ET_DYN;
  if (strcasecmp (type, "none") == 0)
    return ET_NONE;

  error (_("Unknown type: %s\n"), type);

  return -1;
}

enum command_line_switch
  {
    OPTION_INPUT_MACH = 150,
    OPTION_OUTPUT_MACH,
    OPTION_INPUT_TYPE,
    OPTION_OUTPUT_TYPE,
    OPTION_INPUT_OSABI,
    OPTION_OUTPUT_OSABI
  };

static struct option options[] =
{
  {"input-mach",	required_argument, 0, OPTION_INPUT_MACH},
  {"output-mach",	required_argument, 0, OPTION_OUTPUT_MACH},
  {"input-type",	required_argument, 0, OPTION_INPUT_TYPE},
  {"output-type",	required_argument, 0, OPTION_OUTPUT_TYPE},
  {"input-osabi",	required_argument, 0, OPTION_INPUT_OSABI},
  {"output-osabi",	required_argument, 0, OPTION_OUTPUT_OSABI},
  {"version",		no_argument, 0, 'v'},
  {"help",		no_argument, 0, 'h'},
  {0,			no_argument, 0, 0}
};

static void
usage (FILE *stream, int exit_status)
{
  fprintf (stream, _("Usage: %s <option(s)> elffile(s)\n"),
	   program_name);
  fprintf (stream, _(" Update the ELF header of ELF files\n"));
  fprintf (stream, _(" The options are:\n"));
  fprintf (stream, _("\
  --input-mach <machine>      Set input machine type to <machine>\n\
  --output-mach <machine>     Set output machine type to <machine>\n\
  --input-type <type>         Set input file type to <type>\n\
  --output-type <type>        Set output file type to <type>\n\
  --input-osabi <osabi>       Set input OSABI to <osabi>\n\
  --output-osabi <osabi>      Set output OSABI to <osabi>\n\
  -h --help                   Display this information\n\
  -v --version                Display the version number of %s\n\
"),
	   program_name);
  if (REPORT_BUGS_TO[0] && exit_status == 0)
    fprintf (stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
  exit (exit_status);
}

int
main (int argc, char ** argv)
{
  int c, status;

#if defined (HAVE_SETLOCALE) && defined (HAVE_LC_MESSAGES)
  setlocale (LC_MESSAGES, "");
#endif
#if defined (HAVE_SETLOCALE)
  setlocale (LC_CTYPE, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  expandargv (&argc, &argv);

  while ((c = getopt_long (argc, argv, "hv",
			   options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case OPTION_INPUT_MACH:
	  input_elf_machine = elf_machine (optarg);
	  if (input_elf_machine < 0)
	    return 1;
	  input_elf_class = elf_class (input_elf_machine);
	  if (input_elf_class < 0)
	    return 1;
	  break;

	case OPTION_OUTPUT_MACH:
	  output_elf_machine = elf_machine (optarg);
	  if (output_elf_machine < 0)
	    return 1;
	  break;

	case OPTION_INPUT_TYPE:
	  input_elf_type = elf_type (optarg);
	  if (input_elf_type < 0)
	    return 1;
	  break;

	case OPTION_OUTPUT_TYPE:
	  output_elf_type = elf_type (optarg);
	  if (output_elf_type < 0)
	    return 1;
	  break;

	case OPTION_INPUT_OSABI:
	  input_elf_osabi = elf_osabi (optarg);
	  if (input_elf_osabi < 0)
	    return 1;
	  break;

	case OPTION_OUTPUT_OSABI:
	  output_elf_osabi = elf_osabi (optarg);
	  if (output_elf_osabi < 0)
	    return 1;
	  break;

	case 'h':
	  usage (stdout, 0);

	case 'v':
	  print_version (program_name);
	  break;

	default:
	  usage (stderr, 1);
	}
    }

  if (optind == argc
      || (output_elf_machine == -1
	  && output_elf_type == -1
	  && output_elf_osabi == -1))
    usage (stderr, 1);

  status = 0;
  while (optind < argc)
    status |= process_file (argv[optind++]);

  return status;
}
