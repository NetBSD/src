/* gettext - retrieve text string from message catalog and print it.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, May 1995.

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

#include <getopt.h>
#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
char *getenv ();
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#include "error.h"
#include "system.h"

#include "libgettext.h"

#define _(str) gettext (str)

/* If nonzero add newline after last string.  This makes only sense in
   the `echo' emulation mode.  */
int add_newline;
/* If nonzero expand escape sequences in strings before looking in the
   message catalog.  */
int do_expand;

/* Name the program is called with.  */
char *program_name;

/* Long options.  */
static const struct option long_options[] =
{
  { "domain", required_argument, NULL, 'd' },
  { "help", no_argument, NULL, 'h' },
  { "shell-script", no_argument, NULL, 's' },
  { "version", no_argument, NULL, 'V' },
  { NULL, 0, NULL, 0 }
};

/* Prototypes for local functions.  */
static void usage PARAMS ((int __status))
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
     __attribute__ ((noreturn))
#endif
;
static const char *expand_escape PARAMS((const char *__str));

int
main (argc, argv)
     int argc;
     char *argv[];
{
  int optchar;
  int do_help = 0;
  int do_shell = 0;
  int do_version = 0;
  const char *msgid;
  const char *domain = getenv ("TEXTDOMAIN");
  const char *domaindir = getenv ("TEXTDOMAINDIR");

  /* Set program name for message texts.  */
  program_name = argv[0];
  add_newline = 1;
  do_expand = 0;

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  while ((optchar = getopt_long (argc, argv, "+d:eEhnsV", long_options, NULL))
	 != EOF)
    switch (optchar)
    {
    case '\0':		/* Long option.  */
      break;
    case 'd':
      domain = optarg;
      break;
    case 'e':
      do_expand = 1;
      break;
    case 'E':
      /* Ignore.  Just for compatibility.  */
      break;
    case 'h':
      do_help = 1;
      break;
    case 'n':
      add_newline = 0;
      break;
    case 's':
      do_shell = 1;
      break;
    case 'V':
      do_version = 1;
      break;
    default:
      usage (EXIT_FAILURE);
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
	      "1995, 1996, 1997");
      printf (_("Written by %s.\n"), "Ulrich Drepper");
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* We have two major modes: use following Uniforum spec and as
     internationalized `echo' program.  */
  if (do_shell == 0)
    {
      /* We have to write a single strings translation to stdout.  */

      if (optind >= argc)
	error (EXIT_FAILURE, 0, _("missing arguments"));

      /* Get arguments.  */
      msgid = argv[optind++];
      if (optind < argc)
	{
	  domain = msgid;
	  msgid = argv[optind++];

	  if (optind < argc)
	    error (EXIT_FAILURE, 0, _("too many arguments"));
	}

      /* If no domain name is given we print the original string.  */
      if (domain == NULL || domain[0] == '\0')
	{
	  fputs (msgid, stdout);
	  exit (EXIT_SUCCESS);
	}

      /* Bind domain to appropriate directory.  */
      if (domaindir != NULL && domaindir[0] != '\0')
	bindtextdomain__ (domain, domaindir);

      /* Expand escape sequences is enabled.  */
      if (do_expand)
	msgid = expand_escape (msgid);

      /* Write out the result.  */
      fputs (dgettext__ (domain, msgid), stdout);
    }
  else
    {
      /* If no domain name is given we print the original string.
	 We mark this assigning NULL to domain.  */
      if (domain == NULL || domain[0] == '\0')
	domain = NULL;
      else
	/* Bind domain to appropriate directory.  */
	if (domaindir != NULL && domaindir[0] != '\0')
	  bindtextdomain__ (domain, domaindir);

      /* We have to simulate `echo'.  All arguments are strings.  */
      while (optind < argc)
	{
	  msgid = argv[optind++];

	  /* Expand escape sequences is enabled.  */
	  if (do_expand)
	    msgid = expand_escape (msgid);

	  /* Write out the result.  */
	  fputs (domain == NULL ? msgid : dgettext__ (domain, msgid), stdout);

	  /* We separate the arguments by a single ' '.  */
	  if (optind < argc)
	    fputc (' ', stdout);
	}

      /* If not otherwise told add trailing newline.  */
      if (add_newline)
	fputc ('\n', stdout);
    }

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
Usage: %s [OPTION] [[[TEXTDOMAIN] MSGID] | [-s [MSGID]...]]\n\
  -d, --domain=TEXTDOMAIN   retrieve translated messages from TEXTDOMAIN\n\
  -e                        enable expansion of some escape sequences\n\
  -E                        (ignored for compatibility)\n\
  -h, --help                display this help and exit\n\
  -n                        suppress trailing newline\n\
  -V, --version             display version information and exit\n\
  [TEXTDOMAIN] MSGID        retrieve translated message corresponding\n\
                            to MSGID from TEXTDOMAIN\n"),
	      program_name);
      /* xgettext: no-wrap */
      printf (_("\
\n\
If the TEXTDOMAIN parameter is not given, the domain is determined from the\n\
environment variable TEXTDOMAIN.  If the message catalog is not found in the\n\
regular directory, another location can be specified with the environment\n\
variable TEXTDOMAINDIR.\n\
When used with the -s option the program behaves like the `echo' command.\n\
But it does not simply copy its arguments to stdout.  Instead those messages\n\
found in the selected catalog are translated.\n\
Standard search directory: %s\n"), LOCALEDIR);
      fputs (_("Report bugs to <bug-gnu-utils@gnu.org>.\n"), stdout);
    }

  exit (status);
}


/* Expand some escape sequences found in the argument string.  */
static const char *
expand_escape (str)
     const char *str;
{
  char *retval, *rp;
  const char *cp = str;

  do
    {
      while (cp[0] != '\0' && cp[0] != '\\')
	++cp;
    }
  while (cp[0] != '\0' && cp[1] != '\0'
	 && strchr ("bcfnrt\\01234567", cp[1]) == NULL);

  if (cp[0] == '\0')
    return str;

  retval = (char *) xmalloc (strlen (str));

  rp = retval + (cp - str);
  memcpy (retval, str, cp - str);

  do
    {
      switch (*++cp)
	{
	case 'b':		/* backspace */
	  *rp++ = '\b';
	  ++cp;
	  break;
	case 'c':		/* suppress trailing newline */
	  add_newline = 0;
	  ++cp;
	  break;
	case 'f':		/* form feed */
	  *rp++ = '\f';
	  ++cp;
	  break;
	case 'n':		/* new line */
	  *rp++ = '\n';
	  ++cp;
	  break;
	case 'r':		/* carriage return */
	  *rp++ = '\r';
	  ++cp;
	  break;
	case 't':		/* horizontal tab */
	  *rp++ = '\t';
	  ++cp;
	  break;
	case '\\':
	  *rp = '\\';
	  ++cp;
	  break;
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	  {
	    int ch = *cp++ - '0';

	    if (*cp >= '0' && *cp <= '7')
	      {
		ch *= 8;
		ch += *cp++ - '0';

		if (*cp >= '0' && *cp <= '7')
		  {
		    ch *= 8;
		    ch += *cp++ - '0';
		  }
	      }
	    *rp = ch;
	  }
	  break;
	default:
	  *rp = '\\';
	  break;
	}

      while (cp[0] != '\0' && cp[0] != '\\')
	*rp++ = *cp++;
    }
  while (cp[0] != '\0');

  /* Terminate string.  */
  *rp = '\0';

  return (const char *) retval;
}
