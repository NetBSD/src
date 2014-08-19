/*	$NetBSD: gethost.c,v 1.8.62.1 2014/08/19 23:45:45 tls Exp $	*/

/*-
 * Copyright (c) 1985, 1988, 1993
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

/* Provide just /etc/hosts lookup support */

#include <sys/cdefs.h>

#ifdef __weak_alias
#define gethostbyaddr		_gethostbyaddr
#define gethostbyname		_gethostbyname
#endif

#include <netdb.h>
#include <string.h>
#include <nsswitch.h>
#include <errno.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "hostent.h"

#ifdef __weak_alias
__weak_alias(gethostbyaddr,_gethostbyaddr);
__weak_alias(gethostbyname,_gethostbyname);
#endif

int h_errno;
FILE *_h_file;
static struct hostent h_ent;
static char h_buf[4096]; 

static struct hostent *
getby(int (*f)(void *, void *, va_list), struct getnamaddr *info, ...)
{
        va_list ap;
        int e;
        
        va_start(ap, info);
        e = (*f)(info, NULL, ap);
        va_end(ap); 
        switch (e) {
        case NS_SUCCESS: 
                return info->hp;  
        default:
		return NULL;
        }       
}                       

struct hostent *
gethostbyname_r(const char *name, struct hostent *hp, char *buf, size_t bufsiz,
    int *he)
{
	struct getnamaddr info;
	info.hp = hp;
	info.buf = buf;
	info.buflen = bufsiz;
	info.he = he;
	return getby(_hf_gethtbyname, &info, name, 0, AF_INET);
}


struct hostent *
gethostbyname(const char *name)
{
	return gethostbyname_r(name, &h_ent, h_buf, sizeof(h_buf), &h_errno);
}

struct hostent *
gethostbyaddr_r(const void *addr, socklen_t len, int type, struct hostent *hp,
    char *buf, size_t bufsiz, int *he)
{
	struct getnamaddr info;
	info.hp = hp;
	info.buf = buf;
	info.buflen = bufsiz;
	info.he = he;
	return getby(_hf_gethtbyaddr, &info, addr, len, type);
}

struct hostent *
gethostbyaddr(const void *addr, socklen_t len, int type)
{
	return gethostbyaddr_r(addr, len, type, &h_ent, h_buf, sizeof(h_buf),
	    &h_errno);
}

struct hostent *
gethostent_r(FILE *hf, struct hostent *hent, char *buf, size_t buflen, int *he)
{
	char *p, *name;
	char *cp, **q;
	int af, len;
	size_t llen, anum;
	char *aliases[MAXALIASES];
	struct in6_addr host_addr;

	if (hf == NULL) {
		*he = NETDB_INTERNAL;
		errno = EINVAL;
		return NULL;
	}
 again:
	if ((p = fgetln(hf, &llen)) == NULL) {
		*he = HOST_NOT_FOUND;
		return NULL;
	}
	if (llen < 1)
		goto again;
	if (*p == '#')
		goto again;
	p[llen] = '\0';
	if (!(cp = strpbrk(p, "#\n")))
		goto again;
	*cp = '\0';
	if (!(cp = strpbrk(p, " \t")))
		goto again;
	*cp++ = '\0';
	if (inet_pton(AF_INET6, p, &host_addr) > 0) {
		af = AF_INET6;
		len = NS_IN6ADDRSZ;
	} else if (inet_pton(AF_INET, p, &host_addr) > 0) {
#if 0
		res_state res = __res_get_state();
		if (res == NULL)
			return NULL;
		if (res->options & RES_USE_INET6) {
			map_v4v6_address(buf, buf);
			af = AF_INET6;
			len = NS_IN6ADDRSZ;
		} else {
#endif
			af = AF_INET;
			len = NS_INADDRSZ;
#if 0
		}
		__res_put_state(res);
#endif
	} else {
		goto again;
	}
	/* if this is not something we're looking for, skip it. */
	if (hent->h_addrtype != 0 && hent->h_addrtype != af)
		goto again;
	if (hent->h_length != 0 && hent->h_length != len)
		goto again;

	while (*cp == ' ' || *cp == '\t')
		cp++;
	if ((cp = strpbrk(name = cp, " \t")) != NULL)
		*cp++ = '\0';
	q = aliases;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q >= &aliases[__arraycount(aliases)])
			goto nospc;
		*q++ = cp;
		if ((cp = strpbrk(cp, " \t")) != NULL)
			*cp++ = '\0';
	}
	hent->h_length = len;
	hent->h_addrtype = af;
	HENT_ARRAY(hent->h_addr_list, 1, buf, buflen);
	anum = (size_t)(q - aliases);
	HENT_ARRAY(hent->h_aliases, anum, buf, buflen);
	HENT_COPY(hent->h_addr_list[0], &host_addr, hent->h_length, buf,
	    buflen);
	hent->h_addr_list[1] = NULL;

	HENT_SCOPY(hent->h_name, name, buf, buflen);
	for (size_t i = 0; i < anum; i++)
		HENT_SCOPY(hent->h_aliases[i], aliases[i], buf, buflen);
	hent->h_aliases[anum] = NULL;

	*he = NETDB_SUCCESS;
	return hent;
nospc:
	errno = ENOSPC;
	*he = NETDB_INTERNAL;
	return NULL;
}
