/*	$NetBSD: lcl_nw.c,v 1.1.1.1.14.1 2012/10/30 18:55:29 yamt Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1995
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

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static const char rcsid[] = "Id: lcl_nw.c,v 1.4 2005/04/27 04:56:31 sra Exp ";
/* from getgrent.c 8.2 (Berkeley) 3/21/94"; */
/* from BSDI Id: getgrent.c,v 2.8 1996/05/28 18:15:14 bostic Exp $	*/
#endif /* LIBC_SCCS and not lint */

/* Imports */

#include "port_before.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <errno.h>
#include <fcntl.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irs.h>
#include <isc/memcluster.h>

#include "port_after.h"

#include <isc/misc.h>
#include "irs_p.h"
#include "lcl_p.h"

#define MAXALIASES 35
#define MAXADDRSIZE 4

struct pvt {
	FILE *		fp;
	char 		line[BUFSIZ+1];
	struct nwent 	net;
	char *		aliases[MAXALIASES];
	char		addr[MAXADDRSIZE];
	struct __res_state *  res;
	void		(*free_res)(void *);
};

/* Forward */

static void 		nw_close(struct irs_nw *);
static struct nwent *	nw_byname(struct irs_nw *, const char *, int);
static struct nwent *	nw_byaddr(struct irs_nw *, void *, int, int);
static struct nwent *	nw_next(struct irs_nw *);
static void		nw_rewind(struct irs_nw *);
static void		nw_minimize(struct irs_nw *);
static struct __res_state * nw_res_get(struct irs_nw *this);
static void		nw_res_set(struct irs_nw *this,
				   struct __res_state *res,
				   void (*free_res)(void *));

static int		init(struct irs_nw *this);

/* Portability. */

#ifndef SEEK_SET
# define SEEK_SET 0
#endif

/* Public */

struct irs_nw *
irs_lcl_nw(struct irs_acc *this) {
	struct irs_nw *nw;
	struct pvt *pvt;

	UNUSED(this);

	if (!(pvt = memget(sizeof *pvt))) {
		errno = ENOMEM;
		return (NULL);
	}
	memset(pvt, 0, sizeof *pvt);
	if (!(nw = memget(sizeof *nw))) {
		memput(pvt, sizeof *pvt);
		errno = ENOMEM;
		return (NULL);
	}
	memset(nw, 0x5e, sizeof *nw);
	nw->private = pvt;
	nw->close = nw_close;
	nw->byname = nw_byname;
	nw->byaddr = nw_byaddr;
	nw->next = nw_next;
	nw->rewind = nw_rewind;
	nw->minimize = nw_minimize;
	nw->res_get = nw_res_get;
	nw->res_set = nw_res_set;
	return (nw);
}

/* Methods */

static void
nw_close(struct irs_nw *this) {
	struct pvt *pvt = (struct pvt *)this->private;
	
	nw_minimize(this);
	if (pvt->res && pvt->free_res)
		(*pvt->free_res)(pvt->res);
	if (pvt->fp)
		(void)fclose(pvt->fp);
	memput(pvt, sizeof *pvt);
	memput(this, sizeof *this);
}

static struct nwent *
nw_byaddr(struct irs_nw *this, void *net, int length, int type) {
	struct nwent *p;
	
	if (init(this) == -1)
		return(NULL);

	nw_rewind(this);
	while ((p = nw_next(this)) != NULL)
		if (p->n_addrtype == type && p->n_length == length)
			if (bitncmp(p->n_addr, net, length) == 0)
				break;
	return (p);
}

static struct nwent *
nw_byname(struct irs_nw *this, const char *name, int type) {
	struct nwent *p;
	char **ap;
	
	if (init(this) == -1)
		return(NULL);

	nw_rewind(this);
	while ((p = nw_next(this)) != NULL) {
		if (ns_samename(p->n_name, name) == 1 &&
		    p->n_addrtype == type)
			break;
		for (ap = p->n_aliases; *ap; ap++)
			if ((ns_samename(*ap, name) == 1) &&
			    (p->n_addrtype == type))
				goto found;
	}
 found:
	return (p);
}

