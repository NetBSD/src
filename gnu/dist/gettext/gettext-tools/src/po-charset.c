/* Charset handling while reading PO files.
   Copyright (C) 2001-2005 Free Software Foundation, Inc.
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
#include "po-charset.h"

#include <stdlib.h>
#include <string.h>

#include "xallocsa.h"
#include "xerror.h"
#include "po-error.h"
#include "basename.h"
#include "progname.h"
#include "strstr.h"
#include "c-strcase.h"
#include "gettext.h"

#define _(str) gettext (str)

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))

static const char ascii[] = "ASCII";

/* The canonicalized encoding name for ASCII.  */
const char *po_charset_ascii = ascii;

static const char utf8[] = "UTF-8";

/* The canonicalized encoding name for UTF-8.  */
const char *po_charset_utf8 = utf8;

/* Canonicalize an encoding name.  */
const char *
po_charset_canonicalize (const char *charset)
{
  /* The list of charsets supported by glibc's iconv() and by the portable
     iconv() across platforms.  Taken from intl/config.charset.  */
  static const char *standard_charsets[] =
  {
    ascii, "ANSI_X3.4-1968", "US-ASCII",	/* i = 0..2 */
    "ISO-8859-1", "ISO_8859-1",			/* i = 3, 4 */
    "ISO-8859-2", "ISO_8859-2",
    "ISO-8859-3", "ISO_8859-3",
    "ISO-8859-4", "ISO_8859-4",
    "ISO-8859-5", "ISO_8859-5",
    "ISO-8859-6", "ISO_8859-6",
    "ISO-8859-7", "ISO_8859-7",
    "ISO-8859-8", "ISO_8859-8",
    "ISO-8859-9", "ISO_8859-9",
    "ISO-8859-13", "ISO_8859-13",
    "ISO-8859-14", "ISO_8859-14",
    "ISO-8859-15", "ISO_8859-15",		/* i = 25, 26 */
    "KOI8-R",
    "KOI8-U",
    "KOI8-T",
    "CP850",
    "CP866",
    "CP874",
    "CP932",
    "CP949",
    "CP950",
    "CP1250",
    "CP1251",
    "CP1252",
    "CP1253",
    "CP1254",
    "CP1255",
    "CP1256",
    "CP1257",
    "GB2312",
    "EUC-JP",
    "EUC-KR",
    "EUC-TW",
    "BIG5",
    "BIG5-HKSCS",
    "GBK",
    "GB18030",
    "SHIFT_JIS",
    "JOHAB",
    "TIS-620",
    "VISCII",
    "GEORGIAN-PS",
    utf8
  };
  size_t i;

  for (i = 0; i < SIZEOF (standard_charsets); i++)
    if (c_strcasecmp (charset, standard_charsets[i]) == 0)
      return standard_charsets[i < 3 ? 0 : i < 27 ? ((i - 3) & ~1) + 3 : i];
  return NULL;
}

/* Test for ASCII compatibility.  */
bool
po_charset_ascii_compatible (const char *canon_charset)
{
  /* There are only a few exceptions to ASCII compatibility.  */
  if (strcmp (canon_charset, "SHIFT_JIS") == 0
      || strcmp (canon_charset, "JOHAB") == 0
      || strcmp (canon_charset, "VISCII") == 0)
    return false;
  else
    return true;
}

/* Test for a weird encoding, i.e. an encoding which has double-byte
   characters ending in 0x5C.  */
bool po_is_charset_weird (const char *canon_charset)
{
  static const char *weird_charsets[] =
  {
    "BIG5",
    "BIG5-HKSCS",
    "GBK",
    "GB18030",
    "SHIFT_JIS",
    "JOHAB"
  };
  size_t i;

  for (i = 0; i < SIZEOF (weird_charsets); i++)
    if (strcmp (canon_charset, weird_charsets[i]) == 0)
      return true;
  return false;
}

/* Test for a weird CJK encoding, i.e. a weird encoding with CJK structure.
   An encoding has CJK structure if every valid character stream is composed
   of single bytes in the range 0x{00..7F} and of byte pairs in the range
   0x{80..FF}{30..FF}.  */
