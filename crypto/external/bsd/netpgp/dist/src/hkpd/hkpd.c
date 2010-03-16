/*-
 * Copyright (c) 2009,2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@NetBSD.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/select.h>

#include <netinet/in.h>

#include <errno.h>
#include <netdb.h>
#include <netpgp.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hkpd.h"

/* make the string have %xx -> %c */
static size_t
frompercent(char *in, size_t insize, char *out, size_t outsize)
{
	size_t	 outcc;
	char	*next;
	char	*pc;

	outcc = 0;
	for (pc = in ; (next = strchr(pc, '%')) != NULL ; pc = next + 3) {
		(void) memcpy(&out[outcc], pc, (size_t)(next - pc));
		outcc += (size_t)(next - pc);
		out[outcc++] = (char)strtol(next + 1, NULL, 16);
	}
	(void) memcpy(&out[outcc], pc, insize - (int)(pc - in));
	outcc += insize - (int)(pc - in);
	out[outcc] = 0x0;
	return outcc;
}

#define HKP_HTTP_LEVEL	"HTTP/1.0"
#define HKP_NAME	"hkpd"
#define HKP_MIME_GET	"application/pgp-keys"
#define HKP_MIME_INDEX	"text/plain"
#define HKP_MACHREAD	"info:1:1\r\n"

#define HKP_SUCCESS	200
#define HKP_NOT_FOUND	404

/* make into html */
static int
htmlify(char *buf, size_t size, const int code, const int get, const char *title, const int mr, const char *body)
{
	return snprintf(buf, size,
		"%s %d %s\r\n"
		"Server: %s/%d\r\n"
		"Content-type: %s\r\n"
		"\r\n"
		"%s"
		"%s",
		HKP_HTTP_LEVEL, code, (code == HKP_SUCCESS) ? "OK" : "not found",
		HKP_NAME, HKPD_VERSION,
		(get) ? HKP_MIME_GET : HKP_MIME_INDEX,
		(get || !mr) ? "" : HKP_MACHREAD,
		body);
}

/* send the response now */
static int
response(int sock, const int code, const char *search, const int get, char *buf, int cc, int mr)
{
	char	outbuf[BUFSIZ * 8];
	char	item[BUFSIZ];
	int	n;

	if (buf == NULL) {
		(void) snprintf(item, sizeof(item),
			"Error handling request: No keys found for '%s'\r\n", search);
		n = htmlify(outbuf, sizeof(outbuf), code, get,
			"Error handling request\r\n",
			mr,
			item);
	} else {
		(void) snprintf(item, sizeof(item), "Search results for '%s'", search);
		n = htmlify(outbuf, sizeof(outbuf), code, get,
			item,
			mr,
			buf);
	}
	if (write(sock, outbuf, n) != n) {
		(void) fprintf(stderr, "Short write\n");
	}
	return 1;
}

/* get a socket (we'll bind it later) */
static int
hkpd_sock_get(const int fam)
{
	int	sock;
	int	on = 1;

	sock = socket((fam == 4) ? AF_INET : AF_INET6, SOCK_STREAM, 0);
	if (sock < 0) {
		(void) fprintf(stderr,"hkpd_sock_get: can't get a socket\n");
		return -1;
        }
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(void *)&on, sizeof(on)) == -1) {
		(void) fprintf(stderr,
			"hkpd_sock_get: can't set SO_REUSEADDR\n");
		return -1;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
			(void *)&on, sizeof(on)) == -1) {
		(void) fprintf(stderr,
			"hkpd_sock_get: can't set SO_KEEPALIVE\n");
		return -1;
	}
	return sock;
}

/**************************************************************************/

/* get a socket and bind it to the server */
int
hkpd_sock_bind(const char *hostname, const int port, const int fam)
{
        struct addrinfo  hints;
        struct addrinfo *res;
        char             portstr[32];
	int		 sock;
        int              rc = 0;

        (void) memset(&hints, 0, sizeof(hints));
        hints.ai_family = (fam == 4) ? PF_INET : PF_INET6;
        hints.ai_socktype = SOCK_STREAM;
        (void) snprintf(portstr, sizeof(portstr), "%d", port);
        /* Attempt connection */
#ifdef AI_NUMERICSERV
        hints.ai_flags = AI_NUMERICSERV;
#endif
        if ((rc = getaddrinfo(hostname, portstr, &hints, &res)) != 0) {
                hints.ai_flags = 0;
                if ((rc = getaddrinfo(hostname, "hkp", &hints, &res)) != 0) {
                        (void) fprintf(stderr, "getaddrinfo: %s",
					gai_strerror(rc));
                        return -1;
                }
        }
	if ((sock = hkpd_sock_get(fam)) < 0) {
                (void) fprintf(stderr, "hkpd_sock_get failed %d\n", errno);
                freeaddrinfo(res);
                return -1;
	}
        if ((rc = bind(sock, res->ai_addr, res->ai_addrlen)) < 0) {
                (void) fprintf(stderr, "bind failed %d\n", errno);
                freeaddrinfo(res);
                return -1;
        }
        freeaddrinfo(res);
        if (rc < 0) {
                (void) fprintf(stderr, "bind() to %s:%d failed (rc %d)\n",
				hostname, port, rc);
        }
        return sock;
}

