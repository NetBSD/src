/* Localization of proper names.
   Copyright (C) 2006 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

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

#include <config.h>

/* Specification.  */
#include "propername.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_ICONV
# include <iconv.h>
#endif

#include "localcharset.h"
#include "c-strcase.h"
#include "xstriconv.h"
#include "c-strstr.h"
#include "strstr.h"
#include "xalloc.h"
#include "gettext.h"


/* Return the localization of NAME.  NAME is written in ASCII.  */

const char *
proper_name (const char *name)
{
  /* See whether there is a translation.   */
  const char *translation = gettext (name);

  if (translation != name)
    {
      /* See whether the translation contains the original name.  */
      if (strstr (translation, name) != NULL)
	return translation;
      else
	{
	  /* Return "TRANSLATION (NAME)".  */
	  char *result =
	    (char *) xmalloc (strlen (translation) + 2 + strlen (name) + 1 + 1);

	  sprintf (result, "%s (%s)", translation, name);
	  return result;
	}
    }
  else
    return name;
}

/* Return the localization of a name whose original writing is not ASCII.
   NAME_UTF8 is the real name, written in UTF-8 with octal or hexadecimal
   escape sequences.  NAME_ASCII is a fallback written only with ASCII
   characters.  */

const char *
proper_name_utf8 (const char *name_ascii, const char *name_utf8)
{
  /* See whether there is a translation.   */
  const char *translation = gettext (name_ascii);

  /* Try to convert NAME_UTF8 to the locale encoding.  */
  const char *locale_code = locale_charset ();
  char *alloc_name_converted = NULL;
  char *alloc_name_converted_translit = NULL;
  const char *name_converted = NULL;
  const char *name_converted_translit = NULL;
  const char *name;

  if (c_strcasecmp (locale_code, "UTF-8") != 0)
    {
#if HAVE_ICONV
      name_converted = alloc_name_converted =
	xstr_iconv (name_utf8, "UTF-8", locale_code);

# if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2) || __GLIBC__ > 2 \
     || _LIBICONV_VERSION >= 0x0105
      {
	size_t len = strlen (locale_code);
	char *locale_code_translit = (char *) xmalloc (len + 10 + 1);
	memcpy (locale_code_translit, locale_code, len);
	memcpy (locale_code_translit + len, "//TRANSLIT", 10 + 1);

	name_converted_translit = alloc_name_converted_translit =
	  xstr_iconv (name_utf8, "UTF-8", locale_code_translit);

	free (locale_code_translit);
      }
# endif
#endif
    }
  else
    {
      name_converted = name_utf8;
      name_converted_translit = name_utf8;
    }

  /* The name in locale encoding.  */
  name = (name_converted != NULL ? name_converted :
	  name_converted_translit != NULL ? name_converted_translit :
	  name_ascii);

  if (translation != name_ascii)
    {
      /* See whether the translation contains the original name.
	 A multibyte-aware strstr() is not absolutely necessary here.  */
      if (c_strstr (translation, name_ascii) != NULL
	  || (name_converted != NULL
	      && strstr (translation, name_converted) != NULL)
	  || (name_converted_translit != NULL
	      && strstr (translation, name_converted_translit) != NULL))
	{
	  if (alloc_name_converted != NULL)
	    free (alloc_name_converted);
	  if (alloc_name_converted_translit != NULL)
	    free (alloc_name_converted_translit);
	  return translation;
	}
      else
	{
	  /* Return "TRANSLATION (NAME)".  */
	  char *result =
	    (char *) xmalloc (strlen (translation) + 2 + strlen (name) + 1 + 1);

	  sprintf (result, "%s (%s)", translation, name);

	  if (alloc_name_converted != NULL)
	    free (alloc_name_converted);
	  if (alloc_name_converted_translit != NULL)
	    free (alloc_name_converted_translit);
	  return result;
	}
    }
  else
    {
      if (alloc_name_converted != NULL && alloc_name_converted != name)
	free (alloc_name_converted);
      if (alloc_name_converted_translit != NULL
	  && alloc_name_converted_translit != name)
	free (alloc_name_converted_translit);
      return name;
    }
}