bool po_is_charset_weird_cjk (const char *canon_charset)
{
  static const char *weird_cjk_charsets[] =
  {			/* single bytes   double bytes       */
    "BIG5",		/* 0x{00..7F},    0x{A1..F9}{40..FE} */
    "BIG5-HKSCS",	/* 0x{00..7F},    0x{88..FE}{40..FE} */
    "GBK",		/* 0x{00..7F},    0x{81..FE}{40..FE} */
    "GB18030",		/* 0x{00..7F},    0x{81..FE}{30..FE} */
    "SHIFT_JIS",	/* 0x{00..7F},    0x{81..F9}{40..FC} */
    "JOHAB"		/* 0x{00..7F},    0x{84..F9}{31..FE} */
  };
  size_t i;

  for (i = 0; i < SIZEOF (weird_cjk_charsets); i++)
    if (strcmp (canon_charset, weird_cjk_charsets[i]) == 0)
      return true;
  return false;
}


/* The PO file's encoding, as specified in the header entry.  */
const char *po_lex_charset;

#if HAVE_ICONV
/* Converter from the PO file's encoding to UTF-8.  */
iconv_t po_lex_iconv;
#endif
/* If no converter is available, some information about the structure of the
   PO file's encoding.  */
bool po_lex_weird_cjk;

void
po_lex_charset_init ()
{
  po_lex_charset = NULL;
#if HAVE_ICONV
  po_lex_iconv = (iconv_t)(-1);
#endif
  po_lex_weird_cjk = false;
}

