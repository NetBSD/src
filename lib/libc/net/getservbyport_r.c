/*	$NetBSD: getservbyport_r.c,v 1.6.18.1 2011/04/05 06:21:19 riz Exp $	*/

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
static char sccsid[] = "@(#)getservbyport.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: getservbyport_r.c,v 1.6.18.1 2011/04/05 06:21:19 riz Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <db.h>

#include "servent.h"

#ifdef __weak_alias
__weak_alias(getservbyport_r,_getservbyport_r)
#endif

static struct servent *
_servent_getbyport(struct servent_data *sd, struct servent *sp, int port,
    const char *proto)
{
	if (sd->db == NULL)
		return NULL;

	if (sd->flags & _SV_DB) {
		char buf[BUFSIZ];
		DBT key, data;
		DB *db = sd->db;
		key.data = buf;

		port = htons(port);
		if (proto == NULL)
			key.size = snprintf(buf, sizeof(buf), "\377%d", port);
		else
			key.size = snprintf(buf, sizeof(buf), "\377%d/%s", port,
			    proto);
		key.size++;
		if (key.size > sizeof(buf))
			return NULL;
			
		if ((*db->get)(db, &key, &data, 0) != 0)
			return NULL;

		if ((*db->get)(db, &data, &key, 0) != 0)
			return NULL;

		if (sd->line)
			free(sd->line);

		sd->line = strdup(key.data);
		return _servent_parseline(sd, sp);
	} else {
		while (_servent_getline(sd) != -1) {
			if (_servent_parseline(sd, sp) == NULL)
				continue;
			if (sp->s_port != port)
				continue;
			if (proto == NULL || strcmp(sp->s_proto, proto) == 0)
				return sp;
		}
		return NULL;
	}
}

struct servent *
getservbyport_r(int port, const char *proto, struct servent *sp,
    struct servent_data *sd)
{
	setservent_r(sd->flags & _SV_STAYOPEN, sd);
	sp = _servent_getbyport(sd, sp, port, proto);
	if (!(sd->flags & _SV_STAYOPEN))
		_servent_close(sd);
	return sp;
}
