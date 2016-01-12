/* Checking of messages in PO files.
   Copyright (C) 1995-1998, 2000-2006 Free Software Foundation, Inc.
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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "msgl-check.h"

#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "c-ctype.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "po-xerror.h"
#include "format.h"
#include "plural-exp.h"
#include "plural-eval.h"
#include "plural-table.h"
#include "c-strstr.h"
#include "vasprintf.h"
#include "exit.h"
#include "message.h"
#include "gettext.h"

#define _(str) gettext (str)

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* Check the values returned by plural_eval.
   Return the number of errors that were seen.
   If no errors, returns in *PLURAL_DISTRIBUTION either NULL or an array
   of length NPLURALS_VALUE describing which plural formula values appear
   infinitely often.  */
static int
check_plural_eval (struct expression *plural_expr,
		   unsigned long nplurals_value,
		   const message_ty *header,
		   unsigned char **plural_distribution)
{
  /* Do as if the plural formula assumes a value N infinitely often if it
     assumes it at least 5 times.  */
#define OFTEN 5
  unsigned char * volatile distribution;

  /* Allocate a distribution array.  */
  if (nplurals_value <= 100)
    distribution = (unsigned char *) xcalloc (nplurals_value, 1);
  else
    /* nplurals_value is nonsense.  Don't risk an out-of-memory.  */
    distribution = NULL;

  if (sigsetjmp (sigfpe_exit, 1) == 0)
    {
      unsigned long n;

      /* Protect against arithmetic exceptions.  */
      install_sigfpe_handler ();

      for (n = 0; n <= 1000; n++)
	{
	  unsigned long val = plural_eval (plural_expr, n);

	  if ((long) val < 0)
	    {
	      /* End of protection against arithmetic exceptions.  */
	      uninstall_sigfpe_handler ();

	      po_xerror (PO_SEVERITY_ERROR, header, NULL, 0, 0, false,
			 _("plural expression can produce negative values"));
	      return 1;
	    }
	  else if (val >= nplurals_value)
	    {
	      char *msg;

	      /* End of protection against arithmetic exceptions.  */
	      uninstall_sigfpe_handler ();

	      msg = xasprintf (_("nplurals = %lu but plural expression can produce values as large as %lu"),
			       nplurals_value, val);
	      po_xerror (PO_SEVERITY_ERROR, header, NULL, 0, 0, false, msg);
	      free (msg);
	      return 1;
	    }

	  if (distribution != NULL && distribution[val] < OFTEN)
	    distribution[val]++;
	}

      /* End of protection against arithmetic exceptions.  */
      uninstall_sigfpe_handler ();

      /* Normalize the distribution[val] statistics.  */
      if (distribution != NULL)
	{
	  unsigned long val;

	  for (val = 0; val < nplurals_value; val++)
	    distribution[val] = (distribution[val] == OFTEN ? 1 : 0);
	}
      *plural_distribution = distribution;

      return 0;
    }
  else
    {
      /* Caught an arithmetic exception.  */
      const char *msg;

      /* End of protection against arithmetic exceptions.  */
      uninstall_sigfpe_handler ();

#if USE_SIGINFO
      switch (sigfpe_code)
#endif
	{
#if USE_SIGINFO
# ifdef FPE_INTDIV
	case FPE_INTDIV:
	  msg = _("plural expression can produce division by zero");
	  break;
# endif
# ifdef FPE_INTOVF
	case FPE_INTOVF:
	  msg = _("plural expression can produce integer overflow");
	  break;
# endif
	default:
#endif
	  msg = _("plural expression can produce arithmetic exceptions, possibly division by zero");
	}

      po_xerror (PO_SEVERITY_ERROR, header, NULL, 0, 0, false, msg);

      if (distribution != NULL)
	free (distribution);

      return 1;
    }
#undef OFTEN
}


/* Try to help the translator by looking up the right plural formula for her.
   Return a freshly allocated multiline help string, or NULL.  */
