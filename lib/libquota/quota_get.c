/*	$NetBSD: quota_get.c,v 1.5 2012/02/01 05:34:40 dholland Exp $	*/
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
__RCSID("$NetBSD: quota_get.c,v 1.5 2012/02/01 05:34:40 dholland Exp $");

#include <errno.h>

#include <quota.h>
#include "quotapvt.h"

void
quotaval_clear(struct quotaval *qv)
{
	qv->qv_hardlimit = QUOTA_NOLIMIT;
	qv->qv_softlimit = QUOTA_NOLIMIT;
	qv->qv_usage = 0;
	qv->qv_expiretime = QUOTA_NOTIME;
	qv->qv_grace = QUOTA_NOTIME;
}

int
quota_get(struct quotahandle *qh, const struct quotakey *qk, struct quotaval *qv)
{
	switch (qh->qh_mode) {
	    case QUOTA_MODE_NFS:
		return __quota_nfs_get(qh, qk, qv);

	    case QUOTA_MODE_OLDFILES:
		return __quota_oldfiles_get(qh, qk, qv);

	    case QUOTA_MODE_KERNEL:
		return __quota_kernel_get(qh, qk, qv);

	    default:
		break;
	}
	errno = EINVAL;
	return -1;
}