static void
nw_rewind(struct irs_nw *this) {
	struct pvt *pvt = (struct pvt *)this->private;
	
	if (pvt->fp) {
		if (fseek(pvt->fp, 0L, SEEK_SET) == 0)
			return;
		(void)fclose(pvt->fp);
	}
	if (!(pvt->fp = fopen(_PATH_NETWORKS, "r")))
		return;
	if (fcntl(fileno(pvt->fp), F_SETFD, 1) < 0) {
		(void)fclose(pvt->fp);
		pvt->fp = NULL;
	}
}

static struct nwent *
nw_next(struct irs_nw *this) {
	struct pvt *pvt = (struct pvt *)this->private;
	struct nwent *ret = NULL;
	char *p, *cp, **q;
	char *bufp, *ndbuf, *dbuf = NULL;
	int c, bufsiz, offset = 0;

	if (init(this) == -1)
		return(NULL);

	if (pvt->fp == NULL)
		nw_rewind(this);
	if (pvt->fp == NULL) {
		RES_SET_H_ERRNO(pvt->res, NETDB_INTERNAL);
		return (NULL);
	}
	bufp = pvt->line;
	bufsiz = sizeof(pvt->line);

 again:
	p = fgets(bufp + offset, bufsiz - offset, pvt->fp);
	if (p == NULL)
		goto cleanup;
	if (!strchr(p, '\n') && !feof(pvt->fp)) {
#define GROWBUF 1024
		/* allocate space for longer line */
	  	if (dbuf == NULL) {
			if ((ndbuf = malloc(bufsiz + GROWBUF)) != NULL)
				strcpy(ndbuf, bufp);
		} else
			ndbuf = realloc(dbuf, bufsiz + GROWBUF);
		if (ndbuf) {
			dbuf = ndbuf;
			bufp = dbuf;
			bufsiz += GROWBUF;
			offset = strlen(dbuf);
		} else {
			/* allocation failed; skip this long line */
			while ((c = getc(pvt->fp)) != EOF)
				if (c == '\n')
					break;
			if (c != EOF)
				ungetc(c, pvt->fp);
		}
		goto again;
	}

	p -= offset;
	offset = 0;

	if (*p == '#')
		goto again;

	cp = strpbrk(p, "#\n");
	if (cp != NULL)
		*cp = '\0';
	pvt->net.n_name = p;
	cp = strpbrk(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = strpbrk(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	pvt->net.n_length = inet_net_pton(AF_INET, cp, pvt->addr,
					  sizeof pvt->addr);
	if (pvt->net.n_length < 0)
		goto again;
	pvt->net.n_addrtype = AF_INET;
	pvt->net.n_addr = pvt->addr;
	q = pvt->net.n_aliases = pvt->aliases;
	if (p != NULL) {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &pvt->aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = strpbrk(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	ret = &pvt->net;

 cleanup:
	if (dbuf)
		free(dbuf);

	return (ret);
}

static void
nw_minimize(struct irs_nw *this) {
	struct pvt *pvt = (struct pvt *)this->private;

	if (pvt->res)
		res_nclose(pvt->res);
	if (pvt->fp != NULL) {
		(void)fclose(pvt->fp);
		pvt->fp = NULL;
	}
}

static struct __res_state *
nw_res_get(struct irs_nw *this) {
	struct pvt *pvt = (struct pvt *)this->private;

	if (!pvt->res) {
		struct __res_state *res;
		res = (struct __res_state *)malloc(sizeof *res);
		if (!res) {
			errno = ENOMEM;
			return (NULL);
		}
		memset(res, 0, sizeof *res);
		nw_res_set(this, res, free);
	}

	return (pvt->res);
}

static void
nw_res_set(struct irs_nw *this, struct __res_state *res,
		void (*free_res)(void *)) {
	struct pvt *pvt = (struct pvt *)this->private;

	if (pvt->res && pvt->free_res) {
		res_nclose(pvt->res);
		(*pvt->free_res)(pvt->res);
	}

	pvt->res = res;
	pvt->free_res = free_res;
}

static int
init(struct irs_nw *this) {
	struct pvt *pvt = (struct pvt *)this->private;
	
	if (!pvt->res && !nw_res_get(this))
		return (-1);
	if (((pvt->res->options & RES_INIT) == 0U) &&
	    res_ninit(pvt->res) == -1)
		return (-1);
	return (0);
}

/*! \file */
