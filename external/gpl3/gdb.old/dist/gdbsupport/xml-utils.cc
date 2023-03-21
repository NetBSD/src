/* Shared helper routines for manipulating XML.

   Copyright (C) 2006-2020 Free Software Foundation, Inc.

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

/* See xml-utils.h.  */

std::string
xml_escape_text (const char *text)
{
  std::string result;

  xml_escape_text_append (&result, text);

  return result;
}

/* See xml-utils.h.  */

void
xml_escape_text_append (std::string *result, const char *text)
{
  /* Expand the result.  */
  for (int i = 0; text[i] != '\0'; i++)
    switch (text[i])
      {
      case '\'':
	*result += "&apos;";
	break;
      case '\"':
	*result += "&quot;";
	break;
      case '&':
	*result += "&amp;";
	break;
      case '<':
	*result += "&lt;";
	break;
      case '>':
	*result += "&gt;";
	break;
      default:
	*result += text[i];
	break;
      }
}
