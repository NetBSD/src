/*	$NetBSD: quota_kernel.c,v 1.4.18.1 2014/08/10 06:52:15 tls Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David A. Holland.
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
__RCSID("$NetBSD: quota_kernel.c,v 1.4.18.1 2014/08/10 06:52:15 tls Exp $");

#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <limits.h>

#include <quota.h>
#include <sys/quotactl.h>

#include "quotapvt.h"

struct kernel_quotacursor {
	/* just wrap the kernel interface type */
	struct quotakcursor kcursor;
};

static int
__quota_kernel_stat(struct quotahandle *qh, struct quotastat *stat)
{
	struct quotactl_args args;

	args.qc_op = QUOTACTL_STAT;
	args.u.stat.qc_info = stat;
	return __quotactl(qh->qh_mountpoint, &args);
}

const char *
__quota_kernel_getimplname(struct quotahandle *qh)
{
	static struct quotastat stat;

	if (__quota_kernel_stat(qh, &stat)) {
		return NULL;
	}
	return stat.qs_implname;
}

unsigned
__quota_kernel_getrestrictions(struct quotahandle *qh)
{
	struct quotastat stat;

	if (__quota_kernel_stat(qh, &stat)) {
		/* XXX no particularly satisfactory thing to do here */
		return 0;
	}
	return stat.qs_restrictions;
}

int
__quota_kernel_getnumidtypes(struct quotahandle *qh)
{
	struct quotastat stat;

	if (__quota_kernel_stat(qh, &stat)) {
		return 0;
	}
	return stat.qs_numidtypes;
}

const char *
__quota_kernel_idtype_getname(struct quotahandle *qh, int idtype)
{
	static struct quotaidtypestat stat;
	struct quotactl_args args;

	args.qc_op = QUOTACTL_IDTYPESTAT;
	args.u.idtypestat.qc_idtype = idtype;
	args.u.idtypestat.qc_info = &stat;
	if (__quotactl(qh->qh_mountpoint, &args)) {
		return NULL;
	}
	return stat.qis_name;
}

int
__quota_kernel_getnumobjtypes(struct quotahandle *qh)
{
	struct quotastat stat;

	if (__quota_kernel_stat(qh, &stat)) {
		return 0;
	}
	return stat.qs_numobjtypes;
}

const char *
__quota_kernel_objtype_getname(struct quotahandle *qh, int objtype)
{
	static struct quotaobjtypestat stat;
	struct quotactl_args args;

	args.qc_op = QUOTACTL_OBJTYPESTAT;
	args.u.objtypestat.qc_objtype = objtype;
	args.u.objtypestat.qc_info = &stat;
	if (__quotactl(qh->qh_mountpoint, &args)) {
		return NULL;
	}
	return stat.qos_name;
}

int
__quota_kernel_objtype_isbytes(struct quotahandle *qh, int objtype)
{
	struct quotaobjtypestat stat;
	struct quotactl_args args;

	args.qc_op = QUOTACTL_OBJTYPESTAT;
	args.u.objtypestat.qc_objtype = objtype;
	args.u.objtypestat.qc_info = &stat;
	if (__quotactl(qh->qh_mountpoint, &args)) {
		return 0;
	}
	return stat.qos_isbytes;
}

int
__quota_kernel_quotaon(struct quotahandle *qh, int idtype)
{
	struct quotactl_args args;
	const char *file;
	char path[PATH_MAX];

	/*
	 * Note that while it is an error to call quotaon on something
	 * that isn't a volume with old-style quotas that expects
	 * quotaon to be called, it's not our responsibility to check
	 * for that; the filesystem will. Also note that it is not an
	 * error to call quotaon repeatedly -- apparently this is to
	 * permit changing the quota file in use on the fly or
	 * something. So all we need to do here is ask the oldfiles
	 * code if the mount option was set in fstab and fetch back
	 * the filename.
	 */

	file = __quota_oldfiles_getquotafile(qh, idtype, path, sizeof(path));
	if (file == NULL) {
		/*
		 * This idtype (or maybe any idtype) was not enabled
		 * in fstab.
		 */
		errno = ENXIO;
		return -1;
	}

	args.qc_op = QUOTACTL_QUOTAON;
	args.u.quotaon.qc_idtype = idtype;
	args.u.quotaon.qc_quotafile = file;
	return __quotactl(qh->qh_mountpoint, &args);
}

int
__quota_kernel_quotaoff(struct quotahandle *qh, int idtype)
{
	struct quotactl_args args;

	args.qc_op = QUOTACTL_QUOTAOFF;
	args.u.quotaoff.qc_idtype = idtype;
	return __quotactl(qh->qh_mountpoint, &args);
}

