/*	$NetBSD: iconv.c,v 1.1.2.2 2001/01/08 14:58:04 bouyer Exp $	*/

/*
 * Copyright (c) 2000, Boris Popov
 * All rights reserved.
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
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#ifndef NetBSD
#include <sys/kobj.h>
#endif
#include <sys/queue.h>
#include <sys/errno.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>

#ifndef NetBSD
#include "iconv_drv_if.h"
#endif

#include "iconv.h"

#ifdef SYSCTL_DECL
SYSCTL_DECL(_kern_iconv);
#endif

#ifndef NetBSD
SYSCTL_NODE(_kern, OID_AUTO, iconv, CTLFLAG_RW, NULL, "kernel iconv interface");

MALLOC_DEFINE(M_ICONV, "ICONV data", "ICONV data");
#endif

#ifdef MODULE_VERSION
MODULE_VERSION(libiconv, 1);
#endif

#ifndef NetBSD
struct sysctl_oid *iconv_oid_hook = &sysctl___kern_iconv;
static struct iconv_drv_list iconv_drivers;
#endif


#ifndef NetBSD
static int
iconv_mod_handler(module_t mod, int type, void *data)
{
	int error;

printf("%s\n", __FUNCTION__);
	switch (type) {
	    case MOD_LOAD:
		TAILQ_INIT(&iconv_drivers);
		error = 0;
		break;
	    case MOD_UNLOAD:
		error = 0;
		break;
	    default:
		error = EINVAL;
	}
	return error;
}

static moduledata_t iconv_mod = {
	"iconv", iconv_mod_handler, NULL
};

DECLARE_MODULE(iconv, iconv_mod, SI_SUB_DRIVERS, SI_ORDER_SECOND);

static int
iconv_add_driver(struct iconv_drv_class *dcp)
{
	kobj_class_compile((struct kobj_class*)dcp);
	dcp->refs++;
	TAILQ_INSERT_TAIL(&iconv_drivers, dcp, dc_link);
	return 0;
}

static int
iconv_rm_driver(struct iconv_drv_class *dcp)
{
	TAILQ_REMOVE(&iconv_drivers, dcp, dc_link);
	kobj_class_free((struct kobj_class*)dcp);
	return 0;
}

int
iconv_drv_initstub(struct iconv_drv_class *dp)
{
	return 0;
}

int
iconv_drv_donestub(struct iconv_drv_class *dp)
{
	return 0;
}

int
iconv_drvmod_handler(module_t mod, int type, void *data)
{
	struct iconv_drv_class *dcp = data;
	int error;

	switch (type) {
	    case MOD_LOAD:
		error = iconv_add_driver(dcp);
		if (error)
			break;
		error = ICONV_DRV_INIT(dcp);
		if (error)
			iconv_rm_driver(dcp);
		break;
	    case MOD_UNLOAD:
		error = ICONV_DRV_DONE(dcp);
		if (!error)
			error = iconv_rm_driver(dcp);
		break;
	    default:
		error = EINVAL;
	}
	return error;
}
#endif

int
iconv_open(const char *to, const char *from, struct iconv_drv **dpp)
{
#ifndef NetBSD
	struct iconv_drv_class *dcp;
	int error;

	TAILQ_FOREACH(dcp, &iconv_drivers, dc_link) {
		error = ICONV_DRV_OPEN(dcp, to, from, dpp);
		if (error == 0)
			return 0;
		if (error != ENOENT)
			return error;
	}
	return ENOENT;
#else
	dpp = NULL;
	return 0;
#endif
}

int
iconv_close(struct iconv_drv *dp)
{
#ifndef NetBSD
	return ICONV_DRV_CLOSE(dp);
#else
	return 0;
#endif
}

int
iconv_conv(struct iconv_drv *dp, const char **inbuf,
	size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
#ifndef NetBSD
	return ICONV_DRV_CONV(dp, inbuf, inbytesleft, outbuf, outbytesleft);
#else
	return 1;
#endif
}

char *
iconv_convstr(struct iconv_drv *dp, char *dst, const char *src)
{
	char *p = dst;
	int inlen, outlen, error;

	if (dp == NULL) {
		strcpy(dst, src);
		return dst;
	}
	inlen = outlen = strlen(src);
	error = iconv_conv(dp, NULL, NULL, &p, &outlen);
	if (error)
		return NULL;
	error = iconv_conv(dp, &src, &inlen, &p, &outlen);
	if (error)
		return NULL;
	*p = 0;
	return dst;
}

void *
iconv_convmem(struct iconv_drv *dp, void *dst, const void *src, int size)
{
	const char *s = src;
	char *d = dst;
	int inlen, outlen, error;

	if (size == 0)
		return dst;
	if (dp == NULL) {
		memcpy(dst, src, size);
		return dst;
	}
	inlen = outlen = size;
	error = iconv_conv(dp, NULL, NULL, &d, &outlen);
	if (error)
		return NULL;
	error = iconv_conv(dp, &s, &inlen, &d, &outlen);
	if (error)
		return NULL;
	return dst;
}

int
iconv_lookupcp(const char **cpp, const char *s)
{
	for (; *cpp; cpp++)
		if (strcmp(*cpp, s) == 0)
			return 0;
	return ENOENT;
}

#ifndef NetBSD
/*
 * Built-in "XLAT" driver
 */