/* netpgp key daemon - does not return */
int
hkpd(netpgp_t *netpgp, int sock4, int sock6)
{
	struct sockaddr_in	from;
	regmatch_t		searchmatches[10];
	regmatch_t		opmatches[10];
	regmatch_t		mrmatch[3];
	socklen_t		fromlen;
	regex_t			machreadterm;
	regex_t			searchterm;
	regex_t			opterm;
	regex_t			get;
	fd_set			sockets;
	char			search[BUFSIZ];
	char			buf[BUFSIZ];
	char			*cp;
	int			newsock;
	int			sock;
	int			code;
	int			mr;
	int			ok;
	int			cc;
	int			n;

/* GET /pks/lookup?search=agc%40netbsd.org&op=index&options=mr HTTP/1.1\n */
#define HTTPGET		"GET /pks/lookup\\?"
#define OPTERM		"op=([a-zA-Z]+)"
#define SEARCHTERM	"search=([^ \t&]+)"
#define MACHREAD	"options=mr"

	(void) regcomp(&get, HTTPGET, REG_EXTENDED);
	(void) regcomp(&opterm, OPTERM, REG_EXTENDED);
	(void) regcomp(&searchterm, SEARCHTERM, REG_EXTENDED);
	(void) regcomp(&machreadterm, MACHREAD, REG_EXTENDED);
	if (sock4 >= 0) {
		listen(sock4, 32);
	}
	if (sock6 >= 0) {
		listen(sock6, 32);
	}
	for (;;) {
		/* find out which socket we have data on */
		FD_ZERO(&sockets);
		if (sock4 >= 0) {
			FD_SET(sock4, &sockets);
		}
		if (sock6 >= 0) {
			FD_SET(sock6, &sockets);
		}
		if (select(32, &sockets, NULL, NULL, NULL) < 0) {
			(void) fprintf(stderr, "bad select call\n");
			continue;
		}
		sock = (sock4 >= 0 && FD_ISSET(sock4, &sockets)) ? sock4 : sock6;
		/* read data from socket */
		fromlen = sizeof(from);
		newsock = accept(sock, (struct sockaddr *) &from, &fromlen);
		cc = read(newsock, buf, sizeof(buf));
		/* parse the request */
		ok = 1;
		if (regexec(&get, buf, 10, opmatches, 0) != 0) {
			(void) fprintf(stderr, "not a valid get request\n");
			ok = 0;
		}
		if (ok && regexec(&opterm, buf, 10, opmatches, 0) != 0) {
			(void) fprintf(stderr, "no operation in request\n");
			ok = 0;
		}
		if (ok) {
			mr = (regexec(&machreadterm, buf, 3, mrmatch, 0) == 0);
		}
		if (ok && regexec(&searchterm, buf, 10, searchmatches, 0) != 0) {
			(void) fprintf(stderr, "no search term in request\n");
			ok = 0;
		}
		if (!ok) {
			(void) close(newsock);
			continue;
		}
		/* convert from %2f to / etc */
		n = frompercent(&buf[searchmatches[1].rm_so],
			(int)(searchmatches[1].rm_eo - searchmatches[1].rm_so),
			search,
			sizeof(search));
		code = HKP_NOT_FOUND;
		cc = 0;
		if (strncmp(&buf[opmatches[1].rm_so], "vindex", 6) == 0 ||
		    strncmp(&buf[opmatches[1].rm_so], "index", 5) == 0) {
			cc = 0;
			if ((cp = netpgp_get_key(netpgp, search, (mr) ? "mr" : "")) != NULL) {
				cc = strlen(cp);
				code = HKP_SUCCESS;
			}
			response(newsock, code, search, 0, cp, cc, mr);
		} else if (strncmp(&buf[opmatches[1].rm_so], "get", 3) == 0) {
			if ((cp = netpgp_export_key(netpgp, search)) != NULL) {
				cc = strlen(cp);
				code = HKP_SUCCESS;
			}
			response(newsock, code, search, 1, cp, cc, mr);
		}
		free(cp);
		(void) close(newsock);
	}
}
