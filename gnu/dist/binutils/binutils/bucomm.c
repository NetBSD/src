/* bucomm.c -- Bin Utils COMmon code.
   Copyright 1991, 1992, 1993, 1994, 1995, 1997, 1998, 2000, 2001, 2002, 2003
   Free Software Foundation, Inc.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* We might put this in a library someday so it could be dynamically
   loaded, but for now it's not necessary.  */

#include "bfd.h"
#include "bfdver.h"
#include "libiberty.h"
#include "bucomm.h"
#include "filenames.h"
#include "libbfd.h"

#include <sys/stat.h>
#include <time.h>		/* ctime, maybe time_t */

#ifndef HAVE_TIME_T_IN_TIME_H
#ifndef HAVE_TIME_T_IN_TYPES_H
typedef long time_t;
#endif
#endif

static const char * endian_string (enum bfd_endian);
static int display_target_list (void);
static int display_info_table (int, int);
static int display_target_tables (void);

/* Error reporting.  */

char *program_name;

void
bfd_nonfatal (const char *string)
{
  const char *errmsg = bfd_errmsg (bfd_get_error ());

  if (string)
    fprintf (stderr, "%s: %s: %s\n", program_name, string, errmsg);
  else
    fprintf (stderr, "%s: %s\n", program_name, errmsg);
}

void
bfd_fatal (const char *string)
{
  bfd_nonfatal (string);
  xexit (1);
}

void
report (const char * format, va_list args)
{
  fprintf (stderr, "%s: ", program_name);
  vfprintf (stderr, format, args);
  putc ('\n', stderr);
}

void
fatal VPARAMS ((const char *format, ...))
{
  VA_OPEN (args, format);
  VA_FIXEDARG (args, const char *, format);

  report (format, args);
  VA_CLOSE (args);
  xexit (1);
}

void
non_fatal VPARAMS ((const char *format, ...))
{
  VA_OPEN (args, format);
  VA_FIXEDARG (args, const char *, format);

  report (format, args);
  VA_CLOSE (args);
}

/* Set the default BFD target based on the configured target.  Doing
   this permits the binutils to be configured for a particular target,
   and linked against a shared BFD library which was configured for a
   different target.  */

void
set_default_bfd_target (void)
{
  /* The macro TARGET is defined by Makefile.  */
  const char *target = TARGET;

  if (! bfd_set_default_target (target))
    fatal (_("can't set BFD default target to `%s': %s"),
	   target, bfd_errmsg (bfd_get_error ()));
}

/* After a FALSE return from bfd_check_format_matches with
   bfd_get_error () == bfd_error_file_ambiguously_recognized, print
   the possible matching targets.  */

void
list_matching_formats (char **p)
{
  fprintf (stderr, _("%s: Matching formats:"), program_name);
  while (*p)
    fprintf (stderr, " %s", *p++);
  fputc ('\n', stderr);
}

/* List the supported targets.  */

void
list_supported_targets (const char *name, FILE *f)
{
  int t;
  const char **targ_names = bfd_target_list ();

  if (name == NULL)
    fprintf (f, _("Supported targets:"));
  else
    fprintf (f, _("%s: supported targets:"), name);

  for (t = 0; targ_names[t] != NULL; t++)
    fprintf (f, " %s", targ_names[t]);
  fprintf (f, "\n");
  free (targ_names);
}

/* List the supported architectures.  */

void
list_supported_architectures (const char *name, FILE *f)
{
  const char **arch;

  if (name == NULL)
    fprintf (f, _("Supported architectures:"));
  else
    fprintf (f, _("%s: supported architectures:"), name);

  for (arch = bfd_arch_list (); *arch; arch++)
    fprintf (f, " %s", *arch);
  fprintf (f, "\n");
}

/* The length of the longest architecture name + 1.  */
#define LONGEST_ARCH sizeof ("powerpc:common")

static const char *
endian_string (enum bfd_endian endian)
{
  switch (endian)
    {
    case BFD_ENDIAN_BIG: return "big endian";
    case BFD_ENDIAN_LITTLE: return "little endian";
    default: return "endianness unknown";
    }
}

/* List the targets that BFD is configured to support, each followed
   by its endianness and the architectures it supports.  */