static char *
plural_help (const char *nullentry)
{
  const char *language;
  size_t j;

  language = c_strstr (nullentry, "Language-Team: ");
  if (language != NULL)
    {
      language += 15;
      for (j = 0; j < plural_table_size; j++)
	if (strncmp (language,
		     plural_table[j].language,
		     strlen (plural_table[j].language)) == 0)
	  {
	    char *helpline1 =
	      xasprintf (_("Try using the following, valid for %s:"),
			 plural_table[j].language);
	    char *help =
	      xasprintf ("%s\n\"Plural-Forms: %s\\n\"\n",
			 helpline1, plural_table[j].value);
	    free (helpline1);
	    return help;
	  }
    }
  return NULL;
}


/* Perform plural expression checking.
   Return the number of errors that were seen.
   If no errors, returns in *PLURAL_DISTRIBUTION either NULL or an array
   describing which plural formula values appear infinitely often.  */
static int
check_plural (message_list_ty *mlp, unsigned char **plural_distribution)
{
  int seen_errors = 0;
  const message_ty *has_plural;
  unsigned long min_nplurals;
  const message_ty *min_pos;
  unsigned long max_nplurals;
  const message_ty *max_pos;
  size_t j;
  message_ty *header;
  unsigned char *distribution = NULL;

  /* Determine whether mlp has plural entries.  */
  has_plural = NULL;
  min_nplurals = ULONG_MAX;
  min_pos = NULL;
  max_nplurals = 0;
  max_pos = NULL;
  for (j = 0; j < mlp->nitems; j++)
    {
      message_ty *mp = mlp->item[j];

      if (!mp->obsolete && mp->msgid_plural != NULL)
	{
	  const char *p;
	  const char *p_end;
	  unsigned long n;

	  if (has_plural == NULL)
	    has_plural = mp;

	  n = 0;
	  for (p = mp->msgstr, p_end = p + mp->msgstr_len;
	       p < p_end;
	       p += strlen (p) + 1)
	    n++;
	  if (min_nplurals > n)
	    {
	      min_nplurals = n;
	      min_pos = mp;
	    }
	  if (max_nplurals < n)
	    {
	      max_nplurals = n;
	      max_pos = mp;
	    }
	}
    }

  /* Look at the plural entry for this domain.
     Cf, function extract_plural_expression.  */
  header = message_list_search (mlp, NULL, "");
  if (header != NULL && !header->obsolete)
    {
      const char *nullentry;
      const char *plural;
      const char *nplurals;

      nullentry = header->msgstr;

      plural = c_strstr (nullentry, "plural=");
      nplurals = c_strstr (nullentry, "nplurals=");
      if (plural == NULL && has_plural != NULL)
	{
	  const char *msg1 =
	    _("message catalog has plural form translations");
	  const char *msg2 =
	    _("but header entry lacks a \"plural=EXPRESSION\" attribute");
	  char *help = plural_help (nullentry);

	  if (help != NULL)
	    {
	      char *msg2ext = xasprintf ("%s\n%s", msg2, help);
	      po_xerror2 (PO_SEVERITY_ERROR,
			  has_plural, NULL, 0, 0, false, msg1,
			  header, NULL, 0, 0, true, msg2ext);
	      free (msg2ext);
	      free (help);
	    }
	  else
	    po_xerror2 (PO_SEVERITY_ERROR,
			has_plural, NULL, 0, 0, false, msg1,
			header, NULL, 0, 0, false, msg2);

	  seen_errors++;
	}
      if (nplurals == NULL && has_plural != NULL)
	{
	  const char *msg1 =
	    _("message catalog has plural form translations");
	  const char *msg2 =
	    _("but header entry lacks a \"nplurals=INTEGER\" attribute");
	  char *help = plural_help (nullentry);

	  if (help != NULL)
	    {
	      char *msg2ext = xasprintf ("%s\n%s", msg2, help);
	      po_xerror2 (PO_SEVERITY_ERROR,
			  has_plural, NULL, 0, 0, false, msg1,
			  header, NULL, 0, 0, true, msg2ext);
	      free (msg2ext);
	      free (help);
	    }
	  else
	    po_xerror2 (PO_SEVERITY_ERROR,
			has_plural, NULL, 0, 0, false, msg1,
			header, NULL, 0, 0, false, msg2);

	  seen_errors++;
	}
      if (plural != NULL && nplurals != NULL)
	{
	  const char *endp;
	  unsigned long int nplurals_value;
	  struct parse_args args;
	  struct expression *plural_expr;

	  /* First check the number.  */
	  nplurals += 9;
	  while (*nplurals != '\0' && c_isspace ((unsigned char) *nplurals))
	    ++nplurals;
	  endp = nplurals;
	  nplurals_value = 0;
	  if (*nplurals >= '0' && *nplurals <= '9')
	    nplurals_value = strtoul (nplurals, (char **) &endp, 10);
	  if (nplurals == endp)
	    {
	      const char *msg = _("invalid nplurals value");
	      char *help = plural_help (nullentry);

	      if (help != NULL)
		{
		  char *msgext = xasprintf ("%s\n%s", msg, help);
		  po_xerror (PO_SEVERITY_ERROR, header, NULL, 0, 0, true,
			     msgext);
		  free (msgext);
		  free (help);
		}
	      else
		po_xerror (PO_SEVERITY_ERROR, header, NULL, 0, 0, false, msg);

	      seen_errors++;
	    }

	  /* Then check the expression.  */
	  plural += 7;
	  args.cp = plural;
	  if (parse_plural_expression (&args) != 0)
	    {
	      const char *msg = _("invalid plural expression");
	      char *help = plural_help (nullentry);

	      if (help != NULL)
		{
		  char *msgext = xasprintf ("%s\n%s", msg, help);
		  po_xerror (PO_SEVERITY_ERROR, header, NULL, 0, 0, true,
			     msgext);
		  free (msgext);
		  free (help);
		}
	      else
		po_xerror (PO_SEVERITY_ERROR, header, NULL, 0, 0, false, msg);

	      seen_errors++;
	    }
	  plural_expr = args.res;

	  /* See whether nplurals and plural fit together.  */
	  if (!seen_errors)
	    seen_errors =
	      check_plural_eval (plural_expr, nplurals_value, header,
				 &distribution);

	  /* Check the number of plurals of the translations.  */
	  if (!seen_errors)
	    {
	      if (min_nplurals < nplurals_value)
		{
		  char *msg1 =
		    xasprintf (_("nplurals = %lu"), nplurals_value);
		  char *msg2 =
		    xasprintf (ngettext ("but some messages have only one plural form",
					 "but some messages have only %lu plural forms",
					 min_nplurals),
			       min_nplurals);
		  po_xerror2 (PO_SEVERITY_ERROR,
			      header, NULL, 0, 0, false, msg1,
			      min_pos, NULL, 0, 0, false, msg2);
		  free (msg2);
		  free (msg1);
		  seen_errors++;
		}
	      else if (max_nplurals > nplurals_value)
		{
		  char *msg1 =
		    xasprintf (_("nplurals = %lu"), nplurals_value);
		  char *msg2 =
		    xasprintf (ngettext ("but some messages have one plural form",
					 "but some messages have %lu plural forms",
					 max_nplurals),
			       max_nplurals);
		  po_xerror2 (PO_SEVERITY_ERROR,
			      header, NULL, 0, 0, false, msg1,
			      max_pos, NULL, 0, 0, false, msg2);
		  free (msg2);
		  free (msg1);
		  seen_errors++;
		}
	      /* The only valid case is max_nplurals <= n <= min_nplurals,
		 which means either has_plural == NULL or
		 max_nplurals = n = min_nplurals.  */
	    }
	}
    }
  else if (has_plural != NULL)
    {
      po_xerror (PO_SEVERITY_ERROR, has_plural, NULL, 0, 0, false,
		 _("message catalog has plural form translations, but lacks a header entry with \"Plural-Forms: nplurals=INTEGER; plural=EXPRESSION;\""));
      seen_errors++;
    }

  /* distribution is not needed if we report errors.
     Also, if there was an error due to  max_nplurals > nplurals_value,
     we must not use distribution because we would be doing out-of-bounds
     array accesses.  */
  if (seen_errors > 0 && distribution != NULL)
    {
      free (distribution);
      distribution = NULL;
    }
  *plural_distribution = distribution;

  return seen_errors;
}


