/*	$KAME: netdb_dnssec.h,v 1.2 2001/04/11 09:52:00 sakane Exp $	*/

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

#ifndef T_CERT
#define T_CERT	37		/* defined by RFC2538 section 2 */
#endif

/* RFC2538 section 2.1 */
#define DNSSEC_TYPE_PKIX	1
#define DNSSEC_TYPE_SPKI	2
#define DNSSEC_TYPE_PGP		3
#define DNSSEC_TYPE_URI		4
#define DNSSEC_TYPE_OID		5

/* RFC2535 section 3.2 */
#define DNSSEC_ALG_RSAMD5	1
#define DNSSEC_ALG_DH		2
#define DNSSEC_ALG_DSA		3
#define DNSSEC_ALG_ECC		4
#define DNSSEC_ALG_PRIVATEDNS	5
#define DNSSEC_ALG_PRIVATEOID	6

/*
 * Structures returned by network data base library.  All addresses are
 * supplied in host order, and returned in network order (suitable for
 * use in system calls).
 */
struct certinfo {
	int ci_type;			/* certificate type */
	int ci_keytag;			/* keytag */
	int ci_algorithm;		/* algorithm */
	int ci_flags;			/* currently, 1:valid or 0:uncertain */
	size_t ci_certlen;		/* length of certificate */
	char *ci_cert;			/* certificate */
	struct certinfo *ci_next;	/* next structure */
};

extern void freecertinfo __P((struct certinfo *));
extern int getcertsbyname __P((char *, struct certinfo **));
