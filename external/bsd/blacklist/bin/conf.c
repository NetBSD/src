/*	$NetBSD: conf.c,v 1.3 2015/01/21 16:16:00 christos Exp $	*/

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
__RCSID("$NetBSD: conf.c,v 1.3 2015/01/21 16:16:00 christos Exp $");

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <netdb.h>
#include <pwd.h>
#include <syslog.h>
#include <errno.h>
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
getsecs(const char *f, size_t l, int *r, const char *p)
{
	int e;
	char *ep;
	intmax_t tot, im;

	tot = 0;
again:
	im = strtoi(p, &ep, 0, 0, INT_MAX, &e);

	if (e == ENOTSUP) {
		switch (*ep) {
		case 'd':
			im *= 24;
			/*FALLTHROUGH*/
		case 'h':
			im *= 60;
			/*FALLTHROUGH*/
		case 'm':
			im *= 60;
			/*FALLTHROUGH*/
		case 's':
			e = 0;
			tot += im;
			if (ep[1] != '\0') {
				p = ep + 2;
				goto again;
			}
			break;
		}
	} else	
		tot = im;
			
	if (e == 0) {
		*r = (int)tot;
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
		*r = (int)pw->pw_uid;
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

	advance(p);
	if (*ep == '*') {
		*r = -1;
		return 0;
	}
	return (*fun)(f, l, r, ep);
}


static int
conf_parseline(const char *f, size_t l, char *p, struct conf *c)
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
	e = getvalue(f, l, &c->c_duration, &p, getsecs);
	if (e) return -1;

	return 0;
}

static int
conf_sort(const void *v1, const void *v2)
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
#undef CMP
	return 0;
}

static int
conf_eq(const struct conf *c1, const struct conf *c2)
{
#define CMP(a, b, f) \
	if ((a)->f != (b)->f && (b)->f != -1) return 0;
	CMP(c1, c2, c_port);
	CMP(c1, c2, c_proto);
	CMP(c1, c2, c_family);
	CMP(c1, c2, c_uid);
#undef CMP
	return 1;
}

const char *
conf_print(char *buf, size_t len, const char *pref, const char *delim,
    const struct conf *c)
{
	snprintf(buf, len, "%s%d%s%d%s%d%s%d%s%d%s%d", pref,
	    c->c_port, delim, c->c_proto, delim, c->c_family, delim,
	    c->c_uid, delim, c->c_nfail, delim, c->c_duration);
	return buf;
}

const struct conf *
conf_find(int fd, uid_t uid, struct conf *cr)
{
	int proto;
	socklen_t slen;
	struct sockaddr_storage ss;
	size_t i;
	char buf[BUFSIZ];

	slen = sizeof(ss);
	memset(&ss, 0, slen);
	if (getsockname(fd, (void *)&ss, &slen) == -1) {
		(*lfun)(LOG_ERR, "getsockname failed (%m)"); 
		return NULL;
	}

	slen = sizeof(proto);
	if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &proto, &slen) == -1) {
		(*lfun)(LOG_ERR, "getsockopt failed (%m)"); 
		return NULL;
	}

	switch (proto) {
	case SOCK_STREAM:
		cr->c_proto = IPPROTO_TCP;
		break;
	case SOCK_DGRAM:
		cr->c_proto = IPPROTO_UDP;
		break;
	default:
		(*lfun)(LOG_ERR, "unsupported protocol %d", proto); 
		return NULL;
	}

	switch (ss.ss_family) {
	case AF_INET:
		cr->c_port = ntohs(((struct sockaddr_in *)&ss)->sin_port);
		break;
	case AF_INET6:
		cr->c_port = ntohs(((struct sockaddr_in6 *)&ss)->sin6_port);
		break;
	default:
		(*lfun)(LOG_ERR, "unsupported family %d", ss.ss_family); 
		return NULL;
	}

	cr->c_uid = (int)uid;
	cr->c_family = ss.ss_family;
	cr->c_nfail = -1;
	cr->c_duration = -1;

	if (debug)
		printf("%s\n", conf_print(buf, sizeof(buf),
		    "look:\t", "\t", cr));

	for (i = 0; i < nconf; i++) {
		if (debug)
			printf("%s\n", conf_print(buf, sizeof(buf), "check:\t",
			    "\t", &conf[i]));
		if (conf_eq(cr, &conf[i])) {
			if (debug)
				printf("%s\n", conf_print(buf, sizeof(buf),
				    "found:\t", "\t", &conf[i]));
			cr->c_nfail = conf[i].c_nfail;
			cr->c_duration = conf[i].c_duration;
			return cr;
		}
	}
	if (debug)
		printf("not found\n");
	return NULL;
}


void
conf_parse(const char *f)
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
		if (conf_parseline(f, lineno, line, &c[nc]) == -1)
			continue;
		nc++;
	}
	fclose(fp);
	qsort(c, nc, sizeof(*c), conf_sort);
	
	tc = conf;
	nconf = nc;
	conf = c;
	free(tc);

	if (debug) {
		char buf[BUFSIZ];
		printf("port\ttype\tproto\towner\tnfail\tduration\n");
		for (nc = 0; nc < nconf; nc++)
			printf("%s\n",
			    conf_print(buf, sizeof(buf), "", "\t", &c[nc]));
	}
}
