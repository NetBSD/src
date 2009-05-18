/*	$NetBSD: sockmisc.h,v 1.10 2009/05/18 17:40:38 tteras Exp $	*/

/* Id: sockmisc.h,v 1.9 2005/10/05 16:55:41 manubsd Exp */

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

#ifndef _SOCKMISC_H
#define _SOCKMISC_H

#ifndef IP_IPSEC_POLICY
#define IP_IPSEC_POLICY 16	/* XXX: from linux/in.h */
#endif

#ifndef IPV6_IPSEC_POLICY
#define IPV6_IPSEC_POLICY 34	/* XXX: from linux/???.h per
				   "Tom Lendacky" <toml@us.ibm.com> */
#endif

union sockaddr_any {
	struct sockaddr sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
};

struct netaddr {
	union sockaddr_any sa;
	unsigned long prefix;
};

extern const int niflags;

extern int cmpsaddrwop __P((const struct sockaddr *, const struct sockaddr *));
extern int cmpsaddrwild __P((const struct sockaddr *, const struct sockaddr *));
extern int cmpsaddrstrict __P((const struct sockaddr *, const struct sockaddr *));
extern int cmpsaddrmagic __P((const struct sockaddr *, const struct sockaddr *));

#ifdef ENABLE_NATT 
#define CMPSADDR(saddr1, saddr2) cmpsaddrstrict((saddr1), (saddr2))
#else 
#define CMPSADDR(saddr1, saddr2) cmpsaddrwop((saddr1), (saddr2))
#endif

extern struct sockaddr *getlocaladdr __P((struct sockaddr *));

extern int recvfromto __P((int, void *, size_t, int,
	struct sockaddr *, socklen_t *, struct sockaddr *, unsigned int *));
extern int sendfromto __P((int, const void *, size_t,
	struct sockaddr *, struct sockaddr *, int));

extern int setsockopt_bypass __P((int, int));

extern struct sockaddr *newsaddr __P((int));
extern struct sockaddr *dupsaddr __P((struct sockaddr *));
extern char *saddr2str __P((const struct sockaddr *));
extern char *saddrwop2str __P((const struct sockaddr *));
extern char *saddr2str_fromto __P((const char *format, 
				   const struct sockaddr *saddr, 
				   const struct sockaddr *daddr));
extern struct sockaddr *str2saddr __P((char *, char *));
extern void mask_sockaddr __P((struct sockaddr *, const struct sockaddr *,
	size_t));

/* struct netaddr functions */
extern char *naddrwop2str __P((const struct netaddr *naddr));
extern char *naddrwop2str_fromto __P((const char *format, const struct netaddr *saddr,
				      const struct netaddr *daddr));
extern int naddr_score(const struct netaddr *naddr, const struct sockaddr *saddr);

/* Some usefull functions for sockaddr port manipulations. */
extern u_int16_t extract_port __P((const struct sockaddr *addr));
extern u_int16_t *set_port __P((struct sockaddr *addr, u_int16_t new_port));
extern u_int16_t *get_port_ptr __P((struct sockaddr *addr));

#endif /* _SOCKMISC_H */
