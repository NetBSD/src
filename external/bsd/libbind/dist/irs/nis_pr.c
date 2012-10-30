/*	$NetBSD: nis_pr.c,v 1.1.1.1.14.1 2012/10/30 18:55:30 yamt Exp $	*/

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996,1999 by Internet Software Consortium.
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
static const char rcsid[] = "Id: nis_pr.c,v 1.4 2005/04/27 04:56:33 sra Exp ";
#endif

/* Imports */

#include "port_before.h"

#ifndef WANT_IRS_NIS
static int __bind_irs_nis_unneeded;
#else

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#ifdef T_NULL
#undef T_NULL			/* Silence re-definition warning of T_NULL. */
#endif
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include <isc/memcluster.h>
#include <irs.h>

#include "port_after.h"

#include "irs_p.h"
#include "nis_p.h"

/* Definitions */

struct pvt {
	int		needrewind;
	char *		nis_domain;
	char *		curkey_data;
	int		curkey_len;
	char *		curval_data;
	int		curval_len;
	struct protoent	proto;
	char *		prbuf;
};

enum do_what { do_none = 0x0, do_key = 0x1, do_val = 0x2, do_all = 0x3 };

static /*const*/ char protocols_byname[] =	"protocols.byname";
static /*const*/ char protocols_bynumber[] =	"protocols.bynumber";

/* Forward */

static void 			pr_close(struct irs_pr *);
static struct protoent *	pr_byname(struct irs_pr *, const char *);
static struct protoent *	pr_bynumber(struct irs_pr *, int);
static struct protoent *	pr_next(struct irs_pr *);
static void			pr_rewind(struct irs_pr *);
static void			pr_minimize(struct irs_pr *);

static struct protoent *	makeprotoent(struct irs_pr *this);
static void			nisfree(struct pvt *, enum do_what);

/* Public */

struct irs_pr *
irs_nis_pr(struct irs_acc *this) {
	struct irs_pr *pr;
	struct pvt *pvt;

	if (!(pr = memget(sizeof *pr))) {
		errno = ENOMEM;
		return (NULL);
	}
	memset(pr, 0x5e, sizeof *pr);
	if (!(pvt = memget(sizeof *pvt))) {
		memput(pr, sizeof *pr);
		errno = ENOMEM;
		return (NULL);
	}
	memset(pvt, 0, sizeof *pvt);
	pvt->needrewind = 1;
	pvt->nis_domain = ((struct nis_p *)this->private)->domain;
	pr->private = pvt;
	pr->byname = pr_byname;
	pr->bynumber = pr_bynumber;
	pr->next = pr_next;
	pr->rewind = pr_rewind;
	pr->close = pr_close;
	pr->minimize = pr_minimize;
	pr->res_get = NULL;
	pr->res_set = NULL;
	return (pr);
}

/* Methods. */

static void
pr_close(struct irs_pr *this) {
	struct pvt *pvt = (struct pvt *)this->private;

	nisfree(pvt, do_all);
	if (pvt->proto.p_aliases)
		free(pvt->proto.p_aliases);
	if (pvt->prbuf)
		free(pvt->prbuf);
	memput(pvt, sizeof *pvt);
	memput(this, sizeof *this);
}

static struct protoent *
pr_byname(struct irs_pr *this, const char *name) {
	struct pvt *pvt = (struct pvt *)this->private;
	int r;
	char *tmp;
	
	nisfree(pvt, do_val);
	DE_CONST(name, tmp);
	r = yp_match(pvt->nis_domain, protocols_byname, tmp,
		     strlen(tmp), &pvt->curval_data, &pvt->curval_len);
	if (r != 0) {
		errno = ENOENT;
		return (NULL);
	}
	return (makeprotoent(this));
}

static struct protoent *
pr_bynumber(struct irs_pr *this, int num) {
	struct pvt *pvt = (struct pvt *)this->private;
	char tmp[sizeof "-4294967295"];
	int r;
	
	nisfree(pvt, do_val);
	(void) sprintf(tmp, "%d", num);
	r = yp_match(pvt->nis_domain, protocols_bynumber, tmp, strlen(tmp),
		     &pvt->curval_data, &pvt->curval_len);
	if (r != 0) {
		errno = ENOENT;
		return (NULL);
	}
	return (makeprotoent(this));
}

