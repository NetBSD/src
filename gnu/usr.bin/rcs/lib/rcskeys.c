/*
 *                     RCS keyword table and match operation
 */

/* Copyright (C) 1982, 1988, 1989 Walter Tichy
   Copyright 1990, 1991 by Paul Eggert
   Distributed under license by the Free Software Foundation, Inc.

This file is part of RCS.

RCS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

RCS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RCS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

Report problems and direct all questions to:

    rcs-bugs@cs.purdue.edu

*/

#include "rcsbase.h"

libId(keysId, "$Id: rcskeys.c,v 1.3.2.1 1994/10/11 10:20:46 mycroft Exp $")


char const *const Keyword[] = {
    /* This must be in the same order as rcsbase.h's enum markers type. */
	nil,
	AUTHOR, DATE, HEADER, IDH,
#ifdef LOCALID
	LOCALID,
#endif
	LOCKER, LOG,
	RCSFILE, REVISION, SOURCE, STATE
};



	enum markers
trymatch(string)
	char const *string;
/* function: Checks whether string starts with a keyword followed
 * by a KDELIM or a VDELIM.
 * If successful, returns the appropriate marker, otherwise Nomatch.
 */
{
        register int j;
	register char const *p, *s;
	for (j = sizeof(Keyword)/sizeof(*Keyword);  (--j);  ) {
		/* try next keyword */
		p = Keyword[j];
		s = string;
		while (*p++ == *s++) {
			if (!*p)
			    switch (*s) {
				case KDELIM:
				case VDELIM:
				    return (enum markers)j;
				default:
				    return Nomatch;
			    }
		}
        }
        return(Nomatch);
}

