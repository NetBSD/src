/* Message list charset and locale charset handling.
   Copyright (C) 2001-2003 Free Software Foundation, Inc.
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
# include "config.h"
#endif
#include <alloca.h>

/* Specification.  */
#include "msgl-iconv.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_ICONV
# include <iconv.h>
#endif

#include "error.h"
#include "progname.h"
#include "basename.h"
#include "message.h"
#include "po-charset.h"
#include "msgl-ascii.h"
#include "xalloc.h"
#include "xallocsa.h"
#include "strstr.h"
#include "exit.h"
#include "gettext.h"

#define _(str) gettext (str)


#if HAVE_ICONV

/* Converts an entire string from one encoding to another, using iconv.
   Return value: 0 if successful, otherwise -1 and errno set.  */
static int
iconv_string (iconv_t cd, const char *start, const char *end,
	      char **resultp, size_t *lengthp)
{
#define tmpbufsize 4096
  size_t length;
  char *result;

  /* Avoid glibc-2.1 bug and Solaris 2.7-2.9 bug.  */
# if defined _LIBICONV_VERSION \
    || !((__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) || defined __sun)
  /* Set to the initial state.  */
  iconv (cd, NULL, NULL, NULL, NULL);
# endif

  /* Determine the length we need.  */
  {
    size_t count = 0;
    char tmpbuf[tmpbufsize];
    const char *inptr = start;
    size_t insize = end - start;

    while (insize > 0)
      {
	char *outptr = tmpbuf;
	size_t outsize = tmpbufsize;
	size_t res = iconv (cd,
			    (ICONV_CONST char **) &inptr, &insize,
			    &outptr, &outsize);

	if (res == (size_t)(-1))
	  {
	    if (errno == E2BIG)
	      ;
	    else if (errno == EINVAL)
	      break;
	    else
	      return -1;
	  }
# if !defined _LIBICONV_VERSION && (defined sgi || defined __sgi)
	/* Irix iconv() inserts a NUL byte if it cannot convert.  */
	else if (res > 0)
	  return -1;
# endif
	count += outptr - tmpbuf;
      }
    /* Avoid glibc-2.1 bug and Solaris 2.7 bug.  */
# if defined _LIBICONV_VERSION \
    || !((__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) || defined __sun)
    {
      char *outptr = tmpbuf;
      size_t outsize = tmpbufsize;
      size_t res = iconv (cd, NULL, NULL, &outptr, &outsize);

      if (res == (size_t)(-1))
	return -1;
      count += outptr - tmpbuf;
    }
# endif
    length = count;
  }

  *lengthp = length;
  *resultp = result = xrealloc (*resultp, length);
  if (length == 0)
    return 0;

  /* Avoid glibc-2.1 bug and Solaris 2.7-2.9 bug.  */
# if defined _LIBICONV_VERSION \
    || !((__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) || defined __sun)
  /* Return to the initial state.  */
  iconv (cd, NULL, NULL, NULL, NULL);
# endif

  /* Do the conversion for real.  */
  {
    const char *inptr = start;
    size_t insize = end - start;
    char *outptr = result;
    size_t outsize = length;

    while (insize > 0)
      {
	size_t res = iconv (cd,
			    (ICONV_CONST char **) &inptr, &insize,
			    &outptr, &outsize);

	if (res == (size_t)(-1))
	  {
	    if (errno == EINVAL)
	      break;
	    else
	      return -1;
	  }
# if !defined _LIBICONV_VERSION && (defined sgi || defined __sgi)
	/* Irix iconv() inserts a NUL byte if it cannot convert.  */
	else if (res > 0)
	  return -1;
# endif
      }
    /* Avoid glibc-2.1 bug and Solaris 2.7 bug.  */
# if defined _LIBICONV_VERSION \
    || !((__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) || defined __sun)
    {
      size_t res = iconv (cd, NULL, NULL, &outptr, &outsize);

      if (res == (size_t)(-1))
	return -1;
    }
# endif
    if (outsize != 0)
      abort ();
  }

  return 0;
#undef tmpbufsize
}