struct iconv_drv_xlat {
	struct iconv_drv *	d_static;
	u_char *		d_table;
};

static TAILQ_HEAD(, iconv_xlat_tbl) iconv_xlat_tables;

int
iconv_xlat_add_table(struct iconv_xlat_tbl *xp)
{
	TAILQ_INSERT_TAIL(&iconv_xlat_tables, xp, x_link);
	return 0;
}

int
iconv_xlat_rm_table(struct iconv_xlat_tbl *xp)
{
	TAILQ_REMOVE(&iconv_xlat_tables, xp, x_link);
	return 0;
}

int
iconv_xlatmod_handler(module_t mod, int type, void *data)
{
	struct iconv_xlat_tbl *xp = data;
	int error;

	switch (type) {
	    case MOD_LOAD:
		error = iconv_xlat_add_table(xp);
		break;
	    case MOD_UNLOAD:
		error = iconv_xlat_rm_table(xp);
		break;
	    default:
		error = EINVAL;
	}
	return error;
}

static int
iconv_xlat_lookup(const char *to, const char *from, struct iconv_xlat_tbl **xpp)
{
	struct iconv_xlat_tbl *xp;

	TAILQ_FOREACH(xp, &iconv_xlat_tables, x_link) {
		if (iconv_lookupcp(xp->x_from, from) == 0 &&
		    iconv_lookupcp(xp->x_to, to) == 0) {
			*xpp = xp;
			return 0;
		}
	}
	return ENOENT;
}

static int
iconv_xlat_open(struct iconv_drv_class *dcp, const char *to,
	const char *from, struct iconv_drv **dpp)
{
	struct iconv_xlat_tbl *xp;
	struct iconv_drv_xlat *dp;
	int error;

	error = iconv_xlat_lookup(to, from, &xp);
	if (error)
		return error;
	dp = (struct iconv_drv_xlat *)kobj_create((struct kobj_class*)dcp, M_ICONV, M_WAITOK);
	if (dp == NULL)
		return ENOENT;
	dp->d_table = xp->x_table;
	*dpp = (struct iconv_drv*)dp;
	return 0;
}

static int
iconv_xlat_close(struct iconv_drv *dp)
{
	kobj_delete((struct kobj*)dp, M_ICONV);
	return 0;
}

static int
iconv_xlat_conv(struct iconv_drv *d2p, const char **inbuf,
	size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
	struct iconv_drv_xlat *dp = (struct iconv_drv_xlat*)d2p;
	const char *src;
	char *dst;
	int n, r;

	if (inbuf == NULL || *inbuf == NULL || outbuf == NULL || *outbuf == NULL)
		return 0;
	r = n = min(*inbytesleft, *outbytesleft);
	src = *inbuf;
	dst = *outbuf;
	while(r--)
		*dst++ = dp->d_table[(u_char)*src++];
	*inbuf += n;
	*outbuf += n;
	*inbytesleft += n;
	*outbytesleft += n;
	return 0;
}

static int
iconv_xlat_init(struct iconv_drv_class *dcp)
{
	TAILQ_INIT(&iconv_xlat_tables);
	return 0;
}

static int
iconv_xlat_done(struct iconv_drv_class *dcp)
{
	/*
	 * TODO: free memory
	 */
	return 0;
}

