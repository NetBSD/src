/*	$NetBSD: eui64.h,v 1.2 1999/08/25 02:07:42 christos Exp $	*/

/*
 * eui64.h - EUI64 routines for IPv6CP.
 *
 * (c) 1999 Tommi Komulainen <Tommi.Komulainen@iki.fi>
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Id: eui64.h,v 1.1 1999/08/13 01:58:43 paulus Exp 
 */

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
