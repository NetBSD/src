/*	$NetBSD: res_init.c,v 1.42 2003/09/06 11:40:52 itojun Exp $	*/

/*
 * Copyright 1997 Niels Provos <provos@physnet.uni-hamburg.de>
 * All rights reserved.
 *
 * Theo de Raadt <deraadt@openbsd.org> came up with the idea of using
 * such a mathematical system to generate more random (yet non-repeating)
 * ids to solve the resolver/named problem.  But Niels designed the
 * actual system based on the constraints.
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
 *      This product includes software developed by Niels Provos.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1985, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)res_init.c	8.1 (Berkeley) 6/7/93";
static char rcsid[] = "Id: res_init.c,v 8.8 1997/06/01 20:34:37 vixie Exp ";
#else
__RCSID("$NetBSD: res_init.c,v 1.42 2003/09/06 11:40:52 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#if defined(_LIBC)
#include "namespace.h"
#endif
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <assert.h>
#include <ctype.h>
#include <netdb.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(_LIBC) && defined(__weak_alias)
__weak_alias(res_init,_res_init)
#endif

static void res_setoptions __P((char *, char *));
static u_int32_t net_mask __P((struct in_addr));
static const char sort_mask[] = "/&";
#define ISSORTMASK(ch) (strchr(sort_mask, ch) != NULL)

/*
 * Resolver state default settings
 */
/* #define __RES_IN_BSS */
#ifdef __RES_IN_BSS
struct __res_state _res;
#else
struct __res_state _res = {
	RES_TIMEOUT,               	/* retransmission time interval */
	4,                         	/* number of times to retransmit */
	RES_DEFAULT,			/* options flags */
	1,                         	/* number of name servers */
};
#endif
#ifdef INET6
struct __res_state_ext _res_ext;
#endif /* INET6 */

/*
 * Set up default settings.  If the configuration file exist, the values
 * there will have precedence.  Otherwise, the server address is set to
 * INADDR_ANY and the default domain name comes from the gethostname().
 *
 * An interrim version of this code (BIND 4.9, pre-4.4BSD) used 127.0.0.1
 * rather than INADDR_ANY ("0.0.0.0") as the default name server address
 * since it was noted that INADDR_ANY actually meant ``the first interface
 * you "ifconfig"'d at boot time'' and if this was a SLIP or PPP interface,
 * it had to be "up" in order for you to reach your own name server.  It
 * was later decided that since the recommended practice is to always 
 * install local static routes through 127.0.0.1 for all your network
 * interfaces, that we could solve this problem without a code change.
 *
 * The configuration file should always be used, since it is the only way
 * to specify a default domain.  If you are running a server on your local
 * machine, you should say "nameserver 0.0.0.0" or "nameserver 127.0.0.1"
 * in the configuration file.
 *
 * Return 0 if completes successfully, -1 on error
 */
