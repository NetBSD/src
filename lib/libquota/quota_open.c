/*	$NetBSD: quota_open.c,v 1.5 2012/01/25 17:43:37 dholland Exp $	*/
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
__RCSID("$NetBSD: quota_open.c,v 1.5 2012/01/25 17:43:37 dholland Exp $");

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
	int mode;
	int serrno;

	/*
	 * Probe order:
	 *
	 *    1. Check for NFS. NFS quota ops don't go to the kernel
	 *    at all but instead do RPCs to the NFS server's
	 *    rpc.rquotad, so it doesn't matter what the kernel
	 *    thinks.
	 *
	 *    2. Check if quotas are enabled in the mount flags. If
	 *    so, we can do quotactl calls.
	 *
	 *    3. Check if the volume is listed in fstab as one of
	 *    the filesystem types supported by quota_oldfiles.c,
	 *    and with the proper mount options to enable quotas.
	 */

	if (statvfs(path, &stv) < 0) {
		return NULL;
	}

	__quota_oldfiles_load_fstab();

	if (!strcmp(stv.f_fstypename, "nfs")) {
		mode = QUOTA_MODE_NFS;
	} else if ((stv.f_flag & ST_QUOTA) != 0) {
		mode = QUOTA_MODE_PROPLIB;
	} else if (__quota_oldfiles_infstab(stv.f_mntonname)) {
		mode = QUOTA_MODE_OLDFILES;
	} else {
		errno = EOPNOTSUPP;
		return NULL;
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

	qh->qh_mode = mode;

	qh->qh_oldfilesopen = 0;
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