static kobj_method_t iconv_xlat_methods[] = {
	KOBJMETHOD(iconv_drv_open,	iconv_xlat_open),
	KOBJMETHOD(iconv_drv_close,	iconv_xlat_close),
	KOBJMETHOD(iconv_drv_conv,	iconv_xlat_conv),
	KOBJMETHOD(iconv_drv_init,	iconv_xlat_init),
	KOBJMETHOD(iconv_drv_done,	iconv_xlat_done),
	{0, 0}
};

ICONV_DRIVER(xlat, sizeof(struct iconv_drv_xlat));

/*
 * sysctl interface to xlat driver
 */
static int
iconv_xlat_outlist(struct sysctl_req *req, const char **cpp)
{
	char comm = ',';
	int len, error = 0;

	for (; *cpp; cpp++) {
		len = strlen(*cpp);
		error = SYSCTL_OUT(req, *cpp, len);
		if (error)
			break;
		if (cpp[1]) {
			error = SYSCTL_OUT(req, &comm, 1);
			if (error)
				break;
		}
	}
	return error;
}

static int
#if __FreeBSD_version > 500000
iconv_xlat_sysctl_list(SYSCTL_HANDLER_ARGS)
#else
iconv_xlat_sysctl_list SYSCTL_HANDLER_ARGS
#endif
{
	struct iconv_xlat_tbl *xp;
	char spc = ' ', smc = ';';
	int error;

	error = 0;
	TAILQ_FOREACH(xp, &iconv_xlat_tables, x_link) {
		error = iconv_xlat_outlist(req, xp->x_from);
		if (error)
			break;
		error = SYSCTL_OUT(req, &spc, 1);
		if (error)
			break;
		error = iconv_xlat_outlist(req, xp->x_to);
		if (error)
			break;
		error = SYSCTL_OUT(req, &smc, 1);
		if (error)
			break;
	}
	if (error)
		return error;
	smc = 0;
	error = SYSCTL_OUT(req, &smc, 1);
	return error;
}

static int
#if __FreeBSD_version > 500000
iconv_xlat_sysctl_add(SYSCTL_HANDLER_ARGS)
#else
iconv_xlat_sysctl_add SYSCTL_HANDLER_ARGS
#endif
{
	struct iconv_xlat_add *xap;
	struct iconv_xlat_tbl *xp, *xp1;
	const char **cpp;
	int error, size;

	size = sizeof(*xp) + sizeof(*xap) + sizeof(char*) * 4;
	xp = malloc(size, M_ICONV, M_WAITOK);
	if (xp == NULL)
		return ENOMEM;
	bzero(xp, size);
	xap = (struct iconv_xlat_add *)(xp + 1);
	error = SYSCTL_IN(req, xap, sizeof(*xap));
	if (error)
		goto bad;	/* sigh, no exceptions... */
	if (xap->xa_version != ICONV_XLAT_ADD_VER) {
		error = EINVAL;
		goto bad;
	}
	if (iconv_xlat_lookup(xap->xa_to, xap->xa_from, &xp1) == 0) {
		error = EEXIST;
		goto bad;
	}
	cpp = (const char**)(xap + 1);
	xp->x_from = cpp;
	*cpp++ = xap->xa_from;
	*cpp++ = NULL;
	xp->x_to = cpp;
	*cpp++ = xap->xa_to;
	*cpp = NULL;
	xp->x_table = xap->xa_table;
	error = iconv_xlat_add_table(xp);
	if (error)
		goto bad;
	return 0;
bad:
	free(xp, M_ICONV);
	return error;
}

SYSCTL_NODE(_kern_iconv, OID_AUTO, xlat, CTLFLAG_RW, NULL, "xlat interface");
SYSCTL_PROC(_kern_iconv_xlat, OID_AUTO, list, CTLFLAG_RD | CTLTYPE_STRING,
	    NULL, 0, iconv_xlat_sysctl_list, "S,xlat", "registered tables");
SYSCTL_PROC(_kern_iconv_xlat, OID_AUTO, add, CTLFLAG_WR | CTLTYPE_OPAQUE,
	    NULL, 0, iconv_xlat_sysctl_add, "O,xlataddd", "input to add a table");
#endif
