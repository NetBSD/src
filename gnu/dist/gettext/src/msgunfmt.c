/* msgunfmt - converts binary .mo files to Uniforum style .po files
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, April 1995.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#include "hash.h"

#include "error.h"
#include "getline.h"
#include "printf.h"
#include <system.h>

#include "gettext.h"
#include "domain.h"
#include "hash-string.h"
#include <libintl.h>
#include "message.h"

#define _(str) gettext (str)

#ifndef errno
extern int errno;
#endif


/* String containing name the program is called with.  */
const char *program_name;

/* Force output of PO file even if empty.  */
static int force_po;

/* Long options.  */
static const struct option long_options[] =
{
  { "escape", no_argument, NULL, 'E' },
  { "force-po", no_argument, &force_po, 1 },
  { "help", no_argument, NULL, 'h' },
  { "indent", no_argument, NULL, 'i' },
  { "no-escape", no_argument, NULL, 'e' },
  { "output-file", required_argument, NULL, 'o' },
  { "strict", no_argument, NULL, 'S' },
  { "version", no_argument, NULL, 'V' },
  { "width", required_argument, NULL, 'w', },
  { NULL, 0, NULL, 0 }
};


/* This defines the byte order within the file.  It needs to be set
   appropriately once we have the file open.  */
static enum { MO_LITTLE_ENDIAN, MO_BIG_ENDIAN } endian;


/* Prototypes for local functions.  */
static void usage PARAMS ((int __status));
static void error_print PARAMS ((void));
static nls_uint32 read32 PARAMS ((FILE *__fp, const char *__fn));
static void seek32 PARAMS ((FILE *__fp, const char *__fn, long __offset));
static char *string32 PARAMS ((FILE *__fp, const char *__fn, long __offset));
static message_list_ty *read_mo_file PARAMS ((message_list_ty *__mlp,
					      const char *__fn));


int
main (argc, argv)
     int argc;
     char **argv;
{
  int optchar;
  int do_help = 0;
  int do_version = 0;
  const char *output_file = "-";
  message_list_ty *mlp = NULL;

  /* Set program name for messages.  */
  program_name = argv[0];
  error_print_progname = error_print;

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  while ((optchar = getopt_long (argc, argv, "hio:Vw:", long_options, NULL))
	 != EOF)
    switch (optchar)
      {
      case '\0':
	/* long option */
	break;

      case 'e':
	message_print_style_escape (0);
	break;

      case 'E':
	message_print_style_escape (1);
	break;

      case 'h':
	do_help = 1;
	break;

      case 'i':
	message_print_style_indent ();
	break;

      case 'o':
	output_file = optarg;
	break;

      case 'S':
	message_print_style_uniforum ();
	break;

      case 'V':
	do_version = 1;
	break;

      case 'w':
	{
	  int value;
	  char *endp;
	  value = strtol (optarg, &endp, 10);
	  if (endp != optarg)
	    message_page_width_set (value);
	}
	break;

      default:
	usage (EXIT_FAILURE);
	break;
      }

  /* Version information is requested.  */
  if (do_version)
    {
      printf ("%s (GNU %s) %s\n", basename (program_name), PACKAGE, VERSION);
      /* xgettext: no-wrap */
      printf (_("Copyright (C) %s Free Software Foundation, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"),
	      "1995, 1996, 1997, 1998");
      printf (_("Written by %s.\n"), "Ulrich Drepper");
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Read the given .mo file. */
  if (optind < argc)
    do
      mlp = read_mo_file (mlp, argv[optind]);
    while (++optind < argc);
  else
    mlp = read_mo_file (NULL, "-");

  /* Write the resulting message list to the given .po file.  */
  message_list_print (mlp, output_file, force_po, 0);

  /* No problems.  */
  exit (EXIT_SUCCESS);
}


/* Display usage information and exit.  */
static void
usage (status)
     int status;
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      /* xgettext: no-wrap */
      printf (_("\
Usage: %s [OPTION] [FILE]...\n\
Mandatory arguments to long options are mandatory for short options too.\n\
  -e, --no-escape          do not use C escapes in output (default)\n\
  -E, --escape             use C escapes in output, no extended chars\n\
      --force-po           write PO file even if empty\n\
  -h, --help               display this help and exit\n\
  -i, --indent             write indented output style\n\
  -o, --output-file=FILE   write output into FILE instead of standard output\n\
      --strict             write strict uniforum style\n\
  -V, --version            output version information and exit\n\
  -w, --width=NUMBER       set output page width\n"),
	      program_name);
      /* xgettext: no-wrap */
      fputs (_("\n\
Convert binary .mo files to Uniforum style .po files.\n\
Both little-endian and big-endian .mo files are handled.\n\
If no input file is given or it is -, standard input is read.\n\
By default the output is written to standard output.\n"), stdout);
      fputs (_("Report bugs to <bug-gnu-utils@gnu.org>.\n"),
	     stdout);
    }

  exit (status);
}


/* The address of this function will be assigned to the hook in the error
   functions.  */
