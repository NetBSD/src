/* Shared helper routines for manipulating XML.

   Copyright (C) 2006-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "common-defs.h"
#include "xml-utils.h"

/* Return a malloc allocated string with special characters from TEXT
   replaced by entity references.  */

char *
xml_escape_text (const char *text)
{
  char *result;
  int i, special;

  /* Compute the length of the result.  */
  for (i = 0, special = 0; text[i] != '\0'; i++)
    switch (text[i])
      {
      case '\'':
      case '\"':
	special += 5;
	break;
      case '&':
	special += 4;
	break;
      case '<':
      case '>':
	special += 3;
	break;
      default:
	break;
      }

  /* Expand the result.  */
  result = (char *) xmalloc (i + special + 1);
  for (i = 0, special = 0; text[i] != '\0'; i++)
    switch (text[i])
      {
      case '\'':
	strcpy (result + i + special, "&apos;");
	special += 5;
	break;
      case '\"':
	strcpy (result + i + special, "&quot;");
	special += 5;
	break;
      case '&':
	strcpy (result + i + special, "&amp;");
	special += 4;
	break;
      case '<':
	strcpy (result + i + special, "&lt;");
	special += 3;
	break;
      case '>':
	strcpy (result + i + special, "&gt;");
	special += 3;
	break;
      default:
	result[i + special] = text[i];
	break;
      }
  result[i + special] = '\0';

  return result;
}