char *
convert_string (iconv_t cd, const char *string)
{
  size_t len = strlen (string) + 1;
  char *result = NULL;
  size_t resultlen;

  if (iconv_string (cd, string, string + len, &result, &resultlen) == 0)
    /* Verify the result has exactly one NUL byte, at the end.  */
    if (resultlen > 0 && result[resultlen - 1] == '\0'
	&& strlen (result) == resultlen - 1)
      return result;

  error (EXIT_FAILURE, 0, _("conversion failure"));
  /* NOTREACHED */
  return NULL;
}

static void
convert_string_list (iconv_t cd, string_list_ty *slp)
{
  size_t i;

  if (slp != NULL)
    for (i = 0; i < slp->nitems; i++)
      slp->item[i] = convert_string (cd, slp->item[i]);
}

static void
convert_msgid (iconv_t cd, message_ty *mp)
{
  mp->msgid = convert_string (cd, mp->msgid);
  if (mp->msgid_plural != NULL)
    mp->msgid_plural = convert_string (cd, mp->msgid_plural);
}

static void
convert_msgstr (iconv_t cd, message_ty *mp)
{
  char *result = NULL;
  size_t resultlen;

  if (!(mp->msgstr_len > 0 && mp->msgstr[mp->msgstr_len - 1] == '\0'))
    abort ();

  if (iconv_string (cd, mp->msgstr, mp->msgstr + mp->msgstr_len,
		    &result, &resultlen) == 0)
    /* Verify the result has a NUL byte at the end.  */
    if (resultlen > 0 && result[resultlen - 1] == '\0')
      /* Verify the result has the same number of NUL bytes.  */
      {
	const char *p;
	const char *pend;
	int nulcount1;
	int nulcount2;

	for (p = mp->msgstr, pend = p + mp->msgstr_len, nulcount1 = 0;
	     p < pend;
	     p += strlen (p) + 1, nulcount1++);
	for (p = result, pend = p + resultlen, nulcount2 = 0;
	     p < pend;
	     p += strlen (p) + 1, nulcount2++);

	if (nulcount1 == nulcount2)
	  {
	    mp->msgstr = result;
	    mp->msgstr_len = resultlen;
	    return;
	  }
      }

  error (EXIT_FAILURE, 0, _("conversion failure"));
}

#endif


