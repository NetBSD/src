/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/

/*
 *	from: @(#)endian.h	7.7 (Berkeley) 4/3/91
 *	$Id: endian.h,v 1.3 1993/12/02 15:02:50 briggs Exp $
 */

/*
   ALICE
      BG 05/24/92,17:10:58
         I don't think we need to change this very much.
 */
#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_

#define _QUAD_HIGHWORD	0
#define _QUAD_LOWWORD	1

/*
 * Definitions for byte order, according to byte significance from low
 * address to high.
 */
#define	LITTLE_ENDIAN	1234	/* LSB first: i386, vax */
#define	BIG_ENDIAN	4321	/* MSB first: 68000, ibm, net */
#define	PDP_ENDIAN	3412	/* LSB first in word, MSW first in long */

#define	BYTE_ORDER	BIG_ENDIAN

#include <sys/cdefs.h>

#define __word_swap_long(x) \
({ register u_long X = (x); \
   asm ("swap %1" \
	: "=r" (X) \
	: "0" (X)); \
   X; })
#if __GNUC__ >= 2
#define __byte_swap_long(x) \
({ register u_long X = (x); \
   asm ("xchgb %h1, %b1\n\tswap %1\n\txchgb %h1, %b1" \
	: "=q" (X) \
	: "0" (X)); \
   X; })
#define __byte_swap_word(x) \
({ register u_short X = (x); \
   asm ("xchgb %h1, %b1" \
	: "=q" (X) \
	: "0" (X)); \
   X; })
#else /* __GNUC__ >= 2 */
#define __byte_swap_long(x) \
({ register u_long X = (x); \
   asm ("rorw #8, %w1\n\tswap %1\n\trorw #8, %w1" \
	: "=r" (X) \
	: "0" (X)); \
   X; })
#define __byte_swap_word(x) \
({ register u_short X = (x); \
   asm ("rorw #8, %w1" \
	: "=r" (X) \
	: "0" (X)); \
   X; })
#endif /* __GNUC__ >= 2 */

__BEGIN_DECLS
unsigned long	htonl __P((unsigned long));
unsigned short	htons __P((unsigned short));
unsigned long	ntohl __P((unsigned long));
unsigned short	ntohs __P((unsigned short));
__END_DECLS

/*
 * Macros for network/external number representation conversion.
 */
#if BYTE_ORDER == BIG_ENDIAN && !defined(lint)
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)

#define	NTOHL(x)	(x)
#define	NTOHS(x)	(x)
#define	HTONL(x)	(x)
#define	HTONS(x)	(x)

#else

#define	ntohl	__byte_swap_long
#define	ntohs	__byte_swap_word
#define	htonl	__byte_swap_long
#define	htons	__byte_swap_word

#define	NTOHL(x)	(x) = ntohl((u_long)x)
#define	NTOHS(x)	(x) = ntohs((u_short)x)
#define	HTONL(x)	(x) = htonl((u_long)x)
#define	HTONS(x)	(x) = htons((u_short)x)
#endif

#endif /* _MACHINE_ENDIAN_H_ */
