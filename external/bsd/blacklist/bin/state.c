/*	$NetBSD: state.c,v 1.11 2015/01/22 17:49:41 christos Exp $	*/

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: state.c,v 1.11 2015/01/22 17:49:41 christos Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <netinet/in.h>

#include "bl.h"
#include "internal.h"
#include "conf.h"
#include "state.h"

static HASHINFO openinfo = {
	4096,		/* bsize */
	32,		/* ffactor */
	256,		/* nelem */
	8 * 1024 * 1024,/* cachesize */
	NULL,		/* hash() */
	0		/* lorder */
};

int
state_close(DB *db)
{
	if (db == NULL)
		return -1;
	if ((*db->close)(db) == -1) {
		    (*lfun)(LOG_ERR, "%s: can't close db (%m)", __func__);
		    return -1;
	}
	return 0;
}

DB *
state_open(const char *dbname, int flags, mode_t perm)
{
	DB *db;

#ifdef __APPLE__
	flags &= O_CREAT|O_EXCL|O_EXLOCK|O_NONBLOCK|O_RDONLY|
	     O_RDWR|O_SHLOCK|O_TRUNC;
#endif
	db = dbopen(dbname, flags, perm, DB_HASH, &openinfo);
	if (db == NULL) {
		if (errno == ENOENT && (flags & O_CREAT) == 0)
			return NULL;
		(*lfun)(LOG_ERR, "%s: can't open `%s' (%m)", __func__, dbname);
	}
	return db;
}

struct dbkey {
	struct conf c;
	struct sockaddr_storage ss;
};

static void
dumpkey(const struct dbkey *k)
{
	char buf[10240];
	size_t z;
	int r;
	const unsigned char *p = (const void *)k;
	const unsigned char *e = p + sizeof(*k);
	r = snprintf(buf, sizeof(buf), "%s: ", __func__);
	if (r == -1 || (z = (size_t)r) >= sizeof(buf))
		z = sizeof(buf);
	while (p < e) {
		r = snprintf(buf + z, sizeof(buf) - z, "%.2x", *p++);
		if (r == -1 || (z += (size_t)r) >= sizeof(buf))
			z = sizeof(buf);
	}
	(*lfun)(LOG_DEBUG, "%s", buf);
}

static void
makekey(struct dbkey *k, const struct sockaddr_storage *ss,
    const struct conf *c)
{
	memset(k, 0, sizeof(*k));
	k->c = *c;
	k->ss = *ss;
	switch (k->ss.ss_family) {
	case AF_INET6:
		((struct sockaddr_in6 *)&k->ss)->sin6_port = htons(c->c_port);
		break;
	case AF_INET:
		((struct sockaddr_in *)&k->ss)->sin_port = htons(c->c_port);
		break;
	default:
		(*lfun)(LOG_ERR, "%s: bad family %d", __func__,
		    k->ss.ss_family);
		break;
	}
	if (debug > 1)
		dumpkey(k);
}

int
state_del(DB *db, const struct sockaddr_storage *ss, const struct conf *c)
{
	struct dbkey key;
	int rv;
	DBT k;

	if (db == NULL)
		return -1;

	makekey(&key, ss, c);

	k.data = &key;
	k.size = sizeof(key);

	switch (rv = (*db->del)(db, &k, 0)) {
	case 0:
	case 1:
		if (debug > 1)
			(*lfun)(LOG_DEBUG, "%s: returns %d", __func__, rv);
		return 0;
	default:
		(*lfun)(LOG_ERR, "%s: failed (%m)", __func__);
		return -1;
	}
}

int
state_get(DB *db, const struct sockaddr_storage *ss, const struct conf *c,
    struct dbinfo *dbi)
{
	struct dbkey key;
	int rv;
	DBT k, v;

	if (db == NULL)
		return -1;

	makekey(&key, ss, c);

	k.data = &key;
	k.size = sizeof(key);

	switch (rv = (*db->get)(db, &k, &v, 0)) {
	case 0:
	case 1:
		if (rv)
			memset(dbi, 0, sizeof(*dbi));
		else
			memcpy(dbi, v.data, sizeof(*dbi));
		if (debug > 1)
			(*lfun)(LOG_DEBUG, "%s: returns %d", __func__, rv);
		return 0;
	default:
		(*lfun)(LOG_ERR, "%s: failed (%m)", __func__);
		return -1;
	}
}

int
state_put(DB *db, const struct sockaddr_storage *ss, const struct conf *c,
    const struct dbinfo *dbi)
{
	struct dbkey key;
	int rv;
	DBT k, v;

	if (db == NULL)
		return -1;

	makekey(&key, ss, c);

	k.data = &key;
	k.size = sizeof(key);
	v.data = __UNCONST(dbi);
	v.size = sizeof(*dbi);

	switch (rv = (*db->put)(db, &k, &v, 0)) {
	case 0:
		if (debug > 1)
			(*lfun)(LOG_DEBUG, "%s: returns %d", __func__, rv);
		return 0;
	case 1:
		errno = EEXIST;
		/*FALLTHROUGH*/
	default:
		(*lfun)(LOG_ERR, "%s: failed (%m)", __func__);
		return -1;
	}
}

int
state_iterate(DB *db, struct sockaddr_storage *ss, struct conf *c,
    struct dbinfo *dbi, unsigned int first)
{
	struct dbkey *kp;
	int rv;
	DBT k, v;

	if (db == NULL)
		return -1;

	first = first ? R_FIRST : R_NEXT;

	switch (rv = (*db->seq)(db, &k, &v, first)) {
	case 0:
		kp = k.data;	
		*ss = kp->ss;
		*c = kp->c;
		if (debug > 2)
			dumpkey(kp);
		memcpy(dbi, v.data, sizeof(*dbi));
		if (debug > 1)
			(*lfun)(LOG_DEBUG, "%s: returns %d", __func__, rv);
		return 1;
	case 1:
		if (debug > 1)
			(*lfun)(LOG_DEBUG, "%s: returns %d", __func__, rv);
		return 0;
	default:
		(*lfun)(LOG_ERR, "%s: failed (%m)", __func__);
		return -1;
	}
}
