/*	$NetBSD: eui64.h,v 1.3 2000/07/16 22:10:12 tron Exp $	*/

/*
    eui64.h - EUI64 routines for IPv6CP.
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

    
    Id: eui64.h,v 1.3 1999/09/30 19:56:37 masputra Exp 
*/

#ifndef __EUI64_H__
#define __EUI64_H__

#if !defined(INET6)
#error	"this file should only be included when INET6 is defined"
#endif /* not defined(INET6) */

#if defined(SOL2)
#include <netinet/in.h>

typedef union {
    uint8_t	e8[8];		/* lower 64-bit IPv6 address */
    uint32_t	e32[2];		/* lower 64-bit IPv6 address */
} eui64_t;

/*
 * Declare the two below, since in.h only defines them when _KERNEL
 * is declared - which shouldn't be true when dealing with user-land programs
 */
#define	s6_addr8	_S6_un._S6_u8
#define	s6_addr32	_S6_un._S6_u32

#else /* else if not defined(SOL2) */

/*
 * TODO:
 *
 * Maybe this should be done by processing struct in6_addr directly...
 */
typedef union
{
    u_int8_t e8[8];
    u_int16_t e16[4];
    u_int32_t e32[2];
} eui64_t;

#endif /* defined(SOL2) */

#define eui64_iszero(e)		(((e).e32[0] | (e).e32[1]) == 0)
#define eui64_equals(e, o)	(((e).e32[0] == (o).e32[0]) && \
				((e).e32[1] == (o).e32[1]))
#define eui64_zero(e)		(e).e32[0] = (e).e32[1] = 0;

#define eui64_copy(s, d)	memcpy(&(d), &(s), sizeof(eui64_t))

#define eui64_magic(e)		do {			\
				(e).e32[0] = magic();	\
				(e).e32[1] = magic();	\
				(e).e8[0] &= ~2;	\
				} while (0)
#define eui64_magic_nz(x)	do {				\
				eui64_magic(x);			\
				} while (eui64_iszero(x))
#define eui64_magic_ne(x, y)	do {				\
				eui64_magic(x);			\
				} while (eui64_equals(x, y))

#define eui64_get(ll, cp)	do {				\
				eui64_copy((*cp), (ll));	\
				(cp) += sizeof(eui64_t);	\
				} while (0)

#define eui64_put(ll, cp)	do {				\
				eui64_copy((ll), (*cp));	\
				(cp) += sizeof(eui64_t);	\
				} while (0)

#define eui64_set32(e, l)	do {			\
				(e).e32[0] = 0;		\
				(e).e32[1] = htonl(l);	\
				} while (0)
#define eui64_setlo32(e, l)	eui64_set32(e, l)

char *eui64_ntoa __P((eui64_t));	/* Returns ascii representation of id */

#endif /* __EUI64_H__ */

