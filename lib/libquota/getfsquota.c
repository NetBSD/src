/*	$NetBSD: getfsquota.c,v 1.1.2.1 2011/06/23 14:18:40 cherry Exp $ */

/*-
  * Copyright (c) 2011 Manuel Bouyer
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
__RCSID("$NetBSD: getfsquota.c,v 1.1.2.1 2011/06/23 14:18:40 cherry Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <string.h>

#include <sys/types.h>
#include <sys/statvfs.h>

#include <quota/quotaprop.h>
#include <quota/quota.h>

/* retrieve quotas with ufs semantics from vfs, for the given user id */
int
getfsquota(const char *path, struct ufs_quota_entry *qv, uid_t id,
    const char *class)
{
	struct statvfs v;

	if (statvfs(path, &v) < 0) {
		return -1;
	}
	if (strcmp(v.f_fstypename, "nfs") == 0)
		return getnfsquota(v.f_mntfromname, qv, id, class);
	else {
		if ((v.f_flag & ST_QUOTA) == 0)
			return 0;
		/*
		 * assume all quota-enabled local filesystems have UFS
		 * semantic for now
		 */
		return getufsquota(v.f_mntonname, qv, id, class);
	}
}
