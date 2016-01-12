/* Format strings.
   Copyright (C) 2001-2006 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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
#include "format.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "message.h"
#include "gettext.h"

#define _(str) gettext (str)

/* Table of all format string parsers.  */
struct formatstring_parser *formatstring_parsers[NFORMATS] =
{
  /* format_c */		&formatstring_c,
  /* format_objc */		&formatstring_objc,
  /* format_sh */		&formatstring_sh,
  /* format_python */		&formatstring_python,
  /* format_lisp */		&formatstring_lisp,
  /* format_elisp */		&formatstring_elisp,
  /* format_librep */		&formatstring_librep,
  /* format_scheme */		&formatstring_scheme,
  /* format_smalltalk */	&formatstring_smalltalk,
  /* format_java */		&formatstring_java,
  /* format_csharp */		&formatstring_csharp,
  /* format_awk */		&formatstring_awk,
  /* format_pascal */		&formatstring_pascal,
  /* format_ycp */		&formatstring_ycp,
  /* format_tcl */		&formatstring_tcl,
  /* format_perl */		&formatstring_perl,
  /* format_perl_brace */	&formatstring_perl_brace,
  /* format_php */		&formatstring_php,
  /* format_gcc_internal */	&formatstring_gcc_internal,
  /* format_qt */		&formatstring_qt,
  /* format_boost */		&formatstring_boost
};

/* Check whether both formats strings contain compatible format
   specifications.
   PLURAL_DISTRIBUTION is either NULL or an array of nplurals elements,
   PLURAL_DISTRIBUTION[j] being true if the value j appears to be assumed
   infinitely often by the plural formula.
   Return the number of errors that were seen.  */
int
check_msgid_msgstr_format (const char *msgid, const char *msgid_plural,
			   const char *msgstr, size_t msgstr_len,
			   const enum is_format is_format[NFORMATS],
			   const unsigned char *plural_distribution,
			   formatstring_error_logger_t error_logger)
{
  int seen_errors = 0;
  size_t i;
  unsigned int j;

  /* We check only those messages for which the msgid's is_format flag
     is one of 'yes' or 'possible'.  We don't check msgids with is_format
     'no' or 'impossible', to obey the programmer's order.  We don't check
     msgids with is_format 'undecided' because that would introduce too
     many checks, thus forcing the programmer to add "xgettext: no-c-format"
     anywhere where a translator wishes to use a percent sign.  */
  for (i = 0; i < NFORMATS; i++)
    if (possible_format_p (is_format[i]))
      {
	/* At runtime, we can assume the program passes arguments that
	   fit well for msgid.  We must signal an error if msgstr wants
	   more arguments that msgid accepts.
	   If msgstr wants fewer arguments than msgid, it wouldn't lead
	   to a crash at runtime, but we nevertheless give an error because
	   1) this situation occurs typically after the programmer has
	      added some arguments to msgid, so we must make the translator
	      specially aware of it (more than just "fuzzy"),
	   2) it is generally wrong if a translation wants to ignore
	      arguments that are used by other translations.  */

	struct formatstring_parser *parser = formatstring_parsers[i];
	char *invalid_reason = NULL;
	void *msgid_descr =
	  parser->parse (msgid_plural != NULL ? msgid_plural : msgid,
			 false, &invalid_reason);

	if (msgid_descr != NULL)
	  {
	    char buf[18+1];
	    const char *pretty_msgstr = "msgstr";
	    bool has_plural_translations = (strlen (msgstr) + 1 < msgstr_len);
	    const char *p_end = msgstr + msgstr_len;
	    const char *p;

	    for (p = msgstr, j = 0; p < p_end; p += strlen (p) + 1, j++)
	      {
		void *msgstr_descr;

		if (msgid_plural != NULL)
		  {
		    sprintf (buf, "msgstr[%u]", j);
		    pretty_msgstr = buf;
		  }

		msgstr_descr = parser->parse (p, true, &invalid_reason);

		if (msgstr_descr != NULL)
		  {
		    /* Use strict checking (require same number of format
		       directives on both sides) if the message has no plurals,
		       or if msgid_plural exists but on the msgstr[] side
		       there is only msgstr[0], or if plural_distribution[j]
		       indicates that the variant applies to infinitely many
		       values of N.
		       Use relaxed checking when there are at least two
		       msgstr[] forms and the plural_distribution array does
		       not give more precise information.  */
		    bool strict_checking =
		      (msgid_plural == NULL
		       || !has_plural_translations
		       || (plural_distribution != NULL && plural_distribution[j]));

		    if (parser->check (msgid_descr, msgstr_descr,
				       strict_checking,
				       error_logger, pretty_msgstr))
		      seen_errors++;

		    parser->free (msgstr_descr);
		  }
		else
		  {
		    error_logger (_("\
'%s' is not a valid %s format string, unlike 'msgid'. Reason: %s"),
				  pretty_msgstr, format_language_pretty[i],
				  invalid_reason);
		    seen_errors++;
		    free (invalid_reason);
		  }
	      }

	    parser->free (msgid_descr);
	  }
	else
	  free (invalid_reason);
      }

  return seen_errors;
}