void
po_lex_charset_set (const char *header_entry, const char *filename)
{
  /* Verify the validity of CHARSET.  It is necessary
     1. for the correct treatment of multibyte characters containing
	0x5C bytes in the PO lexer,
     2. so that at run time, gettext() can call iconv() to convert
	msgstr.  */
  const char *charsetstr = strstr (header_entry, "charset=");

  if (charsetstr != NULL)
    {
      size_t len;
      char *charset;
      const char *canon_charset;

      charsetstr += strlen ("charset=");
      len = strcspn (charsetstr, " \t\n");
      charset = (char *) xallocsa (len + 1);
      memcpy (charset, charsetstr, len);
      charset[len] = '\0';

      canon_charset = po_charset_canonicalize (charset);
      if (canon_charset == NULL)
	{
	  /* Don't warn for POT files, because POT files usually contain
	     only ASCII msgids.  */
	  size_t filenamelen = strlen (filename);

	  if (!(filenamelen >= 4
		&& memcmp (filename + filenamelen - 4, ".pot", 4) == 0
		&& strcmp (charset, "CHARSET") == 0))
	    po_multiline_warning (xasprintf (_("%s: warning: "), filename),
				  xasprintf (_("\
Charset \"%s\" is not a portable encoding name.\n\
Message conversion to user's charset might not work.\n"),
					     charset));
	}
      else
	{
	  const char *envval;

	  po_lex_charset = canon_charset;
#if HAVE_ICONV
	  if (po_lex_iconv != (iconv_t)(-1))
	    iconv_close (po_lex_iconv);
#endif

	  /* The old Solaris/openwin msgfmt and GNU msgfmt <= 0.10.35
	     don't know about multibyte encodings, and require a spurious
	     backslash after every multibyte character whose last byte is
	     0x5C.  Some programs, like vim, distribute PO files in this
	     broken format.  GNU msgfmt must continue to support this old
	     PO file format when the Makefile requests it.  */
	  envval = getenv ("OLD_PO_FILE_INPUT");
	  if (envval != NULL && *envval != '\0')
	    {
	      /* Assume the PO file is in old format, with extraneous
		 backslashes.  */
#if HAVE_ICONV
	      po_lex_iconv = (iconv_t)(-1);
#endif
	      po_lex_weird_cjk = false;
	    }
	  else
	    {
	      /* Use iconv() to parse multibyte characters.  */
#if HAVE_ICONV
	      /* Avoid glibc-2.1 bug with EUC-KR.  */
# if (__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) && !defined _LIBICONV_VERSION
	      if (strcmp (po_lex_charset, "EUC-KR") == 0)
		po_lex_iconv = (iconv_t)(-1);
	      else
# endif
	      /* Avoid Solaris 2.9 bug with GB2312, EUC-TW, BIG5, BIG5-HKSCS,
		 GBK, GB18030.  */
# if defined __sun && !defined _LIBICONV_VERSION
	      if (   strcmp (po_lex_charset, "GB2312") == 0
		  || strcmp (po_lex_charset, "EUC-TW") == 0
		  || strcmp (po_lex_charset, "BIG5") == 0
		  || strcmp (po_lex_charset, "BIG5-HKSCS") == 0
		  || strcmp (po_lex_charset, "GBK") == 0
		  || strcmp (po_lex_charset, "GB18030") == 0)
		po_lex_iconv = (iconv_t)(-1);
	      else
# endif
	      po_lex_iconv = iconv_open ("UTF-8", po_lex_charset);
	      if (po_lex_iconv == (iconv_t)(-1))
		{
		  const char *note;

		  /* Test for a charset which has double-byte characters
		     ending in 0x5C.  For these encodings, the string parser
		     is likely to be confused if it can't see the character
		     boundaries.  */
		  po_lex_weird_cjk = po_is_charset_weird_cjk (po_lex_charset);
		  if (po_is_charset_weird (po_lex_charset)
		      && !po_lex_weird_cjk)
		    note = _("Continuing anyway, expect parse errors.");
		  else
		    note = _("Continuing anyway.");

		  po_multiline_warning (xasprintf (_("%s: warning: "), filename),
					xasprintf (_("\
Charset \"%s\" is not supported. %s relies on iconv(),\n\
and iconv() does not support \"%s\".\n"),
						   po_lex_charset,
						   basename (program_name),
						   po_lex_charset));

# if !defined _LIBICONV_VERSION
		  po_multiline_warning (NULL,
					xasprintf (_("\
Installing GNU libiconv and then reinstalling GNU gettext\n\
would fix this problem.\n")));
# endif

		  po_multiline_warning (NULL, xasprintf (_("%s\n"), note));
		}
#else
	      /* Test for a charset which has double-byte characters
		 ending in 0x5C.  For these encodings, the string parser
		 is likely to be confused if it can't see the character
		 boundaries.  */
	      po_lex_weird_cjk = po_is_charset_weird_cjk (po_lex_charset);
	      if (po_is_charset_weird (po_lex_charset) && !po_lex_weird_cjk)
		{
		  const char *note =
		    _("Continuing anyway, expect parse errors.");

		  po_multiline_warning (xasprintf (_("%s: warning: "), filename),
					xasprintf (_("\
Charset \"%s\" is not supported. %s relies on iconv().\n\
This version was built without iconv().\n"),
						   po_lex_charset,
						   basename (program_name)));

		  po_multiline_warning (NULL,
					xasprintf (_("\
Installing GNU libiconv and then reinstalling GNU gettext\n\
would fix this problem.\n")));

		  po_multiline_warning (NULL, xasprintf (_("%s\n"), note));
		}
#endif
	    }
	}
      freesa (charset);
    }
  else
    {
      /* Don't warn for POT files, because POT files usually contain
	 only ASCII msgids.  */
      size_t filenamelen = strlen (filename);

      if (!(filenamelen >= 4
	    && memcmp (filename + filenamelen - 4, ".pot", 4) == 0))
	po_multiline_warning (xasprintf (_("%s: warning: "), filename),
			      xasprintf (_("\
Charset missing in header.\n\
Message conversion to user's charset will not work.\n")));
    }
}

void
po_lex_charset_close ()
{
  po_lex_charset = NULL;
#if HAVE_ICONV
  if (po_lex_iconv != (iconv_t)(-1))
    {
      iconv_close (po_lex_iconv);
      po_lex_iconv = (iconv_t)(-1);
    }
#endif
  po_lex_weird_cjk = false;
}