/* Signal an error when checking format strings.  */
static const message_ty *curr_mp;
static lex_pos_ty curr_msgid_pos;
static void
formatstring_error_logger (const char *format, ...)
     __attribute__ ((__format__ (__printf__, 1, 2)));
static void
formatstring_error_logger (const char *format, ...)
{
  va_list args;
  char *msg;

  va_start (args, format);
  if (vasprintf (&msg, format, args) < 0)
    error (EXIT_FAILURE, 0, _("memory exhausted"));
  va_end (args);
  po_xerror (PO_SEVERITY_ERROR,
	     curr_mp, curr_msgid_pos.file_name, curr_msgid_pos.line_number,
	     (size_t)(-1), false, msg);
  free (msg);
}


/* Perform miscellaneous checks on a message.
   PLURAL_DISTRIBUTION is either NULL or an array of nplurals elements,
   PLURAL_DISTRIBUTION[j] being true if the value j appears to be assumed
   infinitely often by the plural formula.  */
static int
check_pair (const message_ty *mp,
	    const char *msgid,
	    const lex_pos_ty *msgid_pos,
	    const char *msgid_plural,
	    const char *msgstr, size_t msgstr_len,
	    const enum is_format is_format[NFORMATS],
	    int check_newlines,
	    int check_format_strings, const unsigned char *plural_distribution,
	    int check_compatibility,
	    int check_accelerators, char accelerator_char)
{
  int seen_errors;
  int has_newline;
  unsigned int j;

  /* If the msgid string is empty we have the special entry reserved for
     information about the translation.  */
  if (msgid[0] == '\0')
    return 0;

  seen_errors = 0;

  if (check_newlines)
    {
      /* Test 1: check whether all or none of the strings begin with a '\n'.  */
      has_newline = (msgid[0] == '\n');
#define TEST_NEWLINE(p) (p[0] == '\n')
      if (msgid_plural != NULL)
	{
	  const char *p;

	  if (TEST_NEWLINE(msgid_plural) != has_newline)
	    {
	      po_xerror (PO_SEVERITY_ERROR,
			 mp, msgid_pos->file_name, msgid_pos->line_number,
			 (size_t)(-1), false, _("\
`msgid' and `msgid_plural' entries do not both begin with '\\n'"));
	      seen_errors++;
	    }
	  for (p = msgstr, j = 0; p < msgstr + msgstr_len; p += strlen (p) + 1, j++)
	    if (TEST_NEWLINE(p) != has_newline)
	      {
		char *msg =
		  xasprintf (_("\
`msgid' and `msgstr[%u]' entries do not both begin with '\\n'"), j);
		po_xerror (PO_SEVERITY_ERROR,
			   mp, msgid_pos->file_name, msgid_pos->line_number,
			   (size_t)(-1), false, msg);
		free (msg);
		seen_errors++;
	      }
	}
      else
	{
	  if (TEST_NEWLINE(msgstr) != has_newline)
	    {
	      po_xerror (PO_SEVERITY_ERROR,
			 mp, msgid_pos->file_name, msgid_pos->line_number,
			 (size_t)(-1), false, _("\
`msgid' and `msgstr' entries do not both begin with '\\n'"));
	      seen_errors++;
	    }
	}
#undef TEST_NEWLINE

      /* Test 2: check whether all or none of the strings end with a '\n'.  */
      has_newline = (msgid[strlen (msgid) - 1] == '\n');
#define TEST_NEWLINE(p) (p[0] != '\0' && p[strlen (p) - 1] == '\n')
      if (msgid_plural != NULL)
	{
	  const char *p;

	  if (TEST_NEWLINE(msgid_plural) != has_newline)
	    {
	      po_xerror (PO_SEVERITY_ERROR,
			 mp, msgid_pos->file_name, msgid_pos->line_number,
			 (size_t)(-1), false, _("\
`msgid' and `msgid_plural' entries do not both end with '\\n'"));
	      seen_errors++;
	    }
	  for (p = msgstr, j = 0; p < msgstr + msgstr_len; p += strlen (p) + 1, j++)
	    if (TEST_NEWLINE(p) != has_newline)
	      {
		char *msg =
		  xasprintf (_("\
`msgid' and `msgstr[%u]' entries do not both end with '\\n'"), j);
		po_xerror (PO_SEVERITY_ERROR,
			   mp, msgid_pos->file_name, msgid_pos->line_number,
			   (size_t)(-1), false, msg);
		free (msg);
		seen_errors++;
	      }
	}
      else
	{
	  if (TEST_NEWLINE(msgstr) != has_newline)
	    {
	      po_xerror (PO_SEVERITY_ERROR,
			 mp, msgid_pos->file_name, msgid_pos->line_number,
			 (size_t)(-1), false, _("\
`msgid' and `msgstr' entries do not both end with '\\n'"));
	      seen_errors++;
	    }
	}
#undef TEST_NEWLINE
    }

  if (check_compatibility && msgid_plural != NULL)
    {
      po_xerror (PO_SEVERITY_ERROR,
		 mp, msgid_pos->file_name, msgid_pos->line_number,
		 (size_t)(-1), false, _("\
plural handling is a GNU gettext extension"));
      seen_errors++;
    }

  if (check_format_strings)
    /* Test 3: Check whether both formats strings contain the same number
       of format specifications.  */
    {
      curr_mp = mp;
      curr_msgid_pos = *msgid_pos;
      seen_errors +=
	check_msgid_msgstr_format (msgid, msgid_plural, msgstr, msgstr_len,
				   is_format, plural_distribution,
				   formatstring_error_logger);
    }

  if (check_accelerators && msgid_plural == NULL)
    /* Test 4: Check that if msgid is a menu item with a keyboard accelerator,
       the msgstr has an accelerator as well.  A keyboard accelerator is
       designated by an immediately preceding '&'.  We cannot check whether
       two accelerators collide, only whether the translator has bothered
       thinking about them.  */
    {
      const char *p;

      /* We are only interested in msgids that contain exactly one '&'.  */
      p = strchr (msgid, accelerator_char);
      if (p != NULL && strchr (p + 1, accelerator_char) == NULL)
	{
	  /* Count the number of '&' in msgstr, but ignore '&&'.  */
	  unsigned int count = 0;

	  for (p = msgstr; (p = strchr (p, accelerator_char)) != NULL; p++)
	    if (p[1] == accelerator_char)
	      p++;
	    else
	      count++;

	  if (count == 0)
	    {
	      char *msg =
		xasprintf (_("msgstr lacks the keyboard accelerator mark '%c'"),
			   accelerator_char);
	      po_xerror (PO_SEVERITY_ERROR,
			 mp, msgid_pos->file_name, msgid_pos->line_number,
			 (size_t)(-1), false, msg);
	      free (msg);
	    }
	  else if (count > 1)
	    {
	      char *msg =
		xasprintf (_("msgstr has too many keyboard accelerator marks '%c'"),
			   accelerator_char);
	      po_xerror (PO_SEVERITY_ERROR,
			 mp, msgid_pos->file_name, msgid_pos->line_number,
			 (size_t)(-1), false, msg);
	      free (msg);
	    }
	}
    }

  return seen_errors;
}


