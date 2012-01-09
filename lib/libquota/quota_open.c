/*	$NetBSD: quota_open.c,v 1.4 2012/01/09 15:41:58 dholland Exp $	*/
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
__RCSID("$NetBSD: quota_open.c,v 1.4 2012/01/09 15:41:58 dholland Exp $");

#include <sys/types.h>
#include <sys/statvfs.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <quota.h>
#include "quotapvt.h"

struct quotahandle *
quota_open(const char *path)
{

	struct statvfs stv;
	struct quotahandle *qh;
	int isnfs;
	int serrno;

	if (statvfs(path, &stv) < 0) {
		return NULL;
	}

	/*
	 * We need to know up front if the volume is NFS. If so, we
	 * don't go to the kernel at all but instead do RPCs to the
	 * NFS server's rpc.rquotad. Therefore, don't check the
	 * mount flags for quota support or do anything else that
	 * reaches the kernel.
	 */
	
	if (!strcmp(stv.f_fstypename, "nfs")) {
		isnfs = 1;
	} else {
		isnfs = 0;
		if ((stv.f_flag & ST_QUOTA) == 0) {
			/* XXX: correct errno? */
			errno = EOPNOTSUPP;
			return NULL;
		}
	}

	qh = malloc(sizeof(*qh));
	if (qh == NULL) {
		return NULL;
	}

	/*
	 * Get the mount point from statvfs; this way the passed-in
	 * path can be any path on the volume.
	 */

	qh->qh_mountpoint = strdup(stv.f_mntonname);
	if (qh->qh_mountpoint == NULL) {
		serrno = errno;
		free(qh);
		errno = serrno;
		return NULL;
	}

	qh->qh_mountdevice = strdup(stv.f_mntfromname);
	if (qh->qh_mountdevice == NULL) {
		serrno = errno;
		free(qh->qh_mountpoint);
		free(qh);
		errno = serrno;
		return NULL;
	}

	qh->qh_isnfs = isnfs;

	qh->qh_hasoldfiles = 0;
	qh->qh_userfile = -1;
	qh->qh_groupfile = -1;

	return qh;
}

const char *
quota_getmountpoint(struct quotahandle *qh)
{
	return qh->qh_mountpoint;
}

const char *
quota_getmountdevice(struct quotahandle *qh)
{
	return qh->qh_mountdevice;
}

void
quota_close(struct quotahandle *qh)
{
	if (qh->qh_userfile >= 0) {
		close(qh->qh_userfile);
	}
	if (qh->qh_groupfile >= 0) {
		close(qh->qh_groupfile);
	}
	free(qh->qh_mountdevice);
	free(qh->qh_mountpoint);
	free(qh);
}
