/* MIN, MAX macros.
   Copyright (C) 1995, 1998, 2001, 2003 Free Software Foundation, Inc.

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

#ifndef _MINMAX_H
#define _MINMAX_H

/* Note: MIN, MAX are also defined in <sys/param.h> on some systems
   (glibc, IRIX, HP-UX, OSF/1).  Therefore you might get warnings about
   MIN, MAX macro redefinitions on some systems; the workaround is to
   #include this file as the last one among the #include list.  */

/* Before we define the following symbols we get the <limits.h> file
   since otherwise we get redefinitions on some systems.  */
#include <limits.h>

/* Note: MIN and MAX should preferrably be used with two arguments of the
   same type.  They might not return the minimum and maximum of their two
   arguments, if the arguments have different types or have unusual
   floating-point values.  For example, on a typical host with 32-bit 'int',
   64-bit 'long long', and 64-bit IEEE 754 'double' types:

     MAX (-1, 2147483648) returns 4294967295.
     MAX (9007199254740992.0, 9007199254740993) returns 9007199254740992.0.
     MAX (NaN, 0.0) returns 0.0.
     MAX (+0.0, -0.0) returns -0.0.

   and in each case the answer is in some sense bogus.  */

/* MAX(a,b) returns the maximum of A and B.  */
#ifndef MAX
# if __STDC__ && defined __GNUC__ && __GNUC__ >= 2
#  define MAX(a,b) (__extension__					    \
		     ({__typeof__ (a) _a = (a);				    \
		       __typeof__ (b) _b = (b);				    \
		       _a > _b ? _a : _b;				    \
		      }))
# else
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
# endif
#endif

/* MIN(a,b) returns the minimum of A and B.  */
#ifndef MIN
# if __STDC__ && defined __GNUC__ && __GNUC__ >= 2
#  define MIN(a,b) (__extension__					    \
		     ({__typeof__ (a) _a = (a);				    \
		       __typeof__ (b) _b = (b);				    \
		       _a < _b ? _a : _b;				    \
		      }))
# else
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
# endif
#endif

#endif /* _MINMAX_H */
