/*	$NetBSD: getnetnamadr.c,v 1.11 1999/01/19 08:01:48 lukem Exp $	*/

/* Copyright (c) 1993 Carlos Leandro and Rui Salgueiro
 *	Dep. Matematica Universidade de Coimbra, Portugal, Europe
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
/*
 * Copyright (c) 1983, 1993
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getnetbyaddr.c	8.1 (Berkeley) 6/4/93";
static char sccsid_[] = "from getnetnamadr.c	1.4 (Coimbra) 93/06/03";
static char rcsid[] = "Id: getnetnamadr.c,v 8.8 1997/06/01 20:34:37 vixie Exp ";
#else
__RCSID("$NetBSD: getnetnamadr.c,v 1.11 1999/01/19 08:01:48 lukem Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <nsswitch.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef YP
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#endif

#ifdef __weak_alias
__weak_alias(getnetbyaddr,_getnetbyaddr);
__weak_alias(getnetbyname,_getnetbyname);
#endif

extern int h_errno;
extern int _net_stayopen;

#if defined(mips) && defined(SYSTYPE_BSD43)
extern int errno;
#endif

#define BYADDR 0
#define BYNAME 1
#define	MAXALIASES	35

#if PACKETSZ > 1024
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	1024
#endif

typedef union {
	HEADER	hdr;
	u_char	buf[MAXPACKET];
} querybuf;

typedef union {
	long	al;
	char	ac;
} align;

#ifdef YP
static char *__ypdomain;
static char *__ypcurrent;
static int   __ypcurrentlen;
#endif

static	struct netent net_entry;
static	char *net_aliases[MAXALIASES];

static struct netent *getnetanswer __P((querybuf *, int, int));
int	_getnetbyaddr __P((void *, void *, va_list));
int	_getnetbyname __P((void *, void *, va_list));
int	_dns_getnetbyaddr __P((void *, void *, va_list));
int	_dns_getnetbyname __P((void *, void *, va_list));
#ifdef YP
int	_yp_getnetbyaddr __P((void *, void *, va_list));
int	_yp_getnetbyname __P((void *, void *, va_list));
struct netent *_ypnetent __P((char *));
#endif

static struct netent *
getnetanswer(answer, anslen, net_i)
	querybuf *answer;
	int anslen;
	int net_i;
{

	HEADER *hp;
	u_char *cp;
	int n;
	u_char *eom;
	int type, class, buflen, ancount, qdcount, haveanswer, i, nchar;
	char aux1[30], aux2[30], ans[30], *in, *st, *pauxt, *bp, **ap,
		*paux1 = &aux1[0], *paux2 = &aux2[0], flag = 0;
	static	char netbuf[PACKETSZ];

	/*
	 * find first satisfactory answer
	 *
	 *      answer --> +------------+  ( MESSAGE )
	 *		   |   Header   |
	 *		   +------------+
	 *		   |  Question  | the question for the name server
	 *		   +------------+
	 *		   |   Answer   | RRs answering the question
	 *		   +------------+
	 *		   | Authority  | RRs pointing toward an authority
	 *		   | Additional | RRs holding additional information
	 *		   +------------+
	 */
	eom = answer->buf + anslen;
	hp = &answer->hdr;
	ancount = ntohs(hp->ancount); /* #/records in the answer section */
	qdcount = ntohs(hp->qdcount); /* #/entries in the question section */
	bp = netbuf;
	buflen = sizeof(netbuf);
	cp = answer->buf + HFIXEDSZ;
	if (!qdcount) {
		if (hp->aa)
			h_errno = HOST_NOT_FOUND;
		else
			h_errno = TRY_AGAIN;
		return (NULL);
	}
	while (qdcount-- > 0)
		cp += __dn_skipname(cp, eom) + QFIXEDSZ;
	ap = net_aliases;
	*ap = NULL;
	net_entry.n_aliases = net_aliases;
	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		n = dn_expand(answer->buf, eom, cp, bp, buflen);
		if ((n < 0) || !res_dnok(bp))
			break;
		cp += n;
		ans[0] = '\0';
		(void)strcpy(&ans[0], bp);
		GETSHORT(type, cp);
		GETSHORT(class, cp);
		cp += INT32SZ;		/* TTL */
		GETSHORT(n, cp);
		if (class == C_IN && type == T_PTR) {
			n = dn_expand(answer->buf, eom, cp, bp, buflen);
			if ((n < 0) || !res_hnok(bp)) {
				cp += n;
				return (NULL);
			}
			cp += n; 
			*ap++ = bp;
			bp += strlen(bp) + 1;
			net_entry.n_addrtype =
				(class == C_IN) ? AF_INET : AF_UNSPEC;
			haveanswer++;
		}
	}
	if (haveanswer) {
		*ap = NULL;
		switch (net_i) {
		case BYADDR:
			net_entry.n_name = *net_entry.n_aliases;
			net_entry.n_net = 0L;
			break;
		case BYNAME:
			in = *net_entry.n_aliases;
			net_entry.n_name = &ans[0];
			aux2[0] = '\0';
			for (i = 0; i < 4; i++) {
				for (st = in, nchar = 0;
				     *st != '.';
				     st++, nchar++)
					;
				if (nchar != 1 || *in != '0' || flag) {
					flag = 1;
					(void)strncpy(paux1,
					    (i==0) ? in : in-1,
					    (size_t)((i==0) ? nchar : nchar+1));
					paux1[(i==0) ? nchar : nchar+1] = '\0';
					pauxt = paux2;
					paux2 = strcat(paux1, paux2);
					paux1 = pauxt;
				}
				in = ++st;
			}		  
			net_entry.n_net = inet_network(paux2);
			break;
		}
		net_entry.n_aliases++;
		return (&net_entry);
	}
	h_errno = TRY_AGAIN;
	return (NULL);
}

