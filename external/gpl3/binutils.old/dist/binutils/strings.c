/* strings -- print the strings of printable characters in files
   Copyright (C) 1993-2016 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

/* Usage: strings [options] file...

   Options:
   --all
   -a
   -		Scan each file in its entirety.

   --data
   -d		Scan only the initialized data section(s) of object files.

   --print-file-name
   -f		Print the name of the file before each string.

   --bytes=min-len
   -n min-len
   -min-len	Print graphic char sequences, MIN-LEN or more bytes long,
		that are followed by a NUL or a newline.  Default is 4.

   --radix={o,x,d}
   -t {o,x,d}	Print the offset within the file before each string,
		in octal/hex/decimal.

  --include-all-whitespace
  -w		By default tab and space are the only whitepace included in graphic
		char sequences.  This option considers all of isspace() valid.

   -o		Like -to.  (Some other implementations have -o like -to,
		others like -td.  We chose one arbitrarily.)

   --encoding={s,S,b,l,B,L}
   -e {s,S,b,l,B,L}
		Select character encoding: 7-bit-character, 8-bit-character,
		bigendian 16-bit, littleendian 16-bit, bigendian 32-bit,
		littleendian 32-bit.

   --target=BFDNAME
   -T {bfdname}
		Specify a non-default object file format.

  --output-separator=sep_string
  -s sep_string	String used to separate parsed strings in output.
		Default is newline.

   --help
   -h		Print the usage message on the standard output.

   --version
   -V
   -v		Print the program version number.

   Written by Richard Stallman <rms@gnu.ai.mit.edu>
   and David MacKenzie <djm@gnu.ai.mit.edu>.  */

#include "sysdep.h"
#include "bfd.h"
#include "getopt.h"
#include "libiberty.h"
#include "safe-ctype.h"
#include "bucomm.h"

#define STRING_ISGRAPHIC(c) \
      (   (c) >= 0 \
       && (c) <= 255 \
       && ((c) == '\t' || ISPRINT (c) || (encoding == 'S' && (c) > 127) \
           || (include_all_whitespace == TRUE && ISSPACE (c))) \
      )

#ifndef errno
extern int errno;
#endif

/* The BFD section flags that identify an initialized data section.  */
#define DATA_FLAGS (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS)

/* Radix for printing addresses (must be 8, 10 or 16).  */
static int address_radix;

/* Minimum length of sequence of graphic chars to trigger output.  */
static int string_min;

/* Whether or not we include all whitespace as a graphic char.   */
static bfd_boolean include_all_whitespace;

/* TRUE means print address within file for each string.  */
static bfd_boolean print_addresses;

/* TRUE means print filename for each string.  */
static bfd_boolean print_filenames;

/* TRUE means for object files scan only the data section.  */
static bfd_boolean datasection_only;

/* TRUE if we found an initialized data section in the current file.  */
static bfd_boolean got_a_section;

/* The BFD object file format.  */
static char *target;

/* The character encoding format.  */
static char encoding;
static int encoding_bytes;

/* Output string used to separate parsed strings  */
static char *output_separator;