/* Perform miscellaneous checks on a header entry.  */
static void
check_header_entry (const message_ty *mp, const char *msgstr_string)
{
  static const char *required_fields[] =
  {
    "Project-Id-Version", "PO-Revision-Date", "Last-Translator",
    "Language-Team", "MIME-Version", "Content-Type",
    "Content-Transfer-Encoding"
  };
  static const char *default_values[] =
  {
    "PACKAGE VERSION", "YEAR-MO-DA", "FULL NAME", "LANGUAGE", NULL,
    "text/plain; charset=CHARSET", "ENCODING"
  };
  const size_t nfields = SIZEOF (required_fields);
  int initial = -1;
  int cnt;

  for (cnt = 0; cnt < nfields; ++cnt)
    {
      char *endp = c_strstr (msgstr_string, required_fields[cnt]);

      if (endp == NULL)
	{
	  char *msg =
	    xasprintf (_("headerfield `%s' missing in header\n"),
		       required_fields[cnt]);
	  po_xerror (PO_SEVERITY_ERROR, mp, NULL, 0, 0, true, msg);
	  free (msg);
	}
      else if (endp != msgstr_string && endp[-1] != '\n')
	{
	  char *msg =
	    xasprintf (_("\
header field `%s' should start at beginning of line\n"),
		       required_fields[cnt]);
	  po_xerror (PO_SEVERITY_ERROR, mp, NULL, 0, 0, true, msg);
	  free (msg);
	}
      else if (default_values[cnt] != NULL
	       && strncmp (default_values[cnt],
			   endp + strlen (required_fields[cnt]) + 2,
			   strlen (default_values[cnt])) == 0)
	{
	  if (initial != -1)
	    {
	      po_xerror (PO_SEVERITY_ERROR,
			 mp, NULL, 0, 0, true, _("\
some header fields still have the initial default value\n"));
	      initial = -1;
	      break;
	    }
	  else
	    initial = cnt;
	}
    }

  if (initial != -1)
    {
      char *msg =
	xasprintf (_("field `%s' still has initial default value\n"),
		   required_fields[initial]);
      po_xerror (PO_SEVERITY_ERROR, mp, NULL, 0, 0, true, msg);
      free (msg);
    }
}


