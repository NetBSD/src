/*
 * Copyright (c) 1990, 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) Header: md-mips.h,v 1.9 94/06/14 20:12:40 leres Exp (LBL)
 */

#define TCPDUMP_ALIGN

/*
 * There are MIPS machines, and then there are SPIM machines,
 * and you can tell because SPIM <=> Ultrix.
 */

#ifdef	ultrix
/* SPIM */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/* These should be fixed to be real macros, for speed */

#ifndef NTOHL
#define NTOHL(x)	(x) = ntohl(x)
#define NTOHS(x)	(x) = ntohs(x)
#define HTONL(x)	(x) = htonl(x)
#define HTONS(x)	(x) = htons(x)
#endif

#else
/* MIPS */
#ifndef BYTE_ORDER
#define BYTE_ORDER BIG_ENDIAN
#endif

#ifndef NTOHL
#define NTOHL(x)
#define NTOHS(x)
#define HTONL(x)
#define HTONS(x)
#endif

#ifndef ntohl
#define	ntohl(x) (x)
#define	ntohs(x) (x)
#define	htonl(x) (x)
#define	htons(x) (x)
#endif
#endif

/* 32-bit data types */
/* N.B.: this doesn't address printf()'s %d vs. %ld formats */
typedef	long	int32;		/* signed 32-bit integer */
#if !(defined(sgi) && defined(AUTH_UNIX))
typedef	u_long	u_int32;	/* unsigned 32-bit integer */
#endif
