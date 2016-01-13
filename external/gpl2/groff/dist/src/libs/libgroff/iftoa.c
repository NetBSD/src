/*	$NetBSD: iftoa.c,v 1.1.1.1 2016/01/13 18:41:48 christos Exp $	*/

/* Copyright (C) 1989, 1990, 1991, 1992, 2000, 2004
   Free Software Foundation, Inc.
     Written by James Clark (jjc@jclark.com)

This file is part of groff.

groff is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

groff is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with groff; see the file COPYING.  If not, write to the Free Software
Foundation, 51 Franklin St - Fifth Floor, Boston, MA 02110-1301, USA. */

#define INT_DIGITS 19		/* enough for 64-bit integer */

#ifdef __cplusplus
extern "C" {
#endif

char *if_to_a(int i, int decimal_point)
{
  /* room for a -, INT_DIGITS digits, a decimal point, and a terminating '\0' */
  static char buf[INT_DIGITS + 3];			
  char *p = buf + INT_DIGITS + 2;
  int point = 0;
  buf[INT_DIGITS + 2] = '\0';
  /* assert(decimal_point <= INT_DIGITS); */
  if (i >= 0) {
    do {
      *--p = '0' + (i % 10);
      i /= 10;
      if (++point == decimal_point)
	*--p = '.';
    } while (i != 0 || point < decimal_point);
  }
  else {			/* i < 0 */
    do {
      *--p = '0' - (i % 10);
      i /= 10;
      if (++point == decimal_point)
	*--p = '.';
    } while (i != 0 || point < decimal_point);
    *--p = '-';
  }
  if (decimal_point > 0) {
    char *q;
    /* there must be a dot, so this will terminate */
    for (q = buf + INT_DIGITS + 2; q[-1] == '0'; --q)
      ;
    if (q[-1] == '.') {
      if (q - 1 == p) {
	q[-1] = '0';
	q[0] = '\0';
      }
      else
	q[-1] = '\0';
    }
    else
      *q = '\0';
  }
  return p;
}

#ifdef __cplusplus
}
#endif