int
__quota_kernel_get(struct quotahandle *qh, const struct quotakey *qk,
		   struct quotaval *qv)
{
	struct quotactl_args args;

	args.qc_op = QUOTACTL_GET;
	args.u.get.qc_key = qk;
	args.u.get.qc_val = qv;
	return __quotactl(qh->qh_mountpoint, &args);
}

int
__quota_kernel_put(struct quotahandle *qh, const struct quotakey *qk,
		   const struct quotaval *qv)
{
	struct quotactl_args args;

	args.qc_op = QUOTACTL_PUT;
	args.u.put.qc_key = qk;
	args.u.put.qc_val = qv;
	return __quotactl(qh->qh_mountpoint, &args);
}

int
__quota_kernel_delete(struct quotahandle *qh, const struct quotakey *qk)
{
	struct quotactl_args args;

	args.qc_op = QUOTACTL_DEL;
	args.u.del.qc_key = qk;
	return __quotactl(qh->qh_mountpoint, &args);
}

struct kernel_quotacursor *
__quota_kernel_cursor_create(struct quotahandle *qh)
{
	struct quotactl_args args;
	struct kernel_quotacursor *cursor;
	int sverrno;

	cursor = malloc(sizeof(*cursor));
	if (cursor == NULL) {
		return NULL;
	}

	args.qc_op = QUOTACTL_CURSOROPEN;
	args.u.cursoropen.qc_cursor = &cursor->kcursor;
	if (__quotactl(qh->qh_mountpoint, &args)) {
		sverrno = errno;
		free(cursor);
		errno = sverrno;
		return NULL;
	}
	return cursor;
}

void
__quota_kernel_cursor_destroy(struct quotahandle *qh,
			      struct kernel_quotacursor *cursor)
{
	struct quotactl_args args;

	args.qc_op = QUOTACTL_CURSORCLOSE;
	args.u.cursorclose.qc_cursor = &cursor->kcursor;
	if (__quotactl(qh->qh_mountpoint, &args)) {
		/* XXX should we really print from inside the library? */
		warn("__quotactl cursorclose");
	}
	free(cursor);
}

int
__quota_kernel_cursor_skipidtype(struct quotahandle *qh,
				 struct kernel_quotacursor *cursor,
				 int idtype)
{
	struct quotactl_args args;

	args.qc_op = QUOTACTL_CURSORSKIPIDTYPE;
	args.u.cursorskipidtype.qc_cursor = &cursor->kcursor;
	args.u.cursorskipidtype.qc_idtype = idtype;
	return __quotactl(qh->qh_mountpoint, &args);
}

int
__quota_kernel_cursor_get(struct quotahandle *qh,
			  struct kernel_quotacursor *cursor,
			  struct quotakey *key, struct quotaval *val)
{
	int ret;

	ret = __quota_kernel_cursor_getn(qh, cursor, key, val, 1);
	if (ret < 0) {
		return -1;
	}
	return 0;
}

int
__quota_kernel_cursor_getn(struct quotahandle *qh,
			   struct kernel_quotacursor *cursor,
			   struct quotakey *keys, struct quotaval *vals,
			   unsigned maxnum)
{
	struct quotactl_args args;
	unsigned ret;

	if (maxnum > INT_MAX) {
		/* joker, eh? */
		errno = EINVAL;
		return -1;
	}

	args.qc_op = QUOTACTL_CURSORGET;
	args.u.cursorget.qc_cursor = &cursor->kcursor;
	args.u.cursorget.qc_keys = keys;
	args.u.cursorget.qc_vals = vals;
	args.u.cursorget.qc_maxnum = maxnum;
	args.u.cursorget.qc_ret = &ret;
	if (__quotactl(qh->qh_mountpoint, &args) < 0) {
		return -1;
	}
	return ret;
}

int
__quota_kernel_cursor_atend(struct quotahandle *qh,
			    struct kernel_quotacursor *cursor)
{
	struct quotactl_args args;
	int ret;

	args.qc_op = QUOTACTL_CURSORATEND;
	args.u.cursoratend.qc_cursor = &cursor->kcursor;
	args.u.cursoratend.qc_ret = &ret;
	if (__quotactl(qh->qh_mountpoint, &args)) {
		/*
		 * Return -1 so naive callers, who test for the return
		 * value being nonzero, stop iterating, and
		 * sophisticated callers can tell an error from
		 * end-of-data.
		 */
		return -1;
	}
	return ret;
}

int
__quota_kernel_cursor_rewind(struct quotahandle *qh,
			     struct kernel_quotacursor *cursor)
{
	struct quotactl_args args;

	args.qc_op = QUOTACTL_CURSORREWIND;
	args.u.cursorrewind.qc_cursor = &cursor->kcursor;
	return __quotactl(qh->qh_mountpoint, &args);
}
