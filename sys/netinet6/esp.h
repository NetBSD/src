/*	$NetBSD: esp.h,v 1.5 1999/12/02 05:08:16 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * RFC1827/2406 Encapsulated Security Payload.
 */

#ifndef _NETINET6_ESP_H_
#define _NETINET6_ESP_H_

#if (defined(__FreeBSD__) && __FreeBSD__ >= 3) || defined(__NetBSD__)
#if defined(_KERNEL) && !defined(_LKM)
#include "opt_inet.h"
#endif
#endif

#include <netkey/keydb.h>		/* for struct secas */

struct esp {
	u_int32_t	esp_spi;	/* ESP */
	/*variable size, 32bit bound*/	/* Initialization Vector */
	/*variable size*/		/* Payload data */
	/*variable size*/		/* padding */
	/*8bit*/			/* pad size */
	/*8bit*/			/* next header */
	/*8bit*/			/* next header */
	/*variable size, 32bit bound*/	/* Authentication data (new IPsec) */
};

struct newesp {
	u_int32_t	esp_spi;	/* ESP */
	u_int32_t	esp_seq;	/* Sequence number */
	/*variable size*/		/* (IV and) Payload data */
	/*variable size*/		/* padding */
	/*8bit*/			/* pad size */
	/*8bit*/			/* next header */
	/*8bit*/			/* next header */
	/*variable size, 32bit bound*/	/* Authentication data */
};

struct esptail {
	u_int8_t	esp_padlen;	/* pad length */
	u_int8_t	esp_nxt;	/* Next header */
	/*variable size, 32bit bound*/	/* Authentication data (new IPsec)*/
};

struct esp_algorithm_state {
	struct secas *sa;
	void* foo;	/*per algorithm data - maybe*/
};

/* XXX yet to be defined */
struct esp_algorithm {
	size_t padbound;	/* pad boundary, in byte */
	int (*mature) __P((struct secas *));
	int keymin;	/* in bits */
	int keymax;	/* in bits */
	int (*ivlen) __P((struct secas *));
	int (*decrypt) __P((struct mbuf *, size_t,
		struct secas *, struct esp_algorithm *, int));
	int (*encrypt) __P((struct mbuf *, size_t, size_t,
		struct secas *, struct esp_algorithm *, int));
};

#ifdef _KERNEL
extern struct esp_algorithm esp_algorithms[];

/* crypt routines */
struct secasb;
extern int esp4_output __P((struct mbuf *, struct ipsecrequest *));
extern void esp4_input __P((struct mbuf *, ...));
extern size_t esp_hdrsiz __P((struct ipsecrequest *));

#ifdef INET6
extern int esp6_output __P((struct mbuf *, u_char *, struct mbuf *,
	struct ipsecrequest *));
extern int esp6_input __P((struct mbuf **, int *, int));
#endif /* INET6 */
#endif /*_KERNEL*/

struct secas;
extern int esp_auth __P((struct mbuf *, size_t, size_t,
	struct secas *, u_char *));

#endif /*_NETINET6_ESP_H_*/
