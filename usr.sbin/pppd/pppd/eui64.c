/*	$NetBSD: eui64.c,v 1.2.8.1 2000/07/18 16:15:08 tron Exp $	*/

/*
    eui64.c - EUI64 routines for IPv6CP.
    Copyright (C) 1999  Tommi Komulainen <Tommi.Komulainen@iki.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


    Id: eui64.c,v 1.3 1999/08/25 04:15:51 paulus Exp 
*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
#define RCSID	"Id: eui64.c,v 1.3 1999/08/25 04:15:51 paulus Exp "
#else
__RCSID("$NetBSD: eui64.c,v 1.2.8.1 2000/07/18 16:15:08 tron Exp $");
#endif
#endif

#include "pppd.h"

#ifdef RCSID
static const char rcsid[] = RCSID;
#endif

#ifdef INET6
/*
 * eui64_ntoa - Make an ascii representation of an interface identifier
 */
char *
eui64_ntoa(e)
    eui64_t e;
{
    static char buf[32];

    snprintf(buf, 32, "%02x%02x:%02x%02x:%02x%02x:%02x%02x",
	     e.e8[0], e.e8[1], e.e8[2], e.e8[3], 
	     e.e8[4], e.e8[5], e.e8[6], e.e8[7]);
    return buf;
}
#endif