int
_getnetbyaddr(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	struct netent *p;
	unsigned long net;
	int type;

	net = va_arg(ap, unsigned long);
	type = va_arg(ap, int);

	setnetent(_net_stayopen);
	while ((p = getnetent()) != NULL)
		if (p->n_addrtype == type && p->n_net == net)
			break;
	if (!_net_stayopen)
		endnetent();
	*((struct netent **)rv) = p;
	if (p==NULL) {
		h_errno = HOST_NOT_FOUND;
		return NS_NOTFOUND;
	}
	return NS_SUCCESS;
}

int
_dns_getnetbyaddr(rv, cb_data, ap)
	void    *rv;
	void    *cb_data;
	va_list  ap;
{
	unsigned int netbr[4];
	int nn, anslen;
	querybuf buf;
	char qbuf[MAXDNAME];
	unsigned long net2;
	struct netent *np;
	unsigned long net;
	int type;

	net = va_arg(ap, unsigned long);
	type = va_arg(ap, int);

	if (type != AF_INET)
		return NS_UNAVAIL;

	for (nn = 4, net2 = net; net2; net2 >>= 8)
		netbr[--nn] = (unsigned int)(net2 & 0xff);
	switch (nn) {
	default:
		return NS_UNAVAIL;
	case 3: 	/* Class A */
		sprintf(qbuf, "0.0.0.%u.in-addr.arpa", netbr[3]);
		break;
	case 2: 	/* Class B */
		sprintf(qbuf, "0.0.%u.%u.in-addr.arpa", netbr[3], netbr[2]);
		break;
	case 1: 	/* Class C */
		sprintf(qbuf, "0.%u.%u.%u.in-addr.arpa", netbr[3], netbr[2],
		    netbr[1]);
		break;
	case 0: 	/* Class D - E */
		sprintf(qbuf, "%u.%u.%u.%u.in-addr.arpa", netbr[3], netbr[2],
		    netbr[1], netbr[0]);
		break;
	}
	anslen = res_query(qbuf, C_IN, T_PTR, (u_char *)(void *)&buf,
	    sizeof(buf));
	if (anslen < 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("res_query failed\n");
#endif
		return NS_NOTFOUND;
	}
	np = getnetanswer(&buf, anslen, BYADDR);
	if (np) {
		/* maybe net should be unsigned? */
		unsigned long u_net = net;

		/* Strip trailing zeros */
		while ((u_net & 0xff) == 0 && u_net != 0)
			u_net >>= 8;
		np->n_net = u_net;
	}
	*((struct netent **)rv) = np;
	if (np == NULL) {
		h_errno = HOST_NOT_FOUND;
		return NS_NOTFOUND;
	}
	return NS_SUCCESS;
}


struct netent *
getnetbyaddr(net, net_type)
	u_long net;
	int net_type;
{
	struct netent *np;
	static ns_dtab dtab[] = {
		NS_FILES_CB(_getnetbyaddr, NULL)
		{ NSSRC_DNS, _dns_getnetbyaddr, NULL },	/* force -DHESIOD */
		NS_NIS_CB(_yp_getnetbyaddr, NULL)
		{ 0 }
	};

	if ((_res.options & RES_INIT) == 0 && res_init() == -1) {
		h_errno = NETDB_INTERNAL;
		return (NULL);
	}

	np = NULL;
	h_errno = NETDB_INTERNAL;
	if (nsdispatch(&np, dtab, NSDB_NETWORKS, "getnetbyaddr", __nsdefaultsrc,
	    net, net_type) != NS_SUCCESS)
		return (NULL);
	h_errno = NETDB_SUCCESS;
	return (np);
}

int
_getnetbyname(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	struct netent *p;
	char **cp;
	const char *name;

	name = va_arg(ap, const char *);
	setnetent(_net_stayopen);
	while ((p = getnetent()) != NULL) {
		if (strcasecmp(p->n_name, name) == 0)
			break;
		for (cp = p->n_aliases; *cp != 0; cp++)
			if (strcasecmp(*cp, name) == 0)
				goto found;
	}
found:
	if (!_net_stayopen)
		endnetent();
	*((struct netent **)rv) = p;
	if (p==NULL) {
		h_errno = HOST_NOT_FOUND;
		return NS_NOTFOUND;
	}
	return NS_SUCCESS;
}