void
iconv_message_list (message_list_ty *mlp,
		    const char *canon_from_code, const char *canon_to_code,
		    const char *from_filename)
{
  bool canon_from_code_overridden = (canon_from_code != NULL);
  size_t j;

  /* If the list is empty, nothing to do.  */
  if (mlp->nitems == 0)
    return;

  /* Search the header entry, and extract and replace the charset name.  */
  for (j = 0; j < mlp->nitems; j++)
    if (mlp->item[j]->msgid[0] == '\0' && !mlp->item[j]->obsolete)
      {
	const char *header = mlp->item[j]->msgstr;

	if (header != NULL)
	  {
	    const char *charsetstr = strstr (header, "charset=");

	    if (charsetstr != NULL)
	      {
		size_t len;
		char *charset;
		const char *canon_charset;
		size_t len1, len2, len3;
		char *new_header;

		charsetstr += strlen ("charset=");
		len = strcspn (charsetstr, " \t\n");
		charset = (char *) xallocsa (len + 1);
		memcpy (charset, charsetstr, len);
		charset[len] = '\0';

		canon_charset = po_charset_canonicalize (charset);
		if (canon_charset == NULL)
		  {
		    if (!canon_from_code_overridden)
		      {
			/* Don't give an error for POT files, because POT
			   files usually contain only ASCII msgids.  */
			const char *filename = from_filename;
			size_t filenamelen;

			if (filename != NULL
			    && (filenamelen = strlen (filename)) >= 4
			    && memcmp (filename + filenamelen - 4, ".pot", 4)
			       == 0
			    && strcmp (charset, "CHARSET") == 0)
			  canon_charset = po_charset_ascii;
			else
			  error (EXIT_FAILURE, 0,
				 _("\
present charset \"%s\" is not a portable encoding name"),
				 charset);
		      }
		  }
		else
		  {
		    if (canon_from_code == NULL)
		      canon_from_code = canon_charset;
		    else if (canon_from_code != canon_charset)
		      error (EXIT_FAILURE, 0,
			     _("\
two different charsets \"%s\" and \"%s\" in input file"),
			     canon_from_code, canon_charset);
		  }
		freesa (charset);

		len1 = charsetstr - header;
		len2 = strlen (canon_to_code);
		len3 = (header + strlen (header)) - (charsetstr + len);
		new_header = (char *) xmalloc (len1 + len2 + len3 + 1);
		memcpy (new_header, header, len1);
		memcpy (new_header + len1, canon_to_code, len2);
		memcpy (new_header + len1 + len2, charsetstr + len, len3 + 1);
		mlp->item[j]->msgstr = new_header;
		mlp->item[j]->msgstr_len = len1 + len2 + len3 + 1;
	      }
	  }
      }
  if (canon_from_code == NULL)
    {
      if (is_ascii_message_list (mlp))
	canon_from_code = po_charset_ascii;
      else
	error (EXIT_FAILURE, 0, _("\
input file doesn't contain a header entry with a charset specification"));
    }

  /* If the two encodings are the same, nothing to do.  */
  if (canon_from_code != canon_to_code)
    {
#if HAVE_ICONV
      iconv_t cd;
      bool msgids_changed;

      /* Avoid glibc-2.1 bug with EUC-KR.  */
# if (__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) && !defined _LIBICONV_VERSION
      if (strcmp (canon_from_code, "EUC-KR") == 0)
	cd = (iconv_t)(-1);
      else
# endif
      cd = iconv_open (canon_to_code, canon_from_code);
      if (cd == (iconv_t)(-1))
	error (EXIT_FAILURE, 0, _("\
Cannot convert from \"%s\" to \"%s\". %s relies on iconv(), \
and iconv() does not support this conversion."),
	       canon_from_code, canon_to_code, basename (program_name));

      msgids_changed = false;
      for (j = 0; j < mlp->nitems; j++)
	{
	  message_ty *mp = mlp->item[j];

	  if (!is_ascii_string (mp->msgid))
	    msgids_changed = true;
	  convert_string_list (cd, mp->comment);
	  convert_string_list (cd, mp->comment_dot);
	  convert_msgid (cd, mp);
	  convert_msgstr (cd, mp);
	}

      iconv_close (cd);

      if (msgids_changed)
	if (message_list_msgids_changed (mlp))
	  error (EXIT_FAILURE, 0, _("\
Conversion from \"%s\" to \"%s\" introduces duplicates: \
some different msgids become equal."),
		 canon_from_code, canon_to_code);
#else
	  error (EXIT_FAILURE, 0, _("\
Cannot convert from \"%s\" to \"%s\". %s relies on iconv(). \
This version was built without iconv()."),
		 canon_from_code, canon_to_code, basename (program_name));
#endif
    }
}

msgdomain_list_ty *
iconv_msgdomain_list (msgdomain_list_ty *mdlp,
		      const char *to_code,
		      const char *from_filename)
{
  const char *canon_to_code;
  size_t k;

  /* Canonicalize target encoding.  */
  canon_to_code = po_charset_canonicalize (to_code);
  if (canon_to_code == NULL)
    error (EXIT_FAILURE, 0,
	   _("target charset \"%s\" is not a portable encoding name."),
	   to_code);

  for (k = 0; k < mdlp->nitems; k++)
    iconv_message_list (mdlp->item[k]->messages, mdlp->encoding, canon_to_code,
			from_filename);

  mdlp->encoding = canon_to_code;
  return mdlp;
}
