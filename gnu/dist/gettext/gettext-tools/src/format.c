/* Format strings.
   Copyright (C) 2001-2004 Free Software Foundation, Inc.
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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "format.h"

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
  /* format_qt */		&formatstring_qt
};

/* Check whether both formats strings contain compatible format
   specifications.  Return true if there is an error.  */
bool
check_msgid_msgstr_format (const char *msgid, const char *msgid_plural,
			   const char *msgstr, size_t msgstr_len,
			   enum is_format is_format[NFORMATS],
			   formatstring_error_logger_t error_logger)
{
  bool err = false;
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
		    if (parser->check (msgid_descr, msgstr_descr,
				       msgid_plural == NULL,
				       error_logger, pretty_msgstr))
		      err = true;

		    parser->free (msgstr_descr);
		  }
		else
		  {
		    error_logger (_("\
'%s' is not a valid %s format string, unlike 'msgid'. Reason: %s"),
				  pretty_msgstr, format_language_pretty[i],
				  invalid_reason);
		    err = true;
		    free (invalid_reason);
		  }
	      }

	    parser->free (msgid_descr);
	  }
	else
	  free (invalid_reason);
      }

  return err;
}