int
_dns_getnetbyname(rv, cb_data, ap)
	void    *rv;
	void    *cb_data;
	va_list  ap;
{
	int anslen;
	querybuf buf;
	char qbuf[MAXDNAME];
	struct netent *np;
	const char *net;

	net = va_arg(ap, const char *);
	strcpy(&qbuf[0], net);
	anslen = res_search(qbuf, C_IN, T_PTR, (u_char *)(void *)&buf,
	    sizeof(buf));
	if (anslen < 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("res_query failed\n");
#endif
		return NS_NOTFOUND;
	}
	np = getnetanswer(&buf, anslen, BYNAME);

	*((struct netent **)rv) = np;
	if (np == NULL) {
		h_errno = HOST_NOT_FOUND;
		return NS_NOTFOUND;
	}
	return NS_SUCCESS;
}

struct netent *
getnetbyname(net)
	const char *net;
{
	struct netent *np;
	static ns_dtab dtab[] = {
		NS_FILES_CB(_getnetbyname, NULL)
		{ NSSRC_DNS, _dns_getnetbyname, NULL },	/* force -DHESIOD */
		NS_NIS_CB(_yp_getnetbyname, NULL)
		{ 0 }
	};

	if ((_res.options & RES_INIT) == 0 && res_init() == -1) {
		h_errno = NETDB_INTERNAL;
		return (NULL);
	}

	np = NULL;
	h_errno = NETDB_INTERNAL;
	if (nsdispatch(&np, dtab, NSDB_NETWORKS, "getnetbyname", __nsdefaultsrc,
	    net) != NS_SUCCESS)
		return (NULL);
	h_errno = NETDB_SUCCESS;
	return (np);
}

#ifdef YP

int
_yp_getnetbyaddr(rv, cb_data, ap)
	void    *rv;
	void    *cb_data;
	va_list  ap;
{
	struct netent *np;
	char qbuf[MAXDNAME];
	unsigned int netbr[4];
	unsigned long net, net2;
	int type, r;

	net = va_arg(ap, unsigned long);
	type = va_arg(ap, int);

	if (type != AF_INET)
		return NS_UNAVAIL;

	if (!__ypdomain) {
		if (_yp_check(&__ypdomain) == 0)
			return NS_UNAVAIL;
	}
	np = NULL;
	if (__ypcurrent)
		free(__ypcurrent);
	__ypcurrent = NULL;
	for (r = 4, net2 = net; net2; net2 >>= 8)
		netbr[--r] = (unsigned int)(net2 & 0xff);
	switch (r) {
	default:
		return NS_UNAVAIL;
	case 3: 	/* Class A */
		sprintf(qbuf, "%u", netbr[0]);
		break;
	case 2: 	/* Class B */
		sprintf(qbuf, "%u.%u", netbr[0], netbr[1]);
		break;
	case 1: 	/* Class C */
		sprintf(qbuf, "%u.%u.%u", netbr[0], netbr[1], netbr[2]);
		break;
	case 0: 	/* Class D - E */
		sprintf(qbuf, "%u.%u.%u.%u", netbr[0], netbr[1], netbr[2],
		    netbr[3]);
		break;
	}
	r = yp_match(__ypdomain, "networks.byaddr", qbuf, (int)strlen(qbuf),
	    &__ypcurrent, &__ypcurrentlen);
	if (r == 0)
		np = _ypnetent(__ypcurrent);

	*((struct netent **)rv) = np;
	if (np == NULL) {
		h_errno = HOST_NOT_FOUND;
		return NS_NOTFOUND;
	}
	return NS_SUCCESS;

}

int
_yp_getnetbyname(rv, cb_data, ap)
	void    *rv;
	void    *cb_data;
	va_list  ap;
{
	struct netent *np;
	const char *name;
	int r;

	name = va_arg(ap, const char *);

	if (!__ypdomain) {
		if (_yp_check(&__ypdomain) == 0)
			return NS_UNAVAIL;
	}
	np = NULL;
	if (__ypcurrent)
		free(__ypcurrent);
	__ypcurrent = NULL;
	r = yp_match(__ypdomain, "networks.byname", name, (int)strlen(name),
	    &__ypcurrent, &__ypcurrentlen);
	if (r == 0)
		np = _ypnetent(__ypcurrent);

	*((struct netent **)rv) = np;
	if (np == NULL) {
		h_errno = HOST_NOT_FOUND;
		return NS_NOTFOUND;
	}
	return NS_SUCCESS;
}

struct netent *
_ypnetent(line)
	char *line;
{
	char *cp, *p, **q;

	net_entry.n_name = line;
	cp = strpbrk(line, " \t");
	if (cp == NULL)
		return (NULL);
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = strpbrk(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	net_entry.n_net = inet_network(cp);
	net_entry.n_addrtype = AF_INET;
	q = net_entry.n_aliases = net_aliases;
	if (p != NULL)  {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &net_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = strpbrk(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;

	return (&net_entry);
}
#endif