int
res_init()
{
	register FILE *fp;
	register char *cp, **pp;
	register int n;
	char buf[MAXDNAME];
	int nserv = 0;    /* number of nameserver records read from file */
	int haveenv = 0;
	int havesearch = 0;
	int nsort = 0;
	char *net;
#ifndef RFC1535
	int dots;
#endif

#ifdef __RES_IN_BSS
	/*
	 * These three fields used to be statically initialized.  This made
	 * it hard to use this code in a shared library.  It is necessary,
	 * now that we're doing dynamic initialization here, that we preserve
	 * the old semantics: if an application modifies one of these three
	 * fields of _res before res_init() is called, res_init() will not
	 * alter them.  Of course, if an application is setting them to
	 * _zero_ before calling res_init(), hoping to override what used
	 * to be the static default, we can't detect it and unexpected results
	 * will follow.  Zero for any of these fields would make no sense,
	 * so one can safely assume that the applications were already getting
	 * unexpected results.
	 *
	 * _res.options is tricky since some apps were known to diddle the bits
	 * before res_init() was first called. We can't replicate that semantic
	 * with dynamic initialization (they may have turned bits off that are
	 * set in RES_DEFAULT).  Our solution is to declare such applications
	 * "broken".  They could fool us by setting RES_INIT but none do (yet).
	 */
	if (!_res.retrans)
		_res.retrans = RES_TIMEOUT;
	if (!_res.retry)
		_res.retry = 4;
	if (!(_res.options & RES_INIT))
		_res.options = RES_DEFAULT;

	/*
	 * This one used to initialize implicitly to zero, so unless the app
	 * has set it to something in particular, we can randomize it now.
	 */
	if (!_res.id)
		_res.id = res_randomid();
#else
	_res.id = res_randomid();
#endif

#ifdef USELOOPBACK
	_res.nsaddr.sin_addr = inet_makeaddr(IN_LOOPBACKNET, 1);
#else
	_res.nsaddr.sin_addr.s_addr = INADDR_ANY;
#endif
	_res.nsaddr.sin_family = AF_INET;
	_res.nsaddr.sin_port = htons(NAMESERVER_PORT);
	_res.nsaddr.sin_len = sizeof(struct sockaddr_in);
#ifdef INET6
	if (sizeof(_res_ext.nsaddr) >= _res.nsaddr.sin_len)
		memcpy(&_res_ext.nsaddr, &_res.nsaddr,
		    (size_t)_res.nsaddr.sin_len);
#endif
	_res.nscount = 1;
	_res.ndots = 1;
	_res.pfcode = 0;

	/* Allow user to override the local domain definition */
	if ((cp = getenv("LOCALDOMAIN")) != NULL) {
		(void)strlcpy(_res.defdname, cp, sizeof(_res.defdname));
		if ((cp = strpbrk(_res.defdname, " \t\n")) != NULL)
			*cp = '\0';
		haveenv++;

		/*
		 * Set search list to be blank-separated strings
		 * from rest of env value.  Permits users of LOCALDOMAIN
		 * to still have a search list, and anyone to set the
		 * one that they want to use as an individual (even more
		 * important now that the rfc1535 stuff restricts searches)
		 */
		cp = _res.defdname;
		pp = _res.dnsrch;
		*pp++ = cp;
		for (n = 0; *cp && pp < _res.dnsrch + MAXDNSRCH; cp++) {
			if (*cp == '\n')	/* silly backwards compat */
				break;
			else if (*cp == ' ' || *cp == '\t') {
				*cp = 0;
				n = 1;
			} else if (n) {
				*pp++ = cp;
				n = 0;
				havesearch = 1;
			}
		}
		/* null terminate last domain if there are excess */
		while (*cp != '\0' && *cp != ' ' && *cp != '\t' && *cp != '\n')
			cp++;
		*cp = '\0';
		*pp++ = 0;
	}

#define	MATCH(line, name) \
	(!strncmp(line, name, sizeof(name) - 1) && \
	(line[sizeof(name) - 1] == ' ' || \
	 line[sizeof(name) - 1] == '\t'))

	if ((fp = fopen(_PATH_RESCONF, "r")) != NULL) {
	    /* read the config file */
	    while (fgets(buf, sizeof(buf), fp) != NULL) {
		/* skip comments */
		if ((*buf == ';') || (*buf == '#'))
			continue;
		/* read default domain name */
		if (MATCH(buf, "domain")) {
		    if (haveenv)	/* skip if have from environ */
			    continue;
		    cp = buf + sizeof("domain") - 1;
		    while (*cp == ' ' || *cp == '\t')
			    cp++;
		    if ((*cp == '\0') || (*cp == '\n'))
			    continue;
		    (void)strlcpy(_res.defdname, cp, sizeof(_res.defdname));
		    if ((cp = strpbrk(_res.defdname, " \t\n")) != NULL)
			    *cp = '\0';
		    havesearch = 0;
		    continue;
		}
		/* lookup types; deprecated in favour of nsswitch.conf(5) */
		if (MATCH(buf, "lookup"))
		    continue;
		/* set search list */
		if (MATCH(buf, "search")) {
		    if (haveenv)	/* skip if have from environ */
			    continue;
		    cp = buf + sizeof("search") - 1;
		    while (*cp == ' ' || *cp == '\t')
			    cp++;
		    if ((*cp == '\0') || (*cp == '\n'))
			    continue;
		    (void)strlcpy(_res.defdname, cp, sizeof(_res.defdname));
		    if ((cp = strchr(_res.defdname, '\n')) != NULL)
			    *cp = '\0';
		    /*
		     * Set search list to be blank-separated strings
		     * on rest of line.
		     */
		    cp = _res.defdname;
		    pp = _res.dnsrch;
		    *pp++ = cp;
		    for (n = 0; *cp && pp < _res.dnsrch + MAXDNSRCH; cp++) {
			    if (*cp == ' ' || *cp == '\t') {
				    *cp = 0;
				    n = 1;
			    } else if (n) {
				    *pp++ = cp;
				    n = 0;
			    }
		    }
		    /* null terminate last domain if there are excess */
		    while (*cp != '\0' && *cp != ' ' && *cp != '\t')
			    cp++;
		    *cp = '\0';
		    *pp++ = 0;
		    havesearch = 1;
		    continue;
		}
		/* read nameservers to query */
		if (MATCH(buf, "nameserver") && nserv < MAXNS) {
		    struct addrinfo hints, *res;
		    char pbuf[NI_MAXSERV];
		    char *q;
#ifdef INET6
		    const size_t minsiz = sizeof(_res_ext.nsaddr_list[0]);
#else
		    const size_t minsiz = sizeof(_res.nsaddr_list[0]);
#endif

		    cp = buf + sizeof("nameserver") - 1;
		    while (*cp == ' ' || *cp == '\t')
			cp++;
		    if ((*cp == '\0') || (*cp == '\n'))
			continue;
		    for (q = cp; *q; q++) {
			if (isspace((u_char) *q)) {
			    *q = '\0';
			    break;
			}
		    }
		    memset(&hints, 0, sizeof(hints));
		    hints.ai_family = PF_UNSPEC;
		    hints.ai_socktype = SOCK_DGRAM;
		    hints.ai_flags = AI_NUMERICHOST;
		    snprintf(pbuf, sizeof(pbuf), "%u", NAMESERVER_PORT);
		    res = NULL;
		    if (getaddrinfo(cp, pbuf, &hints, &res) == 0 &&
			minsiz >= res->ai_addrlen) {
			memset(&_res.nsaddr_list[nserv], 0,
			    sizeof(_res.nsaddr_list[nserv]));
#ifdef INET6
			/*
			 * we copy the address to _res.nsaddr_list for
			 * backward compatibility only.
			 */
#endif
			if (sizeof(_res.nsaddr_list[nserv]) >= res->ai_addrlen) {
			    memcpy(&_res.nsaddr_list[nserv], res->ai_addr,
				res->ai_addrlen);
			}
#ifdef INET6
			memset(&_res_ext.nsaddr_list[nserv], 0,
			    sizeof(_res_ext.nsaddr_list[nserv]));
			memcpy(&_res_ext.nsaddr_list[nserv], res->ai_addr,
			    res->ai_addrlen);
#endif
			nserv++;
		    }
		    if (res)
			freeaddrinfo(res);
		    continue;
		}
		if (MATCH(buf, "sortlist")) {
		    struct in_addr a;
#ifdef INET6
		    struct in6_addr a6;
		    int m, i;
		    u_char *u;
#endif /* INET6 */

		    cp = buf + sizeof("sortlist") - 1;
		    while (nsort < MAXRESOLVSORT) {
			while (*cp == ' ' || *cp == '\t')
			    cp++;
			if (*cp == '\0' || *cp == '\n' || *cp == ';')
			    break;
			net = cp;
			while (*cp && !ISSORTMASK(*cp) && *cp != ';' &&
			       isascii(*cp) && !isspace((u_char) *cp))
				cp++;
			n = *cp;
			*cp = 0;
			if (inet_aton(net, &a)) {
			    _res.sort_list[nsort].addr = a;
			    if (ISSORTMASK(n)) {
				*cp++ = n;
				net = cp;
				while (*cp && *cp != ';' &&
					isascii(*cp) && !isspace((u_char) *cp))
				    cp++;
				n = *cp;
				*cp = 0;
				if (inet_aton(net, &a)) {
				    _res.sort_list[nsort].mask = a.s_addr;
				} else {
				    _res.sort_list[nsort].mask = 
					net_mask(_res.sort_list[nsort].addr);
				}
			    } else {
				_res.sort_list[nsort].mask = 
				    net_mask(_res.sort_list[nsort].addr);
			    }
#ifdef INET6
			    _res_ext.sort_list[nsort].af = AF_INET;
			    _res_ext.sort_list[nsort].addr.ina =
				_res.sort_list[nsort].addr;
			    _res_ext.sort_list[nsort].mask.ina.s_addr =
				_res.sort_list[nsort].mask;
#endif /* INET6 */
			    nsort++;
			}
#ifdef INET6
			else if (inet_pton(AF_INET6, net, &a6) == 1) {
			    _res_ext.sort_list[nsort].af = AF_INET6;
			    _res_ext.sort_list[nsort].addr.in6a = a6;
			    u = (u_char *)(void *)
				&_res_ext.sort_list[nsort].mask.in6a;
			    *cp++ = n;
			    net = cp;
			    while (*cp && *cp != ';' &&
				    isascii(*cp) && !isspace((u_char) *cp))
				cp++;
			    m = n;
			    n = *cp;
			    *cp = 0;
			    switch (m) {
			    case '/':
				m = atoi(net);
				break;
			    case '&':
				if (inet_pton(AF_INET6, net, u) == 1) {
				    m = -1;
				    break;
				}
				/*FALLTHRU*/
			    default:
				m = sizeof(struct in6_addr) * NBBY;
				break;
			    }
			    if (m >= 0) {
				for (i = 0; i < sizeof(struct in6_addr); i++) {
				    if (m <= 0) {
					*u = 0;
				    } else {
					m -= NBBY;
					*u = (u_char)~0;
					if (m < 0)
					    *u <<= -m;
				    }
				    u++;
				}
			    }
			    nsort++;
			}
#endif /* INET6 */
			*cp = n;
		    }
		    continue;
		}
		if (MATCH(buf, "options")) {
		    res_setoptions(buf + sizeof("options") - 1, "conf");
		    continue;
		}
	    }
	    if (nserv > 1) 
		_res.nscount = nserv;
	    _res.nsort = nsort;
	    (void) fclose(fp);
	}
	if (_res.defdname[0] == 0) {
		int rv;

		rv = gethostname(buf, sizeof(_res.defdname));
		_res.defdname[sizeof(_res.defdname) - 1] = '\0';
		if (rv == 0 && (cp = strchr(buf, '.')))
			(void)strlcpy(_res.defdname, cp + 1,
			    sizeof(_res.defdname));
	}

	/* find components of local domain that might be searched */
	if (havesearch == 0) {
		pp = _res.dnsrch;
		*pp++ = _res.defdname;
		*pp = NULL;

#ifndef RFC1535
		dots = 0;
		for (cp = _res.defdname; *cp; cp++)
			dots += (*cp == '.');

		cp = _res.defdname;
		while (pp < _res.dnsrch + MAXDFLSRCH) {
			if (dots < LOCALDOMAINPARTS)
				break;
			cp = strchr(cp, '.') + 1;    /* we know there is one */
			*pp++ = cp;
			dots--;
		}
		*pp = NULL;
#ifdef DEBUG
		if (_res.options & RES_DEBUG) {
			printf(";; res_init()... default dnsrch list:\n");
			for (pp = _res.dnsrch; *pp; pp++)
				printf(";;\t%s\n", *pp);
			printf(";;\t..END..\n");
		}
#endif /* DEBUG */
#endif /* !RFC1535 */
	}

	if ((cp = getenv("RES_OPTIONS")) != NULL)
		res_setoptions(cp, "env");
	_res.options |= RES_INIT;
	return (0);
}