static int
display_target_list (void)
{
  char *dummy_name;
  int t;
  int ret = 1;

  dummy_name = make_temp_file (NULL);
  for (t = 0; bfd_target_vector[t]; t++)
    {
      const bfd_target *p = bfd_target_vector[t];
      bfd *abfd = bfd_openw (dummy_name, p->name);
      int a;

      printf ("%s\n (header %s, data %s)\n", p->name,
	      endian_string (p->header_byteorder),
	      endian_string (p->byteorder));

      if (abfd == NULL)
	{
          bfd_nonfatal (dummy_name);
          ret = 0;
	  continue;
	}

      if (! bfd_set_format (abfd, bfd_object))
	{
	  if (bfd_get_error () != bfd_error_invalid_operation)
            {
	      bfd_nonfatal (p->name);
              ret = 0;
            }
	  bfd_close_all_done (abfd);
	  continue;
	}

      for (a = (int) bfd_arch_obscure + 1; a < (int) bfd_arch_last; a++)
	if (bfd_set_arch_mach (abfd, (enum bfd_architecture) a, 0))
	  printf ("  %s\n",
		  bfd_printable_arch_mach ((enum bfd_architecture) a, 0));
      bfd_close_all_done (abfd);
    }
  unlink (dummy_name);
  free (dummy_name);

  return ret;
}

/* Print a table showing which architectures are supported for entries
   FIRST through LAST-1 of bfd_target_vector (targets across,
   architectures down).  */

static int
display_info_table (int first, int last)
{
  int t;
  int a;
  int ret = 1;
  char *dummy_name;

  /* Print heading of target names.  */
  printf ("\n%*s", (int) LONGEST_ARCH, " ");
  for (t = first; t < last && bfd_target_vector[t]; t++)
    printf ("%s ", bfd_target_vector[t]->name);
  putchar ('\n');

  dummy_name = make_temp_file (NULL);
  for (a = (int) bfd_arch_obscure + 1; a < (int) bfd_arch_last; a++)
    if (strcmp (bfd_printable_arch_mach (a, 0), "UNKNOWN!") != 0)
      {
	printf ("%*s ", (int) LONGEST_ARCH - 1,
		bfd_printable_arch_mach (a, 0));
	for (t = first; t < last && bfd_target_vector[t]; t++)
	  {
	    const bfd_target *p = bfd_target_vector[t];
	    bfd_boolean ok = TRUE;
	    bfd *abfd = bfd_openw (dummy_name, p->name);

	    if (abfd == NULL)
	      {
		bfd_nonfatal (p->name);
                ret = 0;
		ok = FALSE;
	      }

	    if (ok)
	      {
		if (! bfd_set_format (abfd, bfd_object))
		  {
		    if (bfd_get_error () != bfd_error_invalid_operation)
                      {
		        bfd_nonfatal (p->name);
                        ret = 0;
                      }
		    ok = FALSE;
		  }
	      }

	    if (ok)
	      {
		if (! bfd_set_arch_mach (abfd, a, 0))
		  ok = FALSE;
	      }

	    if (ok)
	      printf ("%s ", p->name);
	    else
	      {
		int l = strlen (p->name);
		while (l--)
		  putchar ('-');
		putchar (' ');
	      }
	    if (abfd != NULL)
	      bfd_close_all_done (abfd);
	  }
	putchar ('\n');
      }
  unlink (dummy_name);
  free (dummy_name);

  return ret;
}

/* Print tables of all the target-architecture combinations that
   BFD has been configured to support.  */

static int
display_target_tables (void)
{
  int t;
  int columns;
  int ret = 1;
  char *colum;

  columns = 0;
  colum = getenv ("COLUMNS");
  if (colum != NULL)
    columns = atoi (colum);
  if (columns == 0)
    columns = 80;

  t = 0;
  while (bfd_target_vector[t] != NULL)
    {
      int oldt = t, wid;

      wid = LONGEST_ARCH + strlen (bfd_target_vector[t]->name) + 1;
      ++t;
      while (wid < columns && bfd_target_vector[t] != NULL)
	{
	  int newwid;

	  newwid = wid + strlen (bfd_target_vector[t]->name) + 1;
	  if (newwid >= columns)
	    break;
	  wid = newwid;
	  ++t;
	}
      if (! display_info_table (oldt, t))
        ret = 0;
    }

  return ret;
}

