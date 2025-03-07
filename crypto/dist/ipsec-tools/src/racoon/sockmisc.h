/*	$NetBSD: sockmisc.h,v 1.14 2025/03/07 15:55:29 christos Exp $	*/

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

#define CMPSADDR_MATCH		0
#define CMPSADDR_WILDPORT_MATCH	1
#define CMPSADDR_WOP_MATCH	2
#define CMPSADDR_MISMATCH	3

extern int cmpsaddr(const struct sockaddr *, const struct sockaddr *);

extern struct sockaddr *getlocaladdr(struct sockaddr *);

extern int recvfromto(int, void *, size_t, int,
    struct sockaddr *, socklen_t *, struct sockaddr *, unsigned int *);
extern int sendfromto(int, const void *, size_t,
    struct sockaddr *, struct sockaddr *, int);

extern int setsockopt_bypass(int, int);

extern struct sockaddr *newsaddr(int);
extern struct sockaddr *dupsaddr(struct sockaddr *);
extern char *saddr2str(const struct sockaddr *);
extern char *saddrwop2str(const struct sockaddr *);
extern char *saddr2str_fromto(const char *format, 
    const struct sockaddr *saddr, 
    const struct sockaddr *daddr);
extern struct sockaddr *str2saddr(char *, char *);
extern void mask_sockaddr(struct sockaddr *, const struct sockaddr *, size_t);

/* struct netaddr functions */
extern char *naddrwop2str(const struct netaddr *naddr);
extern char *naddrwop2str_fromto(const char *format,
    const struct netaddr *saddr, const struct netaddr *daddr);
extern int naddr_score(const struct netaddr *naddr, const struct sockaddr *saddr);

/* Some useful functions for sockaddr port manipulations. */
extern uint16_t extract_port(const struct sockaddr *addr);
extern uint16_t *set_port(struct sockaddr *addr, uint16_t new_port);
extern uint16_t *get_port_ptr(struct sockaddr *addr);

#endif /* _SOCKMISC_H */