static struct protoent *
pr_next(struct irs_pr *this) {
	struct pvt *pvt = (struct pvt *)this->private;
	struct protoent *rval;
	int r;

	do {
		if (pvt->needrewind) {
			nisfree(pvt, do_all);
			r = yp_first(pvt->nis_domain, protocols_bynumber,
				     &pvt->curkey_data, &pvt->curkey_len,
				     &pvt->curval_data, &pvt->curval_len);
			pvt->needrewind = 0;
		} else {
			char *newkey_data;
			int newkey_len;

			nisfree(pvt, do_val);
			r = yp_next(pvt->nis_domain, protocols_bynumber,
				    pvt->curkey_data, pvt->curkey_len,
				    &newkey_data, &newkey_len,
				    &pvt->curval_data, &pvt->curval_len);
			nisfree(pvt, do_key);
			pvt->curkey_data = newkey_data;
			pvt->curkey_len = newkey_len;
		}
		if (r != 0) {
			errno = ENOENT;
			return (NULL);
		}
		rval = makeprotoent(this);
	} while (rval == NULL);
	return (rval);
}

static void
pr_rewind(struct irs_pr *this) {
	struct pvt *pvt = (struct pvt *)this->private;

	pvt->needrewind = 1;
}

static void
pr_minimize(struct irs_pr *this) {
	UNUSED(this);
	/* NOOP */
}

/* Private */

static struct protoent *
makeprotoent(struct irs_pr *this) {
	struct pvt *pvt = (struct pvt *)this->private;
	char *p, **t;
	int n, m;

	if (pvt->prbuf)
		free(pvt->prbuf);
	pvt->prbuf = pvt->curval_data;
	pvt->curval_data = NULL;

	for (p = pvt->prbuf; *p && *p != '#';)
		p++;
	while (p > pvt->prbuf && isspace((unsigned char)(p[-1])))
		p--;
	*p = '\0';

	p = pvt->prbuf;
	n = m = 0;

	pvt->proto.p_name = p;
	while (*p && !isspace((unsigned char)*p))
		p++;
	if (!*p)
		return (NULL);
	*p++ = '\0';

	while (*p && isspace((unsigned char)*p))
		p++;
	pvt->proto.p_proto = atoi(p);
	while (*p && !isspace((unsigned char)*p))
		p++;
	*p++ = '\0';

	while (*p) {
		if ((n + 1) >= m || !pvt->proto.p_aliases) {
			m += 10;
			t = realloc(pvt->proto.p_aliases,
				      m * sizeof(char *));
			if (!t) {
				errno = ENOMEM;
				goto cleanup;
			}
			pvt->proto.p_aliases = t;
		}
		pvt->proto.p_aliases[n++] = p;
		while (*p && !isspace((unsigned char)*p))
			p++;
		if (*p)
			*p++ = '\0';
	}
	if (!pvt->proto.p_aliases)
		pvt->proto.p_aliases = malloc(sizeof(char *));
	if (!pvt->proto.p_aliases)
		goto cleanup;
	pvt->proto.p_aliases[n] = NULL;
	return (&pvt->proto);
	
 cleanup:
	if (pvt->proto.p_aliases) {
		free(pvt->proto.p_aliases);
		pvt->proto.p_aliases = NULL;
	}
	if (pvt->prbuf) {
		free(pvt->prbuf);
		pvt->prbuf = NULL;
	}
	return (NULL);
}

static void
nisfree(struct pvt *pvt, enum do_what do_what) {
	if ((do_what & do_key) && pvt->curkey_data) {
		free(pvt->curkey_data);
		pvt->curkey_data = NULL;
	}
	if ((do_what & do_val) && pvt->curval_data) {
		free(pvt->curval_data);
		pvt->curval_data = NULL;
	}
}

#endif /*WANT_IRS_NIS*/

/*! \file */
