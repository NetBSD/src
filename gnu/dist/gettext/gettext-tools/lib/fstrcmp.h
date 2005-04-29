/* Fuzzy string comparison.
   Copyright (C) 1995, 2000, 2002-2003 Free Software Foundation, Inc.

   This file was written by Peter Miller <pmiller@agso.gov.au>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _FSTRCMP_H
#define _FSTRCMP_H

/* Fuzzy compare of S1 and S2.  Return a measure for the similarity of S1
   and S1.  The higher the result, the more similar the strings are.  */
extern double fstrcmp (const char *s1, const char *s2);

#endif