/* ARGSUSED */
static void
res_setoptions(options, source)
	char *options, *source;
{
	char *cp = options;
	int i;

	_DIAGASSERT(options != NULL);
	_DIAGASSERT(source != NULL);

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf(";; res_setoptions(\"%s\", \"%s\")...\n",
		       options, source);
#endif
	while (*cp) {
		/* skip leading and inner runs of spaces */
		while (*cp == ' ' || *cp == '\t')
			cp++;
		/* search for and process individual options */
		if (!strncmp(cp, "ndots:", sizeof("ndots:") - 1)) {
			i = atoi(cp + sizeof("ndots:") - 1);
			if (i <= RES_MAXNDOTS)
				_res.ndots = i;
			else
				_res.ndots = RES_MAXNDOTS;
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf(";;\tndots=%u\n", _res.ndots);
#endif
		} else if (!strncmp(cp, "debug", sizeof("debug") - 1)) {
#ifdef DEBUG
			if (!(_res.options & RES_DEBUG)) {
				printf(";; res_setoptions(\"%s\", \"%s\")..\n",
				       options, source);
				_res.options |= RES_DEBUG;
			}
			printf(";;\tdebug\n");
#endif
		} else if (!strncmp(cp, "inet6", sizeof("inet6") - 1)) {
			_res.options |= RES_USE_INET6;
		} else if (!strncmp(cp, "insecure1", sizeof("insecure1") - 1)) {
			_res.options |= RES_INSECURE1;
		} else if (!strncmp(cp, "insecure2", sizeof("insecure2") - 1)) {
			_res.options |= RES_INSECURE2;
		}
#ifdef RES_USE_EDNS0
		else if (!strncmp(cp, "edns0", sizeof("edns0") - 1)) {
		       _res.options |= RES_USE_EDNS0;
		}
#endif
		else {
			/* XXX - print a warning here? */
		}
		/* skip to next run of spaces */
		while (*cp && *cp != ' ' && *cp != '\t')
			cp++;
	}
}

