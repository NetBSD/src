/*	$NetBSD: sockaddr_snprintf.c,v 1.3 2004/12/11 06:41:16 christos Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: sockaddr_snprintf.c,v 1.3 2004/12/11 06:41:16 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <netatalk/at.h>
#include <net/if_dl.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <util.h>
#include <netdb.h>

int
sockaddr_snprintf(char *buf, size_t len, const char *fmt,
    const struct sockaddr *sa)
{
	const void *a = NULL;
	char abuf[1024], nbuf[1024], *addr, *w;
	char *ebuf = &buf[len - 1], *sbuf = buf;
	const char *ptr, *s;
	in_port_t p = (in_port_t)~0;
	const struct sockaddr_at *sat = NULL;
	const struct sockaddr_in *sin4 = NULL;
	const struct sockaddr_in6 *sin6 = NULL;
	const struct sockaddr_un *sun = NULL;
	const struct sockaddr_dl *sdl = NULL;

#define ADDC(c) if (buf < ebuf) *buf++ = c; else buf++
#define ADDN()	if (buf < ebuf) *buf++ = '\0'; else buf[len - 1] = '\0'
#define ADDS(p) for (s = p; *s; s++) ADDC(*s)
#define ADDNA() ADDS("N/A")

	switch (sa->sa_family) {
	case AF_UNSPEC:
		goto done;
	case AF_APPLETALK:
		sat = ((const struct sockaddr_at *)(const void *)sa);
		p = ntohs(sat->sat_port);
		(void)snprintf(addr = abuf, sizeof(abuf), "%u.%u",
			ntohs(sat->sat_addr.s_net), sat->sat_addr.s_node);
		break;
	case AF_LOCAL:
		sun = ((const struct sockaddr_un *)(const void *)sa);
		(void)strlcpy(addr = abuf, sun->sun_path, SUN_LEN(sun));
		break;
	case AF_INET:
		sin4 = ((const struct sockaddr_in *)(const void *)sa);
		p = ntohs(sin4->sin_port);
		a = &sin4->sin_addr;
		break;
	case AF_INET6:
		sin6 = ((const struct sockaddr_in6 *)(const void *)sa);
		p = ntohs(sin6->sin6_port);
		a = &sin6->sin6_addr;
		break;
	case AF_LINK:
		sdl = ((const struct sockaddr_dl *)(const void *)sa);
		(void)strlcpy(addr = abuf, link_ntoa(sdl), sizeof(abuf));
		if ((w = strchr(addr, ':')) != 0) {
			*w++ = '\0';
			addr = w;
			
		}
		break;
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (a && getnameinfo(sa, (socklen_t)sa->sa_len, addr = abuf,
	    sizeof(abuf), NULL, 0, NI_NUMERICHOST|NI_NUMERICSERV) != 0)
		return -1;

	for (ptr = fmt; *ptr; ptr++) {
		if (*ptr != '%') {
			ADDC(*ptr);
			continue;
		}
		switch (*++ptr) {
		case '\0':
			ADDC('%');
			goto done;
		case 'a':
			ADDS(addr);
			break;
		case 'p':
			if (p != (in_port_t)~0) {
			    (void)snprintf(nbuf, sizeof(nbuf), "%d", p);
			    ADDS(nbuf);
			} else {
			    ADDNA();
			}
			break;
		case 'f':
			(void)snprintf(nbuf, sizeof(nbuf), "%d", sa->sa_family);
			ADDS(nbuf);
			break;
		case 'l':
			(void)snprintf(nbuf, sizeof(nbuf), "%d", sa->sa_len);
			ADDS(nbuf);
			break;
		case 'I':
			if (sdl && addr != abuf) {
				ADDS(abuf);
			} else {
				ADDNA();
			}
			break;
		case 'F':
			if (sin6) {
				(void)snprintf(nbuf, sizeof(nbuf), "%d",
				    sin6->sin6_flowinfo);
				ADDS(nbuf);
				break;
			} else {
				ADDNA();
			}
			break;
		case 'S':
			if (sin6) {
				(void)snprintf(nbuf, sizeof(nbuf), "%d",
				    sin6->sin6_scope_id);
				ADDS(nbuf);
				break;
			} else {
				ADDNA();
			}
			break;
		case 'R':
			if (sat) {
				const struct netrange *n =
				    &sat->sat_range.r_netrange;
				(void)snprintf(nbuf, sizeof(nbuf),
				    "%d:[%d,%d]", n->nr_phase , n->nr_firstnet,
				    n->nr_lastnet);
				ADDS(nbuf);
			} else {
				ADDNA();
			}
			break;
		default:
			ADDC('%');
			ADDC(*ptr);
			break;
		}
	}
done:
	ADDN();
	return buf - sbuf;
}
