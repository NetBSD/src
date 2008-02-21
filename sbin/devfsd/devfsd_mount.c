/* 	$NetBSD: devfsd_mount.c,v 1.1.6.1 2008/02/21 20:44:55 mjf Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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

#include <sys/queue.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "devfsd.h"

extern struct mlist_head  mount_list;

/*
 * Create a new devfs_mount structure and add it to the list
 * of all mounts.
 */
struct devfs_mount *
mount_create(int32_t cookie, const char *mountpath, int visibility)
{
	struct devfs_mount *dmp;

	if ((dmp = malloc(sizeof(*dmp))) == NULL)
		return NULL;

	strlcpy(dmp->m_pathname, mountpath, sizeof(dmp->m_pathname));
	dmp->m_id = cookie;
	dmp->m_visibility = visibility;
	SLIST_INSERT_HEAD(&mount_list, dmp, m_next);

	return dmp;
}

struct devfs_mount *
mount_lookup(int32_t cookie)
{
	struct devfs_mount *dmp;

	SLIST_FOREACH(dmp, &mount_list, m_next) {
		if (dmp->m_id == cookie)
			break;
	}
	return dmp;
}

void
mount_destroy(struct devfs_mount *dmp)
{
	SLIST_REMOVE(&mount_list, dmp, devfs_mount, m_next);
	free(dmp);
}
