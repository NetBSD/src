/*	$NetBSD: quota_cursor.c,v 1.2 2012/01/09 15:40:10 dholland Exp $	*/
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
__RCSID("$NetBSD: quota_cursor.c,v 1.2 2012/01/09 15:40:10 dholland Exp $");

#include <stdlib.h>
#include <errno.h>

#include <quota.h>
#include "quotapvt.h"

struct quotacursor *
quota_opencursor(struct quotahandle *qh)
{
	struct quotacursor *qc;
	int serrno;

	if (qh->qh_isnfs) {
		errno = EOPNOTSUPP;
		return NULL;
	}

	qc = malloc(sizeof(*qc));
	if (qc == NULL) {
		return NULL;
	}

	qc->qc_qh = qh;
	qc->u.qc_proplib = __quota_proplib_cursor_create();

	if (qc->u.qc_proplib == NULL) {
		serrno = errno;
		free(qc);
		errno = serrno;
		return NULL;
	}
	return qc;
}

void
quotacursor_close(struct quotacursor *qc)
{
	__quota_proplib_cursor_destroy(qc->u.qc_proplib);
	free(qc);
}

int
quotacursor_skipidtype(struct quotacursor *qc, unsigned idtype)
{
	return __quota_proplib_cursor_skipidtype(qc->u.qc_proplib, idtype);
}

int
quotacursor_get(struct quotacursor *qc,
		struct quotakey *qk_ret, struct quotaval *qv_ret)
{
	return __quota_proplib_cursor_get(qc->qc_qh, qc->u.qc_proplib,
					  qk_ret, qv_ret);
}

int
quotacursor_getn(struct quotacursor *qc,
		 struct quotakey *keys, struct quotaval *vals, 
		 unsigned maxnum)
{
	return __quota_proplib_cursor_getn(qc->qc_qh, qc->u.qc_proplib,
					   keys, vals, maxnum);
}

int
quotacursor_atend(struct quotacursor *qc)
{
	return __quota_proplib_cursor_atend(qc->qc_qh,
					    qc->u.qc_proplib);
}

int
quotacursor_rewind(struct quotacursor *qc)
{
	return __quota_proplib_cursor_rewind(qc->u.qc_proplib);
}
