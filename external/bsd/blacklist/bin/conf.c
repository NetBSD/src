/*	$NetBSD: conf.c,v 1.1 2015/01/19 18:52:55 christos Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: conf.c,v 1.1 2015/01/19 18:52:55 christos Exp $");

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <netdb.h>
#include <pwd.h>
#include <syslog.h>
#include <util.h>
#include <stdlib.h>
#include <limits.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "bl.h"
#include "internal.h"
#include "conf.h"

static void
advance(char **p)
{
	char *ep = *p;
	while (*ep && !isspace((unsigned char)*ep))
		ep++;
	while (*ep && isspace((unsigned char)*ep))
		*ep++ = '\0';
	*p = ep;
}

static int
getnum(const char *f, size_t l, int *r, const char *p)
{
	int e;
	intmax_t im;

	im = strtoi(p, NULL, 0, 0, INT_MAX, &e);
	if (e == 0) {
		*r = (int)im;
		return 0;
	}

	if (f == NULL)
		return -1;
	(*lfun)(LOG_ERR, "%s: %s, %zu: Bad number [%s]", __func__, f, l, p);
	return -1;

}

static int
getport(const char *f, size_t l, int *r, const char *p)
{
	struct servent *sv;

	// XXX: Pass in the proto instead
	if ((sv = getservbyname(p, "tcp")) != NULL) {
		*r = ntohs(sv->s_port);
		return 0;
	}
	if ((sv = getservbyname(p, "udp")) != NULL) {
		*r = ntohs(sv->s_port);
		return 0;
	}

	if (getnum(NULL, 0, r, p) == 0)
		return 0;

	(*lfun)(LOG_ERR, "%s: %s, %zu: Bad service [%s]", __func__, f, l, p);
	return -1;
}

static int
getproto(const char *f, size_t l, int *r, const char *p)
{
	if (strcmp(p, "stream") == 0) {
		*r = IPPROTO_TCP;
		return 0;
	}
	if (strcmp(p, "dgram") == 0) {
		*r = IPPROTO_UDP;
		return 0;
	}
	if (getnum(NULL, 0, r, p) == 0)
		return 0;

	(*lfun)(LOG_ERR, "%s: %s, %zu: Bad protocol [%s]", __func__, f, l, p);
	return -1;
}

static int
getfamily(const char *f, size_t l, int *r, const char *p)
{
	if (strncmp(p, "tcp", 3) == 0 || strncmp(p, "udp", 3) == 0) {
		*r = p[3] == '6' ? AF_INET6 : AF_INET;
		return 0;
	}
	if (getnum(NULL, 0, r, p) == 0)
		return 0;

	(*lfun)(LOG_ERR, "%s: %s, %zu: Bad family [%s]", __func__, f, l, p);
	return -1;
}

static int
getuid(const char *f, size_t l, int *r, const char *p)
{
	struct passwd *pw;

	if ((pw = getpwnam(p)) != NULL) {
		*r = pw->pw_uid;
		return 0;
	}

	if (getnum(NULL, 0, r, p) == 0)
		return 0;

	(*lfun)(LOG_ERR, "%s: %s, %zu: Bad user [%s]", __func__, f, l, p);
	return -1;
}

static int
getvalue(const char *f, size_t l, int *r, char **p,
    int (*fun)(const char *, size_t, int *, const char *))
{
	char *ep = *p;
	int e;

	advance(p);
	if (*ep == '*') {
		*r = -1;
		return 0;
	}
	return (*fun)(f, l, r, ep);
}


static int
parseconfline(const char *f, size_t l, char *p, struct conf *c)
{
	int e;

	while (*p && isspace((unsigned char)*p))
		p++;

	e = getvalue(f, l, &c->c_port, &p, getport);
	if (e) return -1;
	e = getvalue(f, l, &c->c_proto, &p, getproto);
	if (e) return -1;
	e = getvalue(f, l, &c->c_family, &p, getfamily);
	if (e) return -1;
	e = getvalue(f, l, &c->c_uid, &p, getuid);
	if (e) return -1;
	e = getvalue(f, l, &c->c_nfail, &p, getnum);
	if (e) return -1;
	e = getvalue(f, l, &c->c_duration, &p, getnum);
	if (e) return -1;

	return 0;
}

static int
sortconf(const void *v1, const void *v2)
{
	const struct conf *c1 = v1;
	const struct conf *c2 = v2;
#define CMP(a, b, f) \
	if ((a)->f > (b)->f) return -1; \
	else if ((a)->f < (b)->f) return 1

	CMP(c1, c2, c_port);
	CMP(c1, c2, c_proto);
	CMP(c1, c2, c_family);
	CMP(c1, c2, c_uid);
	return 0;
}

static void
printconf(const char *pref, const struct conf *c)
{
	printf("%s%d\t%d\t%d\t%d\t%d\t%d\n", pref,
	    c->c_port, c->c_proto, c->c_family,
	    c->c_uid, c->c_nfail, c->c_duration);
}

const struct conf *
findconf(bl_info_t *bi)
{
	int lfd;
	int proto;
	socklen_t slen;
	struct sockaddr_storage ss;
	struct conf c;
	size_t i;

	lfd = bi->bi_fd[0];
	slen = sizeof(ss);
	if (getsockname(lfd, (void *)&ss, &slen) == -1) {
		(*lfun)(LOG_ERR, "getsockname failed (%m)"); 
		return NULL;
	}

	slen = sizeof(proto);
	if (getsockopt(lfd, SOL_SOCKET, SO_TYPE, &proto, &slen) == -1) {
		(*lfun)(LOG_ERR, "getsockopt failed (%m)"); 
		return NULL;
	}

	switch (proto) {
	case SOCK_STREAM:
		c.c_proto = IPPROTO_TCP;
		break;
	case SOCK_DGRAM:
		c.c_proto = IPPROTO_UDP;
		break;
	default:
		(*lfun)(LOG_ERR, "unsupported protocol %d", proto); 
		return NULL;
	}

	switch (ss.ss_family) {
	case AF_INET:
		c.c_port = ntohs(((struct sockaddr_in *)&ss)->sin_port);
		break;
	case AF_INET6:
		c.c_port = ntohs(((struct sockaddr_in6 *)&ss)->sin6_port);
		break;
	default:
		(*lfun)(LOG_ERR, "unsupported family %d", ss.ss_family); 
		return NULL;
	}

	c.c_uid = bi->bi_cred->sc_euid;
	c.c_family = ss.ss_family;
	c.c_nfail = -1;
	c.c_duration = -1;

	if (debug)
		printconf("look:\t", &c);

	for (i = 0; i < nconf; i++) {
		if (debug)
			printconf("check:\t", &conf[i]);
		if (sortconf(&c, &conf[i]) == 0) {
			if (debug)
				printconf("found: ", &conf[i]);
			return &conf[i];
		}
	}
	if (debug)
		printf("not found\n");
	return NULL;
}


void
parseconf(const char *f)
{
	FILE *fp;
	char *line;
	size_t lineno, len, nc, mc;
	struct conf *c, *tc;

	if ((fp = fopen(f, "r")) == NULL) {
		(*lfun)(LOG_ERR, "%s: Cannot open `%s' (%m)", __func__, f);
		return;
	}

	lineno = 1;
	nc = mc = 0;
	c = NULL;
	for (; (line = fparseln(fp, &len, &lineno, NULL, 0)) != NULL;
	    free(line))
	{
		if (nc == mc) {
			mc += 10;
			tc = realloc(c, mc * sizeof(*c));
			if (tc == NULL) {
				free(c);
				fclose(fp);
				return;
			}
			c = tc;
		}
		if (parseconfline(f, lineno, line, &c[nc]) == -1)
			continue;
		nc++;
	}
	fclose(fp);
	qsort(c, nc, sizeof(*c), sortconf);
	
	tc = conf;
	nconf = nc;
	conf = c;
	free(tc);

	if (debug) {
		printf("port\ttype\tproto\towner\tnfail\tduration\n");
		for (nc = 0; nc < nconf; nc++)
			printconf("", &c[nc]);
	}
}