/* XXX - should really support CIDR which means explicit masks always. */
static u_int32_t
net_mask(in)		/* XXX - should really use system's version of this */
	struct in_addr in;
{
	register u_int32_t i = ntohl(in.s_addr);

	if (IN_CLASSA(i))
		return (htonl(IN_CLASSA_NET));
	else if (IN_CLASSB(i))
		return (htonl(IN_CLASSB_NET));
	return (htonl(IN_CLASSC_NET));
}

/*	$OpenBSD: res_random.c,v 1.12 2002/06/27 10:14:02 itojun Exp $	*/

/* 
 * seed = random 15bit
 * n = prime, g0 = generator to n,
 * j = random so that gcd(j,n-1) == 1
 * g = g0^j mod n will be a generator again.
 *
 * X[0] = random seed.
 * X[n] = a*X[n-1]+b mod m is a Linear Congruential Generator
 * with a = 7^(even random) mod m, 
 *      b = random with gcd(b,m) == 1
 *      m = 31104 and a maximal period of m-1.
 *
 * The transaction id is determined by:
 * id[n] = seed xor (g^X[n] mod n)
 *
 * Effectivly the id is restricted to the lower 15 bits, thus
 * yielding two different cycles by toggling the msb on and off.
 * This avoids reuse issues caused by reseeding.
 *
 * The 16 bit space is very small and brute force attempts are
 * entirly feasible, we skip a random number of transaction ids
 * so that an attacker will not get sequential ids.
 */

