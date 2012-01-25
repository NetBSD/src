/*	$NetBSD: quota_cursor.c,v 1.4 2012/01/25 17:43:37 dholland Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: quota_cursor.c,v 1.4 2012/01/25 17:43:37 dholland Exp $");

#include <stdlib.h>
#include <errno.h>

#include <quota.h>
#include "quotapvt.h"

struct quotacursor *
quota_opencursor(struct quotahandle *qh)
{
	struct quotacursor *qc;
	int8_t version;
	int serrno;

	switch (qh->qh_mode) {
	    case QUOTA_MODE_NFS:
		errno = EOPNOTSUPP;
		return NULL;

	    case QUOTA_MODE_PROPLIB:
		if (__quota_proplib_getversion(qh, &version)) {
			return NULL;
		}
		break;

	    case QUOTA_MODE_OLDFILES:
		version = 1;
		break;

	    default:
		errno = EINVAL;
		break;
	}

	/*
	 * For the time being at least the version 1 kernel code
	 * cannot do cursors.
	 */
	if (version == 1 && !qh->qh_oldfilesopen) {
		if (__quota_oldfiles_initialize(qh)) {
			return NULL;
		}
	}

	qc = malloc(sizeof(*qc));
	if (qc == NULL) {
		return NULL;
	}

	qc->qc_qh = qh;

	if (version == 1) {
		qc->qc_type = QC_OLDFILES;
		qc->u.qc_oldfiles = __quota_oldfiles_cursor_create(qh);
		if (qc->u.qc_oldfiles == NULL) {
			serrno = errno;
			free(qc);
			errno = serrno;
			return NULL;
		}
	} else {
		qc->qc_type = QC_PROPLIB;
		qc->u.qc_proplib = __quota_proplib_cursor_create();
		if (qc->u.qc_proplib == NULL) {
			serrno = errno;
			free(qc);
			errno = serrno;
			return NULL;
		}
	}
	return qc;
}

void
quotacursor_close(struct quotacursor *qc)
{
	switch (qc->qc_type) {
	    case QC_PROPLIB:
		__quota_proplib_cursor_destroy(qc->u.qc_proplib);
		break;
	    case QC_OLDFILES:
		__quota_oldfiles_cursor_destroy(qc->u.qc_oldfiles);
		break;
	}
	free(qc);
}

int
quotacursor_skipidtype(struct quotacursor *qc, unsigned idtype)
{
	switch (qc->qc_type) {
	    case QC_PROPLIB:
		return __quota_proplib_cursor_skipidtype(qc->u.qc_proplib,
							 idtype);
	    case QC_OLDFILES:
		return __quota_oldfiles_cursor_skipidtype(qc->u.qc_oldfiles,
							  idtype);
	}
	errno = EINVAL;
	return -1;
}

int
quotacursor_get(struct quotacursor *qc,
		struct quotakey *qk_ret, struct quotaval *qv_ret)
{
	switch (qc->qc_type) {
	    case QC_PROPLIB:
		return __quota_proplib_cursor_get(qc->qc_qh, qc->u.qc_proplib,
						  qk_ret, qv_ret);
	    case QC_OLDFILES:
		return __quota_oldfiles_cursor_get(qc->qc_qh,
						   qc->u.qc_oldfiles,
						   qk_ret, qv_ret);
	}
	errno = EINVAL;
	return -1;
}

int
quotacursor_getn(struct quotacursor *qc,
		 struct quotakey *keys, struct quotaval *vals, 
		 unsigned maxnum)
{
	switch (qc->qc_type) {
	    case QC_PROPLIB:
		return __quota_proplib_cursor_getn(qc->qc_qh, qc->u.qc_proplib,
						   keys, vals, maxnum);
	    case QC_OLDFILES:
		return __quota_oldfiles_cursor_getn(qc->qc_qh,
						    qc->u.qc_oldfiles,
						    keys, vals, maxnum);
	}
	errno = EINVAL;
	return -1;
}

int
quotacursor_atend(struct quotacursor *qc)
{
	switch (qc->qc_type) {
	    case QC_PROPLIB:
		return __quota_proplib_cursor_atend(qc->qc_qh,
						    qc->u.qc_proplib);
	    case QC_OLDFILES:
		return __quota_oldfiles_cursor_atend(qc->u.qc_oldfiles);
	}
	errno = EINVAL;
	return -1;
}

int
quotacursor_rewind(struct quotacursor *qc)
{
	switch (qc->qc_type) {
	    case QC_PROPLIB:
		return __quota_proplib_cursor_rewind(qc->u.qc_proplib);
	    case QC_OLDFILES:
		return __quota_oldfiles_cursor_rewind(qc->u.qc_oldfiles);
	}
	errno = EINVAL;
	return -1;
}
