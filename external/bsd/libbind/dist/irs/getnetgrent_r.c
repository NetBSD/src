/*	$NetBSD: getnetgrent_r.c,v 1.1.1.1.14.1 2012/10/30 18:55:27 yamt Exp $	*/

/*
 * Copyright (C) 2004, 2005, 2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998, 1999, 2001, 2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static const char rcsid[] = "Id: getnetgrent_r.c,v 1.14 2008/11/14 02:36:51 marka Exp ";
#endif /* LIBC_SCCS and not lint */

#include <port_before.h>
#if !defined(_REENTRANT) || !defined(DO_PTHREADS)
	static int getnetgrent_r_not_required = 0;
#else
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <port_after.h>

#ifdef NGR_R_RETURN
#ifndef NGR_R_PRIVATE
#define NGR_R_PRIVATE 0
#endif

static NGR_R_RETURN
copy_protoent(NGR_R_CONST char **, NGR_R_CONST char **, NGR_R_CONST char **,
	      const char *, const char *, const char *, NGR_R_COPY_ARGS);

NGR_R_RETURN
innetgr_r(const char *netgroup, const char *host, const char *user,
	  const char *domain) {
	char *ng, *ho, *us, *dom;

	DE_CONST(netgroup, ng);
	DE_CONST(host, ho);
	DE_CONST(user, us);
	DE_CONST(domain, dom);

	return (innetgr(ng, ho, us, dom));
}

/*%
 *	These assume a single context is in operation per thread.
 *	If this is not the case we will need to call irs directly
 *	rather than through the base functions.
 */

NGR_R_RETURN
getnetgrent_r(NGR_R_CONST char **machinep, NGR_R_CONST char **userp,
	      NGR_R_CONST char **domainp, NGR_R_ARGS)
{
	NGR_R_CONST char *mp, *up, *dp;
	int res = getnetgrent(&mp, &up, &dp);

	if (res != 1)
		return (res);

	return (copy_protoent(machinep, userp, domainp,
				mp, up, dp, NGR_R_COPY));
}

#if NGR_R_PRIVATE == 2
struct private {
	char *buf;
};

#endif
NGR_R_SET_RETURN
#ifdef NGR_R_SET_ARGS
setnetgrent_r(NGR_R_SET_CONST char *netgroup, NGR_R_SET_ARGS)
#else
setnetgrent_r(NGR_R_SET_CONST char *netgroup)
#endif
{
#if NGR_R_PRIVATE == 2
	struct private *p;
#endif
	char *tmp;
#if defined(NGR_R_SET_ARGS) && NGR_R_PRIVATE == 0
	UNUSED(buf);
	UNUSED(buflen);
#endif

	DE_CONST(netgroup, tmp);
	setnetgrent(tmp);

#if NGR_R_PRIVATE == 1
	*buf = NULL;
#elif NGR_R_PRIVATE == 2
	*buf = p = malloc(sizeof(struct private));
	if (p == NULL)
#ifdef NGR_R_SET_RESULT
		return (NGR_R_BAD);
#else
		return;
#endif
	p->buf = NULL;
#endif
#ifdef NGR_R_SET_RESULT
	return (NGR_R_SET_RESULT);
#endif
}

NGR_R_END_RETURN
#ifdef NGR_R_END_ARGS
endnetgrent_r(NGR_R_END_ARGS)
#else
endnetgrent_r(void)
#endif
{
#if NGR_R_PRIVATE == 2
	struct private *p = buf;
#endif
#if defined(NGR_R_SET_ARGS) && NGR_R_PRIVATE == 0
	UNUSED(buf);
	UNUSED(buflen);
#endif

	endnetgrent();
#if NGR_R_PRIVATE == 1
	if (*buf != NULL)
		free(*buf);
	*buf = NULL;
#elif NGR_R_PRIVATE == 2
	if (p->buf != NULL)
		free(p->buf);
	free(p);
#endif
	NGR_R_END_RESULT(NGR_R_OK);
}

/* Private */

static int
copy_protoent(NGR_R_CONST char **machinep, NGR_R_CONST char **userp,
	      NGR_R_CONST char **domainp, const char *mp, const char *up,
	      const char *dp, NGR_R_COPY_ARGS)
{
#if NGR_R_PRIVATE == 2
	struct private *p = buf;
#endif
	char *cp;
	int n;
	int len;

	/* Find out the amount of space required to store the answer. */
	len = 0;
	if (mp != NULL) len += strlen(mp) + 1;
	if (up != NULL) len += strlen(up) + 1;
	if (dp != NULL) len += strlen(dp) + 1;

#if NGR_R_PRIVATE == 1
	if (*buf != NULL)
		free(*buf);
	*buf = malloc(len);
	if (*buf == NULL)
		return(NGR_R_BAD);
	cp = *buf;
#elif NGR_R_PRIVATE == 2
	if (p->buf)
		free(p->buf);
	p->buf = malloc(len);
	if (p->buf == NULL)
		return(NGR_R_BAD);
	cp = p->buf;
#else
	if (len > (int)buflen) {
		errno = ERANGE;
		return (NGR_R_BAD);
	}
	cp = buf;
#endif

	if (mp != NULL) {
		n = strlen(mp) + 1;
		strcpy(cp, mp);
		*machinep = cp;
		cp += n;
	} else
		*machinep = NULL;

	if (up != NULL) {
		n = strlen(up) + 1;
		strcpy(cp, up);
		*userp = cp;
		cp += n;
	} else
		*userp = NULL;

	if (dp != NULL) {
		n = strlen(dp) + 1;
		strcpy(cp, dp);
		*domainp = cp;
		cp += n;
	} else
		*domainp = NULL;

	return (NGR_R_OK);
}
#else /* NGR_R_RETURN */
	static int getnetgrent_r_unknown_system = 0;
#endif /* NGR_R_RETURN */
#endif /* !defined(_REENTRANT) || !defined(DO_PTHREADS) */
/*! \file */