#define RU_OUT  180             /* Time after wich will be reseeded */
#define RU_MAX	30000		/* Uniq cycle, avoid blackjack prediction */
#define RU_GEN	2		/* Starting generator */
#define RU_N	32749		/* RU_N-1 = 2*2*3*2729 */
#define RU_AGEN	7               /* determine ru_a as RU_AGEN^(2*rand) */
#define RU_M	31104           /* RU_M = 2^7*3^5 - don't change */

#define PFAC_N 3
const static u_int16_t pfacts[PFAC_N] = {
	2, 
	3,
	2729
};

static u_int16_t ru_x;
static u_int16_t ru_seed, ru_seed2;
static u_int16_t ru_a, ru_b;
static u_int16_t ru_g;
static u_int16_t ru_counter = 0;
static u_int16_t ru_msb = 0;
static long ru_reseed;
static u_int32_t ru_tmp;		/* Storage for unused random */

static u_int16_t pmod(u_int16_t, u_int16_t, u_int16_t);
static void res_initid(void);

/*
 * Do a fast modular exponation, returned value will be in the range
 * of 0 - (mod-1)
 */

static u_int16_t
pmod(u_int16_t gen, u_int16_t exp, u_int16_t mod)
{
	u_int16_t s, t, u;

	s = 1;
	t = gen;
	u = exp;

	while (u) {
		if (u & 1)
			s = (s * t) % mod;
		u >>= 1;
		t = (t * t) % mod;
	}
	return (s);
}