/* Perform all checks on a non-obsolete message.
   PLURAL_DISTRIBUTION is either NULL or an array of nplurals elements,
   PLURAL_DISTRIBUTION[j] being true if the value j appears to be assumed
   infinitely often by the plural formula.
   Return the number of errors that were seen.  */
int
check_message (const message_ty *mp,
	       const lex_pos_ty *msgid_pos,
	       int check_newlines,
	       int check_format_strings, const unsigned char *plural_distribution,
	       int check_header,
	       int check_compatibility,
	       int check_accelerators, char accelerator_char)
{
  if (check_header && is_header (mp))
    check_header_entry (mp, mp->msgstr);

  return check_pair (mp,
		     mp->msgid, msgid_pos, mp->msgid_plural,
		     mp->msgstr, mp->msgstr_len,
		     mp->is_format,
		     check_newlines,
		     check_format_strings, plural_distribution,
		     check_compatibility,
		     check_accelerators, accelerator_char);
}


/* Perform all checks on a message list.
   Return the number of errors that were seen.  */
int
check_message_list (message_list_ty *mlp,
		    int check_newlines,
		    int check_format_strings,
		    int check_header,
		    int check_compatibility,
		    int check_accelerators, char accelerator_char)
{
  int seen_errors = 0;
  unsigned char *plural_distribution = NULL;
  size_t j;

  if (check_header)
    seen_errors += check_plural (mlp, &plural_distribution);

  for (j = 0; j < mlp->nitems; j++)
    {
      message_ty *mp = mlp->item[j];

      if (!mp->obsolete)
	seen_errors += check_message (mp, &mp->pos,
				      check_newlines,
				      check_format_strings, plural_distribution,
				      check_header, check_compatibility,
				      check_accelerators, accelerator_char);
    }

  return seen_errors;
}
