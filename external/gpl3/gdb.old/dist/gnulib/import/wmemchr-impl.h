/* Search wide character array for a wide character.
   Copyright (C) 1999, 2011-2020 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 1999.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

wchar_t *
wmemchr (const wchar_t *s, wchar_t c, size_t n)
{
  for (; n > 0; s++, n--)
    {
      if (*s == c)
        return (wchar_t *) s;
    }
  return NULL;
}