int
display_info (void)
{
  printf (_("BFD header file version %s\n"), BFD_VERSION_STRING);
  if (! display_target_list () || ! display_target_tables ())
    return 1;
  else
    return 0;
}

/* Display the archive header for an element as if it were an ls -l listing:

   Mode       User\tGroup\tSize\tDate               Name */

void
print_arelt_descr (FILE *file, bfd *abfd, bfd_boolean verbose)
{
  struct stat buf;

  if (verbose)
    {
      if (bfd_stat_arch_elt (abfd, &buf) == 0)
	{
	  char modebuf[11];
	  char timebuf[40];
	  time_t when = buf.st_mtime;
	  const char *ctime_result = (const char *) ctime (&when);

	  /* POSIX format:  skip weekday and seconds from ctime output.  */
	  sprintf (timebuf, "%.12s %.4s", ctime_result + 4, ctime_result + 20);

	  mode_string (buf.st_mode, modebuf);
	  modebuf[10] = '\0';
	  /* POSIX 1003.2/D11 says to skip first character (entry type).  */
	  fprintf (file, "%s %ld/%ld %6ld %s ", modebuf + 1,
		   (long) buf.st_uid, (long) buf.st_gid,
		   (long) buf.st_size, timebuf);
	}
    }

  fprintf (file, "%s\n", bfd_get_filename (abfd));
}

/* Return the name of a temporary file in the same directory as FILENAME.  */

char *
make_tempname (char *filename)
{
  static char template[] = "stXXXXXX";
  char *tmpname;
  char *slash = strrchr (filename, '/');

#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  {
    /* We could have foo/bar\\baz, or foo\\bar, or d:bar.  */
    char *bslash = strrchr (filename, '\\');
    if (slash == NULL || (bslash != NULL && bslash > slash))
      slash = bslash;
    if (slash == NULL && filename[0] != '\0' && filename[1] == ':')
      slash = filename + 1;
  }
#endif

  if (slash != (char *) NULL)
    {
      char c;

      c = *slash;
      *slash = 0;
      tmpname = xmalloc (strlen (filename) + sizeof (template) + 2);
      strcpy (tmpname, filename);
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
      /* If tmpname is "X:", appending a slash will make it a root
	 directory on drive X, which is NOT the same as the current
	 directory on drive X.  */
      if (tmpname[1] == ':' && tmpname[2] == '\0')
	strcat (tmpname, ".");
#endif
      strcat (tmpname, "/");
      strcat (tmpname, template);
      mktemp (tmpname);
      *slash = c;
    }
  else
    {
      tmpname = xmalloc (sizeof (template));
      strcpy (tmpname, template);
      mktemp (tmpname);
    }
  return tmpname;
}

/* Parse a string into a VMA, with a fatal error if it can't be
   parsed.  */

bfd_vma
parse_vma (const char *s, const char *arg)
{
  bfd_vma ret;
  const char *end;

  ret = bfd_scan_vma (s, &end, 0);

  if (*end != '\0')
    fatal (_("%s: bad number: %s"), arg, s);

  return ret;
}

/* Returns the size of the named file.  If the file does not
   exist, or if it is not a real file, then a suitable non-fatal
   error message is printed and zero is returned.  */

off_t
get_file_size (const char * file_name)
{
  struct stat statbuf;
  
  if (stat (file_name, &statbuf) < 0)
    {
      if (errno == ENOENT)
	non_fatal (_("'%s': No such file"), file_name);
      else
	non_fatal (_("Warning: could not locate '%s'.  reason: %s"),
		   file_name, strerror (errno));
    }  
  else if (! S_ISREG (statbuf.st_mode))
    {
      if (!S_ISCHR(statbuf.st_mode))
	{
	  non_fatal (_("Warning: '%s' is not an ordinary file"), file_name);
	  return 0;
	}
      return statbuf.st_size ? statbuf.st_size : 1;
    }
  else
    return statbuf.st_size;

  return 0;
}
