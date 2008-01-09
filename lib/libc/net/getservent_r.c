/*	$NetBSD: getservent_r.c,v 1.8.8.1 2008/01/09 01:34:12 matt Exp $	*/

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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getservent.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: getservent_r.c,v 1.8.8.1 2008/01/09 01:34:12 matt Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <db.h>

#include "servent.h"

#ifdef __weak_alias
__weak_alias(endservent_r,_endservent_r)
__weak_alias(getservent_r,_getservent_r)
__weak_alias(setservent_r,_setservent_r)
#endif

int
_servent_open(struct servent_data *sd)
{
	sd->flags |= _SV_FIRST;
	if (sd->db == NULL) {
		if ((sd->db = dbopen(_PATH_SERVICES_DB, O_RDONLY, 0,
		    DB_HASH, NULL)) != NULL)
			sd->flags |= _SV_DB;
		else 
			sd->db = fopen(_PATH_SERVICES, "r");
	}
	return sd->db ? 0 : -1;
}

void
_servent_close(struct servent_data *sd)
{
	if (sd->db) {
		if (sd->flags & _SV_DB) {
			DB *db = sd->db;
			(*db->close)(db);
		} else
			(void)fclose((FILE *)sd->db);
		sd->db = NULL;
	}
	sd->flags &= ~_SV_STAYOPEN;
}


int
_servent_getline(struct servent_data *sd)
{
	if (sd->line) {
		free(sd->line);
		sd->line = NULL;
	}

	if (sd->db == NULL)
		return -1;

	if (sd->flags & _SV_DB) {
		DB *db = sd->db;
		DBT key, data;
		u_int flags  = (sd->flags & _SV_FIRST) ? R_FIRST : R_NEXT;

		while ((*db->seq)(db, &key, &data, flags) == 0) {
			flags = R_NEXT;
			switch (((u_char *)key.data)[0]) {
			case (u_char)'\377':
			case (u_char)'\376':
				continue;
			default:
				break;
			}
			sd->line = strdup(data.data);
			break;
		}
	} else {
		if (sd->flags & _SV_FIRST)
			(void)rewind((FILE *)sd->db);
		sd->line = fparseln((FILE *)sd->db, NULL, NULL, NULL,
		    FPARSELN_UNESCALL);
	}
	sd->flags &= ~_SV_FIRST;
	return sd->line == NULL ? -1 : 0;
}


struct servent *
_servent_parseline(struct servent_data *sd, struct servent *sp)
{
	size_t i = 0;
	int oerrno;
	char *p, *cp, **q;

	if (sd->line == NULL)
		return NULL;

	sp->s_name = p = sd->line;
	p = strpbrk(p, " \t");
	if (p == NULL)
		return NULL;
	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	cp = strpbrk(p, ",/");
	if (cp == NULL)
		return NULL;
	*cp++ = '\0';
	sp->s_port = htons((u_short)atoi(p));
	sp->s_proto = cp;
	if (sd->aliases == NULL) {
		sd->maxaliases = 10;
		sd->aliases = malloc(sd->maxaliases * sizeof(char *));
		if (sd->aliases == NULL) {
			oerrno = errno;
			endservent_r(sd);
			errno = oerrno;
			return NULL;
		}
	}
	q = sp->s_aliases = sd->aliases;
	cp = strpbrk(cp, " \t");
	if (cp != NULL)
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (i == sd->maxaliases - 2) {
			sd->maxaliases *= 2;
			q = realloc(q,
			    sd->maxaliases * sizeof(char *));
			if (q == NULL) {
				oerrno = errno;
				endservent_r(sd);
				errno = oerrno;
				return NULL;
			}
			sp->s_aliases = sd->aliases = q;
		}
		q[i++] = cp;
		cp = strpbrk(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	q[i] = NULL;
	return sp;
}

void
setservent_r(int f, struct servent_data *sd)
{
	(void)_servent_open(sd);
	sd->flags |= f ? _SV_STAYOPEN : 0;
}

void
endservent_r(struct servent_data *sd)
{
	_servent_close(sd);
	if (sd->aliases) {
		free(sd->aliases);
		sd->aliases = NULL;
		sd->maxaliases = 0;
	}
	if (sd->line) {
		free(sd->line);
		sd->line = NULL;
	}
}

struct servent *
getservent_r(struct servent *sp, struct servent_data *sd)
{
	if (sd->db == NULL && _servent_open(sd) == -1)
		return NULL;

	for (;;) {
		if (_servent_getline(sd) == -1)
			return NULL;
		if (_servent_parseline(sd, sp) == NULL)
			continue;
		return sp;
	}
}