static void
error_print ()
{
  /* We don't want the program name to be printed in messages.  Emacs'
     compile.el does not like this.  */
}


/* This function reads a 32-bit number from the file, and assembles it
   according to the current ``endian'' setting.  */
static nls_uint32
read32 (fp, fn)
     FILE *fp;
     const char *fn;
{
  int c1, c2, c3, c4;

  c1 = getc (fp);
  if (c1 == EOF)
    {
    bomb:
      if (ferror (fp))
	error (EXIT_FAILURE, errno, _("error while reading \"%s\""), fn);
      error (EXIT_FAILURE, 0, _("file \"%s\" truncated"), fn);
    }
  c2 = getc (fp);
  if (c2 == EOF)
    goto bomb;
  c3 = getc (fp);
  if (c3 == EOF)
    goto bomb;
  c4 = getc (fp);
  if (c4 == EOF)
    goto bomb;
  if (endian == MO_LITTLE_ENDIAN)
    return (((nls_uint32) c1)
	    | ((nls_uint32) c2 << 8)
	    | ((nls_uint32) c3 << 16)
	    | ((nls_uint32) c4 << 24));

  return (((nls_uint32) c1 << 24)
	  | ((nls_uint32) c2 << 16)
	  | ((nls_uint32) c3 << 8)
	  | ((nls_uint32) c4));
}


static void
seek32 (fp, fn, offset)
     FILE *fp;
     const char *fn;
     long offset;
{
  if (fseek (fp, offset, 0) < 0)
    error (EXIT_FAILURE, errno, _("seek \"%s\" offset %ld failed"),
	   fn, offset);
}


static char *
string32 (fp, fn, offset)
     FILE *fp;
     const char *fn;
     long offset;
{
  long length;
  char *buffer;
  long n;

  /* Read the string_desc structure, describing where in the file to
     find the string.  */
  seek32 (fp, fn, offset);
  length = read32 (fp, fn);
  offset = read32 (fp, fn);

  /* Allocate memory for the string to be read into.  Leave space for
     the NUL on the end.  */
  buffer = xmalloc (length + 1);

  /* Read in the string.  Complain if there is an error or it comes up
     short.  Add the NUL ourselves.  */
  seek32 (fp, fn, offset);
  n = fread (buffer, 1, length, fp);
  if (n != length)
    {
      if (ferror (fp))
	error (EXIT_FAILURE, errno, _("error while reading \"%s\""), fn);
      error (EXIT_FAILURE, 0, _("file \"%s\" truncated"), fn);
    }
  buffer[length] = 0;

  /* Return the string to the caller.  */
  return buffer;
}


/* This function reads and existing .mo file.  Return a message list.  */
static message_list_ty *
read_mo_file (mlp, fn)
     message_list_ty *mlp;
     const char *fn;
{
  FILE *fp;
  struct mo_file_header header;
  int j;

  if (strcmp (fn, "-") == 0 || strcmp (fn, "/dev/stdout") == 0)
    fp = stdin;
  else
    {
      fp = fopen (fn, "rb");
      if (fp == NULL)
	error (EXIT_FAILURE, errno,
	       _("error while opening \"%s\" for reading"), fn);
    }

  /* We must grope the file to determine which endian it is.
     Perversity of the universe tends towards maximum, so it will
     probably not match the currently executing architecture.  */
  endian = MO_BIG_ENDIAN;
  header.magic = read32 (fp, fn);
  if (header.magic != _MAGIC)
    {
      endian = MO_LITTLE_ENDIAN;
      seek32 (fp, fn, 0L);
      header.magic = read32 (fp, fn);
      if (header.magic != _MAGIC)
	{
	unrecognised:
	  error (EXIT_FAILURE, 0, _("file \"%s\" is not in GNU .mo format"),
		 fn);
	}
    }

  /* Fill the structure describing the header.  */
  header.revision = read32 (fp, fn);
  if (header.revision != MO_REVISION_NUMBER)
    goto unrecognised;
  header.nstrings = read32 (fp, fn);
  header.orig_tab_offset = read32 (fp, fn);
  header.trans_tab_offset = read32 (fp, fn);
  header.hash_tab_size = read32 (fp, fn);
  header.hash_tab_offset = read32 (fp, fn);

  if (mlp == NULL)
    mlp = message_list_alloc ();
  for (j = 0; j < header.nstrings; ++j)
    {
      static lex_pos_ty pos = { __FILE__, __LINE__ };
      message_ty *mp;
      char *msgid;
      char *msgstr;

      /* Read the msgid.  */
      msgid = string32 (fp, fn, header.orig_tab_offset + j * 8);

      /* Read the msgstr.  */
      msgstr = string32 (fp, fn, header.trans_tab_offset + j * 8);

      mp = message_alloc (msgid);
      message_variant_append (mp, MESSAGE_DOMAIN_DEFAULT, msgstr, &pos);
      message_list_append (mlp, mp);
    }

  if (fp != stdin)
    fclose (fp);
  return mlp;
}