/* 
 * Initializes the seed and chooses a suitable generator. Also toggles 
 * the msb flag. The msb flag is used to generate two distinct
 * cycles of random numbers and thus avoiding reuse of ids.
 *
 * This function is called from res_randomid() when needed, an 
 * application does not have to worry about it.
 */
static void 
res_initid(void)
{
	u_int16_t j, i;
	int noprime = 1;
	struct timeval tv;

	ru_tmp = arc4random();
	ru_x = (ru_tmp & 0xFFFF) % RU_M;

	/* 15 bits of random seed */
	ru_seed = (ru_tmp >> 16) & 0x7FFF;
	ru_tmp = arc4random();
	ru_seed2 = ru_tmp & 0x7FFF;

	ru_tmp = arc4random();

	/* Determine the LCG we use */
	ru_b = (ru_tmp & 0xfffe) | 1;
	ru_a = pmod(RU_AGEN, (ru_tmp >> 16) & 0xfffe, RU_M);
	while (ru_b % 3 == 0)
		ru_b += 2;
	
	ru_tmp = arc4random();
	j = ru_tmp % RU_N;
	ru_tmp = ru_tmp >> 16;

	/* 
	 * Do a fast gcd(j,RU_N-1), so we can find a j with
	 * gcd(j, RU_N-1) == 1, giving a new generator for
	 * RU_GEN^j mod RU_N
	 */

	while (noprime) {
		for (i = 0; i < PFAC_N; i++)
			if (j % pfacts[i] == 0)
				break;

		if (i >= PFAC_N)
			noprime = 0;
		else 
			j = (j + 1) % RU_N;
	}

	ru_g = pmod(RU_GEN, j, RU_N);
	ru_counter = 0;

	gettimeofday(&tv, NULL);
	ru_reseed = tv.tv_sec + RU_OUT;
	ru_msb = ru_msb == 0x8000 ? 0 : 0x8000; 
}

u_int
res_randomid(void)
{
        int i, n;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (ru_counter >= RU_MAX || tv.tv_sec > ru_reseed)
		res_initid();

	if (!ru_tmp)
	        ru_tmp = arc4random();

	/* Skip a random number of ids */
	n = ru_tmp & 0x7; ru_tmp = ru_tmp >> 3;
	if (ru_counter + n >= RU_MAX)
                res_initid();

	for (i = 0; i <= n; i++) {
	        /* Linear Congruential Generator */
	        ru_x = (ru_a * ru_x + ru_b) % RU_M;
	}

	ru_counter += i;

	return (ru_seed ^ pmod(ru_g, ru_seed2 ^ ru_x, RU_N)) | ru_msb;
}