static struct option long_options[] =
{
  {"all", no_argument, NULL, 'a'},
  {"data", no_argument, NULL, 'd'},
  {"print-file-name", no_argument, NULL, 'f'},
  {"bytes", required_argument, NULL, 'n'},
  {"radix", required_argument, NULL, 't'},
  {"include-all-whitespace", required_argument, NULL, 'w'},
  {"encoding", required_argument, NULL, 'e'},
  {"target", required_argument, NULL, 'T'},
  {"output-separator", required_argument, NULL, 's'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {NULL, 0, NULL, 0}
};

/* Records the size of a named file so that we
   do not repeatedly run bfd_stat() on it.  */

typedef struct
{
  const char *  filename;
  bfd_size_type filesize;
} filename_and_size_t;

static void strings_a_section (bfd *, asection *, void *);
static bfd_boolean strings_object_file (const char *);
static bfd_boolean strings_file (char *);
static void print_strings (const char *, FILE *, file_ptr, int, int, char *);
static void usage (FILE *, int);
static long get_char (FILE *, file_ptr *, int *, char **);

int main (int, char **);

int
main (int argc, char **argv)
{
  int optc;
  int exit_status = 0;
  bfd_boolean files_given = FALSE;
  char *s;
  int numeric_opt = 0;

#if defined (HAVE_SETLOCALE)
  setlocale (LC_ALL, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  program_name = argv[0];
  xmalloc_set_program_name (program_name);
  bfd_set_error_program_name (program_name);

  expandargv (&argc, &argv);

  string_min = 4;
  include_all_whitespace = FALSE;
  print_addresses = FALSE;
  print_filenames = FALSE;
  if (DEFAULT_STRINGS_ALL)
    datasection_only = FALSE;
  else
    datasection_only = TRUE;
  target = NULL;
  encoding = 's';
  output_separator = NULL;

  while ((optc = getopt_long (argc, argv, "adfhHn:wot:e:T:s:Vv0123456789",
			      long_options, (int *) 0)) != EOF)
    {
      switch (optc)
	{
	case 'a':
	  datasection_only = FALSE;
	  break;

	case 'd':
	  datasection_only = TRUE;
	  break;

	case 'f':
	  print_filenames = TRUE;
	  break;

	case 'H':
	case 'h':
	  usage (stdout, 0);

	case 'n':
	  string_min = (int) strtoul (optarg, &s, 0);
	  if (s != NULL && *s != 0)
	    fatal (_("invalid integer argument %s"), optarg);
	  break;

	case 'w':
	  include_all_whitespace = TRUE;
	  break;

	case 'o':
	  print_addresses = TRUE;
	  address_radix = 8;
	  break;

	case 't':
	  print_addresses = TRUE;
	  if (optarg[1] != '\0')
	    usage (stderr, 1);
	  switch (optarg[0])
	    {
	    case 'o':
	      address_radix = 8;
	      break;

	    case 'd':
	      address_radix = 10;
	      break;

	    case 'x':
	      address_radix = 16;
	      break;

	    default:
	      usage (stderr, 1);
	    }
	  break;

	case 'T':
	  target = optarg;
	  break;

	case 'e':
	  if (optarg[1] != '\0')
	    usage (stderr, 1);
	  encoding = optarg[0];
	  break;

	case 's':
	  output_separator = optarg;
          break;

	case 'V':
	case 'v':
	  print_version ("strings");
	  break;

	case '?':
	  usage (stderr, 1);

	default:
	  numeric_opt = optind;
	  break;
	}
    }

  if (numeric_opt != 0)
    {
      string_min = (int) strtoul (argv[numeric_opt - 1] + 1, &s, 0);
      if (s != NULL && *s != 0)
	fatal (_("invalid integer argument %s"), argv[numeric_opt - 1] + 1);
    }
  if (string_min < 1)
    fatal (_("invalid minimum string length %d"), string_min);

  switch (encoding)
    {
    case 'S':
    case 's':
      encoding_bytes = 1;
      break;
    case 'b':
    case 'l':
      encoding_bytes = 2;
      break;
    case 'B':
    case 'L':
      encoding_bytes = 4;
      break;
    default:
      usage (stderr, 1);
    }

  bfd_init ();
  set_default_bfd_target ();

  if (optind >= argc)
    {
      datasection_only = FALSE;
      SET_BINARY (fileno (stdin));
      print_strings ("{standard input}", stdin, 0, 0, 0, (char *) NULL);
      files_given = TRUE;
    }
  else
    {
      for (; optind < argc; ++optind)
	{
	  if (strcmp (argv[optind], "-") == 0)
	    datasection_only = FALSE;
	  else
	    {
	      files_given = TRUE;
	      exit_status |= strings_file (argv[optind]) == FALSE;
	    }
	}
    }

  if (!files_given)
    usage (stderr, 1);

  return (exit_status);
}

/* Scan section SECT of the file ABFD, whose printable name is in
   ARG->filename and whose size might be in ARG->filesize.  If it
   contains initialized data set `got_a_section' and print the
   strings in it.

   FIXME: We ought to be able to return error codes/messages for
   certain conditions.  */

static void
strings_a_section (bfd *abfd, asection *sect, void *arg)
{
  filename_and_size_t * filename_and_sizep;
  bfd_size_type *filesizep;
  bfd_size_type sectsize;
  void *mem;

  if ((sect->flags & DATA_FLAGS) != DATA_FLAGS)
    return;

  sectsize = bfd_get_section_size (sect);

  if (sectsize <= 0)
    return;

  /* Get the size of the file.  This might have been cached for us.  */
  filename_and_sizep = (filename_and_size_t *) arg;
  filesizep = & filename_and_sizep->filesize;

  if (*filesizep == 0)
    {
      struct stat st;

      if (bfd_stat (abfd, &st))
	return;

      /* Cache the result so that we do not repeatedly stat this file.  */
      *filesizep = st.st_size;
    }

  /* Compare the size of the section against the size of the file.
     If the section is bigger then the file must be corrupt and
     we should not try dumping it.  */
  if (sectsize >= *filesizep)
    return;

  mem = xmalloc (sectsize);

  if (bfd_get_section_contents (abfd, sect, mem, (file_ptr) 0, sectsize))
    {
      got_a_section = TRUE;

      print_strings (filename_and_sizep->filename, NULL, sect->filepos,
		     0, sectsize, (char *) mem);
    }

  free (mem);
}

/* Scan all of the sections in FILE, and print the strings
   in the initialized data section(s).

   Return TRUE if successful,
   FALSE if not (such as if FILE is not an object file).  */

static bfd_boolean
strings_object_file (const char *file)
{
  filename_and_size_t filename_and_size;
  bfd *abfd;

  abfd = bfd_openr (file, target);

  if (abfd == NULL)
    /* Treat the file as a non-object file.  */
    return FALSE;

  /* This call is mainly for its side effect of reading in the sections.
     We follow the traditional behavior of `strings' in that we don't
     complain if we don't recognize a file to be an object file.  */
  if (!bfd_check_format (abfd, bfd_object))
    {
      bfd_close (abfd);
      return FALSE;
    }

  got_a_section = FALSE;
  filename_and_size.filename = file;
  filename_and_size.filesize = 0;
  bfd_map_over_sections (abfd, strings_a_section, & filename_and_size);

  if (!bfd_close (abfd))
    {
      bfd_nonfatal (file);
      return FALSE;
    }

  return got_a_section;
}

/* Print the strings in FILE.  Return TRUE if ok, FALSE if an error occurs.  */

static bfd_boolean
strings_file (char *file)
{
  struct stat st;

  /* get_file_size does not support non-S_ISREG files.  */

  if (stat (file, &st) < 0)
    {
      if (errno == ENOENT)
	non_fatal (_("'%s': No such file"), file);
      else
	non_fatal (_("Warning: could not locate '%s'.  reason: %s"),
		   file, strerror (errno));
      return FALSE;
    }

  /* If we weren't told to scan the whole file,
     try to open it as an object file and only look at
     initialized data sections.  If that fails, fall back to the
     whole file.  */
  if (!datasection_only || !strings_object_file (file))
    {
      FILE *stream;

      stream = fopen (file, FOPEN_RB);
      if (stream == NULL)
	{
	  fprintf (stderr, "%s: ", program_name);
	  perror (file);
	  return FALSE;
	}

      print_strings (file, stream, (file_ptr) 0, 0, 0, (char *) 0);

      if (fclose (stream) == EOF)
	{
	  fprintf (stderr, "%s: ", program_name);
	  perror (file);
	  return FALSE;
	}
    }

  return TRUE;
}

/* Read the next character, return EOF if none available.
   Assume that STREAM is positioned so that the next byte read
   is at address ADDRESS in the file.

   If STREAM is NULL, do not read from it.
   The caller can supply a buffer of characters
   to be processed before the data in STREAM.
   MAGIC is the address of the buffer and
   MAGICCOUNT is how many characters are in it.  */

static long
get_char (FILE *stream, file_ptr *address, int *magiccount, char **magic)
{
  int c, i;
  long r = 0;

  for (i = 0; i < encoding_bytes; i++)
    {
      if (*magiccount)
	{
	  (*magiccount)--;
	  c = *(*magic)++;
	}
      else
	{
	  if (stream == NULL)
	    return EOF;

	  /* Only use getc_unlocked if we found a declaration for it.
	     Otherwise, libc is not thread safe by default, and we
	     should not use it.  */

#if defined(HAVE_GETC_UNLOCKED) && HAVE_DECL_GETC_UNLOCKED
	  c = getc_unlocked (stream);
#else
	  c = getc (stream);
#endif
	  if (c == EOF)
	    return EOF;
	}

      (*address)++;
      r = (r << 8) | (c & 0xff);
    }

  switch (encoding)
    {
    default:
      break;
    case 'l':
      r = ((r & 0xff) << 8) | ((r & 0xff00) >> 8);
      break;
    case 'L':
      r = (((r & 0xff) << 24) | ((r & 0xff00) << 8)
	   | ((r & 0xff0000) >> 8) | ((r & 0xff000000) >> 24));
      break;
    }

  return r;
}

/* Find the strings in file FILENAME, read from STREAM.
   Assume that STREAM is positioned so that the next byte read
   is at address ADDRESS in the file.
   Stop reading at address STOP_POINT in the file, if nonzero.

   If STREAM is NULL, do not read from it.
   The caller can supply a buffer of characters
   to be processed before the data in STREAM.
   MAGIC is the address of the buffer and
   MAGICCOUNT is how many characters are in it.
   Those characters come at address ADDRESS and the data in STREAM follow.  */

static void
print_strings (const char *filename, FILE *stream, file_ptr address,
	       int stop_point, int magiccount, char *magic)
{
  char *buf = (char *) xmalloc (sizeof (char) * (string_min + 1));

  while (1)
    {
      file_ptr start;
      int i;
      long c;

      /* See if the next `string_min' chars are all graphic chars.  */
    tryline:
      if (stop_point && address >= stop_point)
	break;
      start = address;
      for (i = 0; i < string_min; i++)
	{
	  c = get_char (stream, &address, &magiccount, &magic);
	  if (c == EOF)
	    {
	      free (buf);
	      return;
	    }
	  if (! STRING_ISGRAPHIC (c))
	    /* Found a non-graphic.  Try again starting with next char.  */
	    goto tryline;
	  buf[i] = c;
	}

      /* We found a run of `string_min' graphic characters.  Print up
	 to the next non-graphic character.  */

      if (print_filenames)
	printf ("%s: ", filename);
      if (print_addresses)
	switch (address_radix)
	  {
	  case 8:
#ifdef HAVE_LONG_LONG
	    if (sizeof (start) > sizeof (long))
	      {
# ifndef __MSVCRT__
	        printf ("%7llo ", (unsigned long long) start);
# else
	        printf ("%7I64o ", (unsigned long long) start);
# endif
	      }
	    else
#elif !BFD_HOST_64BIT_LONG
	    if (start != (unsigned long) start)
	      printf ("++%7lo ", (unsigned long) start);
	    else
#endif
	      printf ("%7lo ", (unsigned long) start);
	    break;

	  case 10:
#ifdef HAVE_LONG_LONG
	    if (sizeof (start) > sizeof (long))
	      {
# ifndef __MSVCRT__
	        printf ("%7lld ", (unsigned long long) start);
# else
	        printf ("%7I64d ", (unsigned long long) start);
# endif
	      }
	    else
#elif !BFD_HOST_64BIT_LONG
	    if (start != (unsigned long) start)
	      printf ("++%7llu ", (unsigned long) start);
	    else
#endif
	      printf ("%7ld ", (long) start);
	    break;

	  case 16:
#ifdef HAVE_LONG_LONG
	    if (sizeof (start) > sizeof (long))
	      {
# ifndef __MSVCRT__
	        printf ("%7llx ", (unsigned long long) start);
# else
	        printf ("%7I64x ", (unsigned long long) start);
# endif
	      }
	    else
#elif !BFD_HOST_64BIT_LONG
	    if (start != (unsigned long) start)
	      printf ("%lx%8.8lx ", (unsigned long) (start >> 32),
		      (unsigned long) (start & 0xffffffff));
	    else
#endif
	      printf ("%7lx ", (unsigned long) start);
	    break;
	  }

      buf[i] = '\0';
      fputs (buf, stdout);

      while (1)
	{
	  c = get_char (stream, &address, &magiccount, &magic);
	  if (c == EOF)
	    break;
	  if (! STRING_ISGRAPHIC (c))
	    break;
	  putchar (c);
	}

      if (output_separator)
        fputs (output_separator, stdout);
      else
        putchar ('\n');
    }
  free (buf);
}

static void
usage (FILE *stream, int status)
{
  fprintf (stream, _("Usage: %s [option(s)] [file(s)]\n"), program_name);
  fprintf (stream, _(" Display printable strings in [file(s)] (stdin by default)\n"));
  fprintf (stream, _(" The options are:\n"));

  if (DEFAULT_STRINGS_ALL)
    fprintf (stream, _("\
  -a - --all                Scan the entire file, not just the data section [default]\n\
  -d --data                 Only scan the data sections in the file\n"));
  else
    fprintf (stream, _("\
  -a - --all                Scan the entire file, not just the data section\n\
  -d --data                 Only scan the data sections in the file [default]\n"));

  fprintf (stream, _("\
  -f --print-file-name      Print the name of the file before each string\n\
  -n --bytes=[number]       Locate & print any NUL-terminated sequence of at\n\
  -<number>                   least [number] characters (default 4).\n\
  -t --radix={o,d,x}        Print the location of the string in base 8, 10 or 16\n\
  -w --include-all-whitespace Include all whitespace as valid string characters\n\
  -o                        An alias for --radix=o\n\
  -T --target=<BFDNAME>     Specify the binary file format\n\
  -e --encoding={s,S,b,l,B,L} Select character size and endianness:\n\
                            s = 7-bit, S = 8-bit, {b,l} = 16-bit, {B,L} = 32-bit\n\
  -s --output-separator=<string> String used to separate strings in output.\n\
  @<file>                   Read options from <file>\n\
  -h --help                 Display this information\n\
  -v -V --version           Print the program's version number\n"));
  list_supported_targets (program_name, stream);
  if (REPORT_BUGS_TO[0] && status == 0)
    fprintf (stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
  exit (status);
}
